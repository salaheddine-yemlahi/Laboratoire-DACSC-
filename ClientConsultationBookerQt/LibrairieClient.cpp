#include "LibrairieClient.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

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

int envoyerMessage(int socketServeur, const void* data, size_t taille) {
    int nbBytes = send(socketServeur, data, taille, 0);
    if (nbBytes < 0) {
        perror("Erreur envoi message");
        return -1;
    }
    printf("Envoyé %d octets\n", nbBytes);
    return nbBytes;
}

int recevoirReponse(int socketServeur, void* buffer, size_t taille) {
    size_t totalRecu = 0;
    int nbBytes;
    char *ptr = (char*)buffer;
    
    while(totalRecu < taille)
    {
        nbBytes = recv(socketServeur, ptr + totalRecu, taille - totalRecu, 0);
        
        if (nbBytes < 0) 
        { 
            perror("Erreur reception binaire"); 
            return -1; 
        }
        
        if (nbBytes == 0)
        {
            fprintf(stderr, "Connexion fermée par le client\n");
            return 0;
        }
        
        totalRecu += nbBytes;
    }
    
    return totalRecu;
}

void fermerSocket(int socket) {
    close(socket);
    printf("Connexion fermée\n");
}

