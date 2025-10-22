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


void HandlerSIGINT(int s);
void TraitementConnexion(int sService);
void* FctThreadClient(void* p);
bool CBP_Login(const char* user, const char* password);

int main(){

    FILE *file = fopen("config.conf", "r");
    if (!file) {
        perror("Erreur ouverture config.txt");
        return 1;
    }

    fscanf(file, "PORT_RESERVATION=%d\n", &PORT_RESERVATION);
    fscanf(file, "NB_THREADS_POOL=%d\n", &NB_THREADS_POOL);

    fclose(file);


    pthread_mutex_init(&mutexSocketsAcceptees,NULL);
    pthread_cond_init(&condSocketsAcceptees,NULL);
    for(int i=0;i<TAILLE_FILE_ATTENTE;i++) socketsAcceptees[i]=-1;


    struct sigaction A;
    A.sa_flags = 0;
    sigemptyset(&A.sa_mask);
    A.sa_handler = HandlerSIGINT;
    sigaction(SIGINT,&A,NULL);


    serveur = creerServeur(PORT_RESERVATION);
    if (serveur < 0) { perror("Erreur creerServeur"); exit(1); }


    pthread_t threads[NB_THREADS_POOL];
    for(int i=0;i<NB_THREADS_POOL;i++)
        pthread_create(&threads[i],NULL,FctThreadClient,NULL);


    while(1){
        char ipClient[50];
        int sService = accepterClient(serveur, ipClient);
        if (sService < 0) continue;

        pthread_mutex_lock(&mutexSocketsAcceptees);
        socketsAcceptees[indiceEcriture]=sService;
        indiceEcriture=(indiceEcriture+1)%TAILLE_FILE_ATTENTE;
        pthread_mutex_unlock(&mutexSocketsAcceptees);
        pthread_cond_signal(&condSocketsAcceptees);
    }
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
    char reponse[8192];
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
            envoyerMessage(sService,&reponseBool,sizeof(bool));
            
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
