#ifndef LIBRAIRIESERVEUR_H
#define LIBRAIRIESERVEUR_H

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stddef.h> // pour size_t
#include <stdbool.h>

#define TAILLE_MAX_DATA 10000

// Création et gestion de serveur
int creerServeur(int port);
int accepterClient(int socketServeur, char* ipClient);
void fermerSocket(int socket);

// Pour chaînes de caractères
int recevoirMessage(int socketClient, char* buffer);
int envoyerReponse(int socketClient, const char* message);

// Pour structures binaires
int recevoirMessageBinaire(int socketClient, void* buffer, size_t taille);
int envoyerMessageBinaire(int socketClient, const void* buffer, size_t taille);

#endif
