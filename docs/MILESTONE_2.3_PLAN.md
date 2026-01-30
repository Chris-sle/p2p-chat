# Milestone 2.3: Encrypted Messaging - PLAN

**Status:** 📋 Planlagt  
**Estimated Time:** 1 uke  
**Prerequisites:** Milestone 2.2 ✅

---

## Mål

Implementere end-to-end encrypted messaging ved å bruke session key fra handshake. Alle meldinger mellom peers skal være:
- Kryptert (confidentiality)
- Autentisert (integrity + authenticity)
- Beskyttet mot replay attacks (nonce-based)

---

## Problemet som løses

### Current State (etter Milestone 2.2)

```
Alice                                     Bob
=====                                     ===

✅ Session established                    ✅ Session established
✅ Both have session_key                  ✅ Both have session_key

❌ Messages sent in plaintext             ❌ Messages sent in plaintext
❌ No integrity protection                ❌ No integrity protection
❌ Anyone can read messages               ❌ Anyone can read messages
❌ Anyone can modify messages             ❌ Anyone can modify messages
```

**Problem:** Man har en sikker session, men sender fortsatt data i klartekst!

---

### After Milestone 2.3

```
Alice                                     Bob
=====                                     ===

✅ Session established                    ✅ Session established
✅ All messages encrypted                 ✅ All messages encrypted
✅ Integrity verified (MAC)               ✅ Integrity verified (MAC)
✅ Nonce prevents replay                  ✅ Nonce prevents replay

"Hello Bob" → [encrypted]  ────────→  [decrypted] → "Hello Bob"
              (unreadable)                (verified)

Observer sees: [random bytes] 🔒
Attacker cannot: read, modify, or replay messages ✅
```

---

## Security Properties

### What I achieve:

| Property | How |
|----------|-----|
| **Confidentiality** | ChaCha20-Poly1305 AEAD encryption |
| **Authenticity** | Poly1305 MAC (included in AEAD) |
| **Integrity** | MAC verification (detects tampering) |
| **Replay protection** | Monotonic nonce counter |
| **Forward secrecy** | Already achieved in Milestone 2.2 |

---

## Cryptographic Component: ChaCha20-Poly1305

### What is AEAD?

**AEAD = Authenticated Encryption with Associated Data**

- **Encryption:** ChaCha20 stream cipher
- **Authentication:** Poly1305 MAC
- **Combined:** Single operation (encrypt + MAC)

### Why ChaCha20-Poly1305?

| Feature | ChaCha20-Poly1305 | AES-GCM |
|---------|-------------------|---------|
| **Speed (software)** | ✅ Very fast | ⚠️ Slow without hardware |
| **Security** | ✅ Proven secure | ✅ Proven secure |
| **Patent-free** | ✅ Yes | ✅ Yes |
| **Timing attacks** | ✅ Resistant | ⚠️ Vulnerable (without AES-NI) |
| **Nonce size** | 12 bytes (96 bits) | 12 bytes (96 bits) |

**Used by:** TLS 1.3, WireGuard, Signal, Google Chrome

**libsodium function:** `crypto_aead_chacha20poly1305_ietf_encrypt()`

---

## Encrypted Message Format

### Wire Format

```
┌───────────┬────────────┬──────────────────┬─────────────┐
│ Length(4) │ Nonce(12)  │ Ciphertext(?)    │ MAC(16)     │
│ uint32_t  │ 96 bits    │ Variable length  │ 128 bits    │
└───────────┴────────────┴──────────────────┴─────────────┘

Total overhead: 4 + 12 + 16 = 32 bytes
```

**Fields:**
- **Length:** Total size of (Nonce + Ciphertext + MAC)
- **Nonce:** Counter-based (prevents replay)
- **Ciphertext:** Encrypted plaintext
- **MAC:** Poly1305 authentication tag

---

### Nonce Management

**Critical:** Nonce MUST NEVER be reused with same key!

**Our strategy: Counter-based nonces**

```
Nonce format (12 bytes):
┌──────────────────────┬──────────────────┐
│ Counter (8 bytes)    │ Padding (4 bytes) │
│ uint64_t (big-endian)│ 0x00000000        │
└──────────────────────┴──────────────────┘

Counter starts at: 0
Incremented by: 1 (after each message)
Max messages: 2^64 (18 quintillion) - practically unlimited
```

**Why counter-based?**
- ✅ Guaranteed unique (monotonic)
- ✅ Simple to implement
- ✅ No random number generation needed
- ✅ Detects out-of-order messages
- ⚠️ Requires state (counter stored in session)

---

### Message Flow

```
Sender                                    Receiver
======                                    ========

plaintext = "Hello"
nonce = counter_send++ || 0x00000000

ciphertext || mac = 
  ChaCha20-Poly1305(session_key, nonce, plaintext)

[length || nonce || ciphertext || mac] ──→

                                          Receive: length || nonce || ciphertext || mac
                                          
                                          Verify: nonce > last_received_nonce
                                          
                                          plaintext = 
                                            ChaCha20-Poly1305_decrypt(
                                              session_key, nonce, ciphertext, mac
                                            )
                                          
                                          if decrypt success:
                                            last_received_nonce = nonce
                                            process(plaintext)
                                          else:
                                            reject (tampering detected)
```

---

## API Design

### Encrypted Send/Receive

```c
// Send encrypted message
int p2p_session_send(p2p_session_t* session,
                     p2p_socket_t* sock,
                     const char* data,
                     size_t length);

// Receive and decrypt message
p2p_message_t* p2p_session_recv(p2p_session_t* session,
                                 p2p_socket_t* sock);
```

**Key points:**
- Uses session key automatically
- Manages nonce internally
- Returns `p2p_message_t` (reuse from Milestone 1.2)
- Caller must free message with `p2p_message_free()`

---

### Integration with Existing Message API

**Current (Milestone 1.2):**
```c
p2p_message_t* msg = p2p_message_create("Hello");
p2p_message_send(msg, sock);
// Sent in plaintext ❌
```

**New (Milestone 2.3):**
```c
p2p_session_send(session, sock, "Hello", 5);
// Sent encrypted ✅
```

**Receive:**
```c
p2p_message_t* msg = p2p_session_recv(session, sock);
if (msg) {
    printf("Received: %s\n", msg->data);
    p2p_message_free(msg);
}
```

---

## Implementation Steps

### Week 1: Core Encryption

**Day 1:** Nonce management
- Add `send_nonce` and `recv_nonce` to session struct
- Implement counter increment
- Implement nonce serialization (big-endian)

**Day 2:** Encryption functions
- `p2p_session_send()` implementation
  - Construct nonce from counter
  - Encrypt plaintext with ChaCha20-Poly1305
  - Send: length || nonce || ciphertext || mac

**Day 3:** Decryption functions
- `p2p_session_recv()` implementation
  - Receive: length || nonce || ciphertext || mac
  - Verify nonce > last received
  - Decrypt and verify MAC
  - Return plaintext as `p2p_message_t`

**Day 4-5:** Testing & Integration
- Unit tests (encrypt/decrypt roundtrip)
- Integration tests (client-server encrypted chat)
- Example programs

---

## Error Handling

### Encryption Failures

| Error | Cause | Action |
|-------|-------|--------|
| **Counter overflow** | Sent 2^64 messages | Close session, re-handshake |
| **Nonce reuse detected** | Internal bug | Abort (critical error) |
| **Send failed** | Network error | Return error, caller retries |

---

### Decryption Failures

| Error | Cause | Action |
|-------|-------|--------|
| **MAC verification failed** | Message tampered | Reject message, log warning |
| **Nonce out of order** | Replay attack or packet loss | Reject message |
| **Nonce rewind** | Replay attack | Reject message, log critical |
| **Receive failed** | Connection closed | Return NULL |

---

## Security Considerations

### Threat Model

**Protected against:**
- ✅ Eavesdropping - ChaCha20 encryption
- ✅ Tampering - Poly1305 MAC verification
- ✅ Replay attacks - Monotonic nonce check
- ✅ MITM (already) - Handshake in Milestone 2.2

**Not protected against:**
- ❌ Traffic analysis - Attacker sees message sizes/timing
- ❌ Denial of Service - No rate limiting
- ❌ Side-channel attacks - Timing attacks (mitigated by libsodium)

---

### Nonce Reuse (Critical!)

**What happens if nonce reused?**

```
Message 1: Encrypt("Hello", nonce=5) → ciphertext_1
Message 2: Encrypt("World", nonce=5) → ciphertext_2

Attacker XORs ciphertexts:
ciphertext_1 ⊕ ciphertext_2 = plaintext_1 ⊕ plaintext_2

Result: Attacker can recover plaintexts! 💥
```

**Our protection:**
- Counter stored in session (never resets)
- Counter MUST be incremented atomically
- Check for overflow before sending

---

### MAC Verification

**Always verify MAC before processing plaintext!**

```
❌ BAD:
plaintext = decrypt(ciphertext)
process(plaintext)
if (!verify_mac(mac)) return error;  // Too late!

✅ GOOD:
if (!verify_mac(mac)) return error;
plaintext = decrypt(ciphertext);
process(plaintext);
```

**libsodium does this correctly** (decrypt fails if MAC invalid) ✅

---

## Testing Strategy

### Unit Tests

1. **Encryption roundtrip**
   - Encrypt message → decrypt → verify plaintext matches

2. **Nonce increment**
   - Send 3 messages → verify nonces are 0, 1, 2

3. **Tampering detection**
   - Encrypt message → flip bit in ciphertext → decrypt fails

4. **Replay detection**
   - Send message with nonce=5
   - Try to receive again with nonce=5 → rejected

5. **Out-of-order messages**
   - Send nonce=5, then nonce=4 → second rejected

---

### Integration Tests

1. **Encrypted chat**
   - Client sends: "Hello"
   - Server receives: "Hello"
   - Server sends: "Hi back"
   - Client receives: "Hi back"

2. **Multiple messages**
   - Send 100 messages in sequence
   - Verify all received correctly
   - Verify nonces increment properly

3. **Large messages**
   - Send 1MB message
   - Verify correct encryption/decryption

4. **Connection persistence**
   - Send 10 messages
   - Server restarts (new session)
   - Client handshake again
   - Send 10 more messages

---

## Example Programs

### `12_encrypted_chat_server.exe`

Server that accepts encrypted messages.

**Features:**
- Performs handshake
- Receives encrypted messages
- Echoes back encrypted response

**Usage:**
```bash
build\12_encrypted_chat_server.exe server.key
```

**Flow:**
```
1. Client connects
2. Handshake completes
3. Wait for encrypted message
4. Decrypt and display
5. Echo back (encrypted)
6. Repeat
```

---

### `13_encrypted_chat_client.exe`

Client that sends encrypted messages.

**Features:**
- Performs handshake
- Sends encrypted messages
- Receives encrypted responses

**Usage:**
```bash
build\13_encrypted_chat_client.exe client.key
```

**Flow:**
```
1. Connect to server
2. Handshake completes
3. Read user input
4. Encrypt and send
5. Wait for response
6. Decrypt and display
7. Repeat
```

---

## Performance Expectations

### Encryption Overhead

**ChaCha20-Poly1305 performance:**
```
Operation: Encrypt 1KB message
Time: ~0.05ms (CPU)
Throughput: ~20 MB/s (single core)

Operation: Decrypt 1KB message
Time: ~0.05ms (CPU)
Throughput: ~20 MB/s (single core)
```

**Very fast!** Much faster than network I/O ✅

---

### Message Overhead

```
Plaintext: "Hello" (5 bytes)

Encrypted message:
- Length: 4 bytes
- Nonce: 12 bytes
- Ciphertext: 5 bytes (same as plaintext)
- MAC: 16 bytes
──────────────────
Total: 37 bytes

Overhead: 32 bytes per message (640% for 5-byte message)
```

**For larger messages:**
```
Plaintext: 1KB (1024 bytes)
Encrypted: 1056 bytes
Overhead: 32 bytes (3%)

Plaintext: 1MB (1048576 bytes)
Encrypted: 1048608 bytes
Overhead: 32 bytes (0.003%)
```

**Conclusion:** Overhead negligible for normal messages ✅

---

## Files to Create

```
c-lib/
├── include/p2pnet/
│   └── encryption.h          ← NEW (Encrypted send/recv API)
├── src/crypto/
│   └── encryption.c          ← NEW (ChaCha20-Poly1305 implementation)
├── examples/
│   ├── 12_encrypted_chat_server.c  ← NEW
│   └── 13_encrypted_chat_client.c  ← NEW
└── tests/
    └── test_encryption.c     ← NEW
```

---

## Dependencies

### External
- **libsodium** (already installed)
  - `crypto_aead_chacha20poly1305_ietf_encrypt()`
  - `crypto_aead_chacha20poly1305_ietf_decrypt()`

### Internal
- Milestone 2.2 (Handshake & Session) ✅
- Milestone 1.2 (Message Framing) ✅
- Milestone 1.1 (Socket API) ✅

---

## Known Limitations

### Current Scope

✅ **Implemented:**
- ChaCha20-Poly1305 encryption
- Counter-based nonces
- MAC verification
- Replay protection

❌ **Not implemented (future work):**
- Message fragmentation (large messages)
- Compression (before encryption)
- Padding (hide message length)
- Key ratcheting (Signal Double Ratchet)
- Out-of-order message handling

---

### Design Decisions

**Why counter-based nonces instead of random?**
- Counter guarantees uniqueness
- Detects replay attacks automatically
- Simpler implementation
- Signal/WireGuard use similar approach

**Why not encrypt-then-MAC separately?**
- AEAD is faster (single operation)
- AEAD is safer (hard to misuse)
- Industry standard (TLS 1.3, WireGuard)

**Why ChaCha20 instead of AES?**
- Faster in software (no hardware acceleration needed)
- Timing-attack resistant
- Recommended by security experts
- Used by modern protocols

---

## Success Criteria

Milestone 2.3 complete when:

- [x] Messages encrypted with ChaCha20-Poly1305
- [x] Nonce management (counter-based)
- [x] MAC verification on receive
- [x] Replay attack detection
- [x] Tampering detection (MAC verification)
- [x] Integration with session API
- [x] Example programs work (encrypted chat)
- [x] Unit tests pass (100%)
- [x] Documentation complete

---

## What's Next: Phase 3

### Peer Discovery & DHT (Milestone 3.1)

**Goal:** Find peers without central server

**Key components:**
- Kademlia DHT implementation
- Peer routing table
- Bootstrap nodes
- NAT traversal preparation

**Estimated time:** 2-3 uker

---

## References

### Cryptography
- [ChaCha20-Poly1305 (RFC 8439)](https://datatracker.ietf.org/doc/html/rfc8439)
- [AEAD Overview (RFC 5116)](https://datatracker.ietf.org/doc/html/rfc5116)
- [libsodium AEAD docs](https://doc.libsodium.org/secret-key_cryptography/aead)

### Protocol Design
- [WireGuard whitepaper](https://www.wireguard.com/papers/wireguard.pdf)
- [Signal Protocol](https://signal.org/docs/)
- [TLS 1.3 (RFC 8446)](https://datatracker.ietf.org/doc/html/rfc8446)

---

**Author:** Chris-sle
**Created:** 30. januar 2026  
**Status:** Planning 📋

---

**Previous:** [Milestone 2.2 - Handshake Protocol](MILESTONE_2.2_COMPLETE.md)  
**Next:** Start implementing! 🚀
```