#include "main.h"
#include <algorithm>

/* wątek komunikacyjny; zajmuje się odbiorem i reakcją na komunikaty */
void *startKomWatek(void *ptr)
{
    MPI_Status status;
    int is_message = FALSE;
    lamportPacket packet;
    /* Obrazuje pętlę odbierającą pakiety o różnych typach */
    while (1)
    {
        //tu chyba recv nie bedzie dla dowolnych tagow, tylko bedziemy mieli switcha zaleznego od stanu procesu, gdzie dany case odbiera dane typy wiadomosci
        lamportReceive(&packet, MPI_ANY_SOURCE, MPI_ANY_TAG, &status, &lamportClock);
        // MPI_Recv(&packet, 1, mpiLamportPacket, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

        switch (status.MPI_TAG)
        {
        case REQkucyk:
            if (stan == Inactive || status.MPI_SOURCE == rank)
            {
                debug("!%d - %d! nieaktywny lub do siebie: udzielam ACKkucyk", status.MPI_SOURCE, packet.lamportClock);
                lamportSend(rank, status.MPI_SOURCE, ACKkucyk, &lamportClock);
                LISTkucykOK.push_back(status.MPI_SOURCE);
            }
            else if (stan == PonyQ)
            {
                if (lamportClock < packet.lamportClock)
                {
                    debug("qclock=%d !%d - %d! wyższy priorytet, wrzucam do HALT", lamportClock, status.MPI_SOURCE, packet.lamportClock);
                    //dodać do listy, do których potem wysłać ack
                    LISTkucykHALT.push_back(status.MPI_SOURCE);
                }
                else if (lamportClock > packet.lamportClock)
                {
                    debug("qclock=%d !%d - %d! niższy priorytet, udzielam ACKkucyk", lamportClock, status.MPI_SOURCE, packet.lamportClock);
                    lamportSend(rank, status.MPI_SOURCE, ACKkucyk, &lamportClock);
                    LISTkucykOK.push_back(status.MPI_SOURCE);
                }
                else if (lamportClock == packet.lamportClock)
                {
                    if (rank < status.MPI_SOURCE)
                    {
                        debug("qclock=%d !%d - %d! niższy identyfikator, wrzucam do HALT", lamportClock, status.MPI_SOURCE, packet.lamportClock);
                        //dodac do listy do ktorych potem wyslac ack
                        LISTkucykHALT.push_back(status.MPI_SOURCE);
                    }
                    else
                    {
                        debug("qclock=%d !%d - %d! wyższy identyfikator, udzielam ACKkucyk", lamportClock, status.MPI_SOURCE, packet.lamportClock);
                        lamportSend(rank, status.MPI_SOURCE, ACKkucyk, &lamportClock);
                        LISTkucykOK.push_back(status.MPI_SOURCE);
                    }
                }
            }
            break;
        case ACKkucyk:
            debug("otrzymałem zgode kucyka od %d", status.MPI_SOURCE);
            ponyACKcount++;
            if (ponyACKcount == touristCount)
            {
                changeState(Pony);
                debug("Otrzymalem wszystkie zgody na kucyka");
            }
            //zwieksza licznik zgod, jesli ten sie zgadza to odczekuje chwile i zmienia stan na SubQ chbya
            break;
        case RELkucyk:
            LISTkucykOK.erase(std::remove(LISTkucykOK.begin(), LISTkucykOK.end(), status.MPI_SOURCE), LISTkucykOK.end());
            //tu w sumie nie wiem, jeszcze do przemyslenia bo nie wiem czy stany beda sie zmieniac i w komunikacyjnym i glownym chyba tak
            break;
        case REQlodz:
            //wysyla ACKsub i dodaje proces do odpowiedniej listy
            break;
        case ACKlodz:
            //ehh w sumie trzeba sie bedzie zastanowic nad tym algorytmem bo jakis bez sensu mi sie wydaje xD
            break;
        case RELlodz:
            //tych case juz nie pisze, bo nie wiem
            //mysle ze zaczniemy od tego zeby najpierw algorytm dzialal na zasadzie samych
            //strojow kucyka, zeby sie po nie zglaszaly, po otrzymaniu odczekiwaly chwile
            //a potem zwalnialy i odczekiwaly chwile i to wszystko w petli
            //a dopiero potem robimy lodz
            break;
        case FULLlodz:
            break;
        default:
            break;
        }
    }
}
