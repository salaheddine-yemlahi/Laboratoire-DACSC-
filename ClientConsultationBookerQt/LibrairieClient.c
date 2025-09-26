#include "LibrairieClient.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int connecterServeur(const char* ip, int port) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("Erreur création socket client");
        return -1;
    }
    

    struct sockaddr_in adresseServeur;
    adresseServeur.sin_family = AF_INET;
    adresseServeur.sin_port = htons(port);
    adresseServeur.sin_addr.s_addr = inet_addr(ip);
    
    if (connect(sock, (struct sockaddr*)&adresseServeur, sizeof(adresseServeur)) < 0) {
        perror("Erreur connexion au serveur");
        return -1;
    }
    
    printf("Connecté au serveur %s:%d\n", ip, port);
    return sock;
}

int envoyerMessage(int socketServeur, const char* message) {
    int nbBytes = send(socketServeur, message, strlen(message), 0);
    if (nbBytes < 0) {
        perror("Erreur envoi message");
        return -1;
    }
    
    printf("Envoyé: %s\n", message);
    return nbBytes;
}

int recevoirReponse(int socketServeur, char* buffer) {
    int nbBytes = recv(socketServeur, buffer, TAILLE_MAX_DATA - 1, 0);
    if (nbBytes < 0) {
        perror("Erreur réception réponse");
        return -1;
    }
    
    buffer[nbBytes] = '\0';
    printf("Reçu: %s\n", buffer);
    return nbBytes;
}

void fermerSocket(int socket) {
    close(socket);
    printf("Connexion fermée\n");
}