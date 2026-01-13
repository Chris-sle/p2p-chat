#include <p2pnet/p2pnet.h>
#include <p2pnet/message.h>
#include <p2pnet/event_loop.h>
#include <stdio.h>
#include <signal.h>

// Callback deklarasjoner
void on_server_accept(p2p_socket_t* server_sock, void* user_data);
void on_client_data(p2p_socket_t* client, void* user_data);
void on_client_error(p2p_socket_t* client, int error, void* user_data);

// Global event loop for signal handler
static p2p_event_loop_t* g_loop = NULL;

void signal_handler(int signum) {
    (void)signum;
    printf("\n[INFO] Shutting down...\n");
    if (g_loop) {
        p2p_event_loop_stop(g_loop);
    }
}

// Callback når server socket har ny connection
void on_server_accept(p2p_socket_t* server_sock, void* user_data) {
    p2p_event_loop_t* loop = (p2p_event_loop_t*)user_data;
    
    p2p_socket_t* client = p2p_socket_accept(server_sock);
    if (!client) {
        printf("[ERROR] Failed to accept client\n");
        return;
    }
    
    printf("[OK] New client connected (total: %d)\n", 
           p2p_event_loop_socket_count(loop));
    
    // Legg til client i event loop
    p2p_event_loop_add_socket(loop, client, on_client_data, on_client_error, loop);
}

// Callback når client har data
void on_client_data(p2p_socket_t* client, void* user_data) {
    p2p_event_loop_t* loop = (p2p_event_loop_t*)user_data;
    
    p2p_message_t* msg = p2p_message_recv(client);
    
    if (!msg) {
        // Connection closed gracefully
        printf("[INFO] Client disconnected gracefully (total: %d)\n",
               p2p_event_loop_socket_count(loop) - 1);
        p2p_event_loop_remove_socket(loop, client);
        p2p_socket_close(client);
        return;
    }
    
    p2p_message_print(msg, "[RECV] ");
    
    // Echo back
    if (p2p_message_send(client, msg) == 0) {
        printf("[SEND] Echoed message back\n");
    } else {
        printf("[ERROR] Failed to send echo\n");
    }
    
    p2p_message_free(msg);
}

// Callback ved client error
void on_client_error(p2p_socket_t* client, int error, void* user_data) {
    p2p_event_loop_t* loop = (p2p_event_loop_t*)user_data;
    
    printf("[ERROR] Client error: %d (disconnecting)\n", error);
    
    p2p_event_loop_remove_socket(loop, client);
    p2p_socket_close(client);
}

int main() {
    printf("========================================================\n");
    printf("    Async Event Loop Server (Milestone 1.3)            \n");
    printf("========================================================\n\n");
    
    if (p2p_init() != 0) {
        printf("[ERROR] Init failed\n");
        return 1;
    }
    
    // Opprett server socket
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
    
    if (p2p_socket_listen(server, 10) != 0) {
        printf("[ERROR] Listen failed\n");
        p2p_socket_close(server);
        p2p_cleanup();
        return 1;
    }
    
    printf("[OK] Server listening on port 8080\n");
    printf("[OK] Using async event loop (WSAPoll)\n");
    printf("[OK] Can handle multiple concurrent clients\n\n");
    printf("Test with: build\\05_framed_client.exe\n");
    printf("Press Ctrl+C to stop\n\n");
    
    // Opprett event loop
    p2p_event_loop_t* loop = p2p_event_loop_create();
    if (!loop) {
        printf("[ERROR] Failed to create event loop\n");
        p2p_socket_close(server);
        p2p_cleanup();
        return 1;
    }
    
    g_loop = loop;
    signal(SIGINT, signal_handler);
    
    // Legg til server socket i event loop
    p2p_event_loop_add_socket(loop, server, on_server_accept, NULL, loop);
    
    // Kjør event loop (blokkerer her)
    p2p_event_loop_run(loop);
    
    // Cleanup
    p2p_event_loop_free(loop);
    p2p_socket_close(server);
    p2p_cleanup();
    
    printf("\n[OK] Server shutdown complete\n");
    return 0;
}