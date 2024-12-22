#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <windows.h>
#include <process.h>

#pragma comment(lib, "ws2_32.lib") // Winsock kütüphanesini baðla

#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024

typedef struct {
    SOCKET socket;
    char name[50];
} Client;

Client clients[MAX_CLIENTS];
int client_count = 0;

HANDLE clients_mutex;

char options[BUFFER_SIZE][50]; // Oylama seçenekleri
int votes[BUFFER_SIZE];        // Her seçenek için oy sayýsý
int option_count = 0;
int total_votes = 0;

void handle_client(void *arg) {
    SOCKET client_socket = *(SOCKET *)arg;
    char buffer[BUFFER_SIZE];
    char name[50];

    // Kullanýcý adýný al
    recv(client_socket, name, sizeof(name), 0);

    WaitForSingleObject(clients_mutex, INFINITE);
    strcpy(clients[client_count].name, name);
    clients[client_count].socket = client_socket;
    client_count++;
    ReleaseMutex(clients_mutex);

    printf("%s baglandi.\n", name);

    // Oylama seçeneklerini gönder
    send(client_socket, &option_count, sizeof(int), 0);
    for (int i = 0; i < option_count; i++) {
        send(client_socket, options[i], sizeof(options[i]), 0);
    }

    // Oy al
    int vote;
    recv(client_socket, (char *)&vote, sizeof(int), 0);

    WaitForSingleObject(clients_mutex, INFINITE);
    votes[vote]++;
    total_votes++;
    printf("%s, %s secenegine oy verdi.\n", name, options[vote]);

    // Oy sonucu gönder
    char result_message[BUFFER_SIZE];
    snprintf(result_message, sizeof(result_message),
             "Oyunuz '%s' secenegi icin kaydedildi.", options[vote]);
    send(client_socket, result_message, strlen(result_message), 0);
    ReleaseMutex(clients_mutex);

    // Tüm oylar tamamlandýðýnda sonuçlarý hesapla
    if (total_votes == client_count) {
        int max_votes = 0, winning_option = -1;
        printf("\nOylama Sonuclari:\n");
        for (int i = 0; i < option_count; i++) {
            printf("%s: %d oy\n", options[i], votes[i]);
            if (votes[i] > max_votes) {
                max_votes = votes[i];
                winning_option = i;
            }
        }
        printf("Kazanan: %s (%d oy)\n", options[winning_option], max_votes);
    }

    closesocket(client_socket);
    _endthread();
}

int main() {
    WSADATA wsa;
    SOCKET server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    int addr_len = sizeof(client_addr);

    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        printf("Winsock baslatilamadi. Hata kodu: %d\n", WSAGetLastError());
        exit(1);
    }

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == INVALID_SOCKET) {
        printf("Socket olusturulamadi. Hata kodu: %d\n", WSAGetLastError());
        WSACleanup();
        exit(1);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        printf("Bind basarisiz. Hata kodu: %d\n", WSAGetLastError());
        closesocket(server_socket);
        WSACleanup();
        exit(1);
    }

    if (listen(server_socket, MAX_CLIENTS) == SOCKET_ERROR) {
        printf("Listen basarisiz. Hata kodu: %d\n", WSAGetLastError());
        closesocket(server_socket);
        WSACleanup();
        exit(1);
    }

    printf("Kac kullanici katilacak? ");
    int total_clients;
    scanf("%d", &total_clients);

    printf("Oylama icin kac secenek var? ");
    scanf("%d", &option_count);
    for (int i = 0; i < option_count; i++) {
        printf("Secenek %d: ", i + 1);
        scanf("%s", options[i]);
        votes[i] = 0;
    }

    printf("Sunucu bekliyor...\n");

    clients_mutex = CreateMutex(NULL, FALSE, NULL);
    while (client_count < total_clients) {
        client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &addr_len);
        if (client_socket == INVALID_SOCKET) {
            printf("Accept basarisiz. Hata kodu: %d\n", WSAGetLastError());
            continue;
        }
        _beginthread(handle_client, 0, &client_socket);
    }

    closesocket(server_socket);
    WSACleanup();
    return 0;
}
