#include "p2pnet/event_loop.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>

#define INITIAL_CAPACITY 16

/**
 * Socket registration info
 */
typedef struct {
    p2p_socket_t* sock;
    p2p_read_callback on_read;
    p2p_error_callback on_error;
    void* user_data;
} socket_entry_t;

/**
 * Event loop internal structure
 */
struct p2p_event_loop {
    socket_entry_t* entries;    // Array av socket registrations
    WSAPOLLFD* poll_fds;        // Array for WSAPoll
    int num_sockets;            // Antall aktive sockets
    int capacity;               // Allocated capacity
    int running;                // 1 hvis loop kjører
};

// ============================================================================
// Helper Functions
// ============================================================================

/**
 * Finn index for socket i entries array
 */
static int find_socket_index(p2p_event_loop_t* loop, p2p_socket_t* sock) {
    for (int i = 0; i < loop->num_sockets; i++) {
        if (loop->entries[i].sock == sock) {
            return i;
        }
    }
    return -1;
}

/**
 * Ekspander kapasitet hvis nødvendig
 */
static int ensure_capacity(p2p_event_loop_t* loop) {
    if (loop->num_sockets < loop->capacity) {
        return 0;  // Har plass
    }
    
    int new_capacity = loop->capacity * 2;
    
    socket_entry_t* new_entries = (socket_entry_t*)realloc(
        loop->entries, 
        new_capacity * sizeof(socket_entry_t)
    );
    
    if (!new_entries) {
        fprintf(stderr, "Failed to expand entries array\n");
        return -1;
    }
    
    WSAPOLLFD* new_poll_fds = (WSAPOLLFD*)realloc(
        loop->poll_fds,
        new_capacity * sizeof(WSAPOLLFD)
    );
    
    if (!new_poll_fds) {
        fprintf(stderr, "Failed to expand poll_fds array\n");
        return -1;
    }
    
    loop->entries = new_entries;
    loop->poll_fds = new_poll_fds;
    loop->capacity = new_capacity;
    
    return 0;
}

// ============================================================================
// Public API
// ============================================================================

p2p_event_loop_t* p2p_event_loop_create(void) {
    p2p_event_loop_t* loop = (p2p_event_loop_t*)malloc(sizeof(p2p_event_loop_t));
    if (!loop) return NULL;
    
    loop->entries = (socket_entry_t*)malloc(INITIAL_CAPACITY * sizeof(socket_entry_t));
    if (!loop->entries) {
        free(loop);
        return NULL;
    }
    
    loop->poll_fds = (WSAPOLLFD*)malloc(INITIAL_CAPACITY * sizeof(WSAPOLLFD));
    if (!loop->poll_fds) {
        free(loop->entries);
        free(loop);
        return NULL;
    }
    
    loop->num_sockets = 0;
    loop->capacity = INITIAL_CAPACITY;
    loop->running = 0;
    
    return loop;
}

void p2p_event_loop_free(p2p_event_loop_t* loop) {
    if (!loop) return;
    
    if (loop->entries) free(loop->entries);
    if (loop->poll_fds) free(loop->poll_fds);
    free(loop);
}

int p2p_event_loop_add_socket(p2p_event_loop_t* loop,
                               p2p_socket_t* sock,
                               p2p_read_callback on_read,
                               p2p_error_callback on_error,
                               void* user_data) {
    if (!loop || !sock) return -1;
    
    // Sjekk om socket allerede er registrert
    if (find_socket_index(loop, sock) >= 0) {
        fprintf(stderr, "Socket already registered\n");
        return -1;
    }
    
    // Ekspander hvis nødvendig
    if (ensure_capacity(loop) < 0) {
        return -1;
    }
    
    // Legg til i entries
    int index = loop->num_sockets;
    loop->entries[index].sock = sock;
    loop->entries[index].on_read = on_read;
    loop->entries[index].on_error = on_error;
    loop->entries[index].user_data = user_data;
    
    // Sett opp for WSAPoll
    loop->poll_fds[index].fd = p2p_socket_get_handle(sock);
    loop->poll_fds[index].events = POLLIN;  // Lytt på lesbare events
    loop->poll_fds[index].revents = 0;
    
    loop->num_sockets++;
    
    return 0;
}

int p2p_event_loop_remove_socket(p2p_event_loop_t* loop, p2p_socket_t* sock) {
    if (!loop || !sock) return -1;
    
    int index = find_socket_index(loop, sock);
    if (index < 0) {
        return -1;  // Ikke funnet
    }
    
    // Flytt siste element til denne plassen (swap & pop)
    int last_index = loop->num_sockets - 1;
    if (index != last_index) {
        loop->entries[index] = loop->entries[last_index];
        loop->poll_fds[index] = loop->poll_fds[last_index];
    }
    
    loop->num_sockets--;
    
    return 0;
}

void p2p_event_loop_run(p2p_event_loop_t* loop) {
    if (!loop) return;
    
    loop->running = 1;
    
    printf("[EVENT_LOOP] Started (monitoring %d sockets)\n", loop->num_sockets);
    
    while (loop->running) {
        if (loop->num_sockets == 0) {
            // Ingen sockets å overvåke
            printf("[EVENT_LOOP] No sockets to monitor, stopping\n");
            break;
        }
        
        // Vent på events (timeout 1000ms)
        int result = WSAPoll(loop->poll_fds, loop->num_sockets, 1000);
        
        if (result == SOCKET_ERROR) {
            int err = WSAGetLastError();
            fprintf(stderr, "[EVENT_LOOP] WSAPoll error: %d\n", err);
            break;
        }
        
        if (result == 0) {
            // Timeout (ingen events)
            continue;
        }
        
        // Sjekk hvilke sockets som har events
        // VIKTIG: Iterer baklengs for å håndtere removal under iteration
        for (int i = loop->num_sockets - 1; i >= 0; i--) {
            WSAPOLLFD* pfd = &loop->poll_fds[i];
            socket_entry_t* entry = &loop->entries[i];
            
            if (pfd->revents == 0) {
                continue;  // Ingen event på denne socketen
            }
            
            // Sjekk for error/disconnect
            if (pfd->revents & (POLLERR | POLLHUP | POLLNVAL)) {
                if (entry->on_error) {
                    entry->on_error(entry->sock, pfd->revents, entry->user_data);
                }
                continue;
            }
            
            // Sjekk for lesbar data
            if (pfd->revents & POLLIN) {
                if (entry->on_read) {
                    entry->on_read(entry->sock, entry->user_data);
                }
            }
            
            // Reset revents
            pfd->revents = 0;
        }
    }
    
    printf("[EVENT_LOOP] Stopped\n");
}

void p2p_event_loop_stop(p2p_event_loop_t* loop) {
    if (!loop) return;
    loop->running = 0;
}

int p2p_event_loop_socket_count(p2p_event_loop_t* loop) {
    if (!loop) return -1;
    return loop->num_sockets;
}