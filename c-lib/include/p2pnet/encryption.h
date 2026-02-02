#ifndef P2PNET_ENCRYPTION_H
#define P2PNET_ENCRYPTION_H

#include <p2pnet/session.h>
#include <p2pnet/socket.h>
#include <p2pnet/message.h>
#include <stdint.h>
#include <stddef.h>

/**
 * Encrypted Message Format (on wire):
 * 
 * ┌───────────┬────────────┬──────────────────┬─────────────┐
 * │ Length(4) │ Nonce(12)  │ Ciphertext(?)    │ MAC(16)     │
 * │ uint32_t  │ 96 bits    │ Variable length  │ 128 bits    │
 * └───────────┴────────────┴──────────────────┴─────────────┘
 * 
 * Total overhead: 4 + 12 + 16 = 32 bytes per message
 * 
 * Encryption: ChaCha20-Poly1305 AEAD
 * Nonce: Counter-based (8-byte counter + 4-byte padding)
 */

/**
 * Send encrypted message over session
 * 
 * Uses ChaCha20-Poly1305 AEAD encryption with counter-based nonce.
 * Automatically increments session's send_nonce counter.
 * 
 * @param session Session with shared encryption key
 * @param sock Connected socket
 * @param data Plaintext data to send
 * @param length Length of plaintext
 * @return 0 on success, -1 on error
 * 
 * Errors:
 * - Counter overflow (sent 2^64 messages - extremely unlikely)
 * - Encryption failed (libsodium error)
 * - Network send failed
 * 
 * Security properties:
 * - Confidentiality: ChaCha20 encryption
 * - Authenticity: Poly1305 MAC
 * - Replay protection: Monotonic nonce counter
 * 
 * Example:
 *   p2p_session_send(session, sock, "Hello", 5);
 */
int p2p_session_send(p2p_session_t* session,
                     p2p_socket_t* sock,
                     const uint8_t* data,
                     size_t length);

/**
 * Receive and decrypt message from session
 * 
 * Receives encrypted message, verifies MAC, checks nonce ordering,
 * and decrypts plaintext. Returns message if successful.
 * 
 * @param session Session with shared encryption key
 * @param sock Connected socket
 * @return Decrypted message on success, NULL on error
 * 
 * Errors:
 * - Connection closed by peer
 * - MAC verification failed (tampering detected)
 * - Nonce rewind/replay (replay attack detected)
 * - Decryption failed
 * 
 * Caller must free returned message with p2p_message_free()
 * 
 * Example:
 *   p2p_message_t* msg = p2p_session_recv(session, sock);
 *   if (msg) {
 *       printf("Got: %.*s\n", msg->length, msg->data);
 *       p2p_message_free(msg);
 *   }
 */
p2p_message_t* p2p_session_recv(p2p_session_t* session,
                                 p2p_socket_t* sock);

#endif // P2PNET_ENCRYPTION_H