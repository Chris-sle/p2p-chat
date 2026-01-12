#ifndef P2PNET_EVENT_LOOP_H
#define P2PNET_EVENT_LOOP_H

#include "p2pnet/socket.h"

/**
 * Opaque event loop structure
 */
typedef struct p2p_event_loop p2p_event_loop_t;

/**
 * Callback når socket har data tilgjengelig for lesing
 * 
 * @param sock Socket som har data
 * @param user_data User-supplied data fra add_socket()
 */
typedef void (*p2p_read_callback)(p2p_socket_t* sock, void* user_data);

/**
 * Callback når socket opplever en feil eller disconnect
 * 
 * @param sock Socket som har error
 * @param error Error code
 * @param user_data User-supplied data
 */
typedef void (*p2p_error_callback)(p2p_socket_t* sock, int error, void* user_data);

/**
 * Oppretter en ny event loop
 * 
 * @return Ny event loop, eller NULL ved feil
 */
p2p_event_loop_t* p2p_event_loop_create(void);

/**
 * Frigjør event loop og alle ressurser
 * 
 * @param loop Event loop å frigjøre (kan være NULL)
 */
void p2p_event_loop_free(p2p_event_loop_t* loop);

/**
 * Legger til en socket i event loop
 * 
 * @param loop Event loop
 * @param sock Socket å overvåke
 * @param on_read Callback når data er tilgjengelig (kan være NULL)
 * @param on_error Callback ved feil (kan være NULL)
 * @param user_data User data sendt til callbacks
 * @return 0 ved suksess, -1 ved feil
 */
int p2p_event_loop_add_socket(p2p_event_loop_t* loop,
                               p2p_socket_t* sock,
                               p2p_read_callback on_read,
                               p2p_error_callback on_error,
                               void* user_data);

/**
 * Fjerner en socket fra event loop
 * 
 * @param loop Event loop
 * @param sock Socket å fjerne
 * @return 0 ved suksess, -1 ved feil
 */
int p2p_event_loop_remove_socket(p2p_event_loop_t* loop, p2p_socket_t* sock);

/**
 * Kjører event loop (blokkerer her til p2p_event_loop_stop() kalles)
 * Overvåker alle registrerte sockets og kaller callbacks når events skjer
 * 
 * @param loop Event loop å kjøre
 */
void p2p_event_loop_run(p2p_event_loop_t* loop);

/**
 * Stopper event loop (kan kalles fra en callback)
 * 
 * @param loop Event loop å stoppe
 */
void p2p_event_loop_stop(p2p_event_loop_t* loop);

/**
 * Henter antall sockets i event loop
 * 
 * @param loop Event loop
 * @return Antall sockets, eller -1 ved feil
 */
int p2p_event_loop_socket_count(p2p_event_loop_t* loop);

#endif /* P2PNET_EVENT_LOOP_H */