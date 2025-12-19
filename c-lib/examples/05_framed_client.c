#include <p2pnet/p2pnet.h>
#include <p2pnet/message.h>
#include <stdio.h>
#include <string.h>

int main(int argc, char* argv[]) {
    printf("========================================================\n");
    printf("      Framed Message Client (Milestone 1.2)            \n");
    printf("========================================================\n\n");
    
    const char* server_ip = "127.0.0.1";
    const char* message_text = "Hello from framed client!";
    
    if (argc >= 2) server_ip = argv[1];
    if (argc >= 3) message_text = argv[2];
    
    printf("[CONFIG] Server: %s:8080\n", server_ip);
    printf("[CONFIG] Message: \"%s\" (%zu bytes)\n\n", message_text, strlen(message_text));
    
    if (p2p_init() != 0) {
        printf("[ERROR] Init failed\n");
        return 1;
    }
    
    p2p_socket_t* sock = p2p_socket_create(P2P_TCP);
    if (!sock) {
        printf("[ERROR] Create failed\n");
        p2p_cleanup();
        return 1;
    }
    
    printf("[INFO] Connecting...\n");
    if (p2p_socket_connect(sock, server_ip, 8080) != 0) {
        printf("[ERROR] Connect failed: %s\n", p2p_get_error());
        p2p_socket_close(sock);
        p2p_cleanup();
        return 1;
    }
    printf("[OK] Connected\n\n");
    
    // Opprett framed message
    p2p_message_t* msg = p2p_message_create(message_text);
    if (!msg) {
        printf("[ERROR] Failed to create message\n");
        p2p_socket_close(sock);
        p2p_cleanup();
        return 1;
    }
    
    // Send message
    printf("[SEND] Sending framed message...\n");
    printf("       [4 bytes header: 0x%08x] + [%u bytes data]\n", msg->length, msg->length);
    
    if (p2p_message_send(sock, msg) != 0) {
        printf("[ERROR] Send failed\n");
        p2p_message_free(msg);
        p2p_socket_close(sock);
        p2p_cleanup();
        return 1;
    }
    printf("[OK] Message sent\n\n");
    
    p2p_message_free(msg);
    
    // Motta echo
    printf("[WAIT] Waiting for echo...\n");
    p2p_message_t* echo = p2p_message_recv(sock);
    
    if (echo) {
        p2p_message_print(echo, "[RECV] ");
        p2p_message_free(echo);
    } else {
        printf("[ERROR] Failed to receive echo\n");
    }
    
    p2p_socket_close(sock);
    p2p_cleanup();
    
    printf("\n[OK] Client done\n");
    return 0;
}