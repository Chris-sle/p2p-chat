Perfekt! La oss lage en **high-level plan** for Milestone 2.2 uten masse kode-detaljer. [ppl-ai-file-upload.s3.
# Milestone 2.2: Handshake Protocol - PLAN

**Status:** 📋 Planlagt  
**Estimated Time:** 1-2 uker  
**Prerequisites:** Milestone 2.1 ✅

---

## Mål

Implementere en authenticated handshake protocol med key exchange, slik at to peers kan:
- Verifisere hverandres identitet (mutual authentication)
- Etablere en encrypted session
- Beskytte mot man-in-the-middle (MITM) angrep
- Oppnå perfect forward secrecy

---

## Problemet vi løser

### Current State (etter Milestone 2.1)

```
Alice                                     Bob
=====                                     ===

✅ Has Ed25519 keypair (identity)        ✅ Has Ed25519 keypair (identity)

❌ Cannot verify Bob's identity          ❌ Cannot verify Alice's identity
❌ No shared secret for encryption       ❌ No shared secret for encryption
❌ Anyone can pretend to be Bob          ❌ Anyone can pretend to be Alice
```

**Problem:** Vi har identiteter, men ingen måte å bevise at vi er hvem vi sier vi er.

---

### After Milestone 2.2

```
Alice                                     Bob
=====                                     ===

1. "Hi, I'm Alice (here's my pubkey)"
                    ───────────────────→

2.                  ←───────────────────  "Hi Alice, I'm Bob (here's my pubkey + challenge)"

3. "Prove you know my challenge"         
   [signed with Alice's private key]
                    ───────────────────→

4.                  ←───────────────────  "Verified! Here's my proof too"
                                          [signed with Bob's private key]

✅ Both verified                          ✅ Both verified
✅ Shared secret established              ✅ Shared secret established
✅ Ready for encrypted communication      ✅ Ready for encrypted communication
```

---

## Security Properties

### What we achieve:

| Property | How |
|----------|-----|
| **Mutual Authentication** | Both sides verify identity via Ed25519 signatures |
| **Perfect Forward Secrecy** | Ephemeral X25519 keys (discarded after session) |
| **MITM Protection** | Signatures verified with long-term Ed25519 keys |
| **Replay Protection** | Random challenge (nonce) used once |
| **Identity Binding** | Session key derived from both identities |

---

## Cryptographic Components

### 1. Ed25519 (Digital Signatures)

**Purpose:** Prove identity

**Used in:**
- Signing challenges
- Signing ephemeral public keys
- Verifying peer identity

**From:** Milestone 2.1 (already implemented)

---

### 2. X25519 (Key Exchange - ECDH)

**Purpose:** Establish shared secret

**Why X25519?**
- Perfect forward secrecy (ephemeral keys)
- Fast computation (~0.1ms)
- Small keys (32 bytes)
- Same curve family as Ed25519 (Curve25519)

**Flow:**
```
Alice generates: ephemeral_private_A, ephemeral_public_A
Bob generates:   ephemeral_private_B, ephemeral_public_B

Alice → Bob: ephemeral_public_A
Bob → Alice: ephemeral_public_B

Both compute:
shared_secret = ECDH(my_private, their_public)
```

**libsodium function:** `crypto_scalarmult()`

---

### 3. BLAKE2b (Key Derivation)

**Purpose:** Derive session key from shared secret

**Why BLAKE2b?**
- Faster than SHA-256
- Cryptographically secure
- Variable output length

**Usage:**
```
session_key = BLAKE2b(
    shared_secret || 
    alice_pubkey || 
    bob_pubkey || 
    "P2PNetSessionKey"
)
```

**libsodium function:** `crypto_generichash()`

---

## Handshake Protocol

### 4-Way Handshake

```
Client (Alice)                          Server (Bob)
==============                          ============

1. ClientHello ─────────────────────→
   - Alice's Ed25519 public key

2.              ←───────────────────  ServerHello
                                       - Bob's Ed25519 public key
                                       - Random challenge (32 bytes)

3. KeyExchange ─────────────────────→
   - Alice's X25519 ephemeral public key
   - Signature(challenge || alice_ephemeral_pubkey)

4.              ←───────────────────  Accept
                                       - Bob's X25519 ephemeral public key
                                       - Signature(challenge || bob_ephemeral_pubkey || alice_ephemeral_pubkey)

✅ Session established
Both sides compute: shared_secret = X25519(my_ephemeral_private, their_ephemeral_public)
Both sides derive:  session_key = BLAKE2b(shared_secret || alice_id || bob_id)
```

---

## Message Format

### Wire Protocol

All messages prefixed with **message type** (1 byte):

| Type | Name | Description |
|------|------|-------------|
| `0x01` | ClientHello | Client introduces itself |
| `0x02` | ServerHello | Server responds with challenge |
| `0x03` | KeyExchange | Client proves identity + sends ephemeral key |
| `0x04` | Accept | Server accepts + sends ephemeral key |
| `0xFF` | Error | Handshake failed |

---

### Message 1: ClientHello

```
┌──────────┬─────────────────────────┐
│ Type (1) │ ClientPubKey (32)       │
│ 0x01     │ Ed25519 public key      │
└──────────┴─────────────────────────┘

Total: 33 bytes
```

---

### Message 2: ServerHello

```
┌──────────┬─────────────────────┬──────────────────┐
│ Type (1) │ ServerPubKey (32)   │ Challenge (32)   │
│ 0x02     │ Ed25519 public key  │ Random nonce     │
└──────────┴─────────────────────┴──────────────────┘

Total: 65 bytes
```

**Challenge generation:** `randombytes_buf(challenge, 32)`

---

### Message 3: KeyExchange

```
┌──────────┬──────────────────────┬──────────────────┐
│ Type (1) │ EphemeralPubKey (32) │ Signature (64)   │
│ 0x03     │ X25519 public key    │ Ed25519 sig      │
└──────────┴──────────────────────┴──────────────────┘

Total: 97 bytes

Signature over: challenge || ephemeral_pubkey
```

---

### Message 4: Accept

```
┌──────────┬──────────────────────┬──────────────────┐
│ Type (1) │ EphemeralPubKey (32) │ Signature (64)   │
│ 0x04     │ X25519 public key    │ Ed25519 sig      │
└──────────┴──────────────────────┴──────────────────┘

Total: 97 bytes

Signature over: challenge || server_ephemeral_pubkey || client_ephemeral_pubkey
```

---

## API Design

### Session Management

```c
typedef struct p2p_session p2p_session_t;

// Client-side handshake
p2p_session_t* p2p_handshake_client(
    p2p_socket_t* sock,
    p2p_keypair_t* my_keypair,
    const uint8_t* expected_peer_pubkey  // Can be NULL to allow any peer
);

// Server-side handshake
p2p_session_t* p2p_handshake_server(
    p2p_socket_t* sock,
    p2p_keypair_t* my_keypair,
    const uint8_t** allowed_peers,  // Array of allowed public keys (NULL = allow all)
    size_t num_allowed
);

// Session info
const uint8_t* p2p_session_peer_pubkey(p2p_session_t* session);
const uint8_t* p2p_session_key(p2p_session_t* session);

// Cleanup
void p2p_session_free(p2p_session_t* session);
```

---

### Session Structure (Opaque)

**Internal fields** (ikke eksponert til bruker):
- `session_key[32]` - Derived encryption key
- `peer_pubkey[32]` - Verified peer identity
- `nonce_counter` - For message nonce generation (Milestone 2.3)

---

## Implementation Steps

### Week 1: Core Handshake

**Day 1-2:** Message serialization
- Wire format structs
- Send/receive functions
- Message type handling

**Day 3-4:** Client-side handshake
- `p2p_handshake_client()` implementation
- Send ClientHello
- Receive ServerHello
- Verify challenge
- Send KeyExchange
- Derive session key

**Day 5:** Server-side handshake
- `p2p_handshake_server()` implementation
- Receive ClientHello
- Send ServerHello with challenge
- Verify KeyExchange
- Send Accept
- Derive session key

---

### Week 2: Testing & Integration

**Day 1-2:** Unit tests
- Test each message type
- Test signature verification
- Test key derivation
- Test error cases (invalid signature, wrong peer, etc.)

**Day 3-4:** Integration testing
- Client-server handshake test
- Multiple handshakes (different peers)
- Concurrent handshakes

**Day 5:** Example programs
- `10_secure_server.exe` - Server that requires handshake
- `11_secure_client.exe` - Client that performs handshake

---

## Error Handling

### Handshake Failures

| Error | Cause | Action |
|-------|-------|--------|
| **Invalid signature** | Peer doesn't have private key | Reject connection |
| **Wrong peer identity** | Unexpected public key | Reject connection |
| **Timeout** | Peer doesn't respond | Abort handshake |
| **Malformed message** | Invalid wire format | Reject connection |
| **Replay attack** | Challenge seen before | Reject connection |

**All failures return `NULL` from handshake function.**

---

## Security Considerations

### Threat Model

**Protected against:**
- ✅ Man-in-the-middle (MITM) - Signatures verified
- ✅ Replay attacks - Challenge used once
- ✅ Identity spoofing - Ed25519 signatures
- ✅ Key compromise (past sessions) - Perfect forward secrecy

**Not protected against (out of scope):**
- ❌ Denial of Service (DoS) - Rate limiting in Milestone 2.4
- ❌ Traffic analysis - Encrypted messaging in Milestone 2.3
- ❌ Compromised endpoint - Can't protect if attacker has private key

---

### Best Practices

1. **Verify peer identity**
   - Always check `expected_peer_pubkey` if you know who you're connecting to
   - Use `NULL` only for discovery/initial contact

2. **Ephemeral keys**
   - Generated fresh for each session
   - Wiped from memory after session established

3. **Challenge freshness**
   - New random challenge for each handshake
   - Never reuse challenges

4. **Session key derivation**
   - Include both identities (prevents key confusion)
   - Use domain separation string ("P2PNetSessionKey")

---

## Testing Strategy

### Unit Tests

1. **Message serialization**
   - Serialize → Deserialize roundtrip
   - Invalid format handling

2. **Signature verification**
   - Valid signature passes
   - Invalid signature fails
   - Wrong signer fails

3. **Key derivation**
   - Deterministic (same inputs = same output)
   - Different for different peers

4. **Session creation**
   - Client-server handshake succeeds
   - Both sides derive same session key

---

### Integration Tests

1. **Basic handshake**
   ```
   Server: Start listening
   Client: Connect and handshake
   Verify: Session established
   ```

2. **Wrong peer rejection**
   ```
   Client: Expects Alice
   Server: Is Bob
   Verify: Handshake fails
   ```

3. **Invalid signature**
   ```
   Attacker: Sends wrong signature
   Verify: Handshake fails
   ```

4. **Concurrent handshakes**
   ```
   Multiple clients connect simultaneously
   Verify: All succeed independently
   ```

---

## Example Programs

### `10_secure_server.exe`

**Purpose:** Server that requires authenticated handshake

**Usage:**
```bash
# Generate server identity first
build\08_generate_identity.exe
# Rename
move identity.key server.key

# Start server
build\10_secure_server.exe server.key
```

**Output:**
```
========================================
 Secure Server (Milestone 2.2)         
========================================

[INFO] Loading server identity...
[OK] Server identity: vbX_SEj0gUskjRy...

[INFO] Listening on port 8080
[INFO] Waiting for clients...

[OK] Client connected from 127.0.0.1:52341
[INFO] Starting handshake...
[OK] Handshake complete!
[OK] Peer identity: NYX4xWHz0x3I0gE...
[INFO] Session established
```

---

### `11_secure_client.exe`

**Purpose:** Client that performs handshake

**Usage:**
```bash
# Generate client identity
build\08_generate_identity.exe
# Rename
move identity.key client.key

# Connect to server
build\11_secure_client.exe client.key <server-pubkey>
```

**Output:**
```
========================================
 Secure Client (Milestone 2.2)         
========================================

[INFO] Loading client identity...
[OK] Client identity: NYX4xWHz0x3I0gE...

[INFO] Connecting to 127.0.0.1:8080...
[OK] Connected

[INFO] Starting handshake...
[OK] Handshake complete!
[OK] Peer identity: vbX_SEj0gUskjRy...
[INFO] Session established

Ready for encrypted communication!
```

---

## Files to Create

```
c-lib/
├── include/p2pnet/
│   ├── handshake.h           ← NEW (API)
│   └── session.h             ← NEW (Session management)
├── src/crypto/
│   ├── handshake.c           ← NEW (Handshake logic)
│   └── session.c             ← NEW (Session management)
├── examples/
│   ├── 10_secure_server.c    ← NEW
│   └── 11_secure_client.c    ← NEW
└── tests/
    └── test_handshake.c      ← NEW
```

---

## Dependencies

### External
- **libsodium** (already installed from Milestone 2.1)
  - `crypto_sign()` / `crypto_sign_verify()` - Ed25519 signatures
  - `crypto_box_keypair()` - X25519 keypair generation
  - `crypto_scalarmult()` - X25519 ECDH
  - `crypto_generichash()` - BLAKE2b KDF
  - `randombytes_buf()` - Challenge generation

### Internal
- Milestone 2.1 (Keypair & Identity) ✅
- Milestone 1.2 (Message Framing) ✅
- Milestone 1.1 (Socket API) ✅

---

## Performance Expectations

### Handshake Latency

**Breakdown:**
```
1. ClientHello:           ~1ms  (send 33 bytes)
2. ServerHello:           ~1ms  (receive 65 bytes)
3. KeyExchange:           ~2ms  (sign + send 97 bytes)
4. Accept:                ~2ms  (receive + verify 97 bytes)
5. Session key derivation: ~0.5ms (BLAKE2b)
───────────────────────────────────────
Total:                    ~6.5ms
```

**Plus network RTT:** 2-4ms (localhost) or 20-100ms (internet)

**Total handshake time:** ~10-110ms depending on network

---

### Computational Cost

| Operation | Time | Notes |
|-----------|------|-------|
| X25519 keypair generation | ~0.1ms | Once per handshake |
| Ed25519 signature | ~0.05ms | Twice per handshake |
| Ed25519 verification | ~0.1ms | Twice per handshake |
| X25519 ECDH | ~0.1ms | Once per handshake |
| BLAKE2b KDF | ~0.05ms | Once per handshake |
| **Total CPU** | **~0.6ms** | Very fast! |

---

## Known Limitations

### Current Scope

✅ **Implemented:**
- Mutual authentication
- Perfect forward secrecy
- MITM protection
- Replay protection

❌ **Not implemented (future work):**
- Session resumption (re-establish without full handshake)
- Key ratcheting (Signal Double Ratchet) - Milestone 2.3 kandidat
- Certificate authority / web of trust
- Revocation mechanism

---

### Assumptions

1. **Network reliability**
   - Assumes TCP connection stays alive during handshake
   - No timeout mechanism (yet)

2. **Peer trust**
   - Once handshake succeeds, peer is trusted
   - No re-authentication during session

3. **Single session per connection**
   - One handshake = one session
   - New connection required for new session

---

## Success Criteria

Milestone 2.2 complete when:

- [x] Client can initiate handshake
- [x] Server can respond to handshake
- [x] Both sides verify identity via signatures
- [x] Both sides derive same session key
- [x] Invalid signatures rejected
- [x] Wrong peer identity rejected
- [x] Example programs work (client + server)
- [x] Unit tests pass (100%)
- [x] Documentation complete

---

## What's Next: Milestone 2.3

### Encrypted Messaging 🔒

**Goal:** Use session key to encrypt all messages

**Key components:**
- ChaCha20-Poly1305 AEAD encryption
- Nonce management (counter-based)
- Message authentication (MAC)
- Integration with message framing (Milestone 1.2)

**Estimated time:** 1 uke

---

## References

### Protocol Design
- [Signal Protocol](https://signal.org/docs/) - Inspiration for handshake
- [Noise Protocol Framework](https://noiseprotocol.org/) - Handshake patterns
- [WireGuard](https://www.wireguard.com/protocol/) - Modern VPN handshake

### Cryptography
- [X25519 spec (RFC 7748)](https://datatracker.ietf.org/doc/html/rfc7748)
- [Ed25519 paper](https://ed25519.cr.yp.to/)
- [BLAKE2b spec (RFC 7693)](https://datatracker.ietf.org/doc/html/rfc7693)

---

**Author:** [Your Name]  
**Created:** 21. januar 2026  
**Status:** Planning 📋

---

**Previous:** [Milestone 2.1 - Keypair & Identity](MILESTONE_2.1_COMPLETE.md)  
**Next:** Start implementing! 🚀
