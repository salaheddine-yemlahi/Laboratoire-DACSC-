serveur: Serveur.c LibrairieServeur.c
	gcc -o serveur Serveur.c LibrairieServeur.c -pthread -Wall -I/usr/include/mysql -L/usr/lib64/mysql -lmysqlclient

LibrairieServeur.o: LibrairieServeur.c LibrairieServeur.h
	gcc -c LibrairieServeur.c -o LibrairieServeur.o -Wall

clean:
	rm -f serveur testServeur *.o
