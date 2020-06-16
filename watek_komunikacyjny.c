#include "main.h"
#include "watek_komunikacyjny.h"

/* wątek komunikacyjny; zajmuje się odbiorem i reakcją na komunikaty */
void *startKomWatek(void *ptr)
{
    MPI_Status status;
    int is_message = FALSE;
    packet_t pakiet;
    /* Obrazuje pętlę odbierającą pakiety o różnych typach */
    while (1)
    {
        debug("czekam na recv");
        //tu chyba recv nie bedzie dla dowolnych tagow, tylko bedziemy mieli switcha zaleznego od stanu procesu, gdzie dany case odbiera dane typy wiadomosci
        MPI_Recv(&pakiet, 1, MPI_PAKIET_T, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

        switch (status.MPI_TAG)
        {
        case REQpony:
            //jeżeli stan PonyQ:
            //jeżeli niższy priorytet, odsyła ACKpony i dodaje proces do listy LISTkucykOK
            //jeżeli wyższy priorytet, zapisuje proces do listy procesów, którym wyśle ACKpony po zakończeniu wycieczki
            
            //jeżeli inny stan (chyba w sumie taka wiadomosc bedzie mogl odebrac tylko jak bedzie w stanie Inactive albo PonyQ, 
            //bo w pozostałych stanach chyba będziemy wymuszać odbieranie wiadomości tylko konkretnego typu, 
            //więc dopiero jak przejdzie do Inactive to odbierze wiadomość REQpony czy coś)
            break;
        case ACKpony:
            //zwieksza licznik zgod, jesli ten sie zgadza to odczekuje chwile i zmienia stan na SubQ chbya
            break;
        case RELpony:
            //tu w sumie nie wiem, jeszcze do przemyslenia bo nie wiem czy stany beda sie zmieniac i w komunikacyjnym i glownym chyba tak
            break;
        case REQsub:
            //wysyla ACKsub i dodaje proces do odpowiedniej listy
            break;
        case ACKsub:
            //ehh w sumie trzeba sie bedzie zastanowic nad tym algorytmem bo jakis bez sensu mi sie wydaje xD
            break;
        case RELsub:
            //tych case juz nie pisze, bo nie wiem
            //mysle ze zaczniemy od tego zeby najpierw algorytm dzialal na zasadzie samych
            //strojow kucyka, zeby sie po nie zglaszaly, po otrzymaniu odczekiwaly chwile
            //a potem zwalnialy i odczekiwaly chwile i to wszystko w petli
            //a dopiero potem robimy lodz
            break;
        case FULLsub:
            break;
        default:
            break;
        }
    }
}
