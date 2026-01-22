#ifndef P2PNET_HANDSHAKE_H
#define P2PNET_HANDSHAKE_H

#include "p2pnet/socket.h"
#include "p2pnet/crypto.h"
#include "p2pnet/session.h"
#include <stdint.h>
#include <stddef.h>

/**
 * Perform handshake as client
 * 
 * @param sock Connected socket to peer
 * @param my_keypair My identity keypair
 * @param expected_peer_pubkey Expected peer's public key (32 bytes)
 *                              Pass NULL to accept any peer (not recommended)
 * @return Session on success, NULL on failure
 * 
 * @note Blocks until handshake completes or fails
 * @note Caller must call p2p_session_free() when done
 * 
 * @example
 *   p2p_session_t* session = p2p_handshake_client(sock, my_kp, bob_pubkey);
 *   if (!session) {
 *       fprintf(stderr, "Handshake failed\n");
 *       return -1;
 *   }
 */
p2p_session_t* p2p_handshake_client(p2p_socket_t* sock,
                                     p2p_keypair_t* my_keypair,
                                     const uint8_t* expected_peer_pubkey);

/**
 * Perform handshake as server
 * 
 * @param sock Connected client socket
 * @param my_keypair My identity keypair
 * @param allowed_peers Array of allowed peer public keys (32 bytes each)
 *                      Pass NULL to accept any peer (not recommended)
 * @param num_allowed Number of allowed peers (ignored if allowed_peers is NULL)
 * @return Session on success, NULL on failure
 * 
 * @note Blocks until handshake completes or fails
 * @note Caller must call p2p_session_free() when done
 * 
 * @example
 *   // Accept only Alice
 *   const uint8_t* allowed[] = { alice_pubkey };
 *   p2p_session_t* session = p2p_handshake_server(sock, my_kp, allowed, 1);
 */
p2p_session_t* p2p_handshake_server(p2p_socket_t* sock,
                                     p2p_keypair_t* my_keypair,
                                     const uint8_t** allowed_peers,
                                     size_t num_allowed);

#endif /* P2PNET_HANDSHAKE_H */