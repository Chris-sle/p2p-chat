# Milestone 2.1: Keypair & Identity - PLAN

**Status:** 📋 Planlagt  
**Estimated Time:** 1 uke  
**Prerequisites:** Milestone 1.1-1.3 ✅

---

## Mål

Implementere et identitetssystem basert på Ed25519 public key cryptography, hvor hver peer har en unik keypair som fungerer som deres identitet i P2P nettverket.

---

## Bakgrunn

### Hvorfor Ed25519?

**Ed25519 er en digital signature algorithm med:**
- ✅ Små nøkler (32 bytes public, 64 bytes private)
- ✅ Rask signing og verification
- ✅ Høy sikkerhet (128-bit security level)
- ✅ Deterministic signatures (ingen random nonce needed)
- ✅ Immune to side-channel attacks
- ✅ Widely used (SSH, Signal, Tor, etc.)

**Comparison:**
| Algorithm | Key Size | Security | Speed | Status |
|-----------|----------|----------|-------|--------|
| RSA-2048 | 2048 bits | ~112 bit | Slow | Legacy |
| ECDSA P-256 | 256 bits | ~128 bit | Medium | Good |
| **Ed25519** | **256 bits** | **~128 bit** | **Fast** | **Best** ✅ |

---

### Identity Model

**Concept:** Your public key IS your identity

```
Identity = Ed25519 Public Key (32 bytes)

Example (Base64):
"NYX4xWHz0x3I0gEv2JBV4zEwKjMp9rLfQABC"
```

**Why this works:**
- ✅ No central authority needed (P2P friendly)
- ✅ Self-sovereign (you control your identity)
- ✅ Verifiable (can prove you own identity via signature)
- ✅ Portable (can use same identity across devices)
- ✅ Unique (cryptographically guaranteed to be unique)

---

## Scope

### In Scope

✅ Ed25519 keypair generation  
✅ Secure random number generation  
✅ Keypair serialization (save to file)  
✅ Keypair deserialization (load from file)  
✅ Public key fingerprint (Base64 encoded)  
✅ Keypair verification (check if valid)  
✅ Secure memory handling (`sodium_memzero()`)  
✅ File permission management (Windows + Unix)  
✅ Test programs  

### Out of Scope

❌ Password-protected keypairs (future: encrypt private key with password)  
❌ Hardware security module (HSM) support  
❌ Key revocation mechanism  
❌ Multi-device key synchronization  
❌ Backup/recovery mechanisms  
❌ Key rotation  



## Implementation Details

#### 1. Initialization

Track if libsodium is initialized

---

#### 2. Keypair Generation

**libsodium function used:**
- `crypto_sign_keypair()` - Generates Ed25519 keypair
  - Public key: 32 bytes
  - Secret key: 64 bytes (32B seed + 32B public key copy)

---

#### 3. Secure Memory Cleanup

**Why `sodium_memzero()` instead of `memset()`?**

```c
// BAD: Compiler can optimize this away
memset(keypair, 0, sizeof(p2p_keypair_t));

// GOOD: Guaranteed to wipe memory
sodium_memzero(keypair, sizeof(p2p_keypair_t));
```

---

#### 4. File Serialization

**File format:**
```
-----BEGIN P2P PRIVATE KEY-----
<Base64 encoded secret key (88 characters)>
-----END P2P PRIVATE KEY-----
-----BEGIN P2P PUBLIC KEY-----
<Base64 encoded public key (44 characters)>
-----END P2P PUBLIC KEY-----
```

#### 5. File Deserialization

Encode secret key to Base64
Encode public key to Base64
Write to file
Set file permissions (user read/write only)
    - Windows: Set readonly flag (better than nothing)

---

#### 6. Fingerprint Display

**Output example:**
```
NYX4xWHz0x3I0gEv2JBV4zEwKjMp9rLfQABC
```

- 43 characters (32 bytes * 4/3, Base64 encoding)
- URL-safe (can copy-paste, use in filenames)
- No padding (cleaner look)

---

#### 7. Keypair Verification

**Why this works:**
- Ed25519 secret key is 64 bytes: `[32B seed][32B public key copy]`
- libsodium stores public key in secret key for convenience
- verify they match

---

## Files to Create

```
c-lib/
├── include/p2pnet/
│   ├── p2pnet.h              (update: include crypto.h)
│   └── crypto.h              ← NEW
├── src/
│   └── crypto/
│       └── keypair.c         ← NEW
├── examples/
│   ├── 08_generate_identity.c  ← NEW
│   └── 09_verify_identity.c    ← NEW
└── tests/
    └── test_crypto.c         ← NEW (unit tests)
```

## Dependencies

### Install libsodium (Windows/MSYS2):

```bash
pacman -S mingw-w64-ucrt-x86_64-libsodium
```

### Verify installation:

```bash
pkg-config --cflags libsodium
pkg-config --libs libsodium
```

---

## Security Considerations

### Key Storage
- **File permissions:** 0600 (user read/write only) on Unix
- **Location:** User home directory recommended (`~/.p2p/`)
- **Backup:** User responsible - NO auto-backup to cloud!
- **Encryption at rest:** Out of scope (future: password-protect keys)

### Key Generation
- Uses cryptographically secure RNG:
  - Linux: `/dev/urandom`
  - Windows: `CryptGenRandom` (via libsodium)
- Seeds are never reused

### Memory Safety
- Always use `sodium_memzero()` to wipe sensitive data
- Free keypairs immediately after use
- Never log private keys
- Never transmit private keys over network

### Best Practices
- ✅ Generate new keypair for each identity
- ✅ Keep private key file secure
- ✅ Share only public key / fingerprint
- ❌ Never email private key
- ❌ Never commit private key to git
- ❌ Never store private key in plaintext in database

---

## Success Criteria

Milestone 2.1 complete when:

- [x] Can generate Ed25519 keypair
- [x] Can save keypair to file (PEM-like format)
- [x] Can load keypair from file
- [x] Can get human-readable fingerprint (Base64)
- [x] File permissions set correctly (0600 or Windows equivalent)
- [x] Memory wiped securely after free
- [x] No compiler warnings
- [x] All tests pass
- [x] Example programs work
- [x] Documentation complete

---

## Timeline

**Day 1-2:** Setup + Implementation
- Install libsodium
- Implement keypair generation
- Implement save/load

**Day 3-4:** Testing + Examples
- Write unit tests
- Create example programs
- Test on Windows

**Day 5:** Documentation + Polish
- Write API docs
- Security review
- Final testing

---

## Next Steps

After Milestone 2.1:
→ [Milestone 2.2: Handshake Protocol](MILESTONE_2.2_PLAN.md)

---

**Author:** Chris-sle
**Created:** 14. januar 2026  
**Status:** Planning 📋
```