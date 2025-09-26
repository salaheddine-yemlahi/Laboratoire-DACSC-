#include "LibrairieServeur.h"
#include <stdio.h>
#include <string.h>


int creerServeur(int port){
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    /*
    * AF_INET pour IPV4
    * SOCK_STREAM POUR TCP
    */

   /*
    * struct sockaddr_in :
    * short sin_family;        // Type d'adresse (AF_INET = IPv4)
    * unsigned short sin_port; // Numéro de port (ex: 1234)
    * struct in_addr sin_addr; // Adresse IP (ex: 192.168.1.10)
    * char sin_zero[8];        // Padding (remplissage)
   */
   struct sockaddr_in adresse;
    adresse.sin_family = AF_INET;
    adresse.sin_addr.s_addr = INADDR_ANY; // Toutes les IP de la machine
    adresse.sin_port = htons(port);
    if (bind(sock, (struct sockaddr*)&adresse, sizeof(adresse)) < 0) {
        return -1;
    }
    listen(sock, 5);  // Max 5 connexions en attente
   return sock;
}

int accepterClient(int socketServeur, char* ipClient) {
    struct sockaddr_in adresseClient;
    socklen_t tailleAdresse = sizeof(adresseClient);
    
    int socketClient = accept(socketServeur, (struct sockaddr*)&adresseClient, &tailleAdresse);
    if (socketClient < 0) {
        perror("Erreur accept");
        return -1;
    }
    printf("Client connecté\n");
    strcpy(ipClient, inet_ntoa(adresseClient.sin_addr));
    return socketClient;
}

int recevoirMessage(int socketClient, char* buffer) {
    int nbBytes = recv(socketClient, buffer, TAILLE_MAX_DATA - 1, 0);
    if (nbBytes < 0) {
        perror("Erreur reception");
        return -1;
    }
    buffer[nbBytes] = '\0';  // Terminer la chaîne
    printf("Reçu: %s\n", buffer);
    return nbBytes;
}

int envoyerReponse(int socketClient, const char* message) {
    int nbBytes = send(socketClient, message, strlen(message), 0);
    if (nbBytes < 0) {
        perror("Erreur envoi");
        return -1;
    }
    printf("Envoyé: %s\n", message);
    return nbBytes;
}

void fermerSocket(int socket) {
    close(socket);
    printf("Socket fermé\n");
}