#include <p2pnet/p2pnet.h>
#include <stdio.h>
#include <string.h>

int main() {
    printf("=== P2P Basic Client Example ===\n\n");
    
    // Initialiser
    if (p2p_init() != 0) {
        printf("ERROR: Failed to initialize: %s\n", p2p_get_error());
        return 1;
    }
    printf("[OK] Initialized\n");
    
    // Opprett socket
    p2p_socket_t* sock = p2p_socket_create(P2P_TCP);
    if (!sock) {
        printf("ERROR: Failed to create socket: %s\n", p2p_get_error());
        p2p_cleanup();
        return 1;
    }
    printf("[OK] Socket created\n");
    
    // Koble til server
    printf("Connecting to 127.0.0.1:8080...\n");
    if (p2p_socket_connect(sock, "127.0.0.1", 8080) != 0) {
        printf("ERROR: Failed to connect: %s\n", p2p_get_error());
        printf("(Make sure the server is running first!)\n");
        p2p_socket_close(sock);
        p2p_cleanup();
        return 1;
    }
    printf("[OK] Connected to server!\n");
    
    // Send melding
    const char* message = "Hello from client!";
    printf("Sending: \"%s\"\n", message);
    ssize_t sent = p2p_socket_send(sock, message, strlen(message));
    if (sent < 0) {
        printf("ERROR: Failed to send: %s\n", p2p_get_error());
    } else {
        printf("[OK] Sent %zd bytes\n", sent);
    }
    
    // Motta svar
    char buffer[256];
    printf("Waiting for response...\n");
    ssize_t received = p2p_socket_recv(sock, buffer, sizeof(buffer) - 1);
    
    if (received > 0) {
        buffer[received] = '\0';
        printf("[OK] Received: \"%s\"\n", buffer);
    } else if (received == 0) {
        printf("Server closed connection\n");
    } else {
        printf("ERROR: recv() failed: %s\n", p2p_get_error());
    }
    
    // Cleanup
    p2p_socket_close(sock);
    p2p_cleanup();
    
    printf("\n[OK] Client shutdown complete\n");
    return 0;
}