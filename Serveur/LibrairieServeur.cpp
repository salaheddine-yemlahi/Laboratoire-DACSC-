#include "LibrairieServeur.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>


int creerServeur(int port){
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) return -1;

    struct sockaddr_in adresse;
    adresse.sin_family = AF_INET;
    adresse.sin_addr.s_addr = INADDR_ANY;
    adresse.sin_port = htons(port);

    if (bind(sock, (struct sockaddr*)&adresse, sizeof(adresse)) < 0) return -1;

    listen(sock, 20);
    return sock;
}

int accepterClient(int socketServeur, char* ipClient){
    struct sockaddr_in adresseClient;
    socklen_t tailleAdresse = sizeof(adresseClient);

    int socketClient = accept(socketServeur, (struct sockaddr*)&adresseClient, &tailleAdresse);
    if (socketClient < 0) return -1;

    strcpy(ipClient, inet_ntoa(adresseClient.sin_addr));
    printf("Client connecté : %s\n", ipClient);
    return socketClient;
}

void fermerSocket(int socket){
    close(socket);
    printf("Socket fermé\n");
}

int recevoirMessage(int socketClient, void* buffer, size_t taille)
{
    size_t totalRecu = 0;
    int nbBytes;
    char *ptr = (char*)buffer;
    
    while(totalRecu < taille)
    {
        nbBytes = recv(socketClient, ptr + totalRecu, taille - totalRecu, 0);
        
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

int envoyerMessage(int socketClient, const void* buffer, size_t taille){
    int nbBytes = send(socketClient, buffer, taille, 0);
    if (nbBytes < 0) { perror("Erreur envoi binaire"); return -1; }
    return nbBytes;
}
