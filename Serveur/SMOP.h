#ifndef SMOP_H 
#define SMOP_H 

#define NB_MAX_CLIENTS 100 
typedef struct{
    int id_specialite;
    char nom_specialite[20];
}SPECIALITE;
typedef struct{
    int id_doctor;
    char first_name_doctor[20];
    char last_name_doctor[20];
}DOCTOR;
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
    char dateConsultation[20];
    char hourConsultation[5];
} REPONSE_RECHERCHE;

bool SMOP(char* requete, char* reponse, int socket); 
bool SMOP_Login(int id,const char* user, const char* password);
int SMOP_Register(const char* user, const char* password);
//int  SMOP_Operation(char op, int a, int b); 
void SMOP_Close(); 
SPECIALITE* SMOP_SPECIALITES(int* nbResultats);
REPONSE_RECHERCHE* SMOP_Consultation(int* nbResultats, int id, const char* name, const char* specialtie, const char* datedebut, const char* datefin);
DOCTOR* SMOP_DOCTORS(int* nbResultats);
#endif
