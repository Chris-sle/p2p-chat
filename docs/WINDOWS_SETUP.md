# Windows Development Setup Guide

Dette dokumentet forklarer hvordan du setter opp utviklingsmiljøet for P2P Chat på Windows 10 Pro.

## Forutsetninger

- Windows 10 Pro (64-bit)
- Minimum 4GB RAM
- 2GB ledig diskplass

---

## Steg 1: Installer MSYS2

MSYS2 gir deg et Unix-lignende miljø på Windows med pakkebehandler (pacman).

1. **Last ned MSYS2:**
   - Gå til [https://www.msys2.org/](https://www.msys2.org/)
   - Last ned installeren (msys2-x86_64-*.exe)

2. **Installer MSYS2:**
   - Kjør installeren
   - Installer i standard mappe: `C:\msys64`
   - La "Run MSYS2 now" være huket av

3. **Oppdater MSYS2:**
   - Kjør kommandoen:
   ```bash
   pacman -Syu
   ```
   - Terminalen vil lukke seg. Åpne "MSYS2 MINGW64" på nytt fra Start-menyen.
   - Kjør kommandoen igjen:
   ```bash
   pacman -Syu
   ```

---

## Steg 2: Installer Utviklerverktøy

I **MSYS2 MINGW64** terminalen:

```bash
# GCC Compiler (C/C++)
pacman -S mingw-w64-x86_64-gcc

# Make (build system)
pacman -S mingw-w64-x86_64-make
pacman -S make

# Git (versjonskontroll)
pacman -S git

# Libsodium (kryptering - brukes i Fase 2)
pacman -S mingw-w64-x86_64-libsodium

# Debugging verktøy
pacman -S mingw-w64-x86_64-gdb
```

**Verifiser installasjonen:**
```bash
gcc --version  # Skal vise: gcc.exe (Rev...) 13.x.x eller nyere
make --version # Skal vise: GNU Make 4.x
```

---

## Steg 3: Legg til MinGW i PATH (for VSCode)

For at VSCode skal finne GCC utenfor MSYS2-terminalen:

1. **Åpne System Environment Variables:**
   - Trykk `Win + R`
   - Skriv: `sysdm.cpl` og trykk Enter
   - Gå til "Advanced" → "Environment Variables"

2. **Rediger Path variabelen:**
   - Under "System variables", finn `Path`
   - Klikk "Edit" → "New"
   - Legg til: `C:\msys64\mingw64\bin`
   - Klikk OK på alle vinduer

3. **Test i Command Prompt (CMD):**
   ```bash
   gcc --version
   ```
   - Hvis dette fungerer, er PATH riktig satt opp.

---

## Steg 4: Installer Visual Studio Code

1. **Last ned VSCode:**
   - [https://code.visualstudio.com/](https://code.visualstudio.com/)

2. **Installer Extensions:**
   - Åpne VSCode
   - Trykk `Ctrl+Shift+X` (Extensions)
   - Installer disse:
     - **C/C++** (Microsoft)
     - **C/C++ Extension Pack** (Microsoft)
     - **Makefile Tools** (Microsoft)

---

## Steg 5: Klon/Opprett Prosjektet

### Hvis du starter fra scratch:

```bash
# I MSYS2 MINGW64 terminal
mkdir -p ~/projects
cd ~/projects
mkdir p2p-chat
cd p2p-chat

# Opprett mappestruktur
mkdir -p c-lib/{include/p2pnet,src/{platform,core,protocol,crypto,p2p},examples,build,lib}
mkdir -p .vscode docs scripts python-client

# Initialiser git
git init
```

### Hvis du har en eksisterende repo:

```bash
cd ~/projects
git clone <din-repo-url>
cd p2p-chat
```

---

## Steg 6: Test at Alt Fungerer

Lag en test-fil for å verifisere oppsettet:

**test.c:**
```c
#include <stdio.h>
#include <winsock2.h>

int main() {
    WSADATA wsa;

    printf("Testing Winsock initialization...\n");

    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        printf("Failed to initialize Winsock. Error: %d\n", WSAGetLastError());
        return 1;
    }

    printf("Winsock initialized successfully!\n");
    printf("Winsock version: %d.%d\n", LOBYTE(wsa.wVersion), HIBYTE(wsa.wVersion));

    WSACleanup();
    return 0;
}
```

**Kompiler og kjør:**
```bash
gcc test.c -o test.exe -lws2_32
./test.exe
```

**Forventet output:**
```
Testing Winsock initialization...
Winsock initialized successfully!
Winsock version: 2.2
```

---

## Steg 7: Åpne Prosjektet i VSCode

```bash
cd ~/projects/p2p-chat
code .
```

Første gang VSCode åpner prosjektet:
- Den vil spørre om du vil konfigurere IntelliSense → Velg "Yes"
- Velg "GCC" som compiler

---

## Vanlige Problemer

### Problem: "gcc: command not found"
**Løsning:** MinGW er ikke i PATH. Se Steg 3.

### Problem: "undefined reference to `WSAStartup`"
**Løsning:** Du mangler `-lws2_32` flagget ved kompilering.

### Problem: VSCode klager på #include <winsock2.h>
**Løsning:** IntelliSense er ikke konfigurert. Sjekk at `.vscode/c_cpp_properties.json` peker på riktig MinGW path.

### Problem: "Address already in use" ved kjøring av server
**Løsning:**
```bash
# Finn prosess som bruker porten
netstat -ano | findstr :8080

# Drep prosessen (erstatt PID)
taskkill /PID <PID> /F
```

---

## Tips for Utviklingen

### Bruk MSYS2-terminalen for utvikling
VSCode sin integrerte terminal (`Ctrl+ö`) kan settes til MSYS2:
1. `Ctrl+Shift+P` → "Terminal: Select Default Profile"
2. Velg "MSYS2 MINGW64"

### Testing med netcat
```bash
# Installer netcat i MSYS2
pacman -S netcat

# Test server
nc localhost 8080
```

### Debugging i VSCode
Trykk `F5` for å starte debugger (krever `.vscode/launch.json` - se neste seksjon).

---

## Neste Steg

✅ Miljøet er klart!  
→ Gå til **Milestone 1.1** og begynn å kode `socket.h` og `socket_win.c`
