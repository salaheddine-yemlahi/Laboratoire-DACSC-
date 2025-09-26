#include "LibrairieServeur.h"
#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h> 
#include <signal.h> 
#include <pthread.h>
#include <stdbool.h>
#define TAILLE_FILE_ATTENTE 20 

void HandlerSIGINT(int s);
void TraitementConnexion(int sService); 
void* FctThreadClient(void* p);
pthread_mutex_t mutexSocketsAcceptees; 
pthread_cond_t  condSocketsAcceptees; 
int socketsAcceptees[TAILLE_FILE_ATTENTE]; 
int indiceEcriture=0, indiceLecture=0;
int serveur;


int PORT_RESERVATION, NB_THREADS_POOL;

void lireConfig() {
    FILE* f = fopen("config.txt", "r");
    fscanf(f, "PORT_RESERVATION=%d", &PORT_RESERVATION);
    fscanf(f, "NB_THREADS_POOL=%d", &NB_THREADS_POOL);
    fclose(f);
}

int main(){

    lireConfig();
    // Initialisation socketsAcceptees 
    pthread_mutex_init(&mutexSocketsAcceptees,NULL);  //Initialise un mutex pour protéger une ressource partagée contre les accès concurrents.
    pthread_cond_init(&condSocketsAcceptees,NULL);  // Initialise une variable de condition pour que les threads puissent s'endormir/se réveiller quand une condition change.
    for (int i=0 ; i<TAILLE_FILE_ATTENTE ; i++){
        socketsAcceptees[i] = -1;  // vider le compteur des sockets acceptés.
    }

    // Armement des signaux
    struct sigaction A;
    A.sa_flags = 0; 
    sigemptyset(&A.sa_mask);
    A.sa_handler = HandlerSIGINT;
    if (sigaction(SIGINT,&A,NULL) == -1) { 
        perror("Erreur de sigaction"); 
        exit(1);
    }
    

    // Creation de la socket d'écoute
    serveur = creerServeur(PORT_RESERVATION);
    if (serveur < 0) {
        printf("Erreur: impossible de créer le serveur\n");
        return -1;
    }

    // Creation du pool de threads
    printf("Création du pool de threads.\n");
    pthread_t threads[NB_THREADS_POOL];
    for (int i=0 ; i<NB_THREADS_POOL ; i++){
        pthread_create(&threads[i],NULL,FctThreadClient,NULL);
    }
    

    // Mise en boucle du serveur
    int sService; // le socket de client après l'acceptation.
    char ipClient[50]; // pour stocker un seul ip.
    printf("Demarrage du serveur.\n");

    while(1){
        printf("Attente d'une connexion...\n");
        if ((sService = accepterClient(serveur, ipClient)) == -1) {
            perror("Erreur de Accept"); 
            fermerSocket(serveur);
            exit(1);
        }
        printf("Connexion acceptée : IP=%s socket=%d\n",ipClient,sService);
        // Insertion en liste d'attente et réveil d'un thread du pool
        // (Production d'une tâche) 
        pthread_mutex_lock(&mutexSocketsAcceptees);
        socketsAcceptees[indiceEcriture] = sService;
        indiceEcriture++;
        if (indiceEcriture == TAILLE_FILE_ATTENTE) indiceEcriture = 0; // a modifie ou supprimer après.
        pthread_mutex_unlock(&mutexSocketsAcceptees); 
        pthread_cond_signal(&condSocketsAcceptees);
    }
}


void* FctThreadClient(void* p){
    int sService; 
    while(1) { 
        printf("\t[THREAD %ld] Attente socket...\n", (long)pthread_self());
        // Attente d'une tâche 
        pthread_mutex_lock(&mutexSocketsAcceptees);
        while (indiceEcriture == indiceLecture){
            pthread_cond_wait(&condSocketsAcceptees,&mutexSocketsAcceptees);
        }
        sService = socketsAcceptees[indiceLecture];
        socketsAcceptees[indiceLecture] = -1; 
        indiceLecture++; 
        if (indiceLecture == TAILLE_FILE_ATTENTE) indiceLecture = 0; 
        pthread_mutex_unlock(&mutexSocketsAcceptees); 
        // Traitement de la connexion (consommation de la tâche)
        printf("\t[THREAD %ld] Je m'occupe de la socket %d\n", (long)pthread_self(), sService);
        TraitementConnexion(sService);
    }
}


void HandlerSIGINT(int s) 
{ 
    printf("\nArret du serveur.\n");
    close(serveur);
    pthread_mutex_lock(&mutexSocketsAcceptees);
    for (int i=0 ; i<TAILLE_FILE_ATTENTE ; i++){
        if (socketsAcceptees[i] != -1) close(socketsAcceptees[i]);
    }
    pthread_mutex_unlock(&mutexSocketsAcceptees); 
    //MOP_Close(); 
    exit(0);
}


void TraitementConnexion(int sService) 
{
    char requete[200], reponse[200];
    int nbLus, nbEcrits;
    bool onContinue = true;
    while (onContinue){
        printf("\t[THREAD %ld] Attente requete...\n", (long)pthread_self());
        // ***** Reception Requete ******************
        if ((nbLus = recevoirMessage(sService,requete)) < 0) {
            perror("Erreur de Receive");
            close(sService);
            HandlerSIGINT(0);
        }
        // ***** Fin de connexion ? *****************
        if (nbLus == 0) {
            printf("\t[THREAD %ld] Fin de connexion du client.\n", (long)pthread_self()); 
            close(sService);
            return;
        }
        requete[nbLus] = 0;
        printf("\t[THREAD %ld] Requete recue = %s\n", (long)pthread_self(), requete);
        // ***** Traitement de la requete ***********
        //onContinue = SMOP(requete,reponse,sService);
        // ***** Envoi de la reponse ****************
        if ((nbEcrits = envoyerReponse(sService,reponse)) < 0) {
            perror("Erreur de Send");
            fermerSocket(sService);
            HandlerSIGINT(0);
        }

        printf("\t[THREAD %ld] Reponse envoyee = %s\n", (long)pthread_self(), reponse);

        if (!onContinue){
            printf("\t[THREAD %ld] Fin de connexion de la socket %d\n", (long)pthread_self(), sService);
        }
    }
}