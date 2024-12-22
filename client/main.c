#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>

#pragma comment(lib, "ws2_32.lib")

#define BUFFER_SIZE 1024

int main() {
    WSADATA wsa;
    SOCKET client_socket;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];
    char name[50];

    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        printf("Winsock baslatilamadi. Hata kodu: %d\n", WSAGetLastError());
        exit(1);
    }

    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket == INVALID_SOCKET) {
        printf("Socket olusturulamadi. Hata kodu: %d\n", WSAGetLastError());
        WSACleanup();
        exit(1);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        printf("Baglanti basarisiz. Hata kodu: %d\n", WSAGetLastError());
        closesocket(client_socket);
        WSACleanup();
        exit(1);
    }

    printf("Adinizi girin: ");
    scanf("%s", name);
    send(client_socket, name, sizeof(name), 0);

    // Oylama seçeneklerini al
    int option_count;
    recv(client_socket, (char *)&option_count, sizeof(int), 0);
    char options[option_count][50];
    printf("\nOylama Secenekleri:\n");
    for (int i = 0; i < option_count; i++) {
        recv(client_socket, options[i], sizeof(options[i]), 0);
        printf("%d. %s\n", i + 1, options[i]);
    }

    // Oy kullanma (geçerli bir giriş yapılana kadar tekrar iste)
    int vote;
    do {
        printf("\nOy vermek istediginiz secenegin numarasini girin (1-%d): ", option_count);
        scanf("%d", &vote);
    } while (vote < 1 || vote > option_count);

    vote--; // Sunucu tarafında 0-indexli olduğundan azaltıyoruz
    send(client_socket, (char *)&vote, sizeof(int), 0);

    // Oylama sonucu al
    recv(client_socket, buffer, sizeof(buffer), 0);
    printf("\n%s\n", buffer);

    closesocket(client_socket);
    WSACleanup();
    return 0;
}
