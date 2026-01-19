## 3. `docs/MILESTONE_2.2_PLAN.md` (Placeholder)


# Milestone 2.2: Handshake Protocol - PLAN

**Status:** 📋 Planlagt  
**Estimated Time:** 1-2 uker  
**Prerequisites:** Milestone 2.1 ✅

---

## Mål

Implementere en authenticated handshake protocol med key exchange, slik at peers kan verifisere hverandres identitet og etablere en encrypted session.

---

*Detaljert plan kommer etter Milestone 2.1 er ferdig.*

---

## High-Level Overview

### Handshake Flow

```
Client                          Server
======                          ======

1. ClientHello ───────────────→
   (identity)

2.              ←──────────────  ServerHello
                                 (identity + challenge)

3. KeyExchange ────────────────→
   (ephemeral key + signature)

4.              ←──────────────  Accept
                                 (ephemeral key + signature)

5. Both derive SessionKey via ECDH
   → Encrypted communication ready
```

### Key Components

- Challenge-response authentication
- X25519 key exchange (ECDH)
- Ed25519 signatures for authentication
- Session key derivation (BLAKE2b KDF)
- Replay attack prevention

---

**Previous:** [Milestone 2.1](MILESTONE_2.1_PLAN.md)  
**Next:** [Milestone 2.3](MILESTONE_2.3_PLAN.md)
```