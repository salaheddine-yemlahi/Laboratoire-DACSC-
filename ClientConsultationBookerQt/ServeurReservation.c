#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h> 
#include "LibrairieServeur.h"

typedef struct 
{ 
char nom[20]; 
int  age; 
float poids; 
} PERSONNE;

int main(int argc,char* argv[])
{ 
    if (argc != 2)
    {
        printf("Erreur...\n");
        printf("USAGE : ServeurTest portServeur\n");
        exit(1);
    }
    int sServer; 
    if( sServer = creerServeur(atoi(argv[1])) == -1) // fais quoi atoi.
    {
        perror("Erreur de ServeurSocket");
        exit(1);
    } 
    printf("Attente d'une connexion...\n"); 

    while(1){
        int sService;
        if(sServer = accepterClient(sServer) == -1)
        {
            perror("Erreur de Accept");
            close(sServer);
            exit(1);
        } 
        printf("Connexion acceptee !\n");
    }
    


    
    // ***** Reception texte pur ************************************** 
    char buffer[100]; 
    int nbLus; 
    if ((nbLus = recevoirMessage(sService,buffer)) < 0) 
    { 
        perror("Erreur de Receive"); 
        close(sService); 
        close(sServer); 
        exit(1); 
    } 
    printf("NbLus = %d\n",nbLus); 
    buffer[nbLus] = 0; 
    printf("Lu    = --%s--\n",buffer); 




    // ***** Envoi de texte pur *************************************** 
    char texte[80]; 
    sprintf(texte,"Je vais bien merci ;) !"); 
    int nbEcrits; 
    if ((nbEcrits = envoyerReponse(sService,texte)) < 0) 
    { 
        perror("Erreur de Send"); 
        close(sService); 
        close(sServer); 
        exit(1); 
    } 
    printf("NbEcrits = %d\n",nbEcrits); 
    printf("Ecrit    = --%s--\n",texte);
    PERSONNE p; 
    if ((nbLus = recevoirMessage(sService,(char*)&p)) < 0) 
    { 
        perror("Erreur de Receive"); 
        close(sService); 
        close(sServer); 
        exit(1); 
    } 
    
    printf("NbLus = %d\n",nbLus); 
    printf("Lu    = --%s--%d--%f--\n",p.nom,p.age,p.poids); 
    



    // ***** Envoi d'une structure ************************************* 
    strcpy(p.nom,"charlet"); 
    p.age = 54; 
    p.poids = 71.98f; 
    if ((nbEcrits = envoyerReponse(sService,(char*)&p)) < 0) 
    { 
        perror("Erreur de Send"); 
        close(sService);
        close(sServer);
        exit(1); 
    } 
    
    printf("NbEcrits = %d\n",nbEcrits); 
    printf("Ecrit    = --%s--%d--%f--\n",p.nom,p.age,p.poids); 

    close(sService); 
    close(sServer); 
    
    exit(0); 
}