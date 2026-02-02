#ifndef P2PNET_SESSION_H
#define P2PNET_SESSION_H

#include <stdint.h>
#include <stddef.h>

/**
 * Opaque session structure
 * Contains session key and peer information
 */
typedef struct p2p_session p2p_session_t;

/**
 * Get peer's public key from session
 * 
 * @param session Session
 * @return Pointer to 32-byte public key, or NULL if session is NULL
 */
const uint8_t* p2p_session_peer_pubkey(const p2p_session_t* session);

/**
 * Get session key (for encryption)
 * 
 * @param session Session
 * @return Pointer to 32-byte session key, or NULL if session is NULL
 * 
 * @note Do NOT expose this key - use encryption API instead
 */
const uint8_t* p2p_session_key(const p2p_session_t* session);

/**
 * Free session and securely wipe memory
 * 
 * @param session Session to free (can be NULL)
 */
void p2p_session_free(p2p_session_t* session);

/**
 * Get peer's fingerprint (Base64 encoded public key)
 * 
 * @param session Session
 * @param buffer Output buffer (at least 45 bytes)
 * @param buffer_size Size of buffer
 * @return Pointer to buffer, or NULL on error
 */
const char* p2p_session_peer_fingerprint(const p2p_session_t* session,
                                          char* buffer,
                                          size_t buffer_size);

/**
 * P2P Session
 * 
 * Represents an authenticated session between two peers after successful handshake.
 * Contains the shared session key and peer identity.
 */
typedef struct p2p_session {
    uint8_t session_key[32];    // Shared encryption key (derived from ECDH)
    uint8_t peer_pubkey[32];    // Verified peer's Ed25519 public key
    uint64_t send_nonce;        // Counter for outgoing messages (prevents nonce reuse)
    uint64_t recv_nonce;        // Last received nonce (prevents replay attacks)
} p2p_session_t;

/**
 * Create a new session (internal use by handshake)
 */
p2p_session_t* p2p_session_create(void);

/**
 * Free session and securely wipe memory
 */
void p2p_session_free(p2p_session_t* session);

/**
 * Get peer's fingerprint (Base64 encoded public key)
 * 
 * @param session Session
 * @param buffer Output buffer (at least 45 bytes)
 * @param buffer_size Size of buffer
 * @return Pointer to buffer, or NULL on error
 */
const char* p2p_session_peer_fingerprint(const p2p_session_t* session,
                                          char* buffer,
                                          size_t buffer_size);

#endif /* P2PNET_SESSION_H */