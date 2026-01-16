#include "minunit.h"
#include <p2pnet/p2pnet.h>

MU_TEST(test_socket_create) {
    p2p_init();
    
    p2p_socket_t* sock = p2p_socket_create(P2P_TCP);
    mu_check(sock != NULL);
    
    p2p_socket_close(sock);
    p2p_cleanup();
    
    return NULL;  // ← Legg til
}

MU_TEST(test_socket_bind) {
    p2p_init();
    
    p2p_socket_t* sock = p2p_socket_create(P2P_TCP);
    mu_check(sock != NULL);
    
    // Bind to random port
    int result = p2p_socket_bind(sock, "127.0.0.1", 0);
    mu_check(result == 0);
    
    p2p_socket_close(sock);
    p2p_cleanup();
    
    return NULL;  // ← Legg til
}

MU_TEST(test_socket_listen) {
    p2p_init();
    
    p2p_socket_t* sock = p2p_socket_create(P2P_TCP);
    p2p_socket_bind(sock, "127.0.0.1", 0);
    
    int result = p2p_socket_listen(sock, 5);
    mu_check(result == 0);
    
    p2p_socket_close(sock);
    p2p_cleanup();
    
    return NULL;  // ← Legg til
}

MU_TEST_SUITE(socket_suite) {
    MU_RUN_TEST(test_socket_create);
    MU_RUN_TEST(test_socket_bind);
    MU_RUN_TEST(test_socket_listen);
    return NULL;  // ← Legg til
}

int main() {
    printf("========================================\n");
    printf(" Running Socket Tests (Milestone 1.1)  \n");
    printf("========================================\n\n");
    
    MU_RUN_SUITE(socket_suite);
    MU_REPORT();
    
    return MU_EXIT_CODE;
}