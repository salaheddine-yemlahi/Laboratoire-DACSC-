#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <stdbool.h>
#include <signal.h>
#include <mysql/mysql.h>
#include "LibrairieServeur.h"
#include "SMOP.h"
#include "ACBP.h"

#define TAILLE_FILE_ATTENTE 20

typedef struct { 
    char nom[20];
    char prenom[20];
    int numeroPatient;
    bool nouveauPatient; 
}PATIENT;

typedef struct {
    int type;
    int taille;
}TYPE;
/*
* 1 : LOGIN
* 2 : SEARCH_CONSULTATIONS
* 3 : LOGOUT
* 4 : GET_SPECIALTIES
* 5 : GET_DOCTORS
* 6 : BOOK_CONSULTATION
*/


pthread_mutex_t mutexSocketsAcceptees;
pthread_cond_t condSocketsAcceptees;
int socketsAcceptees[TAILLE_FILE_ATTENTE];
int indiceEcriture=0, indiceLecture=0;
int serveur;

int PORT_RESERVATION;
int NB_THREADS_POOL;
int PORT_ADMIN;


void HandlerSIGINT(int s);
void TraitementConnexion(int sService);
void* FctThreadClient(void* p);
void* FctThreadAdmin(void* arg);
bool CBP_Login(const char* user, const char* password);
char reponse[8192];
int main() {
    // --- Lecture configuration ---
    FILE *file = fopen("config.conf", "r");
    if (!file) {
        perror("Erreur ouverture config.conf");
        return 1;
    }
    fscanf(file, "PORT_RESERVATION=%d\n", &PORT_RESERVATION);
    fscanf(file, "PORT_ADMIN=%d\n", &PORT_ADMIN);
    fscanf(file, "NB_THREADS_POOL=%d\n", &NB_THREADS_POOL);
    fclose(file);
    
    printf("[INFO] Configuration chargée : PORT_RESERVATION=%d, PORT_ADMIN=%d, NB_THREADS=%d\n", 
           PORT_RESERVATION, PORT_ADMIN, NB_THREADS_POOL);
    
    // --- Initialisation synchronisation ---
    pthread_mutex_init(&mutexSocketsAcceptees, NULL);
    pthread_cond_init(&condSocketsAcceptees, NULL);
    for (int i = 0; i < TAILLE_FILE_ATTENTE; i++) 
        socketsAcceptees[i] = -1;
    
    // --- Gestion signal SIGINT ---
    struct sigaction A;
    A.sa_flags = 0;
    sigemptyset(&A.sa_mask);
    A.sa_handler = HandlerSIGINT;
    sigaction(SIGINT, &A, NULL);
    
    // --- Création des deux serveurs ---
    int serveurReservation = creerServeur(PORT_RESERVATION);
    int serveurAdmin = creerServeur(PORT_ADMIN);
    
    if (serveurReservation < 0) {
        perror("Erreur creerServeur PORT_RESERVATION");
        exit(1);
    }
    if (serveurAdmin < 0) {
        perror("Erreur creerServeur PORT_ADMIN");
        close(serveurReservation);
        exit(1);
    }
    
    printf("[INFO] Serveurs créés : Réservation (port %d), Admin (port %d)\n", 
           PORT_RESERVATION, PORT_ADMIN);
    
    // --- Création pool de threads pour réservations ---
    pthread_t threads[NB_THREADS_POOL];
    for (int i = 0; i < NB_THREADS_POOL; i++)
        pthread_create(&threads[i], NULL, FctThreadClient, NULL);
    
    printf("[INFO] Pool de %d threads créé\n", NB_THREADS_POOL);
    
    // --- Boucle principale : écoute sur les deux ports ---
    while(1){
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(serveurReservation, &readfds);
        FD_SET(serveurAdmin, &readfds);
        
        int maxfd = (serveurReservation > serveurAdmin ? serveurReservation : serveurAdmin) + 1;
        
        int ret = select(maxfd, &readfds, NULL, NULL, NULL);
        if(ret < 0){
            // if(errno == EINTR) continue; // Interrompu par signal
            perror("Erreur select");
            continue;
        }
        
        // --- Gestion connexions RESERVATION ---
        if(FD_ISSET(serveurReservation, &readfds)){
            char ipClient[50];
            int sService = accepterClient(serveurReservation, ipClient);
            
            if(sService < 0){
                perror("Erreur accept serveur réservation");
            } else {
                printf("[RESERVATION] Client accepté : socket=%d, IP=%s\n", sService, ipClient);
                
                pthread_mutex_lock(&mutexSocketsAcceptees);
                socketsAcceptees[indiceEcriture] = sService;
                indiceEcriture = (indiceEcriture + 1) % TAILLE_FILE_ATTENTE;
                pthread_mutex_unlock(&mutexSocketsAcceptees);
                
                pthread_cond_signal(&condSocketsAcceptees);
            }
        }
        
        // --- Gestion connexions ADMIN (traitement direct) ---
        if (FD_ISSET(serveurAdmin, &readfds)) {
            char ipClient[50];
            int sService = accepterClient(serveurAdmin, ipClient);
            if (sService < 0) {
                perror("Erreur accept serveurAdmin");
            } else {
                printf("[DEBUG] serveurAdmin : client accepté, socket=%d, ip=%s\n", sService, ipClient);

                pthread_t th;
                int *pSock = (int*) malloc(sizeof(int));
                *pSock = sService;
                pthread_create(&th, NULL, FctThreadAdmin, pSock);
                pthread_detach(th);
            }
        }
    }
    
    // Nettoyage (jamais atteint sauf SIGINT)
    close(serveurReservation);
    close(serveurAdmin);
    
    return 0;
}


void* FctThreadClient(void* p){
    int sService;
    while(1){
        pthread_mutex_lock(&mutexSocketsAcceptees);
        while(indiceEcriture==indiceLecture) pthread_cond_wait(&condSocketsAcceptees,&mutexSocketsAcceptees);
        sService = socketsAcceptees[indiceLecture];
        socketsAcceptees[indiceLecture]=-1;
        indiceLecture=(indiceLecture+1)%TAILLE_FILE_ATTENTE;
        pthread_mutex_unlock(&mutexSocketsAcceptees);
        TraitementConnexion(sService);
    }
}

void HandlerSIGINT(int s){
    close(serveur);
    for(int i=0;i<TAILLE_FILE_ATTENTE;i++)
        if(socketsAcceptees[i]!=-1) close(socketsAcceptees[i]);
    exit(0);
}

void TraitementConnexion(int sService){
    bool onContinue=true;
    char requete[256];
    bool reponseBool;
    while(onContinue){
        TYPE type;
        int lus = recevoirMessage(sService,&type,sizeof(TYPE));
        if(lus<=0) { close(sService); return; }
        printf("message reçu %d\n", type.type);
        if(type.type==1){
            PATIENT patient;
            lus = recevoirMessage(sService,&patient,sizeof(PATIENT));
            if(lus<=0) { close(sService); return; }
            printf("message reçu %s ,%s\n", patient.nom, patient.prenom);
            if (patient.nouveauPatient) {
                sprintf(requete, "LOGIN#nouveau#%s#%s", patient.nom, patient.prenom);
            } else {
                sprintf(requete, "LOGIN#existant#%s#%s#%d", patient.nom, patient.prenom, patient.numeroPatient);
            }
            reponseBool = SMOP(requete, reponse, sService);
            envoyerMessage(sService, &reponseBool, sizeof(bool));

            char *token = strtok(reponse, "#");

            if(token != NULL && strcmp(token, "ok") == 0){
                char *idpatientstr = strtok(NULL, "#");
                if(idpatientstr != NULL) {
                    int idpatient = atoi(idpatientstr);
                    envoyerMessage(sService, &idpatient, sizeof(int));
                    ajoute(sService, idpatient);
                } else {
                    fprintf(stderr, "[ERREUR] ID patient manquant dans la réponse\n");
                }
            }      
            continue;
        }
        else if(type.type==3){
            sprintf(requete, "LOGOUT");
            reponseBool = SMOP(requete, reponse, sService);
            envoyerMessage(sService,&reponseBool,sizeof(bool));
        }
        else if(type.type==5){
            DOCTOR doc;
            int nbResultats;
            sprintf(requete, "GET_DOCTORS#");
            reponseBool = SMOP(requete, reponse, sService);
            char *ptr = strtok(reponse, "#");
            if (strcmp(ptr, "GET_DOCTORS") == 0)
            {
                char *nbStr = strtok(NULL, "#");
                int nbResultats = atoi(nbStr);
                printf("Nombre de doctors : %d\n", nbResultats);

                envoyerMessage(sService, &nbResultats, sizeof(int));

                char *token;
                while ((token = strtok(NULL, "#")) != NULL)
                {
                    int id;
                    char first_name[64];
                    char last_name[64];


                    sscanf(token, "%d:%63[^:]:%63[^:]", &id, first_name, last_name); //[^:] veut dire “tout sauf : 

                    doc.id_doctor = id;
                    strcpy(doc.first_name_doctor, first_name);
                    strcpy(doc.last_name_doctor, last_name);

                    envoyerMessage(sService, &doc, sizeof(DOCTOR));
                }
            }
            else
            {
                printf("Réponse inattendue : %s\n", ptr);
            }
        }
        else if(type.type==4){
            SPECIALITE spec;
            int nbResultats;


            sprintf(requete, "GET_SPECIALTIES#");
            reponseBool = SMOP(requete, reponse, sService);


            char *ptr = strtok(reponse, "#");
            if (strcmp(ptr, "GET_SPECIALTIES") == 0)
            {

                char *nbStr = strtok(NULL, "#");
                int nbResultats = atoi(nbStr); 
                printf("Nombre de spécialités : %d\n", nbResultats);

                envoyerMessage(sService, &nbResultats, sizeof(int));


                char *token;
                while ((token = strtok(NULL, "#")) != NULL)
                {
                    int id;
                    char nom[64];


                    sscanf(token, "%d:%63s", &id, nom);

                    spec.id_specialite = id;
                    strcpy(spec.nom_specialite, nom);

                    envoyerMessage(sService, &spec, sizeof(SPECIALITE));
                }
            }
            else
            {
                printf("Réponse inattendue : %s\n", ptr);
            }
        }
        else if(type.type==2){
        RECHERCHE recherche;
        int nbResultats;
        lus = recevoirMessage(sService,&recherche,sizeof(RECHERCHE));
        if(lus<=0) { close(sService); return; }
        sprintf(requete, "SEARCH_CONSULTATIONS#%s#%s#%s#%s", recherche.nom, recherche.nomSpecialite, recherche.dateDebut, recherche.dateFin);
        reponseBool = SMOP(requete, reponse, sService);

            char *ptr = strtok(reponse, "#");
            if (strcmp(ptr, "SEARCH_CONSULTATIONS") == 0)
            {

                char *nbStr = strtok(NULL, "#");  
                int nbResultats = atoi(nbStr);   
                printf("Nombre de spécialités : %d\n", nbResultats);


                envoyerMessage(sService, &nbResultats, sizeof(int));

                REPONSE_RECHERCHE tabReponse;
                char *token;
                while ((token = strtok(NULL, "#")) != NULL)
                {

                   sscanf(token, "%d:%19[^:]:%19[^:]:%19[^:]:%5[^:]:%39[^:]",
                    &tabReponse.idConsultation,
                    tabReponse.nomMedecin,
                    tabReponse.prenomMedecin,
                    tabReponse.dateConsultation,
                    tabReponse.hourConsultation,
                    tabReponse.nomSpecialite);

                    envoyerMessage(sService, &tabReponse, sizeof(REPONSE_RECHERCHE));
                }
            }
            else
            {
                printf("Réponse inattendue : %s\n", ptr);
            }
            continue;
        }
        else if(type.type == 6){
            BOOK_CONSULTATION bookConsultation;
            lus = recevoirMessage(sService,&bookConsultation,sizeof(BOOK_CONSULTATION));
            if(lus<=0) { close(sService); return; }

            sprintf(requete, "BOOK_CONSULTATION#%d#%d#%s", bookConsultation.id_consultation, bookConsultation.id_patient, bookConsultation.raison_consultation);
            reponseBool = SMOP(requete, reponse, sService);
            char *ptr = strtok(reponse, "#");
            bool repBool;
            if (strcmp(ptr, "BOOK_CONSULTATION") == 0){
                char *nbStr = strtok(NULL, "#"); 
                int nbResultats = atoi(nbStr);   
                envoyerMessage(sService, &nbResultats, sizeof(bool));
            }

        }
        else{
            char* buffer = (char*)malloc(type.taille);
            lus = recevoirMessage(sService,buffer,type.taille);
            free(buffer);
        }
    }
}

void* FctThreadAdmin(void* arg) {
    int sService = *((int*)arg); // récupérer le socket passé en paramètre
    bool repBool;

    repBool = ACBP(reponse, sService); // utiliser le socket passé

    char *ptr = strtok(reponse, "#");
    if (strcmp(ptr, "LIST_CLIENTS") == 0)
    {
        char *nbStr = strtok(NULL, "#");
        int nbResultats = atoi(nbStr); 
        printf("Nombre de clients : %d\n", nbResultats);

        envoyerMessage(sService, &nbResultats, sizeof(int));

        CLIENT tabReponse;
        char *token;
        while ((token = strtok(NULL, "#")) != NULL)
        {
            sscanf(token, "%19[^:]:%19[^:]:%19[^:]:%d",
                tabReponse.adressIP,
                tabReponse.nom,
                tabReponse.prenom,
                &tabReponse.numeroPatient);

            envoyerMessage(sService, &tabReponse, sizeof(CLIENT));
        }
    }
    else
    {
        printf("Réponse inattendue : %s\n", ptr);
    }
    return NULL;
}