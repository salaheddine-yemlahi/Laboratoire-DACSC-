#ifndef LIBRAIRIESERVEUR_H
#define LIBRAIRIESERVEUR_H

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#define TAILLE_MAX_DATA 10000


int creerServeur(int port);
int accepterClient(int socketServeur, char* ipClient);
int recevoirMessage(int socketClient, char* buffer);
int envoyerReponse(int socketClient, const char* message);
void fermerSocket(int socket);

#endif