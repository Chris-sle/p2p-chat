#include <p2pnet/p2pnet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        printf("Usage: %s <identity-file> [server-pubkey]\n", argv[0]);
        printf("\n");
        printf("Arguments:\n");
        printf("  identity-file  - Your identity keypair file\n");
        printf("  server-pubkey  - Expected server public key (Base64, 43 chars)\n");
        printf("                   (optional - omit to accept any server)\n");
        printf("\n");
        printf("Example:\n");
        printf("  %s client.key\n", argv[0]);
        printf("  %s client.key vbX_SEj0gUskjRyCLccqDe_pjFHPMRBEkBBEx8ZTerw\n", argv[0]);
        return 1;
    }

    const char *identity_file = argv[1];
    const char *expected_server_fp = (argc >= 3) ? argv[2] : NULL;

    printf("========================================\n");
    printf(" Secure Client (Milestone 2.2)         \n");
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
    // Step 2: Load client identity
    // ========================================================================

    printf("[INFO] Loading client identity from %s...\n", identity_file);

    p2p_keypair_t *client_keypair = p2p_keypair_load(identity_file);
    if (!client_keypair)
    {
        fprintf(stderr, "[ERROR] Failed to load identity\n");
        p2p_cleanup();
        return 1;
    }

    char fingerprint[64];
    p2p_keypair_fingerprint(client_keypair, fingerprint, sizeof(fingerprint));
    printf("[OK] Client identity: %s\n\n", fingerprint);

    // ========================================================================
    // Step 3: Parse expected server public key (if provided)
    // ========================================================================

    uint8_t expected_server_pubkey[32];
    uint8_t *expected_ptr = NULL;

    if (expected_server_fp)
    {
        if (p2p_pubkey_from_fingerprint(expected_server_pubkey, expected_server_fp) != 0)
        {
            fprintf(stderr, "[ERROR] Invalid server public key format\n");
            p2p_keypair_free(client_keypair);
            p2p_cleanup();
            return 1;
        }

        expected_ptr = expected_server_pubkey;
        printf("[INFO] Expecting server: %s\n\n", expected_server_fp);
    }
    else
    {
        printf("[WARN] No expected server specified - will accept any server\n\n");
    }

    // ========================================================================
    // Step 4: Connect to server
    // ========================================================================

    printf("[INFO] Connecting to 127.0.0.1:8080...\n");

    p2p_socket_t *sock = p2p_socket_create(P2P_TCP);
    if (!sock)
    {
        fprintf(stderr, "[ERROR] Failed to create socket\n");
        p2p_keypair_free(client_keypair);
        p2p_cleanup();
        return 1;
    }

    if (p2p_socket_connect(sock, "127.0.0.1", 8080) != 0)
    {
        fprintf(stderr, "[ERROR] Failed to connect to server\n");
        p2p_socket_close(sock);
        p2p_keypair_free(client_keypair);
        p2p_cleanup();
        return 1;
    }

    printf("[OK] Connected\n\n");

    // ========================================================================
    // Step 5: Perform handshake
    // ========================================================================

    printf("[INFO] Starting handshake...\n");

    p2p_session_t *session = p2p_handshake_client(sock, client_keypair,
                                                  expected_ptr);
    if (!session)
    {
        fprintf(stderr, "\n[ERROR] Handshake failed!\n");
        p2p_socket_close(sock);
        p2p_keypair_free(client_keypair);
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
            printf("Server identity: %s\n", peer_fp);
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
    p2p_socket_close(sock);
    p2p_keypair_free(client_keypair);
    p2p_cleanup();

    return 0;
}