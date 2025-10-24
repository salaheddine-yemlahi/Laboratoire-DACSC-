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


bool ACBP(char* reponse, int socket){
    char requete[256];
    bool reponseBool;
    sprintf(requete, "LIST_CLIENTS");
    reponseBool = SMOP(requete, reponse, socket);
}