#include <p2pnet/p2pnet.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        printf("Usage: %s <identity-file>\n", argv[0]);
        printf("\n");
        printf("Example:\n");
        printf("  %s server.key\n", argv[0]);
        return 1;
    }

    const char *identity_file = argv[1];

    printf("========================================\n");
    printf(" Secure Server (Milestone 2.2)         \n");
    printf("========================================\n\n");

    // ========================================================================
    // Step 1: Initialize library
    // ========================================================================

    if (p2p_init() != 0)
    {
        fprintf(stderr, "[ERROR] Failed to initialize P2PNet\n");
        return 1;
    }

    // ========================================================================
    // Step 2: Load server identity
    // ========================================================================

    printf("[INFO] Loading server identity from %s...\n", identity_file);

    p2p_keypair_t *server_keypair = p2p_keypair_load(identity_file);
    if (!server_keypair)
    {
        fprintf(stderr, "[ERROR] Failed to load identity\n");
        p2p_cleanup();
        return 1;
    }

    // Show fingerprint
    char fingerprint[64];
    p2p_keypair_fingerprint(server_keypair, fingerprint, sizeof(fingerprint));
    printf("[OK] Server identity: %s\n\n", fingerprint);

    // ========================================================================
    // Step 3: Create listening socket
    // ========================================================================

    p2p_socket_t *listen_sock = p2p_socket_create(P2P_TCP);
    if (!listen_sock)
    {
        fprintf(stderr, "[ERROR] Failed to create socket\n");
        p2p_keypair_free(server_keypair);
        p2p_cleanup();
        return 1;
    }

    // Bind to port 8080
    if (p2p_socket_bind(listen_sock, "0.0.0.0", 8080) != 0)
    {
        fprintf(stderr, "[ERROR] Failed to bind to port 8080\n");
        p2p_socket_close(listen_sock);
        p2p_keypair_free(server_keypair);
        p2p_cleanup();
        return 1;
    }

    if (p2p_socket_listen(listen_sock, 5) != 0)
    {
        fprintf(stderr, "[ERROR] Failed to listen\n");
        p2p_socket_close(listen_sock);
        p2p_keypair_free(server_keypair);
        p2p_cleanup();
        return 1;
    }

    printf("[INFO] Listening on port 8080\n");
    printf("[INFO] Waiting for clients...\n\n");

    // ========================================================================
    // Step 4: Accept client connection
    // ========================================================================

    p2p_socket_t *client_sock = p2p_socket_accept(listen_sock);
    if (!client_sock)
    {
        fprintf(stderr, "[ERROR] Failed to accept client\n");
        p2p_socket_close(listen_sock);
        p2p_keypair_free(server_keypair);
        p2p_cleanup();
        return 1;
    }

    printf("[OK] Client connected\n\n");

    // ========================================================================
    // Step 5: Perform handshake
    // ========================================================================

    printf("[INFO] Starting handshake...\n");

    // Accept any peer (NULL = allow all)
    p2p_session_t *session = p2p_handshake_server(client_sock, server_keypair,
                                                  NULL, 0);
    if (!session)
    {
        fprintf(stderr, "\n[ERROR] Handshake failed!\n");
        p2p_socket_close(client_sock);
        p2p_socket_close(listen_sock);
        p2p_keypair_free(server_keypair);
        p2p_cleanup();
        return 1;
    }

    printf("\n");

    // ========================================================================
    // Step 6: Show session info
    // ========================================================================

    const uint8_t *peer_pubkey = p2p_session_peer_pubkey(session);
    if (peer_pubkey)
    {
        char peer_fp[64];
        if (p2p_session_peer_fingerprint(session, peer_fp, sizeof(peer_fp)))
        {
            printf("========================================\n");
            printf(" Session Established                   \n");
            printf("========================================\n");
            printf("Peer identity: %s\n", peer_fp);
            printf("========================================\n\n");
        }
    }

    printf("[INFO] Session established! Ready for encrypted communication.\n");
    printf("[INFO] Press Ctrl+C to exit\n\n");

    // Keep connection open
    getchar();

    // ========================================================================
    // Cleanup
    // ========================================================================

    p2p_session_free(session);
    p2p_socket_close(client_sock);
    p2p_socket_close(listen_sock);
    p2p_keypair_free(server_keypair);
    p2p_cleanup();

    return 0;
}