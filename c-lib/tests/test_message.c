#include "minunit.h"
#include <p2pnet/p2pnet.h>
#include <string.h>

MU_TEST(test_message_create) {
    const char* data = "Hello World";
    p2p_message_t* msg = p2p_message_create(data);
    
    mu_check(msg != NULL);
    mu_check(msg->length == strlen(data));
    mu_check(memcmp(msg->data, data, strlen(data)) == 0);
    
    p2p_message_free(msg);
    return NULL;
}

MU_TEST(test_message_create_empty_returns_null) {
    // Empty string should return NULL
    p2p_message_t* msg = p2p_message_create("");
    mu_check(msg == NULL);  // ← Expect NULL
    return NULL;
}

MU_TEST(test_message_create_single_char) {
    const char* data = "X";
    p2p_message_t* msg = p2p_message_create(data);
    
    mu_check(msg != NULL);
    mu_check(msg->length == 1);
    mu_check(msg->data[0] == 'X');
    
    p2p_message_free(msg);
    return NULL;
}

MU_TEST(test_message_create_large) {
    // Test large message (1KB)
    char data[1024];
    memset(data, 'A', sizeof(data) - 1);
    data[sizeof(data) - 1] = '\0';  // Null-terminate
    
    p2p_message_t* msg = p2p_message_create(data);
    
    mu_check(msg != NULL);
    mu_check(msg->length == strlen(data));
    mu_check(memcmp(msg->data, data, msg->length) == 0);
    
    p2p_message_free(msg);
    return NULL;
}

MU_TEST_SUITE(message_suite) {
    MU_RUN_TEST(test_message_create);
    MU_RUN_TEST(test_message_create_empty_returns_null);
    MU_RUN_TEST(test_message_create_single_char);
    MU_RUN_TEST(test_message_create_large);
    return NULL;
}

int main() {
    printf("========================================\n");
    printf(" Running Message Tests (Milestone 1.2) \n");
    printf("========================================\n\n");
    
    MU_RUN_SUITE(message_suite);
    MU_REPORT();
    
    return MU_EXIT_CODE;
}