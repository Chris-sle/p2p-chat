#include <p2pnet/p2pnet.h>
#include <p2pnet/message.h>
#include <stdio.h>
#include <signal.h>

static volatile int keep_running = 1;

void signal_handler(int signum) {
    (void)signum;
    printf("\n[INFO] Shutting down...\n");
    keep_running = 0;
}

int main() {
    printf("========================================================\n");
    printf("      Framed Message Server (Milestone 1.2)            \n");
    printf("========================================================\n\n");
    
    if (p2p_init() != 0) {
        printf("[ERROR] Init failed: %s\n", p2p_get_error());
        return 1;
    }
    
    p2p_socket_t* server = p2p_socket_create(P2P_TCP);
    if (!server) {
        printf("[ERROR] Create failed\n");
        p2p_cleanup();
        return 1;
    }
    
    if (p2p_socket_bind(server, "0.0.0.0", 8080) != 0) {
        printf("[ERROR] Bind failed: %s\n", p2p_get_error());
        p2p_socket_close(server);
        p2p_cleanup();
        return 1;
    }
    
    if (p2p_socket_listen(server, 5) != 0) {
        printf("[ERROR] Listen failed\n");
        p2p_socket_close(server);
        p2p_cleanup();
        return 1;
    }
    
    printf("[OK] Server listening on port 8080\n");
    printf("[OK] Using length-prefix framing (4-byte header)\n");
    printf("[OK] Max message size: %d bytes\n\n", P2P_MAX_MESSAGE_SIZE);
    printf("Test with: build\\05_framed_client.exe\n");
    printf("Press Ctrl+C to stop\n\n");
    
    signal(SIGINT, signal_handler);
    
    int client_count = 0;
    
    while (keep_running) {
        printf("[WAIT] Waiting for client #%d...\n", client_count + 1);
        
        p2p_socket_t* client = p2p_socket_accept(server);
        if (!client) {
            if (keep_running) {
                printf("[ERROR] Accept failed\n");
            }
            break;
        }
        
        client_count++;
        printf("[OK] Client #%d connected\n", client_count);
        
        // Motta framed message
        p2p_message_t* msg = p2p_message_recv(client);
        
        if (msg) {
            p2p_message_print(msg, "[RECV] ");
            
            // Echo tilbake
            printf("[SEND] Echoing message back...\n");
            if (p2p_message_send(client, msg) == 0) {
                printf("[OK] Message sent\n");
            } else {
                printf("[ERROR] Send failed\n");
            }
            
            p2p_message_free(msg);
        } else {
            printf("[ERROR] Failed to receive message\n");
        }
        
        p2p_socket_close(client);
        printf("[CLOSE] Client #%d disconnected\n\n", client_count);
    }
    
    p2p_socket_close(server);
    p2p_cleanup();
    
    printf("\n[OK] Server shutdown complete (%d clients handled)\n", client_count);
    return 0;
}