#include "main.h"

/* wątek komunikacyjny; zajmuje się odbiorem i reakcją na komunikaty */
void *startKomWatek(void *)
{
    MPI_Status status;

    lamportPacket packet;
    lamportPacket packetOut;

    /* Obrazuje pętlę odbierającą pakiety o różnych typach */
    while (1)
    {
        if (signalhandler == true)
        { // zmiana stanu na Ending i zakończenie wątku komunikacyjnego
            changeState(Ending);
            break;
        }

        // odbiór dowolnej wiadomości
        lamportReceive(&packet, MPI_ANY_SOURCE, MPI_ANY_TAG, &status, &lamportClock);

        // obsługa wiadomości o różnych tagach
        switch (status.MPI_TAG)
        {

        // obsługa REQkucyk
        case REQkucyk:
            LISTkucyk.push_back(Request(status.MPI_SOURCE, packet.lamportClock));                    // dodanie żądania do kolejki związanej ze strojami kucyka
            lamportSend(std::vector<int>(1, status.MPI_SOURCE), ACKkucyk, &lamportClock, packetOut); // odesłanie do nadawcy potwierdzenia ACKkucyk
            break;

        // obsługa ACKkucyk
        case ACKkucyk:
            kucykACKcount++;                   // zwiększenie liczby potwierdzeń dotyczących stroju kucyka
            if (kucykACKcount == touristCount) // w momencie uzyskania potwierdzeń od wszystkich turystów
            {
                changeState(KucykQ); //zmiana stanu na KucykQ
            }
            break;

        // obsługa RELkucyk
        case RELkucyk:
            LISTkucyk.erase(std::remove(LISTkucyk.begin(), LISTkucyk.end(), status.MPI_SOURCE), LISTkucyk.end()); //usunięcie z kolejki związanej ze strojami kucyka nadawcy komunikatu
            break;

        // obsługa REQlodz
        case REQlodz:
            LISTlodz.push_back(Request(status.MPI_SOURCE, packet.lamportClock));                    // dodanie żądania do kolejki związanej z łodziami
            lamportSend(std::vector<int>(1, status.MPI_SOURCE), ACKlodz, &lamportClock, packetOut); // odesłanie do nadawcy potwierdzenia ACKlodz
            break;

        // obsługa ACKlodz
        case ACKlodz:
            lodzACKcount++;                   // zwiększenie liczby potwierdzeń dotyczących łodzi
            if (lodzACKcount == touristCount) // w momencie uzyskania potwierdzeń od wszystkich turystów
            {
                changeState(LodzQ); //zmiana stanu na LodzQ
            }
            break;

        // obsługa RELlodz
        case RELlodz:
            lodzieStan[packet.lodz] = 1; // ustawienie stanu łodzi na oczekujący

            if (nadzorca == status.MPI_SOURCE && rank != nadzorca) // jeśli brał udział w wycieczce nadawcy
            {

                debug("wracam z wycieczki, zwalniam stroj kucyka");
                lamportSend(touristsId, RELkucyk, &lamportClock, packetOut); // zwalnia kucyka
                changeState(Inactive);                                       // zmienia stan na poczatkowy
                nadzorca = -1;                                               // ustawia id nadzorcy na -1
            }
            break;

        // obsługa FULLlodz
        case FULLlodz:
        {
            int liczbaodplywajacych = packet.count;
            int lodz = packet.lodz;
            int odplywajace[liczbaodplywajacych];

            // odebranie listy turystów odpływających bez zwiększania zegaru Lamporta (jako część obsługi zdarzenia FULLlodz)
            MPI_Recv(odplywajace, liczbaodplywajacych, MPI_INT, MPI_ANY_SOURCE, DATA, MPI_COMM_WORLD, &status);

            if (checkIfInArray(odplywajace, liczbaodplywajacych, rank) && status.MPI_SOURCE != rank) // jeśli turysta odpływa tą łodzią
            {
                nadzorca = status.MPI_SOURCE; // ustawienie nadzorcy na nadawcę
                changeState(Wycieczka);       // zmiana stanu na Wycieczka
                debug("jade na wycieczke z %d", nadzorca);
            }

            lodzieStan[lodz] = 0; // zmiana stanu łodzi na wypłyniętą

            // usuwa turystów odplywajacych z listy oczekujących na łódź
            for (int i = 0; i < liczbaodplywajacych; i++)
            {
                LISTlodz.erase(std::remove(LISTlodz.begin(), LISTlodz.end(), odplywajace[i]), LISTlodz.end());
            }
            break;
        }
        default:
            break;
        }
    }
    return EXIT_SUCCESS;
}
