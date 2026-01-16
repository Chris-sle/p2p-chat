#include <p2pnet/p2pnet.h>
#include <p2pnet/crypto.h>
#include <stdio.h>

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Usage: %s <identity-file>\n", argv[0]);
        printf("\n");
        printf("Example:\n");
        printf("  %s identity.key\n", argv[0]);
        return 1;
    }
    
    const char* filepath = argv[1];
    
    printf("========================================\n");
    printf(" P2P Identity Verifier (Milestone 2.1) \n");
    printf("========================================\n\n");
    
    printf("[INFO] Loading identity from %s...\n", filepath);
    
    p2p_keypair_t* keypair = p2p_keypair_load(filepath);
    if (!keypair) {
        printf("[ERROR] Failed to load identity\n");
        printf("\n");
        printf("Possible reasons:\n");
        printf("  - File not found\n");
        printf("  - Invalid file format\n");
        printf("  - Corrupted file\n");
        return 1;
    }
    
    printf("[OK] Identity loaded\n\n");
    
    // Verify keypair
    if (p2p_keypair_verify(keypair) == 0) {
        printf("[OK] Keypair is valid\n\n");
        
        // Show fingerprint
        char fingerprint[64];
        p2p_keypair_fingerprint(keypair, fingerprint, sizeof(fingerprint));
        
        printf("========================================\n");
        printf(" Identity:                             \n");
        printf("========================================\n");
        printf("%s\n", fingerprint);
        printf("========================================\n\n");
        
        printf("Share this fingerprint with peers to\n");
        printf("allow them to verify your identity.\n");
    } else {
        printf("[ERROR] Keypair is invalid or corrupted\n");
        printf("\n");
        printf("DO NOT USE THIS IDENTITY!\n");
    }
    
    p2p_keypair_free(keypair);
    
    return 0;
}