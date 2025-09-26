#include "LibrairieClient.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char* argv[]) {
    if(argc != 3){
        printf("il faut donner l'IP et le PORT svp.");
        return -1;
    }
    printf("=== Test du client ===\n");
    
    // Se connecter au serveur sur le port 1234
    int client = connecterServeur(argv[1], atoi(argv[2]));  // ← "127.0.0.1" au lieu de "localhost"
    if (client < 0) {
        printf("Erreur: impossible de connecter avec le serveur\n");
        return -1;
    }  // ← Pas de ; après }
    
    printf("Connexion réussie!\n");  // ← Ajout pour confirmer
    
    // Optionnel : envoyer un message de test
    envoyerMessage(client, "Hello serveur!");
    
    // Optionnel : recevoir la réponse
    char buffer[TAILLE_MAX_DATA];
    recevoirReponse(client, buffer);
    
    // Fermer la connexion
    fermerSocket(client);
    
    return 0;
}