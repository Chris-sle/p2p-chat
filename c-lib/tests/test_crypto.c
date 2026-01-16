#include "minunit.h"
#include <p2pnet/p2pnet.h>
#include <string.h>

MU_TEST(test_keypair_generate) {
    p2p_keypair_t* kp = p2p_keypair_generate();
    
    mu_check(kp != NULL);
    mu_check(p2p_keypair_verify(kp) == 0);
    
    p2p_keypair_free(kp);
    return NULL;  // ← Legg til
}

MU_TEST(test_keypair_save_load) {
    // Generate
    p2p_keypair_t* kp1 = p2p_keypair_generate();
    mu_check(kp1 != NULL);
    
    // Save
    const char* filepath = "test_identity.key";
    mu_check(p2p_keypair_save(kp1, filepath) == 0);
    
    // Load
    p2p_keypair_t* kp2 = p2p_keypair_load(filepath);
    mu_check(kp2 != NULL);
    
    // Compare
    mu_check(memcmp(kp1->public_key, kp2->public_key, 32) == 0);
    mu_check(memcmp(kp1->secret_key, kp2->secret_key, 64) == 0);
    
    p2p_keypair_free(kp1);
    p2p_keypair_free(kp2);
    
    // Cleanup
    remove(filepath);
    return NULL;  // ← Legg til
}

MU_TEST(test_keypair_fingerprint) {
    p2p_keypair_t* kp = p2p_keypair_generate();
    mu_check(kp != NULL);
    
    char fp[64];
    p2p_keypair_fingerprint(kp, fp, sizeof(fp));
    
    // Base64 of 32 bytes = 43 chars (no padding)
    mu_check(strlen(fp) == 43);
    
    p2p_keypair_free(kp);
    return NULL;  // ← Legg til
}

MU_TEST(test_keypair_uniqueness) {
    p2p_keypair_t* kp1 = p2p_keypair_generate();
    p2p_keypair_t* kp2 = p2p_keypair_generate();
    
    mu_check(kp1 != NULL);
    mu_check(kp2 != NULL);
    
    // Public keys MUST be different
    mu_check(memcmp(kp1->public_key, kp2->public_key, 32) != 0);
    
    p2p_keypair_free(kp1);
    p2p_keypair_free(kp2);
    return NULL;  // ← Legg til
}

MU_TEST_SUITE(crypto_suite) {
    MU_RUN_TEST(test_keypair_generate);
    MU_RUN_TEST(test_keypair_save_load);
    MU_RUN_TEST(test_keypair_fingerprint);
    MU_RUN_TEST(test_keypair_uniqueness);
    return NULL;  // ← Legg til
}

int main() {
    printf("========================================\n");
    printf(" Running Crypto Tests (Milestone 2.1)  \n");
    printf("========================================\n\n");
    
    MU_RUN_SUITE(crypto_suite);
    MU_REPORT();
    
    return MU_EXIT_CODE;
}