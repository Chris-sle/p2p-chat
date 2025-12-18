#include "p2pnet/message.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// Windows-spesifikk: hton1/ntoh1 for byte order onversion
#ifdef _WIN32
    #include <winsock2.h>
#else
    #include <arpa/inet.h>
#endif

// ============================================================================
// Helper Functions
// ============================================================================

/**
 * Leser eksakt N bytes fra socket (håndterer partial reads)
 * Blokkerer til alle bytes er lest, eller connection lukkes
 */

 static int recv_exact(p2p_socket_t* sock, void* buffer, size_t length) {
    size_t total_received = 0;
    uint8_t* buf_ptr = (uint8_t*)buffer;

    while( total_received < length ) {
        ssize_t   received = p2p_socket_recv(sock, buf_ptr + total_received, length - total_received);
        if (received <= 0) {
            return -1;  // Error or disconnect
        } else if (received == 0) {
            return (total_received == 0) ? 0 : -1; // Connection closed
        }

        total_received += received;
    }
    
    return total_received;
}

/**
 * Sender eksakt N bytes til socket (håndterer partial sends)
 */
static int send_exact(p2p_socket_t* sock, const void* data, size_t length) {
    size_t total_sent = 0;
    const uint8_t* ptr = (const uint8_t*)data;
    
    while (total_sent < length) {
        ssize_t sent = p2p_socket_send(sock, ptr + total_sent, length - total_sent);
        
        if (sent < 0) {
            return -1;
        } else if (sent == 0) {
            // Should not happen with blocking sockets
            return -1;
        }
        
        total_sent += sent;
    }
    
    return total_sent;
}

// ============================================================================
// Message API
// ============================================================================

p2p_message_t* p2p_message_create(const char* text) {
    if (!text) return NULL;
    
    size_t length = strlen(text);
    return p2p_message_create_binary(text, length);
}

p2p_message_t* p2p_message_create_binary(const void* data, size_t length) {
    if (!data || length == 0) return NULL;
    
    // Sjekk maks størrelse
    if (length > P2P_MAX_MESSAGE_SIZE) {
        fprintf(stderr, "Message too large: %zu bytes (max: %d)\n", 
                length, P2P_MAX_MESSAGE_SIZE);
        return NULL;
    }
    
    // Allokér message struktur
    p2p_message_t* msg = (p2p_message_t*)malloc(sizeof(p2p_message_t));
    if (!msg) return NULL;
    
    // Allokér data buffer
    msg->data = (uint8_t*)malloc(length);
    if (!msg->data) {
        free(msg);
        return NULL;
    }
    
    // Kopier data
    memcpy(msg->data, data, length);
    msg->length = (uint32_t)length;
    
    return msg;
}

int p2p_message_send(p2p_socket_t* sock, p2p_message_t* msg) {
    if (!sock || !msg || !msg->data) return -1;
    
    // Konverter length til network byte order (big-endian)
    uint32_t network_length = htonl(msg->length);
    
    // Send length header (4 bytes)
    if (send_exact(sock, &network_length, sizeof(network_length)) < 0) {
        return -1;
    }
    
    // Send data
    if (send_exact(sock, msg->data, msg->length) < 0) {
        return -1;
    }
    
    return 0;
}

p2p_message_t* p2p_message_recv(p2p_socket_t* sock) {
    if (!sock) return NULL;
    
    // Motta length header (4 bytes)
    uint32_t network_length;
    int result = recv_exact(sock, &network_length, sizeof(network_length));
    
    if (result <= 0) {
        // Connection closed or error
        return NULL;
    }
    
    // Konverter fra network byte order til host byte order
    uint32_t length = ntohl(network_length);
    
    // Valider størrelse
    if (length == 0) {
        fprintf(stderr, "Received zero-length message\n");
        return NULL;
    }
    
    if (length > P2P_MAX_MESSAGE_SIZE) {
        fprintf(stderr, "Message too large: %u bytes (max: %d)\n", 
                length, P2P_MAX_MESSAGE_SIZE);
        return NULL;
    }
    
    // Allokér message
    p2p_message_t* msg = (p2p_message_t*)malloc(sizeof(p2p_message_t));
    if (!msg) return NULL;
    
    msg->length = length;
    msg->data = (uint8_t*)malloc(length);
    if (!msg->data) {
        free(msg);
        return NULL;
    }
    
    // Motta data
    if (recv_exact(sock, msg->data, length) <= 0) {
        p2p_message_free(msg);
        return NULL;
    }
    
    return msg;
}

void p2p_message_free(p2p_message_t* msg) {
    if (!msg) return;
    
    if (msg->data) {
        free(msg->data);
    }
    
    free(msg);
}

void p2p_message_print(const p2p_message_t* msg, const char* prefix) {
    if (!msg || !msg->data) {
        printf("%s(null message)\n", prefix ? prefix : "");
        return;
    }
    
    printf("%s(%u bytes) ", prefix ? prefix : "", msg->length);
    
    // Print som string hvis det ser ut som text
    int is_text = 1;
    for (uint32_t i = 0; i < msg->length && i < 100; i++) {
        if (msg->data[i] < 32 && msg->data[i] != '\n' && msg->data[i] != '\t') {
            is_text = 0;
            break;
        }
    }
    
    if (is_text) {
        // Print som text
        printf("\"");
        for (uint32_t i = 0; i < msg->length && i < 200; i++) {
            if (msg->data[i] == '\n') {
                printf("\\n");
            } else {
                printf("%c", msg->data[i]);
            }
        }
        if (msg->length > 200) printf("...");
        printf("\"\n");
    } else {
        // Print som hex
        printf("[");
        for (uint32_t i = 0; i < msg->length && i < 32; i++) {
            printf("%02x ", msg->data[i]);
        }
        if (msg->length > 32) printf("...");
        printf("]\n");
    }
}