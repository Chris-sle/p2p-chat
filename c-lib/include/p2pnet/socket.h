#ifndef P2PNET_SOCKET_H
#define P2PNET_SOCKET_H

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #include <sys/types.h>
#endif

#include <stddef.h>
#include <stdint.h>

/**
 * Socket typer
 */
#define P2P_TCP SOCK_STREAM
#define P2P_UDP SOCK_DGRAM

/**
 * Opaque socket struktur
 * Implementasjonen er platform-spesifikk (socket_win.c eller socket_unix.c)
 */
typedef struct p2p_socket p2p_socket_t;

/**
 * Initialiserer nettverksbiblioteket
 * MÅ kalles før noen andre funksjoner.
 * På Windows: initialiserer Winsock
 * På Unix: gjør ingenting (men inkluder for kompatibilitet)
 * 
 * @return 0 ved suksess, -1 ved feil
 */
int p2p_init(void);

/**
 * Rydder opp nettverksressurser
 * Skal kalles ved programavslutning
 */
void p2p_cleanup(void);

/**
 * Oppretter en ny socket
 * 
 * @param type P2P_TCP eller P2P_UDP
 * @return Pointer til socket, eller NULL ved feil
 */
p2p_socket_t* p2p_socket_create(int type);

/**
 * Binder socket til en IP-adresse og port (for servere)
 * 
 * @param sock Socket å binde
 * @param ip IP-adresse ("0.0.0.0" for alle interfaces)
 * @param port Port nummer (f.eks. 8080)
 * @return 0 ved suksess, -1 ved feil
 */
int p2p_socket_bind(p2p_socket_t* sock, const char* ip, uint16_t port);

/**
 * Setter socket i lyttemodus (kun for TCP servere)
 * 
 * @param sock Socket å sette i listen mode
 * @param backlog Maksimalt antall ventende connections
 * @return 0 ved suksess, -1 ved feil
 */
int p2p_socket_listen(p2p_socket_t* sock, int backlog);

/**
 * Aksepterer en innkommende tilkobling (blokkerende)
 * 
 * @param sock Server socket (må være i listen mode)
 * @return Ny socket for klienten, eller NULL ved feil
 */
p2p_socket_t* p2p_socket_accept(p2p_socket_t* sock);

/**
 * Kobler til en server (for klienter)
 * 
 * @param sock Socket å koble fra
 * @param ip Server IP-adresse
 * @param port Server port
 * @return 0 ved suksess, -1 ved feil
 */
int p2p_socket_connect(p2p_socket_t* sock, const char* ip, uint16_t port);

/**
 * Sender data over socket
 * 
 * @param sock Socket å sende fra
 * @param data Buffer med data
 * @param len Lengde på data
 * @return Antall bytes sendt, eller -1 ved feil
 */
intptr_t p2p_socket_send(p2p_socket_t* sock, const void* data, size_t len);

/**
 * Mottar data fra socket (blokkerende)
 * 
 * @param sock Socket å motta fra
 * @param buffer Buffer å lagre data i
 * @param len Maksimal lengde å motta
 * @return Antall bytes mottatt, 0 ved disconnect, -1 ved feil
 */
intptr_t p2p_socket_recv(p2p_socket_t* sock, void* buffer, size_t len);

/**
 * Lukker socket og frigjør ressurser
 * 
 * @param sock Socket å lukke (kan være NULL)
 */
void p2p_socket_close(p2p_socket_t* sock);

/**
 * Henter feilmelding for siste socket-operasjon
 * 
 * @return Beskrivelse av feil (statisk buffer, ikke free())
 */
const char* p2p_get_error(void);

/**
 * Henter filnummeret/socket handleren (for event loop)
 * 
 * @param sock Socket
 * @return Socket handle (SOCKET på Windows, int på Unix)
 */
#ifdef _WIN32
SOCKET p2p_socket_get_handle(p2p_socket_t* sock);
#else
int p2p_socket_get_handle(p2p_socket_t* sock);
#endif

#endif /* P2PNET_SOCKET_H */