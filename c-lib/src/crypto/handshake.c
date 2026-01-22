#include "p2pnet/handshake.h"
#include "p2pnet/message.h"
#include <sodium.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Forward declaration of session_create (internal)
p2p_session_t* p2p_session_create(const uint8_t* session_key,
                                   const uint8_t* peer_pubkey);

// Message types
#define MSG_CLIENT_HELLO  0x01
#define MSG_SERVER_HELLO  0x02
#define MSG_KEY_EXCHANGE  0x03
#define MSG_ACCEPT        0x04
#define MSG_ERROR         0xFF

// Message sizes
#define SIZE_CLIENT_HELLO  33   // 1 + 32
#define SIZE_SERVER_HELLO  65   // 1 + 32 + 32
#define SIZE_KEY_EXCHANGE  97   // 1 + 32 + 64
#define SIZE_ACCEPT        97   // 1 + 32 + 64

/**
 * Send raw bytes over socket
 */
static int send_bytes(p2p_socket_t* sock, const uint8_t* data, size_t len) {
    size_t total_sent = 0;
    
    while (total_sent < len) {
        int sent = p2p_socket_send(sock, (const char*)(data + total_sent), 
                                   len - total_sent);
        if (sent <= 0) {
            return -1;  // Error or connection closed
        }
        total_sent += sent;
    }
    
    return 0;
}

/**
 * Receive exact number of bytes from socket
 */
static int recv_bytes(p2p_socket_t* sock, uint8_t* buffer, size_t len) {
    size_t total_received = 0;
    
    while (total_received < len) {
        int received = p2p_socket_recv(sock, (char*)(buffer + total_received),
                                       len - total_received);
        if (received <= 0) {
            return -1;  // Error or connection closed
        }
        total_received += received;
    }
    
    return 0;
}

/**
 * Derive session key from shared secret + identities
 * 
 * session_key = BLAKE2b(shared_secret || alice_id || bob_id || "P2PNetSessionKey")
 */
static void derive_session_key(uint8_t* session_key,
                                const uint8_t* shared_secret,
                                const uint8_t* alice_pubkey,
                                const uint8_t* bob_pubkey) {
    // Prepare input for BLAKE2b
    uint8_t input[32 + 32 + 32 + 17];  // shared + alice + bob + domain separator
    
    memcpy(input, shared_secret, 32);
    memcpy(input + 32, alice_pubkey, 32);
    memcpy(input + 64, bob_pubkey, 32);
    memcpy(input + 96, "P2PNetSessionKey", 17);
    
    // BLAKE2b hash (output: 32 bytes)
    crypto_generichash(session_key, 32, input, sizeof(input), NULL, 0);
    
    // Wipe intermediate data
    sodium_memzero(input, sizeof(input));
}

/**
 * Check if peer is in allowed list
 */
static int is_peer_allowed(const uint8_t* peer_pubkey,
                            const uint8_t** allowed_peers,
                            size_t num_allowed) {
    // NULL = allow all
    if (!allowed_peers) {
        return 1;
    }
    
    // Check each allowed peer
    for (size_t i = 0; i < num_allowed; i++) {
        if (sodium_memcmp(peer_pubkey, allowed_peers[i], 32) == 0) {
            return 1;  // Found!
        }
    }
    
    return 0;  // Not allowed
}

p2p_session_t* p2p_handshake_client(p2p_socket_t* sock,
                                     p2p_keypair_t* my_keypair,
                                     const uint8_t* expected_peer_pubkey) {
    if (!sock || !my_keypair) {
        return NULL;
    }
    
    printf("[HANDSHAKE] Starting client handshake...\n");
    
    // ========================================================================
    // Step 1: Generate ephemeral X25519 keypair
    // ========================================================================
    
    uint8_t ephemeral_public[32];
    uint8_t ephemeral_secret[32];
    
    crypto_box_keypair(ephemeral_public, ephemeral_secret);
    
    // ========================================================================
    // Step 2: Send ClientHello (my identity)
    // ========================================================================
    
    uint8_t client_hello[SIZE_CLIENT_HELLO];
    client_hello[0] = MSG_CLIENT_HELLO;
    memcpy(client_hello + 1, my_keypair->public_key, 32);
    
    if (send_bytes(sock, client_hello, SIZE_CLIENT_HELLO) != 0) {
        fprintf(stderr, "[HANDSHAKE] Failed to send ClientHello\n");
        sodium_memzero(ephemeral_secret, 32);
        return NULL;
    }
    
    printf("[HANDSHAKE] Sent ClientHello\n");
    
    // ========================================================================
    // Step 3: Receive ServerHello (peer identity + challenge)
    // ========================================================================
    
    uint8_t server_hello[SIZE_SERVER_HELLO];
    if (recv_bytes(sock, server_hello, SIZE_SERVER_HELLO) != 0) {
        fprintf(stderr, "[HANDSHAKE] Failed to receive ServerHello\n");
        sodium_memzero(ephemeral_secret, 32);
        return NULL;
    }
    
    if (server_hello[0] != MSG_SERVER_HELLO) {
        fprintf(stderr, "[HANDSHAKE] Invalid message type (expected ServerHello)\n");
        sodium_memzero(ephemeral_secret, 32);
        return NULL;
    }
    
    uint8_t server_pubkey[32];
    uint8_t challenge[32];
    
    memcpy(server_pubkey, server_hello + 1, 32);
    memcpy(challenge, server_hello + 33, 32);
    
    printf("[HANDSHAKE] Received ServerHello\n");
    
    // ========================================================================
    // Step 4: Verify peer identity (if expected)
    // ========================================================================
    
    if (expected_peer_pubkey) {
        if (sodium_memcmp(server_pubkey, expected_peer_pubkey, 32) != 0) {
            fprintf(stderr, "[HANDSHAKE] Peer identity mismatch!\n");
            sodium_memzero(ephemeral_secret, 32);
            return NULL;
        }
        printf("[HANDSHAKE] Peer identity verified\n");
    }
    
    // ========================================================================
    // Step 5: Sign challenge + ephemeral key
    // ========================================================================
    
    // Prepare data to sign: challenge || ephemeral_pubkey
    uint8_t to_sign[64];
    memcpy(to_sign, challenge, 32);
    memcpy(to_sign + 32, ephemeral_public, 32);
    
    uint8_t signature[64];
    crypto_sign_detached(signature, NULL, to_sign, 64, my_keypair->secret_key);
    
    // ========================================================================
    // Step 6: Send KeyExchange
    // ========================================================================
    
    uint8_t key_exchange[SIZE_KEY_EXCHANGE];
    key_exchange[0] = MSG_KEY_EXCHANGE;
    memcpy(key_exchange + 1, ephemeral_public, 32);
    memcpy(key_exchange + 33, signature, 64);
    
    if (send_bytes(sock, key_exchange, SIZE_KEY_EXCHANGE) != 0) {
        fprintf(stderr, "[HANDSHAKE] Failed to send KeyExchange\n");
        sodium_memzero(ephemeral_secret, 32);
        return NULL;
    }
    
    printf("[HANDSHAKE] Sent KeyExchange\n");
    
    // ========================================================================
    // Step 7: Receive Accept
    // ========================================================================
    
    uint8_t accept[SIZE_ACCEPT];
    if (recv_bytes(sock, accept, SIZE_ACCEPT) != 0) {
        fprintf(stderr, "[HANDSHAKE] Failed to receive Accept\n");
        sodium_memzero(ephemeral_secret, 32);
        return NULL;
    }
    
    if (accept[0] != MSG_ACCEPT) {
        fprintf(stderr, "[HANDSHAKE] Invalid message type (expected Accept)\n");
        sodium_memzero(ephemeral_secret, 32);
        return NULL;
    }
    
    uint8_t server_ephemeral[32];
    uint8_t server_signature[64];
    
    memcpy(server_ephemeral, accept + 1, 32);
    memcpy(server_signature, accept + 33, 64);
    
    printf("[HANDSHAKE] Received Accept\n");
    
    // ========================================================================
    // Step 8: Verify server signature
    // ========================================================================
    
    // Server signed: challenge || server_ephemeral || client_ephemeral
    uint8_t server_signed[96];
    memcpy(server_signed, challenge, 32);
    memcpy(server_signed + 32, server_ephemeral, 32);
    memcpy(server_signed + 64, ephemeral_public, 32);
    
    if (crypto_sign_verify_detached(server_signature, server_signed, 96, 
                                     server_pubkey) != 0) {
        fprintf(stderr, "[HANDSHAKE] Server signature verification failed!\n");
        sodium_memzero(ephemeral_secret, 32);
        return NULL;
    }
    
    printf("[HANDSHAKE] Server signature verified\n");
    
    // ========================================================================
    // Step 9: Derive shared secret (X25519 ECDH)
    // ========================================================================
    
    uint8_t shared_secret[32];
    if (crypto_scalarmult(shared_secret, ephemeral_secret, server_ephemeral) != 0) {
        fprintf(stderr, "[HANDSHAKE] Key exchange failed\n");
        sodium_memzero(ephemeral_secret, 32);
        return NULL;
    }
    
    // ========================================================================
    // Step 10: Derive session key
    // ========================================================================
    
    uint8_t session_key[32];
    derive_session_key(session_key, shared_secret, 
                       my_keypair->public_key, server_pubkey);
    
    printf("[HANDSHAKE] Session key derived\n");
    
    // ========================================================================
    // Step 11: Create session object
    // ========================================================================
    
    p2p_session_t* session = p2p_session_create(session_key, server_pubkey);
    
    // ========================================================================
    // Step 12: Cleanup ephemeral secrets
    // ========================================================================
    
    sodium_memzero(ephemeral_secret, 32);
    sodium_memzero(shared_secret, 32);
    sodium_memzero(session_key, 32);
    sodium_memzero(to_sign, 64);
    sodium_memzero(server_signed, 96);
    
    printf("[HANDSHAKE] ✅ Client handshake complete!\n");
    
    return session;
}

p2p_session_t* p2p_handshake_server(p2p_socket_t* sock,
                                     p2p_keypair_t* my_keypair,
                                     const uint8_t** allowed_peers,
                                     size_t num_allowed) {
    if (!sock || !my_keypair) {
        return NULL;
    }
    
    printf("[HANDSHAKE] Starting server handshake...\n");
    
    // ========================================================================
    // Step 1: Generate ephemeral X25519 keypair
    // ========================================================================
    
    uint8_t ephemeral_public[32];
    uint8_t ephemeral_secret[32];
    
    crypto_box_keypair(ephemeral_public, ephemeral_secret);
    
    // ========================================================================
    // Step 2: Generate random challenge
    // ========================================================================
    
    uint8_t challenge[32];
    randombytes_buf(challenge, 32);
    
    // ========================================================================
    // Step 3: Receive ClientHello
    // ========================================================================
    
    uint8_t client_hello[SIZE_CLIENT_HELLO];
    if (recv_bytes(sock, client_hello, SIZE_CLIENT_HELLO) != 0) {
        fprintf(stderr, "[HANDSHAKE] Failed to receive ClientHello\n");
        sodium_memzero(ephemeral_secret, 32);
        return NULL;
    }
    
    if (client_hello[0] != MSG_CLIENT_HELLO) {
        fprintf(stderr, "[HANDSHAKE] Invalid message type (expected ClientHello)\n");
        sodium_memzero(ephemeral_secret, 32);
        return NULL;
    }
    
    uint8_t client_pubkey[32];
    memcpy(client_pubkey, client_hello + 1, 32);
    
    printf("[HANDSHAKE] Received ClientHello\n");
    
    // ========================================================================
    // Step 4: Check if client is allowed
    // ========================================================================
    
    if (!is_peer_allowed(client_pubkey, allowed_peers, num_allowed)) {
        fprintf(stderr, "[HANDSHAKE] Client not in allowed list!\n");
        sodium_memzero(ephemeral_secret, 32);
        return NULL;
    }
    
    printf("[HANDSHAKE] Client is allowed\n");
    
    // ========================================================================
    // Step 5: Send ServerHello
    // ========================================================================
    
    uint8_t server_hello[SIZE_SERVER_HELLO];
    server_hello[0] = MSG_SERVER_HELLO;
    memcpy(server_hello + 1, my_keypair->public_key, 32);
    memcpy(server_hello + 33, challenge, 32);
    
    if (send_bytes(sock, server_hello, SIZE_SERVER_HELLO) != 0) {
        fprintf(stderr, "[HANDSHAKE] Failed to send ServerHello\n");
        sodium_memzero(ephemeral_secret, 32);
        return NULL;
    }
    
    printf("[HANDSHAKE] Sent ServerHello\n");
    
    // ========================================================================
    // Step 6: Receive KeyExchange
    // ========================================================================
    
    uint8_t key_exchange[SIZE_KEY_EXCHANGE];
    if (recv_bytes(sock, key_exchange, SIZE_KEY_EXCHANGE) != 0) {
        fprintf(stderr, "[HANDSHAKE] Failed to receive KeyExchange\n");
        sodium_memzero(ephemeral_secret, 32);
        return NULL;
    }
    
    if (key_exchange[0] != MSG_KEY_EXCHANGE) {
        fprintf(stderr, "[HANDSHAKE] Invalid message type (expected KeyExchange)\n");
        sodium_memzero(ephemeral_secret, 32);
        return NULL;
    }
    
    uint8_t client_ephemeral[32];
    uint8_t client_signature[64];
    
    memcpy(client_ephemeral, key_exchange + 1, 32);
    memcpy(client_signature, key_exchange + 33, 64);
    
    printf("[HANDSHAKE] Received KeyExchange\n");
    
    // ========================================================================
    // Step 7: Verify client signature
    // ========================================================================
    
    // Client signed: challenge || client_ephemeral
    uint8_t client_signed[64];
    memcpy(client_signed, challenge, 32);
    memcpy(client_signed + 32, client_ephemeral, 32);
    
    if (crypto_sign_verify_detached(client_signature, client_signed, 64,
                                     client_pubkey) != 0) {
        fprintf(stderr, "[HANDSHAKE] Client signature verification failed!\n");
        sodium_memzero(ephemeral_secret, 32);
        return NULL;
    }
    
    printf("[HANDSHAKE] Client signature verified\n");
    
    // ========================================================================
    // Step 8: Sign challenge + ephemeral keys
    // ========================================================================
    
    // Sign: challenge || server_ephemeral || client_ephemeral
    uint8_t to_sign[96];
    memcpy(to_sign, challenge, 32);
    memcpy(to_sign + 32, ephemeral_public, 32);
    memcpy(to_sign + 64, client_ephemeral, 32);
    
    uint8_t signature[64];
    crypto_sign_detached(signature, NULL, to_sign, 96, my_keypair->secret_key);
    
    // ========================================================================
    // Step 9: Send Accept
    // ========================================================================
    
    uint8_t accept[SIZE_ACCEPT];
    accept[0] = MSG_ACCEPT;
    memcpy(accept + 1, ephemeral_public, 32);
    memcpy(accept + 33, signature, 64);
    
    if (send_bytes(sock, accept, SIZE_ACCEPT) != 0) {
        fprintf(stderr, "[HANDSHAKE] Failed to send Accept\n");
        sodium_memzero(ephemeral_secret, 32);
        return NULL;
    }
    
    printf("[HANDSHAKE] Sent Accept\n");
    
    // ========================================================================
    // Step 10: Derive shared secret (X25519 ECDH)
    // ========================================================================
    
    uint8_t shared_secret[32];
    if (crypto_scalarmult(shared_secret, ephemeral_secret, client_ephemeral) != 0) {
        fprintf(stderr, "[HANDSHAKE] Key exchange failed\n");
        sodium_memzero(ephemeral_secret, 32);
        return NULL;
    }
    
    // ========================================================================
    // Step 11: Derive session key
    // ========================================================================
    
    uint8_t session_key[32];
    derive_session_key(session_key, shared_secret,
                       client_pubkey, my_keypair->public_key);
    
    printf("[HANDSHAKE] Session key derived\n");
    
    // ========================================================================
    // Step 12: Create session object
    // ========================================================================
    
    p2p_session_t* session = p2p_session_create(session_key, client_pubkey);
    
    // ========================================================================
    // Step 13: Cleanup ephemeral secrets
    // ========================================================================
    
    sodium_memzero(ephemeral_secret, 32);
    sodium_memzero(shared_secret, 32);
    sodium_memzero(session_key, 32);
    sodium_memzero(client_signed, 64);
    sodium_memzero(to_sign, 96);
    
    printf("[HANDSHAKE] ✅ Server handshake complete!\n");
    
    return session;
}