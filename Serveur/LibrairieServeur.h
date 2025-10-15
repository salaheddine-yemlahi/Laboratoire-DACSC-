#ifndef LIBRAIRIESERVEUR_H
#define LIBRAIRIESERVEUR_H

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stddef.h> // pour size_t
#include <stdbool.h>

#define TAILLE_MAX_DATA 10000


int creerServeur(int port);
int accepterClient(int socketServeur, char* ipClient);
void fermerSocket(int socket);


int recevoirMessage(int socketClient, void* buffer, size_t taille);
int envoyerMessage(int socketClient, const void* buffer, size_t taille);

#endif
