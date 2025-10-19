serveur: Serveur.o LibrairieServeur.o SMOP.o
	g++ -o serveur Serveur.o LibrairieServeur.o SMOP.o -pthread -L/usr/lib64/mysql -lmysqlclient

Serveur.o: Serveur.cpp LibrairieServeur.h SMOP.h
	g++ -c Serveur.cpp -Wall

LibrairieServeur.o: LibrairieServeur.cpp LibrairieServeur.h
	g++ -c LibrairieServeur.cpp -Wall

SMOP.o: SMOP.cpp SMOP.h
	g++ -c SMOP.cpp -Wall -I/usr/include/mysql

clean:
	rm -f serveur *.o