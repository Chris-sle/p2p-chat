#ifndef P2PNET_MESSAGE_H
#define P2PNET_MESSAGE_H

#include "p2pnet/socket.h"
#include <stdint.h>
#include <stddef.h>

/**
 * * Maximum message size (1MB)
 * Dette forhindrer at noen sender en 4GB melding og crasher serveren.
 */
#define P2P_MAX_MESSAGE_SIZE (1024 * 1024)

/**
 * Message struktur
 * Representerer en komplett, framed melding
 */
typedef struct {
    uint32_t length;   // Lengde på data
    uint8_t* data;     // Pointer til data buffer
} p2p_message_t;

/**
 * Oppretter en ny melding fra string
 * 
 * @param text Null-terminated string
 * @return Ny melding, eller NULL ved feil
 * 
 * Merk: Kalleren må kalle p2p_message_free() senere
 */
p2p_message_t* p2p_message_create(const char* text);

/**
 * Oppretter en ny melding fra binær data
 * 
 * @param data Binær data
 * @param length Lengde på data
 * @return Ny melding, eller NULL ved feil
 */
p2p_message_t* p2p_message_create_binary(const void* data, size_t length);

/**
 * Sender en framed melding over socket
 * Sender først length-header (4 bytes), deretter data
 * 
 * @param sock Socket å sende over
 * @param msg Melding å sende
 * @return 0 ved suksess, -1 ved feil
 * 
 * Merk: Håndterer partial sends automatisk (blokkerende)
 */
int p2p_message_send(p2p_socket_t* sock, p2p_message_t* msg);

/**
 * Mottar en komplett framed melding fra socket (blokkerende)
 * Leser først 4-byte header, deretter eksakt så mye data
 * 
 * @param sock Socket å motta fra
 * @return Ny melding, eller NULL ved feil/disconnect
 * 
 * Merk: Kalleren må kalle p2p_message_free() senere
 */
p2p_message_t* p2p_message_recv(p2p_socket_t* sock);

/**
 * Frigjør minne allokert for melding
 * 
 * @param msg Melding å frigjøre (kan være NULL)
 */
void p2p_message_free(p2p_message_t* msg);

/**
 * Hjelpefunksjon: Skriv ut melding som string (for debugging)
 * 
 * @param msg Melding å printe
 * @param prefix Prefix for output (f.eks. "Received: ")
 */
void p2p_message_print(const p2p_message_t* msg, const char* prefix);

#endif /* P2PNET_MESSAGE_H */