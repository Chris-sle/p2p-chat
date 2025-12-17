#include <p2pnet/p2pnet.h>
#include <stdio.h>
#include <string.h>
#include <windows.h>

// Test hvor mange sekvenielle connections serveren kan håndtere
int main(int argc, char* argv[]) {
    int num_connections = 10;
    
    if (argc >= 2) {
        num_connections = atoi(argv[1]);
        if (num_connections <= 0) num_connections = 10;
    }
    
    printf("╔════════════════════════════════════════════════════════╗\n");
    printf("║              Stress Test - Sequential Clients         ║\n");
    printf("╚════════════════════════════════════════════════════════╝\n\n");
    printf("📋 Testing %d sequential connections\n", num_connections);
    printf("   (Make sure server is running!)\n\n");
    
    if (p2p_init() != 0) {
        printf("❌ ERROR: Init failed\n");
        return 1;
    }
    
    int success_count = 0;
    int fail_count = 0;
    
    DWORD start_time = GetTickCount();
    
    for (int i = 0; i < num_connections; i++) {
        printf("[%3d/%3d] ", i + 1, num_connections);
        
        p2p_socket_t* sock = p2p_socket_create(P2P_TCP);
        if (!sock) {
            printf("❌ Create failed\n");
            fail_count++;
            continue;
        }
        
        if (p2p_socket_connect(sock, "127.0.0.1", 8080) != 0) {
            printf("❌ Connect failed\n");
            p2p_socket_close(sock);
            fail_count++;
            continue;
        }
        
        char message[64];
        snprintf(message, sizeof(message), "Test message #%d", i + 1);
        
        if (p2p_socket_send(sock, message, strlen(message)) < 0) {
            printf("❌ Send failed\n");
            p2p_socket_close(sock);
            fail_count++;
            continue;
        }
        
        char buffer[256];
        ssize_t received = p2p_socket_recv(sock, buffer, sizeof(buffer) - 1);
        
        if (received > 0) {
            printf("✅ OK (sent %zu bytes, received %zd bytes)\n", 
                   strlen(message), received);
            success_count++;
        } else {
            printf("❌ Receive failed\n");
            fail_count++;
        }
        
        p2p_socket_close(sock);
        
        // Small delay mellom connections
        Sleep(50);
    }
    
    DWORD end_time = GetTickCount();
    DWORD elapsed = end_time - start_time;
    
    p2p_cleanup();
    
    printf("\n╔════════════════════════════════════════════════════════╗\n");
    printf("║                    Test Results                        ║\n");
    printf("╚════════════════════════════════════════════════════════╝\n");
    printf("✅ Successful: %d/%d\n", success_count, num_connections);
    printf("❌ Failed:     %d/%d\n", fail_count, num_connections);
    printf("⏱️  Time:       %lu ms\n", elapsed);
    printf("📊 Avg time:   %.2f ms per connection\n", 
           (float)elapsed / num_connections);
    
    return (fail_count > 0) ? 1 : 0;
}