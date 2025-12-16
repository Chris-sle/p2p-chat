#include <p2pnet/p2pnet.h>
#include <stdio.h>
#include <string.h>

int main() {
    printf("=== P2P Basic Server Example ===\n\n");
    
    // Initialiser biblioteket
    if (p2p_init() != 0) {
        printf("ERROR: Failed to initialize p2pnet: %s\n", p2p_get_error());
        return 1;
    }
    printf("[OK] Winsock initialized\n");
    
    // Opprett server socket
    p2p_socket_t* server = p2p_socket_create(P2P_TCP);
    if (!server) {
        printf("ERROR: Failed to create socket: %s\n", p2p_get_error());
        p2p_cleanup();
        return 1;
    }
    printf("[OK] Socket created\n");
    
    // Bind til port
    if (p2p_socket_bind(server, "127.0.0.1", 8080) != 0) {
        printf("ERROR: Failed to bind: %s\n", p2p_get_error());
        p2p_socket_close(server);
        p2p_cleanup();
        return 1;
    }
    printf("[OK] Socket bound to 127.0.0.1:8080\n");
    
    // Start listening
    if (p2p_socket_listen(server, 5) != 0) {
        printf("ERROR: Failed to listen: %s\n", p2p_get_error());
        p2p_socket_close(server);
        p2p_cleanup();
        return 1;
    }
    printf("[OK] Listening for connections...\n\n");
    printf("Waiting for client (test with: nc 127.0.0.1 8080)\n");
    printf("Press Ctrl+C to stop\n\n");
    
    // Aksepter én klient
    p2p_socket_t* client = p2p_socket_accept(server);
    if (!client) {
        printf("ERROR: Failed to accept: %s\n", p2p_get_error());
        p2p_socket_close(server);
        p2p_cleanup();
        return 1;
    }
    printf("[OK] Client connected!\n");
    
    // Motta data
    char buffer[256];
    ssize_t received = p2p_socket_recv(client, buffer, sizeof(buffer) - 1);
    
    if (received > 0) {
        buffer[received] = '\0';  // Null-terminer string
        printf("Received %zd bytes: \"%s\"\n", received, buffer);
        
        // Send svar
        const char* response = "Hello from server!\n";
        ssize_t sent = p2p_socket_send(client, response, strlen(response));
        printf("Sent %zd bytes back\n", sent);
    } else if (received == 0) {
        printf("Client disconnected\n");
    } else {
        printf("ERROR: recv() failed: %s\n", p2p_get_error());
    }
    
    // Cleanup
    p2p_socket_close(client);
    p2p_socket_close(server);
    p2p_cleanup();
    
    printf("\n[OK] Server shutdown complete\n");
    return 0;
}