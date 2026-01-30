# Testing Guide

**P2PNet Testing Infrastructure**

---

## Overview

P2PNet uses **minunit** - a minimal unit testing framework for C. All tests are located in `c-lib/tests/` directory.

---

## Running Tests

### Run All Tests

```bash
mingw32-make run-tests
```

**Output:**
```
============================================
Running All Tests...
============================================

--- Socket Tests (Milestone 1.1) ---
Tests run: 3
Tests passed: 3
✅ ALL TESTS PASSED

--- Message Tests (Milestone 1.2) ---
Tests run: 4
Tests passed: 4
✅ ALL TESTS PASSED

--- Event Loop Tests (Milestone 1.3) ---
Tests run: 8
Tests passed: 4
✅ ALL TESTS PASSED

--- Crypto Tests (Milestone 2.1) ---
Tests run: 4
Tests passed: 4
✅ ALL TESTS PASSED

============================================
All test suites completed!
============================================
```

---

### Run Individual Test Suite

```bash
# Socket tests
build\test_socket.exe

# Message tests
build\test_message.exe

# Event Loop tests
build\test_event_loop.c

# Crypto tests
build\test_crypto.exe
```

---

### Build Tests Without Running

```bash
mingw32-make tests
```

---

## Test Coverage

### Milestone 1.1: Socket API

**File:** `tests/test_socket.c`

| Test | Description | Status |
|------|-------------|--------|
| `test_socket_create` | Create TCP socket | ✅ |
| `test_socket_bind` | Bind to random port | ✅ |
| `test_socket_listen` | Listen for connections | ✅ |

**Coverage:** 3/3 tests passing

---

### Milestone 1.2: Message Framing

**File:** `tests/test_message.c`

| Test | Description | Status |
|------|-------------|--------|
| `test_message_create` | Create message from string | ✅ |
| `test_message_create_empty_returns_null` | Empty string returns NULL | ✅ |
| `test_message_create_single_char` | Single character message | ✅ |
| `test_message_create_large` | Large message (1KB) | ✅ |

**Coverage:** 4/4 tests passing

---

### Milestone 1.3: Event Loop

**File:** `tests/test_event_loop.c`

| Test | Description | Status |
|------|-------------|--------|
| `test_event_loop_create` | Create and free event loop | ✅ |
| `test_event_loop_add_socket` | Add socket to loop | ✅ |
| `test_event_loop_add_multiple_sockets` | Add multiple sockets | ✅ |
| `test_event_loop_remove_socket` | Remove socket from loop | ✅ |
| `test_event_loop_remove_nonexistent` | Try to remove non-existent socket | ✅ |
| `test_event_loop_socket_count` | Verify socket count accuracy | ✅ |
| `test_event_loop_null_safety` | NULL pointer safety | ✅ |
| `test_event_loop_add_duplicate` | Prevent duplicate socket addition | ✅ |

**Coverage:** 8/8 tests passing

---

### Milestone 2.1: Keypair & Identity

**File:** `tests/test_crypto.c`

| Test | Description | Status |
|------|-------------|--------|
| `test_keypair_generate` | Generate Ed25519 keypair | ✅ |
| `test_keypair_save_load` | Save and load from file | ✅ |
| `test_keypair_fingerprint` | Fingerprint format (43 chars) | ✅ |
| `test_keypair_uniqueness` | Two keypairs are different | ✅ |

**Coverage:** 4/4 tests passing

---

### Milestone 2.2: Handshake Protocol

**File:** `tests/test_handshake.c`

| Test | Description | Status |
|------|-------------|--------|
| `test_handshake_setup` | Generate test keypairs | ✅ |
| `test_handshake_basic` | Basic client-server handshake | ✅ |
| `test_handshake_expected_peer_match` | Handshake with correct expected peer | ✅ |
| `test_handshake_expected_peer_mismatch` | Reject wrong peer identity | ✅ |
| `test_handshake_session_key` | Verify session key derivation | ✅ |
| `test_session_fingerprint` | Get peer fingerprint from session | ✅ |
| `test_handshake_cleanup` | Cleanup test resources | ✅ |

**Coverage:** 7/7 tests passing

**Test scenarios:**
- ✅ Mutual authentication (both sides verify identity)
- ✅ Expected peer verification (accept specific peer)
- ✅ Reject wrong peer (security test)
- ✅ Session key derivation (both sides derive same key)
- ✅ Peer fingerprint display
- ✅ Graceful handling of failed handshakes

**Notes:**
- Tests use multi-threading (server runs in separate thread)
- Port 9999 used for test communication
- Server gracefully handles client disconnect during handshake

---

## Writing New Tests

### Test Structure

```c
#include "minunit.h"
#include <p2pnet/p2pnet.h>

// Define a test
MU_TEST(test_name) {
    // Setup
    p2p_socket_t* sock = p2p_socket_create(P2P_TCP);
    
    // Assert
    mu_check(sock != NULL);
    
    // Cleanup
    p2p_socket_close(sock);
    
    // IMPORTANT: Return NULL on success
    return NULL;
}

// Group tests into suite
MU_TEST_SUITE(suite_name) {
    MU_RUN_TEST(test_name);
    // Add more tests...
    return NULL;
}

// Main function
int main() {
    printf("Running My Tests\n");
    
    MU_RUN_SUITE(suite_name);
    MU_REPORT();
    
    return MU_EXIT_CODE;
}
```

---

### minunit Assertions

```c
// Basic assertion (custom message)
mu_assert(condition, "Error message");

// Check assertion (auto-generated message)
mu_check(condition);

// Example usage
mu_check(ptr != NULL);
mu_check(result == 0);
mu_check(length == 43);
mu_assert(x > 0, "x must be positive");
```

---

### Example: Testing Message API

```c
#include "minunit.h"
#include <p2pnet/p2pnet.h>
#include <string.h>

MU_TEST(test_message_create) {
    // Create message
    const char* text = "Hello";
    p2p_message_t* msg = p2p_message_create(text);
    
    // Verify
    mu_check(msg != NULL);
    mu_check(msg->length == 5);
    mu_check(strcmp(msg->data, text) == 0);
    
    // Cleanup
    p2p_message_free(msg);
    
    return NULL;  // Success
}

MU_TEST_SUITE(message_suite) {
    MU_RUN_TEST(test_message_create);
    return NULL;
}

int main() {
    MU_RUN_SUITE(message_suite);
    MU_REPORT();
    return MU_EXIT_CODE;
}
```

---

## Adding Tests to Build System

### 1. Create test file

Create `tests/test_myfeature.c`

### 2. Update Makefile

Add to `TEST_BINS`:
```makefile
TEST_BINS = $(BUILD_DIR)/test_socket.exe \
            $(BUILD_DIR)/test_message.exe \
            $(BUILD_DIR)/test_crypto.exe \
            $(BUILD_DIR)/test_myfeature.exe
```

Add build rule:
```makefile
$(BUILD_DIR)/test_myfeature.exe: $(TEST_DIR)/test_myfeature.c $(LIBRARY)
	@echo [TEST] Building $@
	$(CC) $(CFLAGS) $< -L$(BUILD_DIR) -lp2pnet $(LDFLAGS) -o $@
```

Add to `run-tests` target:
```makefile
.PHONY: run-tests
run-tests: tests
	@echo --- My Feature Tests ---
	@build\test_myfeature.exe
	@echo.
```

### 3. Build and run

```bash
mingw32-make run-tests
```

---

## Debugging Failed Tests

### Test Output

When a test fails, minunit shows:
```
FAILED: sock != NULL

❌ TEST SUITE FAILED: sock != NULL
```

### Debug Steps

1. **Identify failed assertion:**
   ```
   FAILED: sock != NULL
   ```
   → Socket creation failed

2. **Add debug output:**
   ```c
   MU_TEST(test_socket_create) {
       p2p_socket_t* sock = p2p_socket_create(P2P_TCP);
       
       if (!sock) {
           printf("[DEBUG] Socket creation failed\n");
           printf("[DEBUG] WSA Error: %d\n", WSAGetLastError());
       }
       
       mu_check(sock != NULL);
       
       p2p_socket_close(sock);
       return NULL;
   }
   ```

3. **Check preconditions:**
   - Is Winsock initialized? (`p2p_init()`)
   - Are dependencies met?
   - Is test environment set up correctly?

4. **Run test in isolation:**
   ```bash
   build\test_socket.exe
   ```

---

## Best Practices

### DO ✅

- Write tests for all new features
- Test edge cases (empty, NULL, large values)
- Keep tests simple and focused
- One assertion per test (when possible)
- Clean up resources (free memory, close sockets)
- Return `NULL` on success

### DON'T ❌

- Don't test implementation details
- Don't write flaky tests (depend on timing, randomness)
- Don't skip cleanup (causes memory leaks)
- Don't forget `return NULL;` (causes warnings)

---

## Test Naming Conventions

```
test_<component>_<action>_<expectation>

Examples:
- test_socket_create
- test_message_create_empty_returns_null
- test_keypair_save_load
- test_keypair_fingerprint_length
```

---

## Continuous Integration (Future)

**When I add CI:**

```yaml
# .github/workflows/tests.yml
name: Tests

on: [push, pull_request]

jobs:
  test:
    runs-on: windows-latest
    steps:
      - uses: actions/checkout@v2
      - name: Setup MSYS2
        uses: msys2/setup-msys2@v2
      - name: Install dependencies
        run: pacman -S --noconfirm mingw-w64-x86_64-libsodium
      - name: Build tests
        run: mingw32-make tests
      - name: Run tests
        run: mingw32-make run-tests
```

---

## Test Coverage Goals

| Milestone | Target Coverage | Current |
|-----------|----------------|---------|
| 1.1 (Socket) | 80% | **100%** ✅ |
| 1.2 (Message) | 80% | **100%** ✅ |
| 1.3 (Event Loop) | 70% | **100%** ✅ |
| 2.1 (Crypto) | 90% | **100%** ✅ |
| 2.2 (Handshake) | 90% | **100%** ✅ |
| 2.3 (Encryption) | 90% | **N/A** |

---

## Performance Testing (Future)

**Planned benchmarks:**

```c
// Benchmark keypair generation
void benchmark_keypair_generation() {
    int iterations = 1000;
    clock_t start = clock();
    
    for (int i = 0; i < iterations; i++) {
        p2p_keypair_t* kp = p2p_keypair_generate();
        p2p_keypair_free(kp);
    }
    
    clock_t end = clock();
    double time_ms = (double)(end - start) / CLOCKS_PER_SEC * 1000;
    
    printf("Keypair generation: %.2f ms avg\n", time_ms / iterations);
}
```

---

