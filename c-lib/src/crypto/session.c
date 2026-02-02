#include "p2pnet/session.h"
#include "p2pnet/crypto.h" 
#include <sodium.h>
#include <stdlib.h>
#include <string.h>

/**
 * Session structure (internal)
 */
struct p2p_session {
    uint8_t session_key[32];    // Derived encryption key
    uint8_t peer_pubkey[32];    // Verified peer identity
    uint64_t nonce_counter;     // For message nonce generation (Milestone 2.3)
};

/**
 * Create new session (internal use only)
 */
p2p_session_t* p2p_session_create(const uint8_t* session_key,
                                   const uint8_t* peer_pubkey) {
    if (!session_key || !peer_pubkey) {
        return NULL;
    }
    
    p2p_session_t* session = (p2p_session_t*)malloc(sizeof(p2p_session_t));
    if (!session) {
        return NULL;
    }
    
    // Copy session key
    memcpy(session->session_key, session_key, 32);
    
    // Copy peer public key
    memcpy(session->peer_pubkey, peer_pubkey, 32);
    
    // Initialize nonce counter
    session->nonce_counter = 0;
    
    return session;
}

// Get peer's public key from session
const uint8_t* p2p_session_peer_pubkey(const p2p_session_t* session) {
    if (!session) return NULL;
    return session->peer_pubkey;
}

// Get session key (for encryption)
const uint8_t* p2p_session_key(const p2p_session_t* session) {
    if (!session) return NULL;
    return session->session_key;
}

// Free session and securely wipe memory
void p2p_session_free(p2p_session_t* session) {
    if (!session) return;
    
    // Securely wipe session key and all data
    sodium_memzero(session, sizeof(p2p_session_t));
    
    free(session);
}

// Get peer's fingerprint (Base64 encoded public key)
const char* p2p_session_peer_fingerprint(const p2p_session_t* session,
                                          char* buffer,
                                          size_t buffer_size) {
    if (!session || !buffer || buffer_size < 45) {
        return NULL;
    }
    
    // Create temporary keypair for fingerprint function
    p2p_keypair_t temp;
    memcpy(temp.public_key, session->peer_pubkey, 32);
    memset(temp.secret_key, 0, 64);  // Not used
    
    return p2p_keypair_fingerprint(&temp, buffer, buffer_size);
}

// Create a new session (internal use by handshake)
p2p_session_t* p2p_session_create(void) {
    p2p_session_t* session = (p2p_session_t*)malloc(sizeof(p2p_session_t));
    if (!session) {
        return NULL;
    }
    
    // Initialize nonces to 0
    session->send_nonce = 0;
    session->recv_nonce = 0;
    
    return session;
}

// Free session and securely wipe memory
void p2p_session_free(p2p_session_t* session) {
    if (!session) {
        return;
    }
    
    // Securely wipe session key and nonces
    sodium_memzero(session, sizeof(p2p_session_t));
    free(session);
}

// Get peer's fingerprint (Base64 encoded public key)
const char* p2p_session_peer_fingerprint(const p2p_session_t* session,
                                          char* buffer,
                                          size_t buffer_size) {
    if (!session || !buffer || buffer_size < 45) {
        return NULL;
    }
    
    // Base64 encode public key (URL-safe, no padding)
    sodium_bin2base64(buffer, buffer_size,
                      session->peer_pubkey, 32,
                      sodium_base64_VARIANT_URLSAFE_NO_PADDING);
    
    return buffer;
}