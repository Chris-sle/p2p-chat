#ifndef P2PNET_H
#define P2PNET_H

/**
 * P2PNet - Peer-to-Peer Networking Library
 * 
 * Dette er hovedheaderen. Inkluder denne i dine programmer.
 */

 // Core functionality
#include "p2pnet/socket.h"
#include "p2pnet/message.h"
#include "p2pnet/event_loop.h"

// Cryptography (Phase 2)
#include "p2pnet/crypto.h" 
#include "p2pnet/session.h"
#include "p2pnet/handshake.h"
#include "p2pnet/encryption.h"
// Flere headers kommer senere...

/**
 * Initialize P2PNet library (Winsock + libsodium)
 * 
 * @return 0 on success, -1 on error
 */
int p2p_init(void);

/**
 * Cleanup P2PNet library
 */
void p2p_cleanup(void);


#endif /* P2PNET_H */