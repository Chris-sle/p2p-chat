#include <p2pnet/p2pnet.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>

// Global flag for å stoppe server gracefully
static volatile int keep_running = 1;

void signal_handler(int signum) {
    (void)signum;
    printf("\n\n[INFO] Received shutdown signal...\n");
    keep_running = 0;
}

int main() {
    printf("========================================================\n");
    printf("        P2P Basic Server Example (Enhanced)            \n");
    printf("========================================================\n\n");
    
    // Initialiser biblioteket
    if (p2p_init() != 0) {
        printf("[ERROR] Failed to initialize p2pnet: %s\n", p2p_get_error());
        return 1;
    }
    printf("[OK] Winsock initialized\n");
    
    // Opprett server socket
    p2p_socket_t* server = p2p_socket_create(P2P_TCP);
    if (!server) {
        printf("[ERROR] Failed to create socket: %s\n", p2p_get_error());
        p2p_cleanup();
        return 1;
    }
    printf("[OK] Socket created\n");
    
    // Bind til port (0.0.0.0 = alle interfaces)
    if (p2p_socket_bind(server, "0.0.0.0", 8080) != 0) {
        printf("[ERROR] Failed to bind: %s\n", p2p_get_error());
        printf("        Tip: Port might be in use. Try: netstat -ano | findstr :8080\n");
        p2p_socket_close(server);
        p2p_cleanup();
        return 1;
    }
    printf("[OK] Socket bound to 0.0.0.0:8080\n");
    
    // Start listening
    if (p2p_socket_listen(server, 5) != 0) {
        printf("[ERROR] Failed to listen: %s\n", p2p_get_error());
        p2p_socket_close(server);
        p2p_cleanup();
        return 1;
    }
    printf("[OK] Listening for connections (backlog: 5)\n\n");
    
    printf("--------------------------------------------------------\n");
    printf(" Server is ready!                                      \n");
    printf("                                                       \n");
    printf(" Test with:                                            \n");
    printf("   - build\\02_basic_client.exe (in another terminal)  \n");
    printf("   - echo \"Test\" | nc 127.0.0.1 8080                  \n");
    printf("   - telnet 127.0.0.1 8080                             \n");
    printf("                                                       \n");
    printf(" Press Ctrl+C to stop                                  \n");
    printf("--------------------------------------------------------\n\n");
    
    // Setup signal handler for graceful shutdown
    signal(SIGINT, signal_handler);
    
    int client_count = 0;
    
    // Hovedloop - aksepter flere klienter sekvensielt
    while (keep_running) {
        printf("[WAIT] Waiting for client #%d...\n", client_count + 1);
        
        // Aksepter innkommende tilkobling (blokkerende)
        p2p_socket_t* client = p2p_socket_accept(server);
        
        if (!client) {
            if (keep_running) {  // Ikke vis feil hvis vi shutdown
                printf("[ERROR] Failed to accept: %s\n", p2p_get_error());
            }
            break;
        }
        
        client_count++;
        printf("[OK] Client #%d connected!\n", client_count);
        
        // Motta data fra klient
        char buffer[256];
        ssize_t received = p2p_socket_recv(client, buffer, sizeof(buffer) - 1);
        
        if (received > 0) {
            buffer[received] = '\0';  // Null-terminer string
            
            // Fjern newline hvis den finnes
            if (buffer[received - 1] == '\n') {
                buffer[received - 1] = '\0';
            }
            
            printf("[RECV] Received %zd bytes: \"%s\"\n", received, buffer);
            
            // Generer svar (økt buffer size for å unngå truncation warning)
            char response[512];
            snprintf(response, sizeof(response), 
                     "Hello from server! You are client #%d. Message received: %s\n", 
                     client_count, buffer);
            
            ssize_t sent = p2p_socket_send(client, response, strlen(response));
            
            if (sent > 0) {
                printf("[SEND] Sent %zd bytes back\n", sent);
            } else {
                printf("[ERROR] Failed to send: %s\n", p2p_get_error());
            }
            
        } else if (received == 0) {
            printf("[INFO] Client disconnected before sending data\n");
        } else {
            printf("[ERROR] recv() failed: %s\n", p2p_get_error());
        }
        
        // Lukk klient-socket
        p2p_socket_close(client);
        printf("[CLOSE] Client #%d connection closed\n\n", client_count);
    }
    
    // Cleanup
    printf("\n--------------------------------------------------------\n");
    printf(" Shutting down...                                      \n");
    printf("--------------------------------------------------------\n");
    
    p2p_socket_close(server);
    p2p_cleanup();
    
    printf("[OK] Server shutdown complete\n");
    printf("[STATS] Total clients handled: %d\n", client_count);
    
    return 0;
}