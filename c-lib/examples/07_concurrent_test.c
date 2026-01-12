#include <p2pnet/p2pnet.h>
#include <p2pnet/message.h>
#include <stdio.h>
#include <windows.h>

#define MAX_CLIENTS 10

typedef struct {
    int client_id;
    p2p_socket_t* sock;
    int messages_sent;
    int messages_received;
} client_state_t;

DWORD WINAPI client_thread(LPVOID param) {
    client_state_t* state = (client_state_t*)param;
    
    printf("[CLIENT %d] Connecting...\n", state->client_id);
    
    if (p2p_socket_connect(state->sock, "127.0.0.1", 8080) != 0) {
        printf("[CLIENT %d] Connect failed\n", state->client_id);
        return 1;
    }
    
    printf("[CLIENT %d] Connected!\n", state->client_id);
    
    // Send 5 messages
    for (int i = 0; i < 5; i++) {
        char text[64];
        snprintf(text, sizeof(text), "Message %d from client %d", i + 1, state->client_id);
        
        p2p_message_t* msg = p2p_message_create(text);
        if (p2p_message_send(state->sock, msg) == 0) {
            state->messages_sent++;
        }
        p2p_message_free(msg);
        
        // Motta echo
        p2p_message_t* echo = p2p_message_recv(state->sock);
        if (echo) {
            state->messages_received++;
            p2p_message_free(echo);
        }
        
        Sleep(100);  // Small delay
    }
    
    printf("[CLIENT %d] Done (sent: %d, received: %d)\n", 
           state->client_id, state->messages_sent, state->messages_received);
    
    return 0;
}

int main() {
    printf("========================================================\n");
    printf("    Concurrent Client Stress Test (Milestone 1.3)      \n");
    printf("========================================================\n\n");
    printf("[INFO] Starting %d concurrent clients\n", MAX_CLIENTS);
    printf("[INFO] Each client will send 5 messages\n");
    printf("[INFO] Make sure async server is running!\n\n");
    
    Sleep(2000);  // Wait for user to read
    
    if (p2p_init() != 0) {
        printf("[ERROR] Init failed\n");
        return 1;
    }
    
    client_state_t clients[MAX_CLIENTS];
    HANDLE threads[MAX_CLIENTS];
    
    DWORD start_time = GetTickCount();
    
    // Start all clients
    for (int i = 0; i < MAX_CLIENTS; i++) {
        clients[i].client_id = i + 1;
        clients[i].sock = p2p_socket_create(P2P_TCP);
        clients[i].messages_sent = 0;
        clients[i].messages_received = 0;
        
        threads[i] = CreateThread(NULL, 0, client_thread, &clients[i], 0, NULL);
    }
    
    // Wait for all threads
    WaitForMultipleObjects(MAX_CLIENTS, threads, TRUE, INFINITE);
    
    DWORD end_time = GetTickCount();
    DWORD elapsed = end_time - start_time;
    
    // Cleanup
    for (int i = 0; i < MAX_CLIENTS; i++) {
        CloseHandle(threads[i]);
        p2p_socket_close(clients[i].sock);
    }
    
    p2p_cleanup();
    
    // Statistics
    int total_sent = 0;
    int total_received = 0;
    
    for (int i = 0; i < MAX_CLIENTS; i++) {
        total_sent += clients[i].messages_sent;
        total_received += clients[i].messages_received;
    }
    
    printf("\n========================================================\n");
    printf("                  Test Results                          \n");
    printf("========================================================\n");
    printf("[OK] Clients:           %d\n", MAX_CLIENTS);
    printf("[OK] Total sent:        %d\n", total_sent);
    printf("[OK] Total received:    %d\n", total_received);
    printf("[OK] Success rate:      %.1f%%\n", 
           (total_received * 100.0) / total_sent);
    printf("[TIME] Total time:      %lu ms\n", elapsed);
    printf("[TIME] Avg per message: %.2f ms\n", 
           (float)elapsed / total_sent);
    
    return (total_received == total_sent) ? 0 : 1;
}