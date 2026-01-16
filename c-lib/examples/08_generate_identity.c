#include <p2pnet/p2pnet.h>
#include <p2pnet/crypto.h>
#include <stdio.h>
#include <stdlib.h>

int main() {
    printf("========================================\n");
    printf(" P2P Identity Generator (Milestone 2.1)\n");
    printf("========================================\n\n");
    
    printf("[INFO] Generating Ed25519 keypair...\n");
    
    p2p_keypair_t* keypair = p2p_keypair_generate();
    if (!keypair) {
        printf("[ERROR] Failed to generate keypair\n");
        return 1;
    }
    
    printf("[OK] Keypair generated\n\n");
    
    // Show fingerprint
    char fingerprint[64];
    p2p_keypair_fingerprint(keypair, fingerprint, sizeof(fingerprint));
    
    printf("========================================\n");
    printf(" Your P2P Identity:                    \n");
    printf("========================================\n");
    printf("%s\n", fingerprint);
    printf("========================================\n\n");
    
    // Save to file
    const char* filepath = "identity.key";
    printf("[INFO] Saving to %s...\n", filepath);
    
    if (p2p_keypair_save(keypair, filepath) == 0) {
        printf("[OK] Identity saved\n");
        printf("\n");
        printf("[WARN] Keep this file secure!\n");
        printf("[WARN] Anyone with this file can impersonate you!\n");
        printf("\n");
        printf("Backup recommendation:\n");
        printf("  - Copy to USB drive\n");
        printf("  - Do NOT upload to cloud\n");
        printf("  - Do NOT share with anyone\n");
    } else {
        printf("[ERROR] Failed to save identity\n");
    }
    
    p2p_keypair_free(keypair);
    
    return 0;
}