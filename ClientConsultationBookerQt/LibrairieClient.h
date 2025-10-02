#ifndef LIBRAIRIECLIENT_H
#define LIBRAIRIECLIENT_H

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stddef.h>

#define TAILLE_MAX_DATA 10000

int connecterServeur(const char* ip, int port);
int envoyerMessage(int socketServeur, const void* data, size_t taille);
int recevoirReponse(int socketServeur, void* buffer, size_t taille);
void fermerSocket(int socket);

#endif