#include <iostream>
#include <cstring>
#include <arpa/inet.h>
#include <unistd.h>

typedef struct {
    char adressIP[20];
    char nom[20];
    char prenom[20];
    int numeroPatient;
} CLIENT;

int main() {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) { perror("socket"); return 1; }

    sockaddr_in servAddr;
    servAddr.sin_family = AF_INET;
    servAddr.sin_port = htons(5000); // port du serveur
    inet_pton(AF_INET, "127.0.0.1", &servAddr.sin_addr);

    if (connect(sock, (struct sockaddr*)&servAddr, sizeof(servAddr)) < 0) {
        perror("connect"); return 1;
    }

    // envoyer un message pour demander les clients
    // const char* msg = "LIST_CLIENTS";

    int nbClients;
    // recevoir d'abord le nombre de clients
    recv(sock, &nbClients, sizeof(int), 0);
    std::cout << "Nombre de clients reçus : " << nbClients << std::endl;

    // recevoir chaque structure CLIENT
    for (int i = 0; i < nbClients; i++) {
        CLIENT clien;
        int n = recv(sock, &clien, sizeof(CLIENT), 0);
        if (n <= 0) {
            perror("recv CLIENT");
            break;
        }

        std::cout << "Client " << i+1 << " :" << std::endl;
        std::cout << "  IP : " << clien.adressIP << std::endl;
        std::cout << "  Nom : " << clien.nom << std::endl;
        std::cout << "  Prenom : " << clien.prenom << std::endl;
        std::cout << "  NumeroPatient : " << clien.numeroPatient << std::endl;
        std::cout << "----------------------" << std::endl;
    }

    close(sock);
    return 0;
}
