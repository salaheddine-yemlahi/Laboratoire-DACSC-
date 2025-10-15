#include "SMOP.h" 
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <stdbool.h>
#include <signal.h>
#include <string.h>
#include <mysql.h>



//***** Etat du protocole : liste des clients loggés **************** 
int clients[NB_MAX_CLIENTS]; 
int nbClients = 0;
int  estPresent(int socket); 
void ajoute(int socket); 
void retire(int socket); 
pthread_mutex_t mutexClients = PTHREAD_MUTEX_INITIALIZER; 
//***** Parsing de la requete et creation de la reponse ************* 

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
                sprintf(reponse, "nouveau#ok#%d", numeroPatient);
                ajoute(socket);
                return true;
            }
            else
            {
                sprintf(reponse, "LOGIN#ko#Mauvais identifiants !");
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
                sprintf(reponse, "LOGIN#ok");
                // ajoute(socket); // si nécessaire
                return true;
            }
            else
            {
                sprintf(reponse, "LOGIN#ko#Mauvais identifiants !");
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

    // char userEsc[128], passwordEsc[128];
    // mysql_real_escape_string(connexion, userEsc, user, strlen(user));
    // mysql_real_escape_string(connexion, passwordEsc, password, strlen(password));

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
        tabDoctors[i].id_doctor = atoi(ligne[0]); // id
        strncpy(tabDoctors[i].first_name_doctor, ligne[1], 19);
        tabDoctors[i].first_name_doctor[19] = '\0'; // sécurité
        strncpy(tabDoctors[i].last_name_doctor, ligne[2], 19);
        tabDoctors[i].last_name_doctor[19] = '\0';  // sécurité
        i++;
    }

    mysql_free_result(ResultSet);
    mysql_close(connexion);

    return tabDoctors;

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

    // INSERT patient
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

    return idPatient; // retourne le numéro du patient créé
}


int estPresent(int socket) 
{ 
    int indice = -1; 
    pthread_mutex_lock(&mutexClients); 
    for(int i=0 ; i<nbClients ; i++) 
    if (clients[i] == socket) { indice  = i; break; } 
    pthread_mutex_unlock(&mutexClients); 
    return indice; 
} 


void ajoute(int socket) 
{ 
    pthread_mutex_lock(&mutexClients); 
    clients[nbClients] = socket; 
    nbClients++;
    pthread_mutex_unlock(&mutexClients); 
} 


void retire(int socket) 
{ 
    int pos = estPresent(socket); 
    if (pos == -1) return; 
    pthread_mutex_lock(&mutexClients); 
    for (int i=pos ; i<=nbClients-2 ; i++) 
    clients[i] = clients[i+1]; 
    nbClients--; 
    pthread_mutex_unlock(&mutexClients); 
} 


void SMOP_Close() 
{ 
    pthread_mutex_lock(&mutexClients); 
    for (int i=0 ; i<nbClients ; i++) 
    close(clients[i]); 
    pthread_mutex_unlock(&mutexClients); 
}