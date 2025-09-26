#include "LibrairieServeur.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char* argv[]) {
    if(argc != 2){
        printf("il faut passer le port svp.");
        return -1;
    }
    printf("=== Test du serveur ===\n");
    
    // Démarrer le serveur sur le port 1234
    int serveur = creerServeur(atoi(argv[1]));
    if (serveur < 0) {
        printf("Erreur: impossible de créer le serveur\n");
        return -1;
    }
    
    printf("Serveur en attente de connexion...\n");
    
    // Accepter un client
    int client = accepterClient(serveur);
    if (client < 0) {
        printf("Erreur: impossible d'accepter le client\n");
        return -1;
    }
    
    // Recevoir un message
    char buffer[TAILLE_MAX_DATA];
    recevoirMessage(client, buffer);
    
    // Répondre
    envoyerReponse(client, "Message bien reçu!");
    
    // Fermer
    fermerSocket(client);
    fermerSocket(serveur);
    
    return 0;
}