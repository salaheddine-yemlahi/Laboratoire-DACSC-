# Nom de l'exécutable
serveur: Serveur.o LibrairieServeur.o SMOP.o ACBP.o
	g++ -o serveur Serveur.o LibrairieServeur.o SMOP.o ACBP.o -pthread -L/usr/lib64/mysql -lmysqlclient

# Serveur.cpp dépend maintenant de ACBP
Serveur.o: Serveur.cpp LibrairieServeur.h SMOP.h ACBP.h
	g++ -c Serveur.cpp -Wall -I/usr/include/mysql

# LibrairieServeur
LibrairieServeur.o: LibrairieServeur.cpp LibrairieServeur.h
	g++ -c LibrairieServeur.cpp -Wall -I/usr/include/mysql

# SMOP
SMOP.o: SMOP.cpp SMOP.h
	g++ -c SMOP.cpp -Wall -I/usr/include/mysql

# ACBP dépend de SMOP
ACBP.o: ACBP.cpp ACBP.h SMOP.h
	g++ -c ACBP.cpp -Wall -I/usr/include/mysql

# Nettoyage
clean:
	rm -f serveur *.o
