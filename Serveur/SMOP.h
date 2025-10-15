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
bool SMOP(char* requete, char* reponse, int socket); 
bool SMOP_Login(int id,const char* user, const char* password);
int SMOP_Register(const char* user, const char* password);
//int  SMOP_Operation(char op, int a, int b); 
void SMOP_Close(); 
SPECIALITE* SMOP_SPECIALITES(int* nbResultats);
DOCTOR* SMOP_DOCTORS(int* nbResultats);
#endif
