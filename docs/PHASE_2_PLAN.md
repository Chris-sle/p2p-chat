## 1. `docs/MILESTONE_2_PLAN.md` (Overordnet oversikt)


# Milestone 2: Security & Cryptography - PLAN

**Status:** 📋 Planlagt  
**Estimated Time:** 3-4 uker  
**Prerequisites:** Milestone 1.1, 1.2, 1.3 ✅

---

## Overordnet Mål

Implementere et komplett security layer for P2P chat med moderne kryptografi, slik at alle meldinger er end-to-end kryptert og peers kan verifisere hverandres identitet.

---

## Hvorfor trenger vi Security?

### Problemet med Milestone 1

**Current state (Milestone 1.3):**

```c
// All data sendes i klartekst!
p2p_message_t* msg = p2p_message_create("Secret message");
p2p_message_send(sock, msg);  // ← Anyone on network can read this!
```

**Trusler:**
- ❌ **Eavesdropping** - Hvem som helst kan lese meldinger
- ❌ **Man-in-the-middle (MITM)** - Noen kan late some de er noen andre
- ❌ **Message tampering** - Meldinger kan endres uten at vi vet det
- ❌ **No authentication** - Vi vet ikke hvem vi snakker med
- ❌ **No privacy** - All kommunikasjon er offentlig

**Eksempel angrep:**
```
Alice → "My password is hunter2" → [Attacker reads]
                                    [Attacker now has password]

Alice → Message to Bob          → [Attacker intercepts]
                                   [Attacker modifies]
                                   [Bob receives fake message]
```

---

### Løsningen: Modern Cryptography

**After Milestone 2:**

```c
// 1. Hver bruker har en identitet (Ed25519 keypair)
p2p_keypair_t* my_identity = p2p_keypair_generate();

// 2. Før kommunikasjon: Authenticated handshake
p2p_session_t* session = p2p_handshake_client(sock, my_identity, bob_pubkey);

// 3. Send encrypted message
p2p_secure_send(session, "Secret message");  // ← Encrypted! 🔒
```

**Beskyttelse:**
- ✅ **Confidentiality** - Bare riktig mottaker kan lese
- ✅ **Authentication** - Vi vet hvem vi snakker med
- ✅ **Integrity** - Vi vet om meldinger er endret
- ✅ **Forward secrecy** - Selv om nøkler leakes senere, er gamle meldinger trygge

---

## Cryptographic Algorithms

| Purpose | Algorithm | Why? |
|---------|-----------|------|
| **Digital signatures** | **Ed25519** | Fast, secure, små nøkler (32 bytes) |
| **Identity** | Ed25519 public key | 32 bytes = enkel å dele |
| **Key exchange** | **X25519 (ECDH)** | Perfect forward secrecy |
| **Encryption** | **ChaCha20-Poly1305** | Fast, secure AEAD cipher |
| **Hashing** | **BLAKE2b** | Raskere enn SHA-256, like sikker |

**Library:** [libsodium](https://libsodium.org/) - battle-tested, modern, lett å bruke

**Why libsodium?**
- ✅ Industry standard (brukes av Signal, WhatsApp concepts)
- ✅ Misuse-resistant API (vanskelig å gjøre feil)
- ✅ Cross-platform (Windows, Linux, macOS)
- ✅ Well-documented
- ✅ Active maintenance

---

## Milestone 2 Breakdown

### Milestone 2.1: Keypair & Identity 🔑
**Estimated:** 1 uke  
**Status:** 📋 Planlagt

**Mål:** Implementere identitetssystem basert på Ed25519 keypairs

**Deliverables:**
- Ed25519 keypair generation
- Public key som identitet (32 bytes)
- Keypair save/load (file-based)
- Key fingerprint (human-readable Base64)
- Memory-safe key handling

**Files:**
- `include/p2pnet/crypto.h`
- `src/crypto/keypair.c`
- `examples/08_generate_identity.c`
- `examples/09_verify_identity.c`

**Dependencies:**
- libsodium (external)
- Milestone 1.1-1.3 ✅

---

### Milestone 2.2: Handshake Protocol 🤝
**Estimated:** 1-2 uker  
**Status:** 📋 Planlagt

**Mål:** Implementere authenticated handshake med key exchange

**Deliverables:**
- Challenge-response authentication
- X25519 key exchange (ECDH)
- Session key derivation
- Replay attack prevention
- Mutual authentication

**Protocol:**
```
Client → Server: ClientHello (identity)
Server → Client: ServerHello (identity + challenge)
Client → Server: KeyExchange (ephemeral key + signature)
Server → Client: Accept (ephemeral key + signature)

Both derive: SessionKey = KDF(SharedSecret)
```

**Files:**
- `include/p2pnet/handshake.h`
- `src/crypto/handshake.c`
- `examples/10_secure_server.c`
- `examples/11_secure_client.c`

**Dependencies:**
- Milestone 2.1 ✅

---

### Milestone 2.3: Encrypted Messaging 🔒
**Estimated:** 1 uke  
**Status:** 📋 Planlagt

**Mål:** Implementere end-to-end encrypted messaging

**Deliverables:**
- ChaCha20-Poly1305 encryption/decryption
- Nonce management (prevents replay)
- Message authentication (AEAD)
- Integration med message framing
- Integration med event loop

**API:**
```c
// Send encrypted message
int p2p_secure_send(p2p_session_t* session, const char* plaintext);

// Receive encrypted message
char* p2p_secure_recv(p2p_session_t* session);
```

**Wire format:**
```
[4B: Length][24B: Nonce][N bytes: Ciphertext + 16B MAC]
```

**Files:**
- `include/p2pnet/secure_message.h`
- `src/crypto/secure_message.c`
- `examples/12_encrypted_chat.c`

**Dependencies:**
- Milestone 2.2 ✅

---

### Milestone 2.4: Security Hardening 🛡️
**Estimated:** 3-5 dager  
**Status:** 📋 Planlagt

**Mål:** Sikre systemet mot vanlige angrep

**Deliverables:**
- Rate limiting (prevent DoS)
- Connection timeout management
- Failed handshake handling
- Security audit checklist
- Basic penetration testing

**Files:**
- `docs/SECURITY.md`
- `docs/THREAT_MODEL.md`
- `tests/security_tests.c`

**Dependencies:**
- Milestone 2.1, 2.2, 2.3 ✅

---

## High-Level Architecture

### Before (Milestone 1)
```
┌────────────────────────────────────────┐
│         Application Layer              │
├────────────────────────────────────────┤
│      Message Framing (1.2)             │
├────────────────────────────────────────┤
│      Event Loop (1.3)                  │
├────────────────────────────────────────┤
│      TCP Sockets (1.1)                 │
└────────────────────────────────────────┘

All traffic: PLAINTEXT ❌
```

### After (Milestone 2)
```
┌────────────────────────────────────────┐
│         Application Layer              │
├────────────────────────────────────────┤
│    Encrypted Messaging (2.3) 🔒        │  ← NEW
├────────────────────────────────────────┤
│    Handshake & Auth (2.2) 🤝           │  ← NEW
├────────────────────────────────────────┤
│    Identity (2.1) 🔑                   │  ← NEW
├────────────────────────────────────────┤
│      Message Framing (1.2)             │
├────────────────────────────────────────┤
│      Event Loop (1.3)                  │
├────────────────────────────────────────┤
│      TCP Sockets (1.1)                 │
└────────────────────────────────────────┘

All traffic: ENCRYPTED ✅
```

---

## Security Properties

### What we achieve:

| Property | How |
|----------|-----|
| **Confidentiality** | ChaCha20 encryption (2.3) |
| **Authentication** | Ed25519 signatures (2.1, 2.2) |
| **Integrity** | Poly1305 MAC (2.3) |
| **Forward Secrecy** | Ephemeral X25519 keys (2.2) |
| **Identity** | Ed25519 public keys (2.1) |
| **Replay Protection** | Nonces (2.3) + Challenge (2.2) |
| **MITM Protection** | Signature verification (2.2) |

---

## Timeline

```
Week 1: Milestone 2.1 (Keypair & Identity)
├─ Day 1-2: Setup libsodium + keypair generation
├─ Day 3-4: Key save/load + fingerprint
└─ Day 5: Testing + examples

Week 2-3: Milestone 2.2 (Handshake Protocol)
├─ Week 2: Protocol design + client side
└─ Week 3: Server side + testing

Week 4: Milestone 2.3 (Encrypted Messaging)
├─ Day 1-2: Encryption/decryption
├─ Day 3-4: Integration with framing + event loop
└─ Day 5: Testing + examples

Week 4-5: Milestone 2.4 (Security Hardening)
└─ Security audit + documentation
```

**Total: 3-4 weeks**

---

## Success Criteria

### Milestone 2 Complete når:

- [x] Keypairs can be generated and stored securely
- [x] Peers can authenticate each other
- [x] All messages are end-to-end encrypted
- [x] Forward secrecy guaranteed
- [x] No plaintext leakage
- [x] Resistant to common attacks (MITM, replay, etc.)
- [x] Integration with event loop works
- [x] Documentation complete
- [x] Example programs work

---

## Risks and Mitigations

| Risk | Impact | Probability | Mitigation |
|------|--------|-------------|------------|
| Crypto implementation bugs | Critical | Medium | Use libsodium (battle-tested) |
| Key management errors | High | Medium | Follow best practices, use `sodium_memzero()` |
| Protocol design flaws | High | Low | Follow Signal protocol patterns |
| Performance degradation | Medium | Medium | Benchmark early, optimize if needed |
| Complex integration | Medium | High | Incremental testing at each step |

---

## Dependencies

### External
- **libsodium** (≥ 1.0.18)
  - Windows: `pacman -S mingw-w64-ucrt-x86_64-libsodium`
  - Linux: `apt install libsodium-dev`

### Internal
- Milestone 1.1 (Socket API) ✅
- Milestone 1.2 (Message Framing) ✅
- Milestone 1.3 (Event Loop) ✅

---

## Future Enhancements (Out of Scope)

**Not in Milestone 2, but possible later:**

- ❌ Multi-device sync (same identity on multiple devices)
- ❌ Group chat encryption
- ❌ Perfect forward secrecy with ratcheting (Signal Double Ratchet)
- ❌ Post-quantum cryptography
- ❌ Hardware security module (HSM) support
- ❌ Certificate authority / web of trust
- ❌ Key revocation mechanism

---

## Documentation Structure

```
docs/
├── MILESTONE_2_PLAN.md           ← This file (overview)
├── MILESTONE_2.1_PLAN.md         ← Detailed plan for keypair
├── MILESTONE_2.2_PLAN.md         ← Detailed plan for handshake
├── MILESTONE_2.3_PLAN.md         ← Detailed plan for encryption
├── MILESTONE_2.4_PLAN.md         ← Detailed plan for hardening
├── SECURITY.md                   ← Security best practices
└── THREAT_MODEL.md               ← Known threats and defenses
```

---

## References

### Cryptographic Standards
- [libsodium documentation](https://libsodium.gitbook.io/)
- [Ed25519 paper](https://ed25519.cr.yp.to/)
- [X25519 spec (RFC 7748)](https://datatracker.ietf.org/doc/html/rfc7748)
- [ChaCha20-Poly1305 (RFC 8439)](https://datatracker.ietf.org/doc/html/rfc8439)

### Protocol Design
- [Signal Protocol](https://signal.org/docs/)
- [Noise Protocol Framework](https://noiseprotocol.org/)

---

**Author:** [Your Name]  
**Created:** 14. januar 2026  
**Status:** Planning 📋

---

**Next Steps:**
1. Review this plan
2. Setup libsodium
3. Start [Milestone 2.1 Implementation](MILESTONE_2.1_PLAN.md)
