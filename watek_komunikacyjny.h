#include "main.h"

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
            LISTkucyk.push_back(Request(status.MPI_SOURCE, packet.lamportClock));
            lamportSend(std::vector<int>(1, status.MPI_SOURCE), ACKkucyk, &lamportClock);
            break;
        case ACKkucyk:
            kucykACKcount++;
            if (kucykACKcount == touristCount)
            {
                changeState(KucykQ);
                debug("Otrzymalem wszystkie potwierdzenia o kucyku");
            }
            //zwieksza licznik zgod, jesli ten sie zgadza to odczekuje chwile i zmienia stan na SubQ chbya
            break;
        case RELkucyk:
            LISTkucyk.erase(std::remove(LISTkucyk.begin(), LISTkucyk.end(), status.MPI_SOURCE), LISTkucyk.end());
            //tu w sumie nie wiem, jeszcze do przemyslenia bo nie wiem czy stany beda sie zmieniac i w komunikacyjnym i glownym chyba tak
            break;
        case REQlodz:
            LISTlodz.push_back(Request(status.MPI_SOURCE, packet.lamportClock));
            debug("otrzymalem zadanie od %d: %s", status.MPI_SOURCE, stringLIST(LISTlodz).c_str());
            lamportSend(std::vector<int>(1, status.MPI_SOURCE), ACKlodz, &lamportClock);
            //wysyla ACKsub i dodaje proces do odpowiedniej listy
            break;
        case ACKlodz:
            debug("otrzymalem zgode od %d", status.MPI_SOURCE);
            lodzACKcount++;
            if (lodzACKcount == touristCount)
            {
                changeState(LodzQ);
                debug("Otrzymalem wszystkie potwierdzenia o lodzi: %s", stringLIST(LISTlodz).c_str());
            }
            //ehh w sumie trzeba sie bedzie zastanowic nad tym algorytmem bo jakis bez sensu mi sie wydaje xD
            break;
        case RELlodz:
            //lodzie[lodz z wiadomosci]=1
            //tych case juz nie pisze, bo nie wiem
            //mysle ze zaczniemy od tego zeby najpierw algorytm dzialal na zasadzie samych
            //strojow kucyka, zeby sie po nie zglaszaly, po otrzymaniu odczekiwaly chwile
            //a potem zwalnialy i odczekiwaly chwile i to wszystko w petli
            //a dopiero potem robimy lodz
            break;
        case FULLlodz:
            int nadzorca = status.MPI_SOURCE;
            int odplywajaceLodzie[count z wiadomosci];
            MPI_Recv(odplywajaceLodzie, count z wiadomosci, MPI_INT, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

            if(rank in odplywajaceLodzie){
                changeState(Trip);
            }

            //usuwa odplywajacelodzie z LISTlodz
            //ustawia lodzie[wybieranaLodz]=0
            //wybieranaLodz=(wybieranaLodz+1)%lodzCount

            
            break;
        default:
            break;
        }
    }
}
