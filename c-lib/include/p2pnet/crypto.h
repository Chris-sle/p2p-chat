#ifndef P2PNET_CRYPTO_H
#define P2PNET_CRYPTO_H

#include <stdint.h>
#include <stddef.h>

/**
 * Ed25519 keypair structure
 */
typedef struct {
    uint8_t public_key[32];   // Ed25519 public key
    uint8_t secret_key[64];   // Ed25519 secret key (32B seed + 32B pubkey)
} p2p_keypair_t;

// ============================================================================
// Keypair Lifecycle
// ============================================================================

/**
 * Generate new Ed25519 keypair
 * 
 * @return New keypair or NULL on error
 * 
 * @note Caller MUST call p2p_keypair_free() when done
 * @note Uses cryptographically secure random number generator
 */
p2p_keypair_t* p2p_keypair_generate(void);

/**
 * Free keypair and securely wipe memory
 * 
 * @param keypair Keypair to free (can be NULL)
 * 
 * @note Memory is zeroed before free (prevents key recovery)
 */
void p2p_keypair_free(p2p_keypair_t* keypair);

// ============================================================================
// Keypair Serialization
// ============================================================================

/**
 * Save keypair to file
 * 
 * @param keypair Keypair to save
 * @param filepath Path to save to (e.g., "identity.key")
 * @return 0 on success, -1 on error
 * 
 * @note File format: PEM-like with Base64 encoding
 * @note Sets file permissions to 0600 (user read/write only)
 * 
 * @example
 *   p2p_keypair_save(kp, "~/.p2p/identity.key");
 */
int p2p_keypair_save(const p2p_keypair_t* keypair, const char* filepath);

/**
 * Load keypair from file
 * 
 * @param filepath Path to load from
 * @return Keypair or NULL on error
 * 
 * @note Caller MUST call p2p_keypair_free() when done
 */
p2p_keypair_t* p2p_keypair_load(const char* filepath);

// ============================================================================
// Keypair Utilities
// ============================================================================

/**
 * Get public key fingerprint (Base64 encoded)
 * 
 * @param keypair Keypair
 * @param buffer Output buffer (at least 45 bytes)
 * @param buffer_size Size of buffer
 * @return Pointer to buffer (for chaining)
 * 
 * @note Output is null-terminated Base64 string (43 chars + null)
 * @note Uses URL-safe Base64 variant (no padding)
 * 
 * @example
 *   char fp;
 *   p2p_keypair_fingerprint(kp, fp, sizeof(fp));
 *   printf("Your identity: %s\n", fp);
 */
const char* p2p_keypair_fingerprint(const p2p_keypair_t* keypair,
                                     char* buffer,
                                     size_t buffer_size);

/**
 * Verify that keypair is valid
 * 
 * @param keypair Keypair to verify
 * @return 0 if valid, -1 if invalid
 * 
 * @note Checks that public key matches secret key
 */
int p2p_keypair_verify(const p2p_keypair_t* keypair);

// ============================================================================
// Initialization
// ============================================================================

/**
 * Initialize crypto subsystem (libsodium)
 * 
 * @return 0 on success, -1 on error
 * 
 * @note Called automatically by other functions if needed
 * @note Safe to call multiple times
 */
int p2p_crypto_init(void);

#endif /* P2PNET_CRYPTO_H */