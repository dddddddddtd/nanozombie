#include "main.h"

/* wątek komunikacyjny; zajmuje się odbiorem i reakcją na komunikaty */
void *startKomWatek(void *ptr)
{
    MPI_Status status;
    int is_message = FALSE;

    std::string test;

    lamportPacket packet;
    lamportPacket packetOut;
    /* Obrazuje pętlę odbierającą pakiety o różnych typach */
    while (1)
    {
        //tu chyba recv nie bedzie dla dowolnych tagow, tylko bedziemy mieli switcha zaleznego od stanu procesu, gdzie dany case odbiera dane typy wiadomosci
        // debug("czekam na wiadomosc");
        lamportReceive(&packet, MPI_ANY_SOURCE, MPI_ANY_TAG, &status, &lamportClock);
        // MPI_Recv(&packet, 1, mpiLamportPacket, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
        // debug("otrzymalem wiadomosc o tagu: %d", status.MPI_TAG);

        switch (status.MPI_TAG)
        {
        case REQkucyk:
            LISTkucyk.push_back(Request(status.MPI_SOURCE, packet.lamportClock));
            lamportSend(std::vector<int>(1, status.MPI_SOURCE), ACKkucyk, &lamportClock, packetOut);
            break;
        case ACKkucyk:
            kucykACKcount++;
            if (kucykACKcount == touristCount)
            {
                changeState(KucykQ);
            }
            //zwieksza licznik zgod, jesli ten sie zgadza to odczekuje chwile i zmienia stan na SubQ chbya
            break;
        case RELkucyk:
            LISTkucyk.erase(std::remove(LISTkucyk.begin(), LISTkucyk.end(), status.MPI_SOURCE), LISTkucyk.end());
            //tu w sumie nie wiem, jeszcze do przemyslenia bo nie wiem czy stany beda sie zmieniac i w komunikacyjnym i glownym chyba tak
            break;
        case REQlodz:
            LISTlodz.push_back(Request(status.MPI_SOURCE, packet.lamportClock));
            lamportSend(std::vector<int>(1, status.MPI_SOURCE), ACKlodz, &lamportClock, packetOut);
            //wysyla ACKsub i dodaje proces do odpowiedniej listy
            break;
        case ACKlodz:
            lodzACKcount++;
            if (lodzACKcount == touristCount)
            {
                debug("%s", stringLIST(LISTlodz).c_str());
                changeState(LodzQ);
            }
            //ehh w sumie trzeba sie bedzie zastanowic nad tym algorytmem bo jakis bez sensu mi sie wydaje xD
            break;
        case RELlodz:
            //trzeba jakos przekazac numer lodzi chyba

            //jesli bral udzial w wycieccze nadawcy
            if (nadzorca == status.MPI_SOURCE)
            {
                debug("wracam z wycieczki");
                lamportSend(touristsId, RELkucyk, &lamportClock, packetOut); //zwalnia kucyka
                changeState(Inactive);                                       //zmienia stan na poczatkowy
                nadzorca = -1;                                               //ustawia id nadzorcy na -1
            }

            lodzieStan[packet.lodz] = 1; //
            break;
        case FULLlodz:
        {
            int liczbaodplywajacych = packet.count;
            int lodz = packet.lodz;
            int odplywajace[liczbaodplywajacych];

            MPI_Recv(odplywajace, liczbaodplywajacych, MPI_INT, MPI_ANY_SOURCE, DATA, MPI_COMM_WORLD, &status);

            // int odplywajace[liczbaodplywajacych];

            test = "[";
            for (int i = 0; i < liczbaodplywajacych; i++)
            {
                test += std::to_string(odplywajace[i]) + ", ";
            }
            test = test.substr(0, test.size() - 2);
            test += "]";
            debug("ci odplywaja: %s", test.c_str());


            if (checkIfInArray(odplywajace, liczbaodplywajacych, rank))
            {
                nadzorca = status.MPI_SOURCE;
                changeState(Wycieczka);
                debug("jade na wycieczke z %d", nadzorca);
            }
            

            //usuwa odplywajace z listy oczekujacych na lodz
            for (int i = 0; i < liczbaodplywajacych; i++)
            {
                LISTlodz.erase(std::remove(LISTlodz.begin(), LISTlodz.end(), odplywajace[i]), LISTlodz.end());
            }
            //ustawia stan lodzi na wyplynela
            lodzieStan[lodz] = 0;
            //zmienia numer wybieranej lodzi na nastepny, nad tym chyba trzeba bedzie popracowac
            // wybieranaLodz = (wybieranaLodz + 1) % lodzCount;

            // jesli nalezy do odplywajacych lodzi
            break;
        }
        default:
            break;
        }
    }
}
