#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cstdint>
#include <cstddef>
#include <unistd.h>
#include <pthread.h>
#include <stdbool.h>
#include <signal.h>
#include <mysql/mysql.h>
#include "SMOP.h"


SOCKET_ID clients[NB_MAX_CLIENTS]; 
int nbClients = 0;
int  estPresent(int socket); 
void retire(int socket); 
pthread_mutex_t mutexClients = PTHREAD_MUTEX_INITIALIZER; 

bool SMOP(char* requete, char* reponse, int socket) 
{ 
    pthread_mutex_init(&mutexClients,NULL);
    // ***** Récupération nom de la requête ***************** 
    char *ptr = strtok(requete, "#");
    char commande[10];
    char user[50], password[50], numeroPatientStr[20];
    int numeroPatient;
    // ***** LOGIN ****************************************** 
    if (strcmp(ptr, "LOGIN") == 0)  
    {
        
        strcpy(commande, strtok(NULL, "#"));
        if (strcmp(commande, "nouveau") == 0)
        {
            // Nouveau patient : récupère user et password
            strcpy(user, strtok(NULL, "#"));
            strcpy(password, strtok(NULL, "#"));
            if (estPresent(socket) >= 0)  // client déjà loggé
            {
                sprintf(reponse, "LOGIN#ko#Client déjà loggé !");
                return false;
            }
            int numeroPatient = SMOP_Register(user, password);
            if (numeroPatient > 0)
            {
                sprintf(reponse, "ok#%d", numeroPatient);
                return true;
            }
            else
            {
                sprintf(reponse, "ko");
                return false;
            }
        }
        else if (strcmp(commande, "existant") == 0)
        {
            // Patient existant : récupère user, password et numéro
            strcpy(user, strtok(NULL, "#"));             // nom
            strcpy(password, strtok(NULL, "#"));         // prenom
            char numeroPatientStr[20];
            strcpy(numeroPatientStr, strtok(NULL, "#")); // numéro
            numeroPatient = atoi(numeroPatientStr);
            // Vérifie login
            if (SMOP_Login(numeroPatient, user, password)) // login réussi
            {
                sprintf(reponse, "ok#%d", numeroPatient);
                return true;
            }
            else
            {
                sprintf(reponse, "ko");
                return false;
            }
        }
        else
        {
            // commande invalide
            sprintf(reponse, "LOGIN#ko#Commande inconnue !");
            return false;
        }
    }
    else if(strcmp(ptr, "LOGOUT") == 0)
    {
        retire(socket);
        sprintf(reponse, "LOGOUT#ok");
        return true;
    }
    else if(strcmp(ptr, "GET_SPECIALTIES") == 0)
    {
        int nb;
        SPECIALITE* tab = SMOP_SPECIALITES(&nb);
        sprintf(reponse, "GET_SPECIALTIES#%d", nb);
        for (int i = 0; i < nb; i++) {
            char ligne[64];
            sprintf(ligne, "#%d:%s#", tab[i].id_specialite, tab[i].nom_specialite);
            strcat(reponse, ligne);
        }
        free(tab);
        return true;
    }
    else if(strcmp(ptr, "GET_DOCTORS")==0)
    {
        int nb;
        DOCTOR* tab = SMOP_DOCTORS(&nb);
        sprintf(reponse, "GET_DOCTORS#%d", nb);
        for (int i = 0; i < nb; i++) {
            char ligne[64];
            sprintf(ligne, "#%d:%s:%s#", tab[i].id_doctor, tab[i].first_name_doctor, tab[i].last_name_doctor);
            strcat(reponse, ligne);
        }
        free(tab);
        return true;
    }
    else if(strcmp(ptr, "SEARCH_CONSULTATIONS")==0){
        int nb, id_consultation;
        char name[20], specialtie[40], datedebut[10], datefin[10];
        strcpy(name, strtok(NULL, "#"));            
        strcpy(specialtie, strtok(NULL, "#"));         
        strcpy(datedebut, strtok(NULL, "#"));         
        strcpy(datefin, strtok(NULL, "#"));    
       REPONSE_RECHERCHE* tab = SMOP_Consultation(&nb, id_consultation, name, specialtie, datedebut, datefin);

        // Buffer assez grand pour toutes les consultations
        memset(reponse, 0, sizeof(reponse));

        // On commence la chaîne
        snprintf(reponse, 8192, "SEARCH_CONSULTATIONS#%d", nb);

        for (int i = 0; i < nb; i++) {

            // Formate l'heure pour remplacer ':' par '-'
            char hourFormatted[10];
            strncpy(hourFormatted, tab[i].hourConsultation, sizeof(hourFormatted) - 1);
            hourFormatted[sizeof(hourFormatted) - 1] = '\0';
            for (int j = 0; hourFormatted[j]; j++)
                if (hourFormatted[j] == ':')
                    hourFormatted[j] = '-';

            // Crée la ligne complète
            char ligne[256];
            snprintf(ligne, sizeof(ligne),
                    "#%d:%s:%s:%s:%s:%s",
                    tab[i].idConsultation,
                    tab[i].nomMedecin,
                    tab[i].prenomMedecin,
                    tab[i].dateConsultation,
                    hourFormatted,
                    tab[i].nomSpecialite);

            // Concatène en vérifiant la taille
            strncat(reponse, ligne, sizeof(reponse) - strlen(reponse) - 1);
        }

        printf("DEBUG - Chaîne complète envoyée : %s\n", reponse);
        free(tab);
        return true;
    }
    else if(strcmp(ptr, "BOOK_CONSULTATION")==0){
        bool resultat;
        int id_patient, id_consultation;
        char raison[50];
        id_consultation = atoi(strtok(NULL, "#"));
        id_patient = atoi(strtok(NULL, "#"));
        strcpy(raison, strtok(NULL, "#"));
        resultat = SMOP_Book_Consultation(id_consultation, id_patient, raison);
        snprintf(reponse, 8192, "BOOK_CONSULTATION#%d", resultat);
        printf("DEBUG - Chaîne complète envoyée : %s\n", reponse);
        return true;
    }
    return true; 
}




bool SMOP_Login(int id, const char* user, const char* password)
{
    MYSQL* connexion = mysql_init(NULL);
    if (!connexion) return false;

    if (!mysql_real_connect(connexion, "localhost", "Student", "PassStudent1_", "PourStudent", 0, NULL, 0)) {
        fprintf(stderr, "Erreur BD: %s\n", mysql_error(connexion));
        mysql_close(connexion);
        return false;
    }

    char requete[512];
    snprintf(requete, sizeof(requete),
             "SELECT p.id FROM patients p "
             "WHERE p.id = %d AND p.last_name='%s' AND p.first_name='%s';",
             id, user, password);

    if (mysql_query(connexion, requete) != 0) {
        fprintf(stderr, "Erreur mysql_query: %s\n", mysql_error(connexion));
        mysql_close(connexion);
        return false;
    }

    MYSQL_RES* res = mysql_store_result(connexion);
    if (!res) {
        fprintf(stderr, "Erreur mysql_store_result: %s\n", mysql_error(connexion));
        mysql_close(connexion);
        return false;
    }

    int ok = (mysql_num_rows(res) == 1);

    mysql_free_result(res);
    mysql_close(connexion);
    return ok;
}

SPECIALITE* SMOP_SPECIALITES(int* nbResultats)
{
    MYSQL* connexion = mysql_init(NULL);
    if (!connexion) return NULL;

    if (!mysql_real_connect(connexion, "localhost", "Student", "PassStudent1_", "PourStudent", 0, NULL, 0)) {
        fprintf(stderr, "Erreur BD: %s\n", mysql_error(connexion));
        mysql_close(connexion);
        return NULL;
    }

    char requete[256];
    snprintf(requete, sizeof(requete), "SELECT id, name FROM specialties");

    if (mysql_query(connexion, requete) != 0)
    {
        fprintf(stderr, "Erreur de mysql_query: %s\n", mysql_error(connexion));
        mysql_close(connexion);
        return NULL;
    }

    MYSQL_RES *ResultSet = mysql_store_result(connexion);
    if (!ResultSet)
    {
        fprintf(stderr, "Erreur de mysql_store_result: %s\n", mysql_error(connexion));
        mysql_close(connexion);
        return NULL;
    }

    *nbResultats = mysql_num_rows(ResultSet);
    MYSQL_ROW ligne;

    SPECIALITE *tabSpecialites = (SPECIALITE *)malloc((*nbResultats) * sizeof(SPECIALITE));
    if (!tabSpecialites)
    {
        fprintf(stderr, "Erreur d’allocation mémoire\n");
        mysql_free_result(ResultSet);
        mysql_close(connexion);
        return NULL;
    }

    int i = 0;
    while ((ligne = mysql_fetch_row(ResultSet)) != NULL && i < *nbResultats)
    {
        tabSpecialites[i].id_specialite = atoi(ligne[0]);
        strncpy(tabSpecialites[i].nom_specialite, ligne[1], 19);
        tabSpecialites[i].nom_specialite[19] = '\0';
        i++;
    }

    mysql_free_result(ResultSet);
    mysql_close(connexion);

    return tabSpecialites;
}

DOCTOR* SMOP_DOCTORS(int* nbResultats)
{
    MYSQL* connexion = mysql_init(NULL);
    if (!connexion) return NULL;

    if (!mysql_real_connect(connexion, "localhost", "Student", "PassStudent1_", "PourStudent", 0, NULL, 0)) {
        fprintf(stderr, "Erreur BD: %s\n", mysql_error(connexion));
        mysql_close(connexion);
        return NULL;
    }

    char requete[256];
    snprintf(requete, sizeof(requete), "SELECT id, last_name, first_name FROM doctors");

    if (mysql_query(connexion, requete) != 0)
    {
        fprintf(stderr, "Erreur de mysql_query: %s\n", mysql_error(connexion));
        mysql_close(connexion);
        return NULL;
    }

    MYSQL_RES *ResultSet = mysql_store_result(connexion);
    if (!ResultSet)
    {
        fprintf(stderr, "Erreur de mysql_store_result: %s\n", mysql_error(connexion));
        mysql_close(connexion);
        return NULL;
    }

    *nbResultats = mysql_num_rows(ResultSet);
    MYSQL_ROW ligne;

    DOCTOR *tabDoctors = (DOCTOR *)malloc((*nbResultats) * sizeof(DOCTOR));
    if (!tabDoctors)
    {
        fprintf(stderr, "Erreur d’allocation mémoire\n");
        mysql_free_result(ResultSet);
        mysql_close(connexion);
        return NULL;
    }

    int i = 0;
    while ((ligne = mysql_fetch_row(ResultSet)) != NULL && i < *nbResultats)
    {
        tabDoctors[i].id_doctor = atoi(ligne[0]);
        strncpy(tabDoctors[i].first_name_doctor, ligne[1], 19);
        tabDoctors[i].first_name_doctor[19] = '\0';
        strncpy(tabDoctors[i].last_name_doctor, ligne[2], 19);
        tabDoctors[i].last_name_doctor[19] = '\0';
        i++;
    }

    mysql_free_result(ResultSet);
    mysql_close(connexion);

    return tabDoctors;

}

REPONSE_RECHERCHE* SMOP_Consultation(int* nbResultats, int id, const char* name, const char* specialtie, const char* datedebut, const char* datefin)
{
    MYSQL* connexion = mysql_init(NULL);
    if (!connexion) return NULL;

    if (!mysql_real_connect(connexion, "localhost", "Student", "PassStudent1_", "PourStudent", 0, NULL, 0)) {
        fprintf(stderr, "Erreur BD: %s\n", mysql_error(connexion));
        mysql_close(connexion);
        return NULL;
    }
    char requete[1024];
    if(strcmp(name, "--- TOUS ---")==0 && strcmp(specialtie, "--- TOUTES ---")==0){
        snprintf(requete, sizeof(requete),
        "SELECT c.id, d.first_name, d.last_name, c.date, c.hour, s.name "
        "FROM consultations c "
        "INNER JOIN doctors d ON c.doctor_id = d.id "
        "INNER JOIN specialties s ON s.id = d.specialty_id "
        "WHERE c.date BETWEEN '%s' AND '%s' "
        "AND c.patient_id IS NULL;",
        datedebut, datefin);
    }
    else if(strcmp(specialtie, "--- TOUTES ---")==0){
        snprintf(requete, sizeof(requete),
        "SELECT c.id, d.first_name, d.last_name, c.date, c.hour, s.name "
        "FROM consultations c "
        "INNER JOIN doctors d ON c.doctor_id = d.id "
        "INNER JOIN specialties s ON s.id = d.specialty_id "
        "WHERE CONCAT(d.last_name, ' ', d.first_name) LIKE '%%%s%%' "
        "AND c.date BETWEEN '%s' AND '%s' "
        "AND c.patient_id IS NULL;",
        name, datedebut, datefin);
    }
    else if (strcmp(name, "--- TOUS ---")==0){
        snprintf(requete, sizeof(requete),
        "SELECT c.id, d.first_name, d.last_name, c.date, c.hour, s.name "
        "FROM consultations c "
        "INNER JOIN doctors d ON c.doctor_id = d.id "
        "INNER JOIN specialties s ON s.id = d.specialty_id "
        "WHERE s.name LIKE '%%%s%%' "
        "AND c.date BETWEEN '%s' AND '%s' "
        "AND c.patient_id IS NULL;",
        specialtie, datedebut, datefin);
    }
    else{
        snprintf(requete, sizeof(requete),
        "SELECT c.id, d.first_name, d.last_name, c.date, c.hour, s.name "
        "FROM consultations c "
        "INNER JOIN doctors d ON c.doctor_id = d.id "
        "INNER JOIN specialties s ON s.id = d.specialty_id "
        "WHERE CONCAT(d.last_name, ' ', d.first_name) LIKE '%%%s%%' "
        "AND s.name LIKE '%%%s%%' "
        "AND c.date BETWEEN '%s' AND '%s' "
        "AND c.patient_id IS NULL;",
        name, specialtie, datedebut, datefin);
    }

    if (mysql_query(connexion, requete) != 0) {
        fprintf(stderr, "Erreur de mysql_query: %s\n", mysql_error(connexion));
        mysql_close(connexion);
        return NULL;
    }

    MYSQL_RES* ResultSet = mysql_store_result(connexion);
    if (!ResultSet) {
        fprintf(stderr, "Erreur de mysql_store_result: %s\n", mysql_error(connexion));
        mysql_close(connexion);
        return NULL;
    }

    *nbResultats = mysql_num_rows(ResultSet);
    MYSQL_ROW ligne;

    REPONSE_RECHERCHE* tabConsultation = (REPONSE_RECHERCHE*)malloc((*nbResultats) * sizeof(REPONSE_RECHERCHE));
    if (!tabConsultation) {
        fprintf(stderr, "Erreur d’allocation mémoire\n");
        mysql_free_result(ResultSet);
        mysql_close(connexion);
        return NULL;
    }

    int i = 0;
    while ((ligne = mysql_fetch_row(ResultSet)) != NULL && i < *nbResultats) {
        tabConsultation[i].idConsultation = atoi(ligne[0]);
        strcpy(tabConsultation[i].prenomMedecin, ligne[1]);
        strcpy(tabConsultation[i].nomMedecin, ligne[2]);
        strncpy(tabConsultation[i].dateConsultation, ligne[3], 10);
        tabConsultation[i].dateConsultation[10] = '\0';
        strcpy(tabConsultation[i].hourConsultation, ligne[4]);
        strcpy(tabConsultation[i].nomSpecialite, ligne[5]);
        i++;
    }

    mysql_free_result(ResultSet);
    mysql_close(connexion);

    *nbResultats = i;
    return tabConsultation;
}


bool SMOP_Book_Consultation(int id_consultation, int id_patient, char* raison){
    MYSQL* connexion = mysql_init(NULL);
    if (!connexion) return false;

    if (!mysql_real_connect(connexion, "localhost", "Student", "PassStudent1_", "PourStudent", 0, NULL, 0)) {
        fprintf(stderr, "Erreur BD: %s\n", mysql_error(connexion));
        mysql_close(connexion);
        return false;
    }

    char requete[1024];
    snprintf(requete, sizeof(requete), "Update consultations set patient_id = '%d', reason = '%s' where id = '%d';",
        id_patient, raison, id_consultation);

    if (mysql_query(connexion, requete) != 0) {
        fprintf(stderr, "Erreur de mysql_query: %s\n", mysql_error(connexion));
        mysql_close(connexion);
        return false;
    }
    mysql_close(connexion);
    return true;
}


int SMOP_Register(const char* user, const char* password) 
{
    MYSQL* connexion = mysql_init(NULL);
    if (connexion == NULL) {
        fprintf(stderr, "Erreur d'initialisation MySQL\n");
        return -1;
    }

    if (!mysql_real_connect(connexion, "localhost", "Student", "PassStudent1_", "PourStudent", 0, NULL, 0)) {
        fprintf(stderr, "Erreur BD: %s\n", mysql_error(connexion));
        return -1;
    }
    printf("Connexion BD OK\n");

    char requete[512];
    snprintf(requete, sizeof(requete),
             "INSERT INTO patients (last_name, first_name) VALUES ('%s', '%s');",
             user, password);

    if (mysql_query(connexion, requete) != 0) {
        fprintf(stderr, "Erreur de mysql_query: %s\n", mysql_error(connexion));
        mysql_close(connexion);
        return -1;
    }

    // Récupérer l'ID généré automatiquement
    int idPatient = mysql_insert_id(connexion);

    mysql_close(connexion);

    return idPatient;
}


int estPresent(int socket) 
{ 
    int indice = -1; 
    pthread_mutex_lock(&mutexClients); 
    for(int i=0 ; i<nbClients ; i++) 
    if (clients[i].socket == socket) { indice  = i; break; } 
    pthread_mutex_unlock(&mutexClients); 
    return indice; 
} 


void ajoute(int socket, int id) 
{ 
    pthread_mutex_lock(&mutexClients); 
    clients[nbClients].socket = socket;
    clients[nbClients].id = id;
    printf("socket ajouté : %d, id: %d\n", clients[nbClients].socket, clients[nbClients].id);
    nbClients++;
    pthread_mutex_unlock(&mutexClients);
} 


void retire(int socket) 
{
    int pos = estPresent(socket);
    if (pos == -1) return;
    pthread_mutex_lock(&mutexClients);
    for (int i=pos ; i<=nbClients-2 ; i++){
        clients[i].socket = clients[i+1].socket;
        clients[i].id = clients[i+1].id;
        nbClients--;
    }
    printf("socket retiré : %d, id: %d\n");
    pthread_mutex_unlock(&mutexClients);
} 


void SMOP_Close() 
{ 
    pthread_mutex_lock(&mutexClients); 
    for (int i=0 ; i<nbClients ; i++){
         close(clients[i].socket); 
    }
    pthread_mutex_unlock(&mutexClients); 
}

