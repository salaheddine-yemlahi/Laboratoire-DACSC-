#include "LibrairieServeur.h"
#include "SMOP.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <stdbool.h>
#include <signal.h>
#include <string.h>
#include <mysql.h>

#define TAILLE_FILE_ATTENTE 20

typedef struct { 
    char nom[20];
    char prenom[20];
    int numeroPatient;
    bool nouveauPatient; 
} PATIENT;

typedef struct {
    int type;
    int taille;
} TYPE;

/*
* 1 : LOGIN
* 2 : SEARCH_CONSULTATIONS
* 3 : LOGOUT
* 4 : GET_SPECIALTIES
* 5 : GET_DOCTORS
*/


pthread_mutex_t mutexSocketsAcceptees;
pthread_cond_t condSocketsAcceptees;
int socketsAcceptees[TAILLE_FILE_ATTENTE];
int indiceEcriture=0, indiceLecture=0;
int serveur;

int PORT_RESERVATION;
int NB_THREADS_POOL;
MYSQL* connexion;

// Prototypes
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


    connexion = mysql_init(NULL);
    if (!mysql_real_connect(connexion,"localhost","Student","PassStudent1_","PourStudent",0,NULL,0)){
        fprintf(stderr,"Erreur BD: %s\n", mysql_error(connexion));
        exit(1);
    }
    printf("Connexion BD OK\n");


    struct sigaction A;
    A.sa_flags = 0;
    sigemptyset(&A.sa_mask);
    A.sa_handler = HandlerSIGINT;
    sigaction(SIGINT,&A,NULL);


    serveur = creerServeur(PORT_RESERVATION);
    if (serveur < 0) { perror("Erreur creerServeur"); exit(1); }

    // Pool threads
    pthread_t threads[NB_THREADS_POOL];
    for(int i=0;i<NB_THREADS_POOL;i++)
        pthread_create(&threads[i],NULL,FctThreadClient,NULL);

    // Boucle accept
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

// Thread client
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

// Traitement connexion
void TraitementConnexion(int sService){
    bool onContinue=true;
    char requete[200];
    bool reponseBool;
    while(onContinue){
        TYPE type;
        char requete[256];
        char reponse[8192];
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
            if(strcmp(reponse, "LOGIN#ok")==0){
                envoyerMessage(sService,&reponseBool,sizeof(bool));
            }
            
            continue;
        }
        else if(type.type==3){
            sprintf(requete, "LOGOUT");
            reponseBool = SMOP(requete, reponse, sService);
            if(strcmp(reponse, "LOGOUT#ok")==0){
                envoyerMessage(sService,&reponseBool,sizeof(bool));
            }
        }
        else if(type.type==5){
            DOCTOR doc;
            int nbResultats;
            sprintf(requete, "GET_DOCTORS#");
            reponseBool = SMOP(requete, reponse, sService);
            char *ptr = strtok(reponse, "#");
            if (strcmp(ptr, "GET_DOCTORS") == 0)
            {
                // Récupère le nombre de spécialités
                char *nbStr = strtok(NULL, "#");  // récupère le nombre juste après #
                int nbResultats = atoi(nbStr);    // convertit en entier
                printf("Nombre de doctors : %d\n", nbResultats);

                //  Envoie d’abord le nombre total au client
                envoyerMessage(sService, &nbResultats, sizeof(int));

                printf("Liste des doctors reçue :\n");

                // Parcourt et envoie chaque spécialité
                char *token;
                while ((token = strtok(NULL, "#")) != NULL)
                {
                    int id;
                    char first_name[64];
                    char last_name[64];

                    // Découpe "id:nom"
                    sscanf(token, "%d:%63[^:]:%63[^:]", &id, first_name, last_name);

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

            // Prépare et envoie la requête
            sprintf(requete, "GET_SPECIALTIES#");
            reponseBool = SMOP(requete, reponse, sService);

            // Vérifie la réponse reçue
            char *ptr = strtok(reponse, "#");
            if (strcmp(ptr, "GET_SPECIALTIES") == 0)
            {
                // Récupère le nombre de spécialités
                char *nbStr = strtok(NULL, "#");  // récupère le nombre juste après #
                int nbResultats = atoi(nbStr);    // convertit en entier
                printf("Nombre de spécialités : %d\n", nbResultats);

                //  Envoie d’abord le nombre total au client
                envoyerMessage(sService, &nbResultats, sizeof(int));

                printf("Liste des spécialités reçue :\n");

                // Parcourt et envoie chaque spécialité
                char *token;
                while ((token = strtok(NULL, "#")) != NULL)
                {
                    int id;
                    char nom[64];

                    // Découpe "id:nom"
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
        printf("DEBUG - Octets reçus pour RECHERCHE: %d/%lu\n", lus, sizeof(RECHERCHE));
        printf("DEBUG - Contenu reçu:\n");
        printf("  nom: '%s'\n", recherche.nom);
        printf("  specialite: '%s'\n", recherche.nomSpecialite);
        printf("  dateDebut: '%s'\n", recherche.dateDebut);
        printf("  dateFin: '%s'\n", recherche.dateFin);
        printf("message reçu %s ,%s, %s, %s\n", recherche.nom, recherche.nomSpecialite, recherche.dateDebut, recherche.dateFin);

        sprintf(requete, "SEARCH_CONSULTATIONS#%s#%s#%s#%s", recherche.nom, recherche.nomSpecialite, recherche.dateDebut, recherche.dateFin);
        reponseBool = SMOP(requete, reponse, sService);
        printf("%s\n", reponse);

        // Vérifie la réponse reçue
            char *ptr = strtok(reponse, "#");
            if (strcmp(ptr, "SEARCH_CONSULTATIONS") == 0)
            {
                // Récupère le nombre de spécialités
                char *nbStr = strtok(NULL, "#");  // récupère le nombre juste après #
                int nbResultats = atoi(nbStr);    // convertit en entier
                printf("Nombre de spécialités : %d\n", nbResultats);

                //  Envoie d’abord le nombre total au client
                envoyerMessage(sService, &nbResultats, sizeof(int));

                printf("Liste des spécialités reçue :\n");

                // Parcourt et envoie chaque spécialité
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
                    
                    printf("DEBUG - idConsultation = %d\n", tabReponse.idConsultation);
                    printf("DEBUG - nomMedecin = '%s'\n", tabReponse.nomMedecin);
                    printf("DEBUG - prenomMedecin = '%s'\n", tabReponse.prenomMedecin);
                    printf("DEBUG - dateConsultation = '%s'\n", tabReponse.dateConsultation);
                    printf("DEBUG - hourConsultation = '%s'\n", tabReponse.hourConsultation);
                    printf("DEBUG - nomSpecialite = '%s'\n", tabReponse.nomSpecialite);

                    envoyerMessage(sService, &tabReponse, sizeof(REPONSE_RECHERCHE));
                }
            }
            else
            {
                printf("Réponse inattendue : %s\n", ptr);
            }
            continue;
        }
        else{
            char* buffer = (char*)malloc(type.taille);
            lus = recevoirMessage(sService,buffer,type.taille);
            free(buffer);
        }
    }
}
