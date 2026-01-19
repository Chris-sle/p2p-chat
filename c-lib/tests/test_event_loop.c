#include "minunit.h"
#include <p2pnet/p2pnet.h>
#include <string.h>

// Global state for testing
static int callback_invoked = 0;
static int error_callback_invoked = 0;

// Test callbacks
void test_read_callback(p2p_socket_t* sock, void* user_data) {
    (void)sock;  // Unused
    (void)user_data;  // Unused
    callback_invoked = 1;
}

void test_error_callback(p2p_socket_t* sock, int error, void* user_data) {
    (void)sock;  // Unused
    (void)error;  // Unused
    (void)user_data;  // Unused
    error_callback_invoked = 1;
}

// ============================================================================
// Test 1: Create and Free Event Loop
// ============================================================================

MU_TEST(test_event_loop_create) {
    p2p_event_loop_t* loop = p2p_event_loop_create();
    
    mu_check(loop != NULL);
    
    // Verify initial state
    mu_check(p2p_event_loop_socket_count(loop) == 0);
    
    p2p_event_loop_free(loop);
    return NULL;
}

// ============================================================================
// Test 2: Add Socket to Event Loop
// ============================================================================

MU_TEST(test_event_loop_add_socket) {
    p2p_init();
    
    p2p_event_loop_t* loop = p2p_event_loop_create();
    mu_check(loop != NULL);
    
    // Create server socket
    p2p_socket_t* server = p2p_socket_create(P2P_TCP);
    mu_check(server != NULL);
    
    p2p_socket_bind(server, "127.0.0.1", 0);
    p2p_socket_listen(server, 5);
    
    // Add to event loop
    int result = p2p_event_loop_add_socket(loop, server, 
                                            test_read_callback, 
                                            test_error_callback, 
                                            NULL);
    mu_check(result == 0);
    
    // Verify socket was added
    mu_check(p2p_event_loop_socket_count(loop) == 1);
    
    // Cleanup
    p2p_event_loop_free(loop);
    p2p_socket_close(server);
    p2p_cleanup();
    
    return NULL;
}

// ============================================================================
// Test 3: Add Multiple Sockets
// ============================================================================

MU_TEST(test_event_loop_add_multiple_sockets) {
    p2p_init();
    
    p2p_event_loop_t* loop = p2p_event_loop_create();
    mu_check(loop != NULL);
    
    // Create 3 server sockets
    p2p_socket_t* server1 = p2p_socket_create(P2P_TCP);
    p2p_socket_t* server2 = p2p_socket_create(P2P_TCP);
    p2p_socket_t* server3 = p2p_socket_create(P2P_TCP);
    
    mu_check(server1 != NULL);
    mu_check(server2 != NULL);
    mu_check(server3 != NULL);
    
    // Bind to different ports
    p2p_socket_bind(server1, "127.0.0.1", 0);
    p2p_socket_bind(server2, "127.0.0.1", 0);
    p2p_socket_bind(server3, "127.0.0.1", 0);
    
    p2p_socket_listen(server1, 5);
    p2p_socket_listen(server2, 5);
    p2p_socket_listen(server3, 5);
    
    // Add all to event loop
    mu_check(p2p_event_loop_add_socket(loop, server1, test_read_callback, 
                                        test_error_callback, NULL) == 0);
    mu_check(p2p_event_loop_add_socket(loop, server2, test_read_callback, 
                                        test_error_callback, NULL) == 0);
    mu_check(p2p_event_loop_add_socket(loop, server3, test_read_callback, 
                                        test_error_callback, NULL) == 0);
    
    // Verify count
    mu_check(p2p_event_loop_socket_count(loop) == 3);
    
    // Cleanup
    p2p_event_loop_free(loop);
    p2p_socket_close(server1);
    p2p_socket_close(server2);
    p2p_socket_close(server3);
    p2p_cleanup();
    
    return NULL;
}

// ============================================================================
// Test 4: Remove Socket from Event Loop
// ============================================================================

MU_TEST(test_event_loop_remove_socket) {
    p2p_init();
    
    p2p_event_loop_t* loop = p2p_event_loop_create();
    mu_check(loop != NULL);
    
    // Create server socket
    p2p_socket_t* server = p2p_socket_create(P2P_TCP);
    mu_check(server != NULL);
    
    p2p_socket_bind(server, "127.0.0.1", 0);
    p2p_socket_listen(server, 5);
    
    // Add to event loop
    p2p_event_loop_add_socket(loop, server, test_read_callback, 
                              test_error_callback, NULL);
    mu_check(p2p_event_loop_socket_count(loop) == 1);
    
    // Remove from event loop
    int result = p2p_event_loop_remove_socket(loop, server);
    mu_check(result == 0);
    mu_check(p2p_event_loop_socket_count(loop) == 0);
    
    // Cleanup
    p2p_event_loop_free(loop);
    p2p_socket_close(server);
    p2p_cleanup();
    
    return NULL;
}

// ============================================================================
// Test 5: Remove Non-Existent Socket (Should Fail)
// ============================================================================

MU_TEST(test_event_loop_remove_nonexistent) {
    p2p_init();
    
    p2p_event_loop_t* loop = p2p_event_loop_create();
    mu_check(loop != NULL);
    
    // Create socket but don't add to loop
    p2p_socket_t* server = p2p_socket_create(P2P_TCP);
    mu_check(server != NULL);
    
    // Try to remove (should fail)
    int result = p2p_event_loop_remove_socket(loop, server);
    mu_check(result == -1);  // Should fail
    
    // Cleanup
    p2p_event_loop_free(loop);
    p2p_socket_close(server);
    p2p_cleanup();
    
    return NULL;
}

// ============================================================================
// Test 6: Socket Count Accuracy
// ============================================================================

MU_TEST(test_event_loop_socket_count) {
    p2p_init();
    
    p2p_event_loop_t* loop = p2p_event_loop_create();
    mu_check(loop != NULL);
    
    // Initially empty
    mu_check(p2p_event_loop_socket_count(loop) == 0);
    
    // Add sockets one by one
    p2p_socket_t* s1 = p2p_socket_create(P2P_TCP);
    p2p_socket_bind(s1, "127.0.0.1", 0);
    p2p_socket_listen(s1, 5);
    p2p_event_loop_add_socket(loop, s1, test_read_callback, 
                              test_error_callback, NULL);
    mu_check(p2p_event_loop_socket_count(loop) == 1);
    
    p2p_socket_t* s2 = p2p_socket_create(P2P_TCP);
    p2p_socket_bind(s2, "127.0.0.1", 0);
    p2p_socket_listen(s2, 5);
    p2p_event_loop_add_socket(loop, s2, test_read_callback, 
                              test_error_callback, NULL);
    mu_check(p2p_event_loop_socket_count(loop) == 2);
    
    // Remove one
    p2p_event_loop_remove_socket(loop, s1);
    mu_check(p2p_event_loop_socket_count(loop) == 1);
    
    // Remove the other
    p2p_event_loop_remove_socket(loop, s2);
    mu_check(p2p_event_loop_socket_count(loop) == 0);
    
    // Cleanup
    p2p_event_loop_free(loop);
    p2p_socket_close(s1);
    p2p_socket_close(s2);
    p2p_cleanup();
    
    return NULL;
}

// ============================================================================
// Test 7: NULL Safety (Edge Cases)
// ============================================================================

MU_TEST(test_event_loop_null_safety) {
    // Free NULL loop (should not crash)
    p2p_event_loop_free(NULL);
    
    // Socket count on NULL loop
    mu_check(p2p_event_loop_socket_count(NULL) == -1);
    
    // Remove from NULL loop
    p2p_init();
    p2p_socket_t* server = p2p_socket_create(P2P_TCP);
    mu_check(p2p_event_loop_remove_socket(NULL, server) == -1);
    p2p_socket_close(server);
    p2p_cleanup();
    
    return NULL;
}

// ============================================================================
// Test 8: Add Same Socket Twice (Should Fail)
// ============================================================================

MU_TEST(test_event_loop_add_duplicate) {
    p2p_init();
    
    p2p_event_loop_t* loop = p2p_event_loop_create();
    mu_check(loop != NULL);
    
    p2p_socket_t* server = p2p_socket_create(P2P_TCP);
    p2p_socket_bind(server, "127.0.0.1", 0);
    p2p_socket_listen(server, 5);
    
    // Add first time (should succeed)
    int result1 = p2p_event_loop_add_socket(loop, server, test_read_callback, 
                                             test_error_callback, NULL);
    mu_check(result1 == 0);
    mu_check(p2p_event_loop_socket_count(loop) == 1);
    
    // Add second time (should fail)
    int result2 = p2p_event_loop_add_socket(loop, server, test_read_callback, 
                                             test_error_callback, NULL);
    mu_check(result2 == -1);  // Should fail
    mu_check(p2p_event_loop_socket_count(loop) == 1);  // Count unchanged
    
    // Cleanup
    p2p_event_loop_free(loop);
    p2p_socket_close(server);
    p2p_cleanup();
    
    return NULL;
}

// ============================================================================
// Test Suite
// ============================================================================

MU_TEST_SUITE(event_loop_suite) {
    MU_RUN_TEST(test_event_loop_create);
    MU_RUN_TEST(test_event_loop_add_socket);
    MU_RUN_TEST(test_event_loop_add_multiple_sockets);
    MU_RUN_TEST(test_event_loop_remove_socket);
    MU_RUN_TEST(test_event_loop_remove_nonexistent);
    MU_RUN_TEST(test_event_loop_socket_count);
    MU_RUN_TEST(test_event_loop_null_safety);
    MU_RUN_TEST(test_event_loop_add_duplicate);
    return NULL;
}

int main() {
    printf("========================================\n");
    printf(" Running Event Loop Tests (M 1.3)      \n");
    printf("========================================\n\n");
    
    MU_RUN_SUITE(event_loop_suite);
    MU_REPORT();
    
    return MU_EXIT_CODE;
}