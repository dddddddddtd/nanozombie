#include "main.h"

/* wątek komunikacyjny; zajmuje się odbiorem i reakcją na komunikaty */
void *startKomWatek(void *)
{
    MPI_Status status;

    lamportPacket packet;
    lamportPacket packetOut;

    std::string test;

    /* Obrazuje pętlę odbierającą pakiety o różnych typach */
    while (1)
    {
        // odbiór dowolnej wiadomości
        lamportReceive(&packet, MPI_ANY_SOURCE, MPI_ANY_TAG, &status, &lamportClock);

        // obsługa wiadomości o różnych tagach
        switch (status.MPI_TAG)
        {
        // obsługa REQkucyk
        case REQkucyk:
            pthread_mutex_lock(&kucykMut);
            LISTkucyk.push_back(Request(status.MPI_SOURCE, packet.lamportClock)); // dodanie żądania do kolejki związanej ze strojami kucyka
            // debug("REQkucyk od %d: %s", status.MPI_SOURCE, stringLIST(LISTkucyk).c_str());
            pthread_mutex_unlock(&kucykMut);
            lamportSend(std::vector<int>(1, status.MPI_SOURCE), ACKkucyk, &lamportClock, packetOut); // odesłanie do nadawcy potwierdzenia ACKkucyk
            break;

        // obsługa ACKkucyk
        case ACKkucyk:
            kucykACKcount++; // zwiększenie liczby potwierdzeń dotyczących stroju kucyka
            // debug("ACKkucyk od %d, mam zgod %d/%d", status.MPI_SOURCE, kucykACKcount, touristCount);
            if (kucykACKcount == touristCount) // w momencie uzyskania potwierdzeń od wszystkich turystów
            {
                // debug("ACKkucyk mam zgod %d/%d: %s", kucykACKcount, touristCount, stringLIST(LISTkucyk).c_str());
                changeState(KucykQ); //zmiana stanu na KucykQ
            }
            break;

        // obsługa RELkucyk
        case RELkucyk:
            pthread_mutex_lock(&kucykMut);
            LISTkucyk.erase(std::remove(LISTkucyk.begin(), LISTkucyk.end(), status.MPI_SOURCE), LISTkucyk.end()); //usunięcie z kolejki związanej ze strojami kucyka nadawcy komunikatu
            // debug("RELkucyk od %d: %s", status.MPI_SOURCE, stringLIST(LISTkucyk).c_str());
            pthread_mutex_unlock(&kucykMut);
            turysciWycieczka--;
            break;

        // obsługa REQlodz
        case REQlodz:
            LISTlodz.push_back(Request(status.MPI_SOURCE, packet.lamportClock)); // dodanie żądania do kolejki związanej z łodziami
            // debug("odsylam na %d", status.MPI_SOURCE);
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
                debug("5. wracam z wycieczki, zwalniam stroj kucyka");
                lamportSend(touristsId, RELkucyk, &lamportClock, packetOut); // zwalnia kucyka
                changeState(Inactive);                                       // zmienia stan na poczatkowy
                nadzorca = -1;                                               // ustawia id nadzorcy na -1
            }
            break;

        // obsługa FULLlodz
        case FULLlodz:
        {
            int liczbaodplywajacych = packet.count;
            lodzieStan[packet.lodz] = 0; // zmiana stanu łodzi na wypłyniętą
            int odplywajace[liczbaodplywajacych];

            // odebranie listy turystów odpływających bez zwiększania zegaru Lamporta (jako część obsługi zdarzenia FULLlodz)
            // debug("!!!przed odebraniem odplywajacych od %d, odplywajacych = %d", status.MPI_SOURCE, liczbaodplywajacych);
            MPI_Recv(odplywajace, liczbaodplywajacych, MPI_INT, status.MPI_SOURCE, DATA, MPI_COMM_WORLD, &status);

            // test = "[";
            // for (int i = 0; i < liczbaodplywajacych; i++)
            // {
            //     test += std::to_string(odplywajace[i]) + ",";
            // }
            // test += "]";

            // debug("!!!odebralem od %d: %s", status.MPI_SOURCE, test.c_str());
            if (checkIfInArray(odplywajace, liczbaodplywajacych, rank) && status.MPI_SOURCE != rank) // jeśli turysta odpływa tą łodzią
            {
                nadzorca = status.MPI_SOURCE; // ustawienie nadzorcy na nadawcę
                changeState(Wycieczka);       // zmiana stanu na Wycieczka
                debug("3. jade na wycieczke: %s", stringArray(odplywajace, liczbaodplywajacych).c_str());
            }

            // usuwa turystów odplywajacych z listy oczekujących na łódź
            pthread_mutex_lock(&lodzMut);
            for (int i = 0; i < liczbaodplywajacych; i++)
            {
                LISTlodz.erase(std::remove(LISTlodz.begin(), LISTlodz.end(), odplywajace[i]), LISTlodz.end());
            }
            pthread_mutex_unlock(&lodzMut);

            turysciWycieczka += liczbaodplywajacych;
            break;
        }
        default:
            break;
        }
    }
    return EXIT_SUCCESS;
}
