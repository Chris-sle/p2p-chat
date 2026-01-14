# Milestone 1.1: Socket Abstraksjon - PLAN

**Status:** ✅ Komplett  
**Estimert tid:** 1 uke  
**Faktisk tid:** 3 dager (15-17. desember 2025)

---

## Mål

Lage en cross-platform socket API som wrapper Winsock (Windows) og BSD sockets (Unix/Linux). Dette blir fundamentet for hele nettverksbiblioteket.

---

## Bakgrunn

### Problemet

Windows og Unix/Linux har ulike socket APIs:

| Aspekt | Windows (Winsock) | Unix/Linux (BSD Sockets) |
|--------|-------------------|--------------------------|
| Header | `<winsock2.h>` | `<sys/socket.h>` |
| Initialisering | `WSAStartup()` required | Ingen |
| Socket type | `SOCKET` | `int` |
| Invalid value | `INVALID_SOCKET` | `-1` |
| Error handling | `WSAGetLastError()` | `errno` |
| Cleanup | `closesocket()` | `close()` |
| Library link | `-lws2_32` | (ingen) |

**trenger:** En unified API som skjuler disse forskjellene.

---

## Scope

### I Scope (Milestone 1.1)

✅ TCP sockets only  
✅ Blocking I/O only  
✅ Windows (Winsock) implementasjon  
✅ Server operations (bind, listen, accept)  
✅ Client operations (connect)  
✅ Data transfer (send, recv)  
✅ Error handling  
✅ Basic test programs  

### Out of Scope (Future milestones)

❌ UDP sockets (later)  
❌ Non-blocking I/O (Milestone 1.3)  
❌ Event loop (Milestone 1.3)  
❌ Message framing (Milestone 1.2)  
❌ Linux/Unix implementation (future)  
❌ IPv6 support (future)  

---

## API Design

### Core Functions

```c
// Initialization
int p2p_init(void);
void p2p_cleanup(void);

// Socket lifecycle
p2p_socket_t* p2p_socket_create(int type);
void p2p_socket_close(p2p_socket_t* sock);

// Server side
int p2p_socket_bind(p2p_socket_t* sock, const char* ip, uint16_t port);
int p2p_socket_listen(p2p_socket_t* sock, int backlog);
p2p_socket_t* p2p_socket_accept(p2p_socket_t* sock);

// Client side
int p2p_socket_connect(p2p_socket_t* sock, const char* ip, uint16_t port);

// Data transfer
ssize_t p2p_socket_send(p2p_socket_t* sock, const void* data, size_t len);
ssize_t p2p_socket_recv(p2p_socket_t* sock, void* buffer, size_t len);

// Error handling
const char* p2p_get_error(void);
```

### Design Principles

1. **Opaque pointers:** `p2p_socket_t` er ikke synlig for brukeren
2. **Consistent return values:** 
   - Functions return `0` for success, `-1` for error
   - Pointers return `NULL` for error
3. **Explicit initialization:** `p2p_init()` må kalles først
4. **Clear ownership:** Caller må kalle `p2p_socket_close()`

---

## Implementation Plan

### Phase 1: Headers
- [x] `include/p2pnet/socket.h` - Public API
- [x] `include/p2pnet/p2pnet.h` - Main header

### Phase 2: Windows Implementation
- [x] `src/platform/socket_win.c` - Winsock wrapper
- [x] Internal struct definition
- [x] WSAStartup/WSACleanup handling
- [x] Error message formatting

### Phase 3: Build System
- [x] Makefile for Windows/MinGW
- [x] VSCode configuration
- [x] Library compilation (libp2pnet.a)

### Phase 4: Testing
- [x] `examples/01_basic_server.c` - Simple echo server
- [x] `examples/02_basic_client.c` - Simple client
- [x] `examples/03_stress_test.c` - Sequential stress test

---

## Technical Challenges

### Challenge 1: ssize_t on Windows

**Problem:** Windows doesn't have `ssize_t` type natively

**Solution:** MinGW/MSYS2 ucrt64 provides it in `<corecrt.h>`. No custom typedef needed.

---

### Challenge 2: Socket Type

**Problem:** Windows uses `SOCKET` (unsigned), Unix uses `int`

**Solution:** Hide in opaque struct:

```c
struct p2p_socket {
    SOCKET handle;  // Windows
    int type;
};
```

---

### Challenge 3: Error Messages

**Problem:** `WSAGetLastError()` returns code, not message

**Solution:** Use `snprintf()` to format error codes:

```c
static char error_buffer[256];
snprintf(error_buffer, sizeof(error_buffer),
         "bind() failed with error: %d", WSAGetLastError());
```

---

## Testing Strategy

### Unit Tests (Not implemented - using examples instead)

I am using example programs as integration tests rather than unit tests for simplicity.

### Integration Tests

#### Test 1: Basic Server-Client

**Goal:** Verify basic connection and data transfer

**Steps:**
1. Start server on port 8080
2. Client connects
3. Client sends "Hello from client!"
4. Server receives and echoes back
5. Client receives echo
6. Both close gracefully

**Pass Criteria:** No errors, data matches

---

#### Test 2: Multiple Sequential Clients

**Goal:** Verify server can handle multiple clients

**Steps:**
1. Start server
2. Run client 3 times sequentially
3. Each client sends unique message

**Pass Criteria:** Server handles all 3, no memory leaks

---

#### Test 3: Stress Test

**Goal:** Test performance and stability

**Steps:**
1. Start server
2. Client makes 10 connections sequentially
3. Each sends message and waits for echo

**Pass Criteria:** 10/10 successful, no crashes

---

#### Test 4: Error Handling

**Goal:** Verify error cases

**Test Cases:**
- Port already in use (bind fails)
- Connect without server (connection refused)
- Server not initialized (WSAStartup not called)

**Pass Criteria:** All errors caught and reported properly

---

## Success Criteria

### Functional Requirements
- [x] Can create TCP socket
- [x] Server can bind to port
- [x] Server can accept connections
- [x] Client can connect to server
- [x] Data can be sent and received
- [x] Errors are reported correctly

### Non-Functional Requirements
- [x] No compiler warnings
- [x] No memory leaks (verified with 10 sequential connections)
- [x] No crashes under normal use
- [x] Code is readable and documented

### Performance Requirements
- [x] Connection setup < 100ms (avg ~52ms achieved)
- [x] Small message latency < 10ms (avg ~5ms achieved)
- [x] Can handle at least 10 sequential connections (tested)

---

## Deliverables

### Code
- [x] `include/p2pnet/socket.h`
- [x] `include/p2pnet/p2pnet.h`
- [x] `src/platform/socket_win.c`
- [x] `examples/01_basic_server.c`
- [x] `examples/02_basic_client.c`
- [x] `examples/03_stress_test.c`

### Build System
- [x] Makefile
- [x] VSCode configuration files

### Documentation
- [x] API.md (Socket API section)
- [x] WINDOWS_SETUP.md
- [x] MILESTONE_1.1_COMPLETE.md
- [x] examples/README.md

---

## Dependencies

### External
- MinGW-w64 GCC (version 14.2.0 used)
- Windows SDK (Winsock2)

### Internal
- None (this is the foundation)

---

## Risks and Mitigations

| Risk | Impact | Probability | Mitigation |
|------|--------|-------------|------------|
| Winsock version incompatibility | High | Low | Use MAKEWORD(2,2) - widely supported |
| Partial send/recv | Medium | High | ✅ Accepted for Milestone 1.1, handled in 1.2 |
| Memory leaks | High | Medium | ✅ Tested with multiple connections |
| Port conflicts in testing | Low | Medium | ✅ Error handling + documentation |

---

## Timeline

**Total estimated:** 1 week

### Day 1-2: API Design and Headers
- Define API
- Write headers with documentation
- Review design

### Day 3-4: Implementation
- Implement socket_win.c
- Handle all error cases
- Test basic functionality

### Day 5: Testing
- Write test programs
- Run all test scenarios
- Fix bugs

### Day 6-7: Documentation and Polish
- Write API documentation
- Setup guide
- Final testing

**Actual:** Completed in 1 days

---

## Follow-up (Milestone 1.2)

**Next:** Message framing to handle TCP stream problem

**Why:** TCP doesn't guarantee message boundaries. I need a protocol to separate messages.

**Approach:** Length-prefix framing (4-byte header)

---

## Notes

### Lessons Learned

1. MinGW ucrt64 already has `ssize_t` - no custom typedef needed
2. Partial I/O is common - needs handling (deferred to 1.2)
3. Clear error messages are critical for debugging
4. ASCII output is more portable than Unicode for console apps

### Future Improvements

- Add timeout support for recv/accept
- Implement SO_REUSEADDR option
- Add get_peer_address() function
- Linux/macOS support (use epoll/kqueue instead of WSAPoll)

---

**Author:** Chris-sle  
**Created:** 15. desember 2025  
**Status:** Completed ✅
