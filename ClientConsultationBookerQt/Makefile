serveur: Serveur.c LibrairieServeur.c
	gcc -o serveur Serveur.c LibrairieServeur.c -pthread -Wall

LibrairieServeur.o: LibrairieServeur.c LibrairieServeur.h
	gcc -c LibrairieServeur.c -o LibrairieServeur.o -Wall

clean:
	rm -f serveur testServeur *.o