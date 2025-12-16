# Examples - Test Programmer

Disse programmene brukes for å teste funksjonaliteten til biblioteket.

## Milestone 1.1: Grunnleggende Socket API

### Bygg eksemplene:
```bash
make examples
```

### Test 1: Basic Server + Client

**Terminal 1 (Server):**
```bash
./build/01_basic_server.exe
```

**Terminal 2 (Client):**
```bash
./build/02_basic_client.exe
```

**Forventet output:**

*Server:*
```
=== P2P Basic Server Example ===
[OK] Winsock initialized
[OK] Socket created
[OK] Socket bound to 127.0.0.1:8080
[OK] Listening for connections...
Waiting for client (test with: nc 127.0.0.1 8080)
Press Ctrl+C to stop
[OK] Client connected!
Received 18 bytes: "Hello from client!"
Sent 20 bytes back
[OK] Server shutdown complete
```

*Client:*
```
=== P2P Basic Client Example ===
[OK] Initialized
[OK] Socket created
Connecting to 127.0.0.1:8080...
[OK] Connected to server!
Sending: "Hello from client!"
[OK] Sent 18 bytes
Waiting for response...
[OK] Received: "Hello from server!"
[OK] Client shutdown complete
```

### Test 2: Med netcat

**Terminal 1:**
```bash
./build/01_basic_server.exe
```

**Terminal 2:**
```bash
echo "Test message" | nc 127.0.0.1 8080
```

---

## Feilsøking

### "Address already in use"
```bash
netstat -ano | findstr :8080
taskkill /PID <PID> /F
```

### Server henger
- Husk at `accept()` er blokkerende
- Trykk Ctrl+C for å avbryte

### Client kan ikke koble til
- Sjekk at serveren kjører først
- Sjekk firewall-innstillinger (første gang kan Windows spørre om tillatelse)