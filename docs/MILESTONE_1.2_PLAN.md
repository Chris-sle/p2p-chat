# Milestone 1.2: Message Framing - PLAN

**Status:** ✅ Komplett  
**Estimated Time:** 1 uke  
**Actual Time:** 2 dager (18-19. desember 2025)

---

## Mål

Implementere et message framing system som løser TCP stream-problemet, slik at applikasjoner kan sende og motta komplette meldinger uten å bekymre seg for partial reads/writes.

---

## Bakgrunn

### Problemet: TCP er en Byte Stream

TCP garanterer:
- ✅ Rekkefølge (in-order delivery)
- ✅ Pålitelighet (no data loss)

TCP garanterer **IKKE**:
- ❌ Message boundaries

**Eksempel:**

```c
// Sender
send(sock, "Hello", 5);
send(sock, "World", 5);

// Receiver kan få:
recv() = "HelloWorld"           // Begge i én pakke
recv() = "Hel"                  // Første del
recv() = "loWorld"              // Resten
recv() = "H", "elloW", "orld"   // Vilkårlig fragmentering
```

**Problem:** Applikasjonen vet ikke hvor en melding slutter og neste begynner.

---

## Løsninger (Evaluated)

### Option 1: Delimiter-Based (e.g., newline)

```
"Hello
"
"World
"
```

**Pros:**
- Simple
- Human-readable
- Common (HTTP, SMTP, etc.)

**Cons:**
- ❌ Delimiter kan være i data (escaping needed)
- ❌ Not suitable for binary data
- ❌ Must scan entire message for delimiter

**Verdict:** ❌ Rejected (not suitable for binary P2P data)

---

### Option 2: Fixed-Length Messages

```
[100 bytes message 1][100 bytes message 2]
```

**Pros:**
- Very simple
- No overhead

**Cons:**
- ❌ Inflexible (all messages same size)
- ❌ Wastes bandwidth on small messages
- ❌ Limits maximum message size

**Verdict:** ❌ Rejected (too inflexible)

---

### Option 3: Length-Prefix Framing ✅ SELECTED

```
[4 bytes: length][N bytes: data]
```

**Pros:**
- ✅ Works with binary data
- ✅ Variable-length messages
- ✅ Minimal overhead (4 bytes)
- ✅ Simple to implement
- ✅ Industry standard

**Cons:**
- Small overhead (4 bytes per message)

**Verdict:** ✅ **Selected** - Best balance of simplicity and flexibility

---

## Scope

### In Scope (Milestone 1.2)

✅ Length-prefix framing (4-byte header)  
✅ Message creation/destruction API  
✅ Send/receive framed messages  
✅ Handle partial sends/receives automatically  
✅ Network byte order conversion (big-endian)  
✅ Maximum message size protection (1MB)  
✅ Test programs (echo server/client)  

### Out of Scope

❌ Message compression  
❌ Encryption (Fase 2)  
❌ Message queuing (async, Milestone 1.3)  
❌ Multi-part messages (fragmentation)  
❌ Message types/versioning (future)  

---

## API Design

### Message Structure

```c
typedef struct {
    uint32_t length;  // Length in bytes
    uint8_t* data;    // Payload (heap-allocated)
} p2p_message_t;
```

### Core Functions

```c
// Creation
p2p_message_t* p2p_message_create(const char* text);
p2p_message_t* p2p_message_create_binary(const void* data, size_t length);

// Transfer
int p2p_message_send(p2p_socket_t* sock, p2p_message_t* msg);
p2p_message_t* p2p_message_recv(p2p_socket_t* sock);

// Cleanup
void p2p_message_free(p2p_message_t* msg);

// Utility
void p2p_message_print(const p2p_message_t* msg, const char* prefix);
```

### Design Principles

1. **Automatic handling:** User doesn't worry about partial I/O
2. **Memory safety:** Clear ownership (caller must free)
3. **Binary support:** No assumptions about null-termination
4. **Size limits:** Protect against DoS (max 1MB)

---

## Wire Format

```
┌─────────────────┬────────────────────────────┐
│  Length (4B)    │  Data (Length bytes)       │
│  (big-endian)   │                            │
└─────────────────┴────────────────────────────┘
 0   1   2   3     4   5   6   7   8   9  ...
```

**Example:** "Hello" (5 bytes)

```
Wire bytes: [0x00 0x00 0x00 0x05] [H e l l o]
            └─ length=5 (big-endian)  └─ data
```

**Why big-endian?**
- Network byte order standard (RFC 1700)
- Cross-platform compatibility
- x86 is little-endian, but network is big-endian

---

## Implementation Plan

### Phase 1: Data Structures
- [x] Define `p2p_message_t` struct
- [x] Define `P2P_MAX_MESSAGE_SIZE` constant

### Phase 2: Helper Functions
- [x] `send_exact()` - Send all bytes (handles partial sends)
- [x] `recv_exact()` - Receive exact N bytes (handles partial reads)

### Phase 3: Core API
- [x] `p2p_message_create()` - Allocate and copy data
- [x] `p2p_message_create_binary()` - Binary version
- [x] `p2p_message_send()` - Send length + data
- [x] `p2p_message_recv()` - Receive length, then data
- [x] `p2p_message_free()` - Free memory
- [x] `p2p_message_print()` - Debug output

### Phase 4: Testing
- [x] `examples/04_framed_server.c` - Echo server
- [x] `examples/05_framed_client.c` - Test client
- [x] Test edge cases (large messages, partial I/O)

---

## Technical Challenges

### Challenge 1: Partial Sends/Receives

**Problem:** `send()` and `recv()` may transfer fewer bytes than requested

**Solution:** Loop until all bytes transferred

```c
static int send_exact(p2p_socket_t* sock, const void* data, size_t length) {
    size_t total_sent = 0;
    const uint8_t* ptr = (const uint8_t*)data;

    while (total_sent < length) {
        ssize_t sent = p2p_socket_send(sock, 
                                       ptr + total_sent, 
                                       length - total_sent);
        if (sent <= 0) return -1;
        total_sent += sent;
    }

    return total_sent;
}
```

---

### Challenge 2: Byte Order

**Problem:** x86 is little-endian, network is big-endian

**Example:**

```
Value: 25 (0x00000019)
Little-endian (x86):    [19 00 00 00]
Big-endian (network):   [00 00 00 19]
```

**Solution:** Use `htonl()` / `ntohl()`

```c
// Sending
uint32_t network_length = htonl(msg->length);
send_exact(sock, &network_length, 4);

// Receiving
uint32_t network_length;
recv_exact(sock, &network_length, 4);
uint32_t host_length = ntohl(network_length);
```

---

### Challenge 3: Memory Management

**Problem:** Who owns the message data?

**Solution:** Clear ownership rules
- `_create()` allocates
- Caller must call `_free()`
- `_recv()` returns newly allocated message
- Safe to call `_free(NULL)`

---

## Testing Strategy

### Test 1: Basic Framing

```
Client sends: "Hello from framed client!"
Server receives: Exact 25 bytes
Server echoes back
Client receives: Exact same message
```

**Pass Criteria:** Byte-for-byte match

---

### Test 2: Multiple Messages

```
Client sends 3 messages sequentially:
  "First"
  "Second"
  "Third"
Server receives 3 separate, complete messages
```

**Pass Criteria:** No mixing between messages

---

### Test 3: Large Message

```
Client sends 1MB message
Server receives complete 1MB
```

**Pass Criteria:** All data intact, no corruption

---

### Test 4: Edge Cases

#### Zero-length message

```c
p2p_message_create("");  // Should return NULL
```

#### Message too large

```c
p2p_message_create_binary(huge_data, 2GB);  // Should return NULL
```

#### Connection close during receive
- Client sends header, closes connection before data
- Server should handle gracefully (return NULL)

---

## Success Criteria

### Functional Requirements
- [x] Messages sent/received without corruption
- [x] Handles partial I/O transparently
- [x] Works with binary data
- [x] Correct byte order conversion

### Performance Requirements
- [x] Small message (25 bytes): < 1ms latency
- [x] Large message (1MB): < 50ms latency
- [x] Overhead: 4 bytes per message (minimal)

### Quality Requirements
- [x] No compiler warnings
- [x] No memory leaks
- [x] Error cases handled gracefully

---

## Deliverables

### Code
- [x] `include/p2pnet/message.h`
- [x] `src/protocol/message.c`
- [x] `examples/04_framed_server.c`
- [x] `examples/05_framed_client.c`

### Documentation
- [x] API.md (Message API section)
- [x] MILESTONE_1.2_COMPLETE.md
- [x] Updated examples/README.md

### Tests
- [x] Basic echo test
- [x] Multiple messages test
- [x] Large message test
- [x] Edge case tests

---

## Dependencies

### External
- Milestone 1.1 (Socket API) ✅

### Internal
- `htonl()` / `ntohl()` from `<winsock2.h>` (Windows) or `<arpa/inet.h>` (Unix)

---

## Risks and Mitigations

| Risk | Impact | Probability | Mitigation |
|------|--------|-------------|------------|
| Byte order bugs | High | Medium | ✅ Unit tests with known values |
| Memory leaks | High | Medium | ✅ Valgrind-style testing |
| Partial I/O not handled | Critical | High | ✅ send_exact/recv_exact helpers |
| Large message DoS | High | Low | ✅ MAX_MESSAGE_SIZE protection |

---

## Timeline

**Total estimated:** 1 week

### Day 1-2: Design and Prototyping
- Design wire format
- Write helper functions
- Test partial I/O handling

### Day 3-4: Implementation
- Implement message API
- Handle all error cases
- Memory management

### Day 5: Testing
- Write test programs
- Test edge cases
- Fix bugs

### Day 6-7: Documentation
- Write API docs
- Completion report
- Update guides

**Actual:** Completed in 2 days

---

## Follow-up (Milestone 1.3)

**Next:** Event loop for handling multiple clients simultaneously

**Why:** Currently server handles one client at a time (sequential). I need async I/O for real P2P.

**Approach:** WSAPoll on Windows, epoll on Linux

---

## Notes

### Design Decisions

**Why 4-byte length header?**
- 2 bytes = max 64KB (too small for some use cases)
- 4 bytes = max 4GB (practical limit)
- 8 bytes = overkill for most applications

**Why 1MB max message size?**
- Protects against DoS attacks
- Large enough for most P2P data
- Can be increased if needed

**Why big-endian?**
- Network byte order standard
- Cross-platform compatibility
- `htonl()`/`ntohl()` are standard functions

---

### Lessons Learned

1. **Partial I/O is real:** Always loop until complete
2. **Byte order matters:** Never assume endianness
3. **Size limits are security:** Always validate message size
4. **Clear ownership:** Explicit free() prevents memory leaks

---

**Author:** Chris-sle 
**Created:** 15. desember 2025  
**Completed:** 19. desember 2025  
**Status:** Completed ✅
