#ifndef LIBRAIRIECLIENT_H
#define LIBRAIRIECLIENT_H

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#define TAILLE_MAX_DATA 10000

int connecterServeur(const char* ip, int port);
int envoyerMessage(int socketServeur, const char* message);
int recevoirReponse(int socketServeur, char* buffer);
void fermerSocket(int socket);

#endif