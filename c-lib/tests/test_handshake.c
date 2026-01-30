#include "minunit.h"
#include <p2pnet/p2pnet.h>
#include <string.h>
#include <stdlib.h>

#ifdef _WIN32
    #include <windows.h>
    #include <process.h>
    #define THREAD_RETURN unsigned int __stdcall
    #define THREAD_HANDLE HANDLE
#else
    #include <pthread.h>
    #define THREAD_RETURN void*
    #define THREAD_HANDLE pthread_t
#endif

// Test configuration
#define TEST_PORT 9999

// Global test state
static p2p_keypair_t* server_keypair = NULL;
static p2p_keypair_t* client_keypair = NULL;
static p2p_session_t* server_session = NULL;
static int server_ready = 0;

// ============================================================================
// Helper: Thread function for server
// ============================================================================

THREAD_RETURN server_thread_func(void* arg) {
    (void)arg;
    
    p2p_init();
    
    // Create listening socket
    p2p_socket_t* listen_sock = p2p_socket_create(P2P_TCP);
    if (!listen_sock) {
        return 0;
    }
    
    p2p_socket_bind(listen_sock, "127.0.0.1", TEST_PORT);
    p2p_socket_listen(listen_sock, 1);
    
    // Signal ready
    server_ready = 1;
    
    // Accept client
    p2p_socket_t* client_sock = p2p_socket_accept(listen_sock);
    if (!client_sock) {
        p2p_socket_close(listen_sock);
        p2p_cleanup();
        return 0;
    }
    
    // Perform handshake (accept any peer)
    // Note: This may fail if client disconnects early
    server_session = p2p_handshake_server(client_sock, server_keypair, NULL, 0);
    
    // Don't fail if handshake fails (test expects this)
    if (server_session) {
        // Keep connection open briefly
        #ifdef _WIN32
            Sleep(1000);
        #else
            sleep(1);
        #endif
    } else {
        // Handshake failed (expected in some tests)
        // Just continue cleanup
    }
    
    // Cleanup
    p2p_socket_close(client_sock);
    p2p_socket_close(listen_sock);
    p2p_cleanup();
    
    return 0;
}

// ============================================================================
// Helper: Start server thread
// ============================================================================

static THREAD_HANDLE start_server_thread() {
    server_ready = 0;
    
    #ifdef _WIN32
        return (HANDLE)_beginthreadex(NULL, 0, server_thread_func, NULL, 0, NULL);
    #else
        THREAD_HANDLE thread;
        pthread_create(&thread, NULL, server_thread_func, NULL);
        return thread;
    #endif
}

// ============================================================================
// Helper: Wait for thread
// ============================================================================

static void wait_for_thread(THREAD_HANDLE thread) {
    #ifdef _WIN32
        WaitForSingleObject(thread, INFINITE);
        CloseHandle(thread);
    #else
        pthread_join(thread, NULL);
    #endif
}

// ============================================================================
// Test 1: Generate keypairs
// ============================================================================

MU_TEST(test_handshake_setup) {
    // Generate server keypair
    server_keypair = p2p_keypair_generate();
    mu_check(server_keypair != NULL);
    
    // Generate client keypair
    client_keypair = p2p_keypair_generate();
    mu_check(client_keypair != NULL);
    
    // Verify they're different
    mu_check(memcmp(server_keypair->public_key, client_keypair->public_key, 32) != 0);
    
    return NULL;
}

// ============================================================================
// Test 2: Basic handshake (client accepts any server)
// ============================================================================

MU_TEST(test_handshake_basic) {
    p2p_init();
    
    // Start server thread
    THREAD_HANDLE server_thread = start_server_thread();
    
    // Wait for server to be ready
    while (!server_ready) {
        #ifdef _WIN32
            Sleep(10);
        #else
            usleep(10000);
        #endif
    }
    
    // Client connects
    p2p_socket_t* client_sock = p2p_socket_create(P2P_TCP);
    mu_check(client_sock != NULL);
    
    int connect_result = p2p_socket_connect(client_sock, "127.0.0.1", TEST_PORT);
    mu_check(connect_result == 0);
    
    // Client performs handshake (accept any server)
    p2p_session_t* client_session = p2p_handshake_client(client_sock, 
                                                          client_keypair, 
                                                          NULL);
    mu_check(client_session != NULL);
    
    // Verify peer identity
    const uint8_t* peer_pubkey = p2p_session_peer_pubkey(client_session);
    mu_check(peer_pubkey != NULL);
    mu_check(memcmp(peer_pubkey, server_keypair->public_key, 32) == 0);
    
    // Wait for server
    wait_for_thread(server_thread);
    
    // Verify server session was created
    mu_check(server_session != NULL);
    
    // Verify server saw client identity
    const uint8_t* server_saw = p2p_session_peer_pubkey(server_session);
    mu_check(server_saw != NULL);
    mu_check(memcmp(server_saw, client_keypair->public_key, 32) == 0);
    
    // Cleanup
    p2p_session_free(client_session);
    p2p_session_free(server_session);
    server_session = NULL;
    p2p_socket_close(client_sock);
    p2p_cleanup();
    
    return NULL;
}

// ============================================================================
// Test 3: Handshake with expected peer (should succeed)
// ============================================================================

MU_TEST(test_handshake_expected_peer_match) {
    p2p_init();
    
    // Start server thread
    THREAD_HANDLE server_thread = start_server_thread();
    
    // Wait for server to be ready
    while (!server_ready) {
        #ifdef _WIN32
            Sleep(10);
        #else
            usleep(10000);
        #endif
    }
    
    // Client connects expecting specific server
    p2p_socket_t* client_sock = p2p_socket_create(P2P_TCP);
    mu_check(client_sock != NULL);
    
    p2p_socket_connect(client_sock, "127.0.0.1", TEST_PORT);
    
    // Handshake with expected peer (should succeed)
    p2p_session_t* client_session = p2p_handshake_client(client_sock,
                                                          client_keypair,
                                                          server_keypair->public_key);
    mu_check(client_session != NULL);
    
    // Wait for server
    wait_for_thread(server_thread);
    
    // Cleanup
    p2p_session_free(client_session);
    p2p_session_free(server_session);
    server_session = NULL;
    p2p_socket_close(client_sock);
    p2p_cleanup();
    
    return NULL;
}

// ============================================================================
// Test 4: Handshake with wrong expected peer (should fail)
// ============================================================================

MU_TEST(test_handshake_expected_peer_mismatch) {
    p2p_init();
    
    // Generate a fake server identity
    p2p_keypair_t* fake_server = p2p_keypair_generate();
    mu_check(fake_server != NULL);
    
    // Start server thread (with real server identity)
    THREAD_HANDLE server_thread = start_server_thread();
    
    // Wait for server to be ready
    while (!server_ready) {
        #ifdef _WIN32
            Sleep(10);
        #else
            usleep(10000);
        #endif
    }
    
    // Client connects expecting fake server
    p2p_socket_t* client_sock = p2p_socket_create(P2P_TCP);
    mu_check(client_sock != NULL);
    
    p2p_socket_connect(client_sock, "127.0.0.1", TEST_PORT);
    
    // Handshake with wrong expected peer (should fail)
    p2p_session_t* client_session = p2p_handshake_client(client_sock,
                                                          client_keypair,
                                                          fake_server->public_key);
    mu_check(client_session == NULL);  // Should fail!
    
    // Close socket immediately so server sees disconnect
    p2p_socket_close(client_sock);
    
    // Give server time to detect disconnect
    #ifdef _WIN32
        Sleep(100);
    #else
        usleep(100000);
    #endif

    // Wait for server
    wait_for_thread(server_thread);
    
    // Cleanup
    p2p_keypair_free(fake_server);
    p2p_cleanup();
    
    return NULL;
}

// ============================================================================
// Test 5: Session key derivation (both sides should have same key)
// ============================================================================

MU_TEST(test_handshake_session_key) {
    p2p_init();
    
    // Start server thread
    THREAD_HANDLE server_thread = start_server_thread();
    
    // Wait for server to be ready
    while (!server_ready) {
        #ifdef _WIN32
            Sleep(10);
        #else
            usleep(10000);
        #endif
    }
    
    // Client connects
    p2p_socket_t* client_sock = p2p_socket_create(P2P_TCP);
    mu_check(client_sock != NULL);
    
    p2p_socket_connect(client_sock, "127.0.0.1", TEST_PORT);
    
    // Perform handshake
    p2p_session_t* client_session = p2p_handshake_client(client_sock,
                                                          client_keypair,
                                                          NULL);
    mu_check(client_session != NULL);
    
    // Wait for server
    wait_for_thread(server_thread);
    mu_check(server_session != NULL);
    
    // Both sides should have derived same session key
    const uint8_t* client_key = p2p_session_key(client_session);
    const uint8_t* server_key = p2p_session_key(server_session);
    
    mu_check(client_key != NULL);
    mu_check(server_key != NULL);
    mu_check(memcmp(client_key, server_key, 32) == 0);
    
    // Cleanup
    p2p_session_free(client_session);
    p2p_session_free(server_session);
    server_session = NULL;
    p2p_socket_close(client_sock);
    p2p_cleanup();
    
    return NULL;
}

// ============================================================================
// Test 6: Session fingerprint
// ============================================================================

MU_TEST(test_session_fingerprint) {
    p2p_init();
    
    // Start server thread
    THREAD_HANDLE server_thread = start_server_thread();
    
    // Wait for server
    while (!server_ready) {
        #ifdef _WIN32
            Sleep(10);
        #else
            usleep(10000);
        #endif
    }
    
    // Client connects
    p2p_socket_t* client_sock = p2p_socket_create(P2P_TCP);
    p2p_socket_connect(client_sock, "127.0.0.1", TEST_PORT);
    
    p2p_session_t* client_session = p2p_handshake_client(client_sock,
                                                          client_keypair,
                                                          NULL);
    mu_check(client_session != NULL);
    
    // Get peer fingerprint
    char peer_fp[64];
    const char* result = p2p_session_peer_fingerprint(client_session, 
                                                       peer_fp, 
                                                       sizeof(peer_fp));
    mu_check(result != NULL);
    mu_check(strlen(peer_fp) == 43);  // Base64 of 32 bytes
    
    // Wait for server
    wait_for_thread(server_thread);
    
    // Cleanup
    p2p_session_free(client_session);
    p2p_session_free(server_session);
    server_session = NULL;
    p2p_socket_close(client_sock);
    p2p_cleanup();
    
    return NULL;
}

// ============================================================================
// Test 7: Cleanup
// ============================================================================

MU_TEST(test_handshake_cleanup) {
    // Free keypairs
    p2p_keypair_free(server_keypair);
    p2p_keypair_free(client_keypair);
    server_keypair = NULL;
    client_keypair = NULL;
    
    return NULL;
}

// ============================================================================
// Test Suite
// ============================================================================

MU_TEST_SUITE(handshake_suite) {
    MU_RUN_TEST(test_handshake_setup);
    MU_RUN_TEST(test_handshake_basic);
    MU_RUN_TEST(test_handshake_expected_peer_match);
    MU_RUN_TEST(test_handshake_expected_peer_mismatch);
    MU_RUN_TEST(test_handshake_session_key);
    MU_RUN_TEST(test_session_fingerprint);
    MU_RUN_TEST(test_handshake_cleanup);
    return NULL;
}

int main() {
    printf("========================================\n");
    printf(" Running Handshake Tests (M 2.2)       \n");
    printf("========================================\n\n");
    
    MU_RUN_SUITE(handshake_suite);
    MU_REPORT();
    
    return MU_EXIT_CODE;
}