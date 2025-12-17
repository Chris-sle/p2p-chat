#include <p2pnet/p2pnet.h>
#include <stdio.h>
#include <string.h>

int main(int argc, char* argv[]) {
    printf("╔════════════════════════════════════════════════════════╗\n");
    printf("║         P2P Basic Client Example (Enhanced)           ║\n");
    printf("╚════════════════════════════════════════════════════════╝\n\n");
    
    // Parse command line arguments
    const char* server_ip = "127.0.0.1";
    const char* message = "Hello from client!";
    
    if (argc >= 2) {
        server_ip = argv[1];
    }
    if (argc >= 3) {
        message = argv[2];
    }
    
    printf("📋 Configuration:\n");
    printf("   Server: %s:8080\n", server_ip);
    printf("   Message: \"%s\"\n\n", message);
    
    // Initialiser
    if (p2p_init() != 0) {
        printf("❌ ERROR: Failed to initialize: %s\n", p2p_get_error());
        return 1;
    }
    printf("✅ [OK] Initialized\n");
    
    // Opprett socket
    p2p_socket_t* sock = p2p_socket_create(P2P_TCP);
    if (!sock) {
        printf("❌ ERROR: Failed to create socket: %s\n", p2p_get_error());
        p2p_cleanup();
        return 1;
    }
    printf("✅ [OK] Socket created\n");
    
    // Koble til server
    printf("🔌 Connecting to %s:8080...\n", server_ip);
    if (p2p_socket_connect(sock, server_ip, 8080) != 0) {
        printf("❌ ERROR: Failed to connect: %s\n", p2p_get_error());
        printf("\n💡 Tips:\n");
        printf("   - Make sure the server is running first\n");
        printf("   - Check firewall settings\n");
        printf("   - Try: netstat -ano | findstr :8080\n");
        p2p_socket_close(sock);
        p2p_cleanup();
        return 1;
    }
    printf("✅ [OK] Connected to server!\n\n");
    
    // Send melding
    printf("📤 Sending message...\n");
    ssize_t sent = p2p_socket_send(sock, message, strlen(message));
    
    if (sent < 0) {
        printf("❌ ERROR: Failed to send: %s\n", p2p_get_error());
        p2p_socket_close(sock);
        p2p_cleanup();
        return 1;
    }
    printf("✅ [OK] Sent %zd bytes\n", sent);
    
    // Motta svar
    char buffer[512];
    printf("⏳ Waiting for response...\n");
    ssize_t received = p2p_socket_recv(sock, buffer, sizeof(buffer) - 1);
    
    if (received > 0) {
        buffer[received] = '\0';
        printf("📨 [OK] Received %zd bytes:\n", received);
        printf("┌────────────────────────────────────────────────────────┐\n");
        printf("│ Server response:                                       │\n");
        printf("│ %s", buffer);
        if (buffer[received - 1] != '\n') {
            printf("\n");
        }
        printf("└────────────────────────────────────────────────────────┘\n");
    } else if (received == 0) {
        printf("⚠️  Server closed connection\n");
    } else {
        printf("❌ ERROR: recv() failed: %s\n", p2p_get_error());
    }
    
    // Cleanup
    p2p_socket_close(sock);
    p2p_cleanup();
    
    printf("\n✅ [OK] Client shutdown complete\n");
    return 0;
}