#include "LibrairieServeur.h"
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
* 1 : IDENTIFIER
* 2 : RECHERCHER
*/

typedef struct 
{
    char nom[50];
    char nomSpecialite[40];
    char dateDebut[10];
    char dateFin[10];
} RECHERCHE;

typedef struct{
    int idConsultation;
    char nomSpecialite[40];
    char nomMedecin[20];
    char prenomMedecin[20];
    char dateConsultation[10];
    char hourConsultation[5];
} REPONSE_RECHERCHE;

// Variables globales
pthread_mutex_t mutexSocketsAcceptees;
pthread_cond_t condSocketsAcceptees;
int socketsAcceptees[TAILLE_FILE_ATTENTE];
int indiceEcriture=0, indiceLecture=0;
int serveur;

int PORT_RESERVATION = 1234; // Exemple
int NB_THREADS_POOL = 5;     // Exemple
MYSQL* connexion;

// Prototypes
void HandlerSIGINT(int s);
void TraitementConnexion(int sService);
void* FctThreadClient(void* p);
bool CBP_Login(const char* user, const char* password);

int main(){
    // Init mutex + cond
    pthread_mutex_init(&mutexSocketsAcceptees,NULL);
    pthread_cond_init(&condSocketsAcceptees,NULL);
    for(int i=0;i<TAILLE_FILE_ATTENTE;i++) socketsAcceptees[i]=-1;

    // Connexion MySQL (exemple)
    connexion = mysql_init(NULL);
    if (!mysql_real_connect(connexion,"localhost","Student","PassStudent1_","PourStudent",0,NULL,0)){
        fprintf(stderr,"Erreur BD: %s\n", mysql_error(connexion));
        exit(1);
    }
    printf("Connexion BD OK\n");

    // Signaux
    struct sigaction A;
    A.sa_flags = 0;
    sigemptyset(&A.sa_mask);
    A.sa_handler = HandlerSIGINT;
    sigaction(SIGINT,&A,NULL);

    // Serveur
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

// Login fictif
bool CBP_Login(const char* user, const char* password){
    if(strcmp(user,"wagner")==0 && strcmp(password,"abc123")==0) return true;
    if(strcmp(user,"charlet")==0 && strcmp(password,"xyz456")==0) return true;
    return false;
}

// Handler SIGINT
void HandlerSIGINT(int s){
    close(serveur);
    for(int i=0;i<TAILLE_FILE_ATTENTE;i++)
        if(socketsAcceptees[i]!=-1) close(socketsAcceptees[i]);
    exit(0);
}

// Traitement connexion
void TraitementConnexion(int sService){
    bool onContinue=true;
    while(onContinue){
        TYPE type;
        char requete[256];
        int lus = recevoirMessageBinaire(sService,&type,sizeof(TYPE));
        if(lus<=0) { close(sService); return; }
        printf("message reçu %d\n", type.type);

        if(type.type==1){
            PATIENT patient;
            lus = recevoirMessageBinaire(sService,&patient,sizeof(PATIENT));
            if(lus<=0) { close(sService); return; }
            printf("message reçu %s ,%s\n", patient.nom, patient.prenom);
            bool reponseBool = CBP_Login(patient.nom, patient.prenom);
            envoyerMessageBinaire(sService,&reponseBool,sizeof(bool));
             continue;
        }
        else if(type.type==2){
        RECHERCHE recherche;
        lus = recevoirMessageBinaire(sService,&recherche,sizeof(RECHERCHE));
        if(lus<=0) { close(sService); return; }
        printf("DEBUG - Octets reçus pour RECHERCHE: %d/%lu\n", lus, sizeof(RECHERCHE));
printf("DEBUG - Contenu reçu:\n");
printf("  nom: '%s'\n", recherche.nom);
printf("  specialite: '%s'\n", recherche.nomSpecialite);
printf("  dateDebut: '%s'\n", recherche.dateDebut);
printf("  dateFin: '%s'\n", recherche.dateFin);
        printf("message reçu %s ,%s, %s, %s\n", recherche.nom, recherche.nomSpecialite, recherche.dateDebut, recherche.dateFin);
            snprintf(requete, sizeof(requete),
         "SELECT c.id, d.first_name, d.last_name, c.date, c.hour "
         "FROM consultations c "
         "INNER JOIN doctors d ON c.doctor_id = d.id "
         "WHERE CONCAT(d.last_name, ' ', d.first_name) LIKE '%%%s%%' "
         "AND c.date BETWEEN '%s' AND '%s';",
         recherche.nom, recherche.dateDebut, recherche.dateFin);
            if (mysql_query(connexion,requete) != 0)
            {
                fprintf(stderr, "Erreur de mysql_query: %s\n",mysql_error(connexion));
                exit(1);
            }
            printf("Requete SELECT réussie.\n");
            REPONSE_RECHERCHE    reponse;
             MYSQL_RES *ResultSet;
            if ((ResultSet = mysql_store_result(connexion)) == NULL)
            {
                fprintf(stderr, "Erreur de mysql_store_result: %s\n",mysql_error(connexion));
                exit(1);
            }
            MYSQL_ROW ligne;
            int nbResultats = mysql_num_rows(ResultSet);
            envoyerMessageBinaire(sService,&nbResultats,sizeof(int));

            int nbChamps = mysql_num_fields(ResultSet);
            while ((ligne = mysql_fetch_row(ResultSet)) != NULL)
            {
                reponse.idConsultation = atoi(ligne[0]);
                strcpy(reponse.prenomMedecin, ligne[1]);
                strcpy(reponse.nomMedecin, ligne[2]);
                strcpy(reponse.dateConsultation, ligne[3]);
                strcpy(reponse.hourConsultation, ligne[4]);
                envoyerMessageBinaire(sService,&reponse,sizeof(REPONSE_RECHERCHE));
            }
            continue;
        }
        else{
            char* buffer = malloc(type.taille);
            lus = recevoirMessageBinaire(sService,buffer,type.taille);
            free(buffer);
        }
    }
}
