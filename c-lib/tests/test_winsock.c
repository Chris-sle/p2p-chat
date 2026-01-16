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