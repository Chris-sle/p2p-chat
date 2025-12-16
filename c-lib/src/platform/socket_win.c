#include "p2pnet/socket.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * Intern socket struktur (kun synlig i denne filen)
 */
struct p2p_socket {
    SOCKET handle;      // Windows socket handle
    int type;           // SOCK_STREAM eller SOCK_DGRAM
    int is_listening;   // 1 hvis socket er i listen mode
};

/**
 * Global error buffer (thread-unsafe, men OK for enkle programmer)
 */
static char error_buffer[256];

// ============================================================================
// Initialisering og cleanup
// ============================================================================

int p2p_init(void) {
    WSADATA wsa_data;
    int result = WSAStartup(MAKEWORD(2, 2), &wsa_data);
    
    if (result != 0) {
        snprintf(error_buffer, sizeof(error_buffer), 
                 "WSAStartup failed with error: %d", result);
        return -1;
    }
    
    return 0;
}

void p2p_cleanup(void) {
    WSACleanup();
}

// ============================================================================
// Socket operasjoner
// ============================================================================

p2p_socket_t* p2p_socket_create(int type) {
    // Allokér minne for socket struktur
    p2p_socket_t* sock = (p2p_socket_t*)malloc(sizeof(p2p_socket_t));
    if (!sock) {
        snprintf(error_buffer, sizeof(error_buffer), "Out of memory");
        return NULL;
    }
    
    // Opprett Windows socket
    sock->handle = socket(AF_INET, type, 0);
    if (sock->handle == INVALID_SOCKET) {
        int err = WSAGetLastError();
        snprintf(error_buffer, sizeof(error_buffer), 
                 "socket() failed with error: %d", err);
        free(sock);
        return NULL;
    }
    
    sock->type = type;
    sock->is_listening = 0;
    
    return sock;
}

int p2p_socket_bind(p2p_socket_t* sock, const char* ip, uint16_t port) {
    if (!sock) {
        snprintf(error_buffer, sizeof(error_buffer), "Socket is NULL");
        return -1;
    }
    
    // Sett opp adresse struktur
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);  // Konverter til network byte order
    
    // Konverter IP string til binært format
    if (inet_pton(AF_INET, ip, &addr.sin_addr) != 1) {
        snprintf(error_buffer, sizeof(error_buffer), 
                 "Invalid IP address: %s", ip);
        return -1;
    }
    
    // Bind socket til adresse
    if (bind(sock->handle, (struct sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        int err = WSAGetLastError();
        snprintf(error_buffer, sizeof(error_buffer), 
                 "bind() failed with error: %d", err);
        return -1;
    }
    
    return 0;
}

int p2p_socket_listen(p2p_socket_t* sock, int backlog) {
    if (!sock) {
        snprintf(error_buffer, sizeof(error_buffer), "Socket is NULL");
        return -1;
    }
    
    if (listen(sock->handle, backlog) == SOCKET_ERROR) {
        int err = WSAGetLastError();
        snprintf(error_buffer, sizeof(error_buffer), 
                 "listen() failed with error: %d", err);
        return -1;
    }
    
    sock->is_listening = 1;
    return 0;
}

p2p_socket_t* p2p_socket_accept(p2p_socket_t* sock) {
    if (!sock || !sock->is_listening) {
        snprintf(error_buffer, sizeof(error_buffer), 
                 "Socket is not in listening mode");
        return NULL;
    }
    
    // Aksepter innkommende tilkobling
    struct sockaddr_in client_addr;
    int addr_len = sizeof(client_addr);
    
    SOCKET client_handle = accept(sock->handle, 
                                   (struct sockaddr*)&client_addr, 
                                   &addr_len);
    
    if (client_handle == INVALID_SOCKET) {
        int err = WSAGetLastError();
        snprintf(error_buffer, sizeof(error_buffer), 
                 "accept() failed with error: %d", err);
        return NULL;
    }
    
    // Opprett ny socket struktur for klienten
    p2p_socket_t* client_sock = (p2p_socket_t*)malloc(sizeof(p2p_socket_t));
    if (!client_sock) {
        closesocket(client_handle);
        snprintf(error_buffer, sizeof(error_buffer), "Out of memory");
        return NULL;
    }
    
    client_sock->handle = client_handle;
    client_sock->type = sock->type;
    client_sock->is_listening = 0;
    
    return client_sock;
}

int p2p_socket_connect(p2p_socket_t* sock, const char* ip, uint16_t port) {
    if (!sock) {
        snprintf(error_buffer, sizeof(error_buffer), "Socket is NULL");
        return -1;
    }
    
    // Sett opp server adresse
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    
    if (inet_pton(AF_INET, ip, &server_addr.sin_addr) != 1) {
        snprintf(error_buffer, sizeof(error_buffer), 
                 "Invalid IP address: %s", ip);
        return -1;
    }
    
    // Koble til server
    if (connect(sock->handle, (struct sockaddr*)&server_addr, 
                sizeof(server_addr)) == SOCKET_ERROR) {
        int err = WSAGetLastError();
        snprintf(error_buffer, sizeof(error_buffer), 
                 "connect() failed with error: %d", err);
        return -1;
    }
    
    return 0;
}

ssize_t p2p_socket_send(p2p_socket_t* sock, const void* data, size_t len) {
    if (!sock) {
        snprintf(error_buffer, sizeof(error_buffer), "Socket is NULL");
        return -1;
    }
    
    int result = send(sock->handle, (const char*)data, (int)len, 0);
    
    if (result == SOCKET_ERROR) {
        int err = WSAGetLastError();
        snprintf(error_buffer, sizeof(error_buffer), 
                 "send() failed with error: %d", err);
        return -1;
    }
    
    return result;
}

ssize_t p2p_socket_recv(p2p_socket_t* sock, void* buffer, size_t len) {
    if (!sock) {
        snprintf(error_buffer, sizeof(error_buffer), "Socket is NULL");
        return -1;
    }
    
    int result = recv(sock->handle, (char*)buffer, (int)len, 0);
    
    if (result == SOCKET_ERROR) {
        int err = WSAGetLastError();
        snprintf(error_buffer, sizeof(error_buffer), 
                 "recv() failed with error: %d", err);
        return -1;
    }
    
    return result;
}

void p2p_socket_close(p2p_socket_t* sock) {
    if (!sock) return;
    
    if (sock->handle != INVALID_SOCKET) {
        closesocket(sock->handle);
    }
    
    free(sock);
}

// ============================================================================
// Error handling
// ============================================================================

const char* p2p_get_error(void) {
    return error_buffer;
}

SOCKET p2p_socket_get_handle(p2p_socket_t* sock) {
    if (!sock) return INVALID_SOCKET;
    return sock->handle;
}