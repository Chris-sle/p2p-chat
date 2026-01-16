#include "p2pnet/crypto.h"
#include <sodium.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
    #include <windows.h>
#else
    #include <sys/stat.h>
#endif

// Track if libsodium is initialized
static int g_crypto_initialized = 0;

int p2p_crypto_init(void) {
    if (g_crypto_initialized) {
        return 0;  // Already initialized
    }
    
    if (sodium_init() < 0) {
        fprintf(stderr, "[CRYPTO] Failed to initialize libsodium\n");
        return -1;
    }
    
    g_crypto_initialized = 1;
    return 0;
}

p2p_keypair_t* p2p_keypair_generate(void) {
    // Ensure libsodium is initialized
    if (p2p_crypto_init() < 0) {
        return NULL;
    }
    
    // Allocate keypair
    p2p_keypair_t* keypair = (p2p_keypair_t*)malloc(sizeof(p2p_keypair_t));
    if (!keypair) {
        return NULL;
    }
    
    // Generate Ed25519 keypair
    // This uses /dev/urandom (Linux) or CryptGenRandom (Windows)
    crypto_sign_keypair(keypair->public_key, keypair->secret_key);
    
    return keypair;
}

void p2p_keypair_free(p2p_keypair_t* keypair) {
    if (!keypair) return;
    
    // Securely wipe memory before free
    // This prevents key recovery via memory dumps
    sodium_memzero(keypair, sizeof(p2p_keypair_t));
    
    free(keypair);
}

int p2p_keypair_save(const p2p_keypair_t* keypair, const char* filepath) {
    if (!keypair || !filepath) return -1;
    
    FILE* f = fopen(filepath, "wb");
    if (!f) {
        fprintf(stderr, "[CRYPTO] Failed to open %s for writing\n", filepath);
        return -1;
    }
    
    // Encode secret key to Base64
    // Step 1: Calculate and allocate the right buffer size
    char b64_secret[sodium_base64_ENCODED_LEN(64, sodium_base64_VARIANT_ORIGINAL)];
    // Step 2: Perform the encoding
    sodium_bin2base64(b64_secret, sizeof(b64_secret),
                      keypair->secret_key, 64,
                      sodium_base64_VARIANT_ORIGINAL);
    
    // Encode public key to Base64
    char b64_public[sodium_base64_ENCODED_LEN(32, sodium_base64_VARIANT_ORIGINAL)];
    sodium_bin2base64(b64_public, sizeof(b64_public),
                      keypair->public_key, 32,
                      sodium_base64_VARIANT_ORIGINAL);
    
    // Write to file
    fprintf(f, "-----BEGIN P2P PRIVATE KEY-----\n");
    fprintf(f, "%s\n", b64_secret);
    fprintf(f, "-----END P2P PRIVATE KEY-----\n");
    fprintf(f, "-----BEGIN P2P PUBLIC KEY-----\n");
    fprintf(f, "%s\n", b64_public);
    fprintf(f, "-----END P2P PUBLIC KEY-----\n");
    
    fclose(f);
    
    // Set file permissions (user read/write only)
    #ifdef _WIN32
        // Windows: Set readonly flag (better than nothing)
        SetFileAttributesA(filepath, FILE_ATTRIBUTE_READONLY);
    #else
        // Unix: chmod 0600
        chmod(filepath, S_IRUSR | S_IWUSR);
    #endif
    
    return 0;
}

p2p_keypair_t* p2p_keypair_load(const char* filepath) {
    if (!filepath) return NULL;
    
    FILE* f = fopen(filepath, "rb");
    if (!f) {
        fprintf(stderr, "[CRYPTO] Failed to open %s for reading\n", filepath);
        return NULL;
    }
    
    p2p_keypair_t* keypair = (p2p_keypair_t*)malloc(sizeof(p2p_keypair_t));
    if (!keypair) {
        fclose(f);
        return NULL;
    }
    
    char line[512];
    char b64_secret[256] = {0};
    char b64_public[256] = {0};
    
    // Parse file
    int found_secret = 0, found_public = 0;
    
    while (fgets(line, sizeof(line), f)) {
        if (strstr(line, "BEGIN P2P PRIVATE KEY")) {
            // Next line is secret key
            if (fgets(line, sizeof(line), f)) {
                strncpy(b64_secret, line, sizeof(b64_secret) - 1);
                // Remove newline
                b64_secret[strcspn(b64_secret, "\r\n")] = 0;
                found_secret = 1;
            }
        } else if (strstr(line, "BEGIN P2P PUBLIC KEY")) {
            // Next line is public key
            if (fgets(line, sizeof(line), f)) {
                strncpy(b64_public, line, sizeof(b64_public) - 1);
                b64_public[strcspn(b64_public, "\r\n")] = 0;
                found_public = 1;
            }
        }
    }
    
    fclose(f);
    
    if (!found_secret || !found_public) {
        fprintf(stderr, "[CRYPTO] Invalid key file format\n");
        free(keypair);
        return NULL;
    }
    
    // Decode from Base64
    size_t secret_len, public_len;
    
    if (sodium_base642bin(keypair->secret_key, 64,
                          b64_secret, strlen(b64_secret),
                          NULL, &secret_len, NULL,
                          sodium_base64_VARIANT_ORIGINAL) != 0) {
        fprintf(stderr, "[CRYPTO] Failed to decode secret key\n");
        free(keypair);
        return NULL;
    }
    
    if (sodium_base642bin(keypair->public_key, 32,
                          b64_public, strlen(b64_public),
                          NULL, &public_len, NULL,
                          sodium_base64_VARIANT_ORIGINAL) != 0) {
        fprintf(stderr, "[CRYPTO] Failed to decode public key\n");
        free(keypair);
        return NULL;
    }
    
    // Verify keypair
    if (p2p_keypair_verify(keypair) != 0) {
        fprintf(stderr, "[CRYPTO] Loaded keypair is invalid\n");
        p2p_keypair_free(keypair);
        return NULL;
    }
    
    return keypair;
}

const char* p2p_keypair_fingerprint(const p2p_keypair_t* keypair,
                                     char* buffer,
                                     size_t buffer_size) {
    if (!keypair || !buffer || buffer_size < 45) {
        return NULL;
    }
    
    // Encode public key as Base64 (URL-safe, no padding)
    sodium_bin2base64(buffer, buffer_size,
                      keypair->public_key, 32,
                      sodium_base64_VARIANT_URLSAFE_NO_PADDING);
    
    return buffer;
}

int p2p_keypair_verify(const p2p_keypair_t* keypair) {
    if (!keypair) return -1;
    
    // Ed25519 secret key format: [32B seed || 32B public key]
    // Extract embedded public key from secret key
    const uint8_t* embedded_pubkey = keypair->secret_key + 32;
    
    // Verify that embedded public key matches standalone public key
    if (sodium_memcmp(embedded_pubkey, keypair->public_key, 32) != 0) {
        return -1;  // Mismatch!
    }
    
    return 0;  // Valid
}