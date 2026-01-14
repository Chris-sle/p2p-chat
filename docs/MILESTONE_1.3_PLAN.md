# Milestone 1.3: Event Loop (Async I/O) - PLAN

**Status:** 🔄 Planlagt  
**Estimated Time:** 2-3 uker  
**Start Date:** TBD

---

## Mål

Implementere en event loop som lar serveren håndtere mange klienter samtidig uten blocking eller threading. Dette gjør biblioteket production-ready for real-time P2P chat.

---

## Bakgrunn

### Problemet: Blocking I/O

**Current implementation (Milestone 1.1-1.2):**

```c
while (1) {
    p2p_socket_t* client = p2p_socket_accept(server);  // BLOCKS here

    p2p_message_t* msg = p2p_message_recv(client);     // BLOCKS here

    p2p_message_send(client, reply);
    p2p_socket_close(client);
}
```

**Problem:**
- Server can only handle ONE client at a time
- While receiving from client A, client B cannot connect
- Real P2P needs many simultaneous connections

---

### The Solution: Event Loop

**Event-driven architecture:**

```c
// Pseudo-code
event_loop = create_event_loop();
event_loop_add_socket(event_loop, server, on_new_client);
event_loop_add_socket(event_loop, client1, on_client_data);
event_loop_add_socket(event_loop, client2, on_client_data);

event_loop_run(event_loop);  // BLOCKS here, but handles ALL sockets

// When data arrives on ANY socket:
void on_client_data(p2p_socket_t* sock) {
    // Called automatically by event loop
    p2p_message_t* msg = p2p_message_recv(sock);
    // Process message...
}
```

**Benefits:**
- ✅ Handle many clients simultaneously
- ✅ No threading complexity
- ✅ Scalable (100+ connections possible)
- ✅ Efficient (CPU only active when data arrives)

---

## Platform-Specific Approaches

### Windows: WSAPoll()

```c
WSAPOLLFD fds[MAX_SOCKETS];
fds[0].fd = server_socket;
fds[0].events = POLLIN;

int result = WSAPoll(fds, num_fds, timeout);

if (fds[0].revents & POLLIN) {
    // Server socket has incoming connection
}
```

**Pros:**
- Standard API (similar to Unix `poll()`)
- Available on Windows Vista+

**Cons:**
- Known bugs in older Windows versions
- Less efficient than IOCP (but simpler)

---

### Linux: epoll (Future work)

```c
int epoll_fd = epoll_create1(0);

struct epoll_event ev;
ev.events = EPOLLIN;
ev.data.fd = socket_fd;

epoll_ctl(epoll_fd, EPOLL_CTL_ADD, socket_fd, &ev);
epoll_wait(epoll_fd, events, MAX_EVENTS, timeout);
```

**Pros:**
- Very efficient (O(1) performance)
- Edge-triggered support

**Cons:**
- Linux-specific

---

### macOS: kqueue (Future work)

```c
int kq = kqueue();

struct kevent ev;
EV_SET(&ev, socket_fd, EVFILT_READ, EV_ADD, 0, 0, NULL);

kevent(kq, &ev, 1, NULL, 0, NULL);
kevent(kq, NULL, 0, events, MAX_EVENTS, &timeout);
```

---

## Scope

### In Scope (Milestone 1.3)

✅ Event loop abstraction  
✅ Windows implementation (WSAPoll)  
✅ Non-blocking sockets  
✅ Callback-based API  
✅ Multiple simultaneous clients  
✅ Server-side focus  
✅ Graceful shutdown  
✅ Test program (async server)  

### Out of Scope

❌ Client-side event loop (not needed yet)  
❌ Linux/macOS implementation (future)  
❌ Threading (not needed with event loop)  
❌ Connection pooling (future)  
❌ Timeout management (basic only)  
❌ IOCP (Windows, more complex alternative)  

---

## API Design

### Event Loop Structure

```c
typedef struct p2p_event_loop p2p_event_loop_t;

// Callback types
typedef void (*p2p_read_callback)(p2p_socket_t* sock, void* user_data);
typedef void (*p2p_write_callback)(p2p_socket_t* sock, void* user_data);
typedef void (*p2p_error_callback)(p2p_socket_t* sock, int error, void* user_data);
```

### Core Functions

```c
// Lifecycle
p2p_event_loop_t* p2p_event_loop_create(void);
void p2p_event_loop_free(p2p_event_loop_t* loop);

// Socket management
int p2p_event_loop_add_socket(p2p_event_loop_t* loop,
                              p2p_socket_t* sock,
                              p2p_read_callback on_read,
                              void* user_data);

int p2p_event_loop_remove_socket(p2p_event_loop_t* loop,
                                  p2p_socket_t* sock);

// Execution
void p2p_event_loop_run(p2p_event_loop_t* loop);
void p2p_event_loop_stop(p2p_event_loop_t* loop);

// Utilities
int p2p_event_loop_socket_count(p2p_event_loop_t* loop);
```

### Design Principles

1. **Callback-based:** User registers callbacks for events
2. **User data:** Each socket can have associated context
3. **Single-threaded:** All callbacks run in main thread
4. **Clear lifecycle:** Create → Add sockets → Run → Stop → Free

---

## Implementation Plan

### Phase 1: Non-Blocking Sockets
- [ ] Add `p2p_socket_set_nonblocking()` function
- [ ] Update `socket_win.c` to support non-blocking mode
- [ ] Test non-blocking recv/send (WSAEWOULDBLOCK handling)

### Phase 2: Event Loop Core
- [ ] `src/core/event_loop.c` - Platform-agnostic interface
- [ ] `src/platform/event_loop_win.c` - WSAPoll implementation
- [ ] Socket registration system (dynamic array or hashtable)

### Phase 3: Callback System
- [ ] Callback management (register/unregister)
- [ ] User data association
- [ ] Error callback handling

### Phase 4: Integration
- [ ] Integrate with message API
- [ ] Handle partial messages (buffer management)
- [ ] Graceful shutdown (close all sockets)

### Phase 5: Testing
- [ ] `examples/06_async_server.c` - Multi-client server
- [ ] `examples/07_async_stress_test.c` - 100 concurrent clients
- [ ] Test edge cases (disconnect, slow clients, etc.)

---

## Technical Challenges

### Challenge 1: Non-Blocking Recv

**Problem:** `recv()` returns WSAEWOULDBLOCK when no data available

**Solution:** Only call `recv()` when WSAPoll signals POLLIN

```c
void on_socket_readable(p2p_socket_t* sock) {
    // WSAPoll guarantees data is available
    p2p_message_t* msg = p2p_message_recv(sock);

    if (!msg) {
        // Connection closed or error
        handle_disconnect(sock);
    }
}
```

---

### Challenge 2: Partial Messages

**Problem:** With non-blocking I/O, we might receive incomplete message

**Scenario:**

```
Message: [4 bytes length] [1000 bytes data]

recv() call 1: Returns 100 bytes (header + partial data)
recv() call 2: Returns 900 bytes (rest of data)
```

**Solution:** Buffer management

```c
typedef struct {
    uint8_t* buffer;       // Partial message buffer
    size_t buffer_used;    // Bytes in buffer
    uint32_t expected_len; // Message length (from header)
} message_buffer_t;

// Associate one buffer per socket
```

---

### Challenge 3: Dynamic Socket List

**Problem:** Number of clients is unknown at compile time

**Solution:** Dynamic array with reallocation

```c
typedef struct {
    WSAPOLLFD* poll_fds;  // Array of pollfd structs
    int num_fds;          // Current count
    int capacity;         // Allocated capacity
} event_loop_t;

// When adding socket:
if (loop->num_fds == loop->capacity) {
    loop->capacity *= 2;
    loop->poll_fds = realloc(loop->poll_fds, 
                             loop->capacity * sizeof(WSAPOLLFD));
}
```

---

### Challenge 4: Server Socket vs Client Sockets

**Different behavior:**

| Socket Type | Event | Action |
|-------------|-------|--------|
| Server | POLLIN | Call `accept()` → New client socket |
| Client | POLLIN | Call `recv()` → Read data |
| Client | POLLOUT | Socket ready for `send()` |
| Any | POLLERR/POLLHUP | Connection error/close |

**Solution:** Flag or callback distinction

```c
typedef enum {
    SOCKET_TYPE_SERVER,
    SOCKET_TYPE_CLIENT
} socket_type_t;

typedef struct {
    p2p_socket_t* sock;
    socket_type_t type;
    p2p_read_callback on_read;
    void* user_data;
} registered_socket_t;
```

---

## Example Usage

### Async Echo Server

```c
#include <p2pnet/p2pnet.h>
#include <p2pnet/event_loop.h>

void on_new_client(p2p_socket_t* server_sock, void* user_data) {
    p2p_event_loop_t* loop = (p2p_event_loop_t*)user_data;

    p2p_socket_t* client = p2p_socket_accept(server_sock);
    if (client) {
        printf("New client connected
");
        p2p_event_loop_add_socket(loop, client, on_client_data, NULL);
    }
}

void on_client_data(p2p_socket_t* client, void* user_data) {
    p2p_message_t* msg = p2p_message_recv(client);

    if (msg) {
        printf("Received: %.*s
", msg->length, msg->data);

        // Echo back
        p2p_message_send(client, msg);
        p2p_message_free(msg);
    } else {
        // Connection closed
        printf("Client disconnected
");
        p2p_socket_close(client);
    }
}

int main() {
    p2p_init();

    p2p_socket_t* server = p2p_socket_create(P2P_TCP);
    p2p_socket_bind(server, "0.0.0.0", 8080);
    p2p_socket_listen(server, 5);

    p2p_event_loop_t* loop = p2p_event_loop_create();
    p2p_event_loop_add_socket(loop, server, on_new_client, loop);

    printf("Server running (async)...
");
    p2p_event_loop_run(loop);  // Handles all clients

    p2p_event_loop_free(loop);
    p2p_socket_close(server);
    p2p_cleanup();

    return 0;
}
```

---

## Testing Strategy

### Test 1: Single Client (Sanity Check)

```
Start async server
Connect one client
Send message
Receive echo
```

**Pass Criteria:** Works like blocking version

---

### Test 2: Multiple Sequential Clients

```
Start async server
Connect client 1, send, disconnect
Connect client 2, send, disconnect
Connect client 3, send, disconnect
```

**Pass Criteria:** All 3 clients handled correctly

---

### Test 3: Concurrent Clients

```
Start async server
Connect 10 clients simultaneously (don't disconnect)
Each client sends message
All clients should receive echo
```

**Pass Criteria:** All 10 clients get echo, no blocking

---

### Test 4: Stress Test

```
Start async server
Connect 100 clients
Each sends 10 messages
Measure latency and throughput
```

**Pass Criteria:**
- No crashes
- All messages delivered
- Latency < 100ms per message

---

### Test 5: Slow Client

```
Connect client that receives slowly (sleep between recv calls)
Connect second client that sends normally
```

**Pass Criteria:** Second client not blocked by slow client

---

### Test 6: Client Disconnect

```
Connect client mid-message (send header, disconnect)
Server should handle gracefully
```

**Pass Criteria:** No crash, other clients unaffected

---

## Success Criteria

### Functional Requirements
- [ ] Can handle 10+ concurrent clients
- [ ] No blocking between clients
- [ ] Graceful handling of disconnects
- [ ] Callbacks invoked correctly

### Performance Requirements
- [ ] Handles 100 concurrent connections
- [ ] Latency < 100ms per message (with 100 clients)
- [ ] CPU usage reasonable (< 10% idle)

### Quality Requirements
- [ ] No memory leaks
- [ ] No crashes under stress
- [ ] Clean shutdown

---

## Deliverables

### Code
- [ ] `include/p2pnet/event_loop.h`
- [ ] `src/core/event_loop.c`
- [ ] `src/platform/event_loop_win.c`
- [ ] `examples/06_async_server.c`
- [ ] `examples/07_async_stress_test.c`

### Documentation
- [ ] API.md (Event Loop section)
- [ ] MILESTONE_1.3_COMPLETE.md
- [ ] Updated examples/README.md

---

## Dependencies

### External
- Milestone 1.1 (Socket API) ✅
- Milestone 1.2 (Message Framing) ✅
- WSAPoll (Windows Vista+)

### Internal
- Non-blocking socket support
- Dynamic data structures (realloc)

---

## Risks and Mitigations

| Risk | Impact | Probability | Mitigation |
|------|--------|-------------|------------|
| WSAPoll bugs (old Windows) | High | Low | Require Windows Vista+ |
| Partial message handling | Critical | Medium | Implement buffering system |
| Memory leaks (many clients) | High | Medium | Careful testing, valgrind-style |
| Callback complexity | Medium | High | Clear documentation + examples |
| Socket limit (Windows) | Medium | Low | Document limits (default: 64) |

---

## Timeline

**Total estimated:** 2-3 weeks

### Week 1: Core Implementation
- Non-blocking socket support
- Event loop structure
- WSAPoll integration
- Basic callback system

### Week 2: Integration and Testing
- Message buffering
- Disconnect handling
- Test programs
- Basic stress testing

### Week 3: Polish and Documentation
- Edge case fixes
- Performance tuning
- Documentation
- Final testing

---

## Follow-up (Fase 2)

**Next:** Security & Cryptography

**Goals:**
- Keypair generation (Ed25519)
- Authenticated handshake
- Encrypted messaging (ChaCha20-Poly1305)
- NAT traversal (STUN/TURN)

---

## Notes

### Design Decisions

**Why WSAPoll over IOCP?**
- Simpler API
- Cross-platform (similar to Unix poll)
- Good enough performance for P2P chat
- IOCP is more complex (callback-heavy)

**Why single-threaded?**
- Simpler (no locks needed)
- Good performance for I/O-bound tasks
- Easier to debug
- Can add threading later if needed

**Why callbacks over coroutines/async-await?**
- C doesn't have native async/await
- Callbacks are traditional C approach
- Clear control flow

---

### Alternatives Considered

**Threading (one thread per client):**
- ❌ Doesn't scale (100 threads = high overhead)
- ❌ Complex synchronization
- ❌ More bugs (race conditions)

**IOCP (Windows I/O Completion Ports):**
- ❌ More complex API
- ❌ Harder to port to Linux
- ✅ Better performance (but overkill for P2P chat)

**libuv (cross-platform event loop library):**
- ❌ External dependency
- ❌ Overhead for learning new API
- ✅ Production-ready, well-tested
- **Decision:** Maybe later, for now keep it simple

---

**Author:** Chris-sle 
**Created:** 15. desember 2025  
**Completed:** 13. januar 2026 
**Status:** Completed ✅
