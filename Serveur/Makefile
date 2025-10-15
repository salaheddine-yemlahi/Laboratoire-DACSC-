serveur: Serveur.c LibrairieServeur.c SMOP.cpp
	g++ -o serveur Serveur.c LibrairieServeur.c SMOP.cpp -pthread -Wall -I/usr/include/mysql -L/usr/lib64/mysql -lmysqlclient

LibrairieServeur.o: LibrairieServeur.c LibrairieServeur.h
	gcc -c LibrairieServeur.c -o LibrairieServeur.o -Wall

SMOP.o: SMOP.cpp SMOP.h
	g++ -c SMOP.cpp -o SMOP.o -Wall

clean:
	rm -f serveur testServeur *.o
