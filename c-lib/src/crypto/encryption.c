#include <p2pnet/encryption.h>
#include <p2pnet/socket.h>
#include <p2pnet/message.h>
#include <sodium.h>
#include <stdio.h>
#include <string.h>

// ChaCha20-Poly1305 constants
#define NONCE_SIZE 12
#define MAC_SIZE 16

/**
 * Helper: Construct nonce from counter
 * 
 * Nonce format (12 bytes):
 * ┌──────────────────────┬──────────────────┐
 * │ Counter (8 bytes)    │ Padding (4 bytes) │
 * │ uint64_t (big-endian)│ 0x00000000        │
 * └──────────────────────┴──────────────────┘
 */
static void construct_nonce(uint64_t counter, uint8_t nonce[NONCE_SIZE]) {
    // Zero-initialize nonce
    memset(nonce, 0, NONCE_SIZE);
    
    // Write counter in big-endian (network byte order)
    nonce[0] = (counter >> 56) & 0xFF;
    nonce[1] = (counter >> 48) & 0xFF;
    nonce[2] = (counter >> 40) & 0xFF;
    nonce[3] = (counter >> 32) & 0xFF;
    nonce[4] = (counter >> 24) & 0xFF;
    nonce[5] = (counter >> 16) & 0xFF;
    nonce[6] = (counter >> 8) & 0xFF;
    nonce[7] = counter & 0xFF;
    
    // Remaining 4 bytes already zero (padding)
}

/**
 * Helper: Extract counter from nonce
 */
static uint64_t extract_counter(const uint8_t nonce[NONCE_SIZE]) {
    uint64_t counter = 0;
    counter |= ((uint64_t)nonce[0] << 56);
    counter |= ((uint64_t)nonce[1] << 48);
    counter |= ((uint64_t)nonce[2] << 40);
    counter |= ((uint64_t)nonce[3] << 32);
    counter |= ((uint64_t)nonce[4] << 24);
    counter |= ((uint64_t)nonce[5] << 16);
    counter |= ((uint64_t)nonce[6] << 8);
    counter |= ((uint64_t)nonce[7]);
    return counter;
}

/**
 * Helper: Send exact amount of data (handles partial sends)
 */
static int send_exact(p2p_socket_t* sock, const void* data, size_t length) {
    size_t total_sent = 0;
    const uint8_t* ptr = (const uint8_t*)data;
    
    while (total_sent < length) {
        ssize_t sent = p2p_socket_send(sock, ptr + total_sent, length - total_sent);
        if (sent <= 0) {
            return -1;  // Error or connection closed
        }
        total_sent += sent;
    }
    
    return 0;
}

/**
 * Helper: Receive exact amount of data (handles partial reads)
 */
static int recv_exact(p2p_socket_t* sock, void* buffer, size_t length) {
    size_t total_received = 0;
    uint8_t* ptr = (uint8_t*)buffer;
    
    while (total_received < length) {
        ssize_t received = p2p_socket_recv(sock, ptr + total_received, length - total_received);
        if (received <= 0) {
            return -1;  // Error or connection closed
        }
        total_received += received;
    }
    
    return 0;
}

int p2p_session_send(p2p_session_t* session,
                     p2p_socket_t* sock,
                     const uint8_t* data,
                     size_t length) {
    if (!session || !sock || !data || length == 0) {
        fprintf(stderr, "[ENCRYPTION] Invalid parameters\n");
        return -1;
    }
    
    // Check for counter overflow (extremely unlikely - 2^64 messages)
    if (session->send_nonce == UINT64_MAX) {
        fprintf(stderr, "[ENCRYPTION] CRITICAL: Send nonce overflow! Re-handshake required.\n");
        return -1;
    }
    
    // Construct nonce from current counter
    uint8_t nonce[NONCE_SIZE];
    construct_nonce(session->send_nonce, nonce);
    
    // Allocate buffer for ciphertext + MAC
    // ChaCha20-Poly1305 adds 16-byte MAC tag
    unsigned long long ciphertext_len;
    uint8_t* ciphertext = (uint8_t*)malloc(length + MAC_SIZE);
    if (!ciphertext) {
        fprintf(stderr, "[ENCRYPTION] Memory allocation failed\n");
        return -1;
    }
    
    // Encrypt with ChaCha20-Poly1305
    int result = crypto_aead_chacha20poly1305_ietf_encrypt(
        ciphertext, &ciphertext_len,
        data, length,
        NULL, 0,  // No additional authenticated data
        NULL,     // nsec (unused)
        nonce,
        session->session_key
    );
    
    if (result != 0) {
        fprintf(stderr, "[ENCRYPTION] Encryption failed\n");
        free(ciphertext);
        return -1;
    }
    
    // Wire format: [4B length][12B nonce][ciphertext + MAC]
    uint32_t total_length = NONCE_SIZE + ciphertext_len;
    uint32_t network_length = htonl(total_length);
    
    // Send length header
    if (send_exact(sock, &network_length, sizeof(network_length)) != 0) {
        fprintf(stderr, "[ENCRYPTION] Failed to send length header\n");
        free(ciphertext);
        return -1;
    }
    
    // Send nonce
    if (send_exact(sock, nonce, NONCE_SIZE) != 0) {
        fprintf(stderr, "[ENCRYPTION] Failed to send nonce\n");
        free(ciphertext);
        return -1;
    }
    
    // Send ciphertext + MAC
    if (send_exact(sock, ciphertext, ciphertext_len) != 0) {
        fprintf(stderr, "[ENCRYPTION] Failed to send ciphertext\n");
        free(ciphertext);
        return -1;
    }
    
    // Increment send nonce (CRITICAL: must happen after successful send)
    session->send_nonce++;
    
    free(ciphertext);
    return 0;
}

p2p_message_t* p2p_session_recv(p2p_session_t* session,
                                 p2p_socket_t* sock) {
    if (!session || !sock) {
        fprintf(stderr, "[ENCRYPTION] Invalid parameters\n");
        return NULL;
    }
    
    // Receive length header
    uint32_t network_length;
    if (recv_exact(sock, &network_length, sizeof(network_length)) != 0) {
        fprintf(stderr, "[ENCRYPTION] Failed to receive length header\n");
        return NULL;
    }
    
    uint32_t total_length = ntohl(network_length);
    
    // Sanity check: minimum length is NONCE_SIZE + MAC_SIZE
    if (total_length < NONCE_SIZE + MAC_SIZE) {
        fprintf(stderr, "[ENCRYPTION] Invalid message length: %u\n", total_length);
        return NULL;
    }
    
    // Sanity check: reasonable maximum (1MB + overhead)
    if (total_length > P2P_MAX_MESSAGE_SIZE + NONCE_SIZE + MAC_SIZE) {
        fprintf(stderr, "[ENCRYPTION] Message too large: %u\n", total_length);
        return NULL;
    }
    
    // Receive nonce
    uint8_t nonce[NONCE_SIZE];
    if (recv_exact(sock, nonce, NONCE_SIZE) != 0) {
        fprintf(stderr, "[ENCRYPTION] Failed to receive nonce\n");
        return NULL;
    }
    
    // Extract and verify nonce counter
    uint64_t received_counter = extract_counter(nonce);
    
    // Replay protection: nonce must be strictly increasing
    if (received_counter <= session->recv_nonce) {
        fprintf(stderr, "[ENCRYPTION] SECURITY: Nonce rewind detected! "
                        "Expected >%llu, got %llu (replay attack?)\n",
                        session->recv_nonce, received_counter);
        return NULL;
    }
    
    // Receive ciphertext + MAC
    size_t ciphertext_len = total_length - NONCE_SIZE;
    uint8_t* ciphertext = (uint8_t*)malloc(ciphertext_len);
    if (!ciphertext) {
        fprintf(stderr, "[ENCRYPTION] Memory allocation failed\n");
        return NULL;
    }
    
    if (recv_exact(sock, ciphertext, ciphertext_len) != 0) {
        fprintf(stderr, "[ENCRYPTION] Failed to receive ciphertext\n");
        free(ciphertext);
        return NULL;
    }
    
    // Allocate buffer for plaintext (same size as ciphertext minus MAC)
    size_t plaintext_max_len = ciphertext_len;  // Includes MAC, but decrypt will adjust
    uint8_t* plaintext = (uint8_t*)malloc(plaintext_max_len);
    if (!plaintext) {
        fprintf(stderr, "[ENCRYPTION] Memory allocation failed\n");
        free(ciphertext);
        return NULL;
    }
    
    // Decrypt and verify MAC
    unsigned long long plaintext_len;
    int result = crypto_aead_chacha20poly1305_ietf_decrypt(
        plaintext, &plaintext_len,
        NULL,     // nsec (unused)
        ciphertext, ciphertext_len,
        NULL, 0,  // No additional authenticated data
        nonce,
        session->session_key
    );
    
    if (result != 0) {
        fprintf(stderr, "[ENCRYPTION] SECURITY: Decryption failed! "
                        "Message tampered or incorrect key.\n");
        free(ciphertext);
        free(plaintext);
        return NULL;
    }
    
    // Update recv_nonce (only after successful decryption)
    session->recv_nonce = received_counter;
    
    // Create message from plaintext
    p2p_message_t* msg = p2p_message_create_binary(plaintext, plaintext_len);
    
    // Cleanup
    free(ciphertext);
    free(plaintext);
    
    return msg;
}