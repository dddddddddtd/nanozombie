#ifndef GLOBALH
#define GLOBALH

// #define _GNU_SOURCE
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <vector>

/* odkomentować, jeżeli się chce DEBUGI */
#define DEBUG 
/* boolean */
#define TRUE 1
#define FALSE 0

/* używane w wątku głównym, determinuje jak często i na jak długo zmieniają się stany */
#define STATE_CHANGE_PROB 50
#define SEC_IN_STATE 2

#define ROOT 0

/* stany procesu */
typedef enum {Inactive, PonyQ, SubQ, Trip, Pony} state_t;
extern state_t stan;
extern int rank;
extern int size;

//zmienne dla każdego procesu

extern int touristCount,ponyCostumes, submarineCount, touristRangeFrom, touristRangeTo, submarineRangeFrom, submarineRangeTo, lamportClock;
extern int ponyQclock;
extern int ponyACKcount;

extern std::vector<int> LISTkucykOK;
extern std::vector<int> LISTkucykHALT;

/* to może przeniesiemy do global... */
typedef struct
{
    int lamportClock;
    int src;
} lamportPacket;
extern MPI_Datatype mpiLamportPacket;

/* Typy wiadomości */
#define REQkucyk 1
#define ACKkucyk 2
#define RELkucyk 3
#define REQlodz 4
#define ACKlodz 5
#define RELlodz 6
#define FULLlodz 7

/* macro debug - działa jak printf, kiedy zdefiniowano
   DEBUG, kiedy DEBUG niezdefiniowane działa jak instrukcja pusta 
   
   używa się dokładnie jak printfa, tyle, że dodaje kolorków i automatycznie
   wyświetla rank

   w związku z tym, zmienna "rank" musi istnieć.

   w printfie: definicja znaku specjalnego "%c[%d;%dm [%d]" escape[styl bold/normal;kolor [RANK]
                                           FORMAT:argumenty doklejone z wywołania debug poprzez __VA_ARGS__
					   "%c[%d;%dm"       wyczyszczenie atrybutów    27,0,37
                                            UWAGA:
                                                27 == kod ascii escape. 
                                                Pierwsze %c[%d;%dm np 27[1;10m definiuje styl i kolor literek
                                                Drugie   %c[%d;%dm czyli 27[0;37m przywraca domyślne kolory i brak pogrubienia (bolda)
                                                ...  w definicji makra oznacza, że ma zmienną liczbę parametrów
                                            
*/
#ifdef DEBUG
#define debug(FORMAT,...) printf("%c[%d;%dm [%d - %d]: " FORMAT "%c[%d;%dm\n",  27, (1+(rank/7))%2, 31+(6+rank)%7, rank, lamportClock, ##__VA_ARGS__, 27,0,37);
#else
#define debug(...) ;
#endif

#define P_WHITE printf("%c[%d;%dm",27,1,37);
#define P_BLACK printf("%c[%d;%dm",27,1,30);
#define P_RED printf("%c[%d;%dm",27,1,31);
#define P_GREEN printf("%c[%d;%dm",27,1,33);
#define P_BLUE printf("%c[%d;%dm",27,1,34);
#define P_MAGENTA printf("%c[%d;%dm",27,1,35);
#define P_CYAN printf("%c[%d;%d;%dm",27,1,36);
#define P_SET(X) printf("%c[%d;%dm",27,1,31+(6+X)%7);
#define P_CLR printf("%c[%d;%dm",27,0,37);

/* printf ale z kolorkami i automatycznym wyświetlaniem RANK. Patrz debug wyżej po szczegóły, jak działa ustawianie kolorków */
#define println(FORMAT, ...) printf("%c[%d;%dm [%d]: " FORMAT "%c[%d;%dm\n",  27, (1+(rank/7))%2, 31+(6+rank)%7, rank, ##__VA_ARGS__, 27,0,37);

/* wysyłanie pakietu, skrót: wskaźnik do pakietu (0 oznacza stwórz pusty pakiet), do kogo, z jakim typem */
void changeState( state_t );

int lamportSend(int src, int dest, int tag, int *lamportClock);
int lamportReceive(lamportPacket * packetIn, int src, int tag, MPI_Status *status, int *lamportClock);

#endif
