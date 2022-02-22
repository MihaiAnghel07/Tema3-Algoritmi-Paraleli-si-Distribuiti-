# Tema3-Algoritmi-Paraleli-si-Distribuiti-
Collaborative computing in a distributed system. Using MPI messages, three cluster coordinator processes communicate with each other in order to minimize the  computation time of tasks, efficiently using the capacity  of each cluster. (Used: MPI, distributed / parallelized algorithm).
***************************
NUME: ANGHEL MIHAI-GABRIEL*
GRUPA: 336CC		          *
***************************



**********Functii si variabile folosite**********
Voi detalia doar acele functii / variabile ce pot crea confuzie.

 - generate_file_name() -> aceasta functie genereaza numele fisierului
   de input pentru fiecare coordonator, in functie de rank-ul acestora;
 
 - process_array() -> aceasta functie este apelata de catre workers.
   Aceasta calculeaza partea array-ului pe care trebuie sa o proceseze
   worker-ul (in functie de rank), apoi efectueaza calculele.
 
 - *topology[3] -> este o matrice ce contine pe fiecare dintre cele 3
   linii topologia aferenta unui coordonator. Fiecare linie este formata
   astfel: primul element: rank-ul coordonatorului, al doilea element:
   numarul de workers ai coordonatorului respectiv, apoi rank-urile
   worker-ilor corespunzatori coordonatorului. In final, aceasta matrice
   va contine topologia intregului sistem.
   



**************FLOW*****************
Fiecare proces are acces la un fisier in care va scrie mesajele M(x, y),
iar la final procesul 0 va parcurge fisierul si va afisa mesajele la stdout.
Am incercat mai multe metode, insa uneori tot mai primeam erori si se anula 
punctajul. Metoda fisierului am preluat-o de pe forum de unde am inteles ca
aceasta este o metoda valida.

Procesele coordonator isi citesc cluster-ul din fisierele de input si in 
acelasi timp salveaza topologia proprie in matricea topology.
Coordonatorii trimit worker-ilor lor rank-ul pentru a-i informa care este 
coordonatorul lor.
Coordonatorii isi trimit unul altuia topologia proprie, iar cand primesc 
topologiile celorlalti, fiecare isi actualizeaza matricea topology si astfel
fiecare detine acum topologia finala a sistemului si o afiseaza. Acestia 
trimit topologia catre workers, afisand-o si acestia din urma.
Coordonatorul 0 genereaza vectorul si trimite intai dimensiunea acestuia apoi
vectorul catre ceilalti coordonatori, urmand ca fiecare coordonator sa execute
aceeasi chestie pentru worker-ii lor.
Worker-ii primesc dimensiunea N si vectorul, isi calculeaza zona pe care 
trebuie sa o proceseze (in functie de rank), fac procesarea, apoi trimit
vectorul catre coordonatorii lor.
Coordonatorii preiau vectorul si pentru a sti ce parte a vectorului sa
updateze, calculeaza marginile fiecarui worker si fac update-ul in vectorul
pe care il vor trimite catre Coordonatorul 0 (mai putin C0, care nu il va 
mai trimite la nimeni).
Coord. 1 si 2 trimit vectorul procesat catre coord. 0, acesta face update-ul
vectorului final si afiseaza rezultatul.
In final, dupa ce toate procesele si-au terminat task-urile, procesul 0 
parcurge fisierul cu mesajele M(x,y) si le afiseaza la stdout.




*****************Mentiuni*******************
Calculele sunt impartite uniform. Cunoscand intreaga topologie, worker-ii
calculeaza numarul total de workers si apoi calculeaza dimensiunea task-ului
pe care trebuie sa il execute in functie de acest numar total.

