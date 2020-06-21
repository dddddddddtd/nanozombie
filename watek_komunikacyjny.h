#include "main.h"

MPI_Request req;

/* wątek komunikacyjny; zajmuje się odbiorem i reakcją na komunikaty */
void *startKomWatek(void *)
{

    MPI_Status status;
    lamportPacket packet;
    lamportPacket packetOut;

    std::string test;
    std::vector<int> receivers;

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
            receivers = std::vector<int>(1, status.MPI_SOURCE);
            lamportSend(receivers, ACKkucyk, &lamportClock, packetOut); // odesłanie do nadawcy potwierdzenia ACKkucyk
            break;

        // obsługa ACKkucyk
        case ACKkucyk:
            kucykACKcount++; // zwiększenie liczby potwierdzeń dotyczących stroju kucyka
            // debug("ACKkucyk od %d, mam zgod %d/%d", status.MPI_SOURCE, kucykACKcount, touristCount);
            if (kucykACKcount == touristCount) // w momencie uzyskania potwierdzeń od wszystkich turystów
            {
                // debug("ACKkucyk mam zgod %d/%d: %s", kucykACKcount, touristCount, stringLIST(LISTkucyk).c_str());
                changeState(KucykQ); //zmiana stanu na KucykQ
                // debug("komplet ack");
                MPI_Send(&kucykACKcount, 1, MPI_INT, rank, 0, SELFCOMM);
            }
            break;

        // obsługa RELkucyk
        case RELkucyk:
            // debug("odebralem RELkucyk");
            pthread_mutex_lock(&kucykMut);
            LISTkucyk.erase(std::remove(LISTkucyk.begin(), LISTkucyk.end(), status.MPI_SOURCE), LISTkucyk.end()); //usunięcie z kolejki związanej ze strojami kucyka nadawcy komunikatu
            pthread_mutex_unlock(&kucykMut);
            turysciStan[status.MPI_SOURCE] = 1;
            turysciWycieczka--;
            // debug("dostalem rel");
            if (stan != Inactive)
            {
                MPI_Send(&kucykACKcount, 1, MPI_INT, rank, 0, SELFCOMM);
            }

            break;

        // obsługa REQlodz
        case REQlodz:
            pthread_mutex_lock(&lodzMut);
            if (turysciStan[status.MPI_SOURCE] != 0)
                LISTlodz.push_back(Request(status.MPI_SOURCE, packet.lamportClock)); // dodanie żądania do kolejki związanej z łodziami
            pthread_mutex_unlock(&lodzMut);
            // debug("odsylam na %d", status.MPI_SOURCE);
            receivers = std::vector<int>(1, status.MPI_SOURCE);
            lamportSend(receivers, ACKlodz, &lamportClock, packetOut); // odesłanie do nadawcy potwierdzenia ACKlodz
            break;

        // obsługa ACKlodz
        case ACKlodz:
            lodzACKcount++; // zwiększenie liczby potwierdzeń dotyczących łodzi
            // debug("uzyskalem %d/%d potwierdzen ACKlodz", lodzACKcount, touristCount);
            if (lodzACKcount == touristCount) // w momencie uzyskania potwierdzeń od wszystkich turystów
            {
                // debug("ACKlodz %d/%d, %s", lodzACKcount, touristCount, stringLIST(LISTlodz).c_str());
                // changeState(LodzQ); //zmiana stanu na LodzQ
                pthread_mutex_lock(&lodzMut);
                if (LISTlodz.size() != 0)
                {
                    std::sort(LISTlodz.begin(), LISTlodz.end());
                    int index = std::distance(LISTlodz.begin(), std::find(LISTlodz.begin(), LISTlodz.end(), rank));
                    if (index == 0)
                    {
                        changeState(LodzTEST);
                        MPI_Send(&kucykACKcount, 1, MPI_INT, rank, 0, SELFCOMM);
                    }
                }
                pthread_mutex_unlock(&lodzMut);
            }
            break;

        // obsługa RELlodz
        case RELlodz:
            lodzieStan[packet.lodz] = 1; // ustawienie stanu łodzi na oczekujący
            if (nadzorca == -1)
            {
                pthread_mutex_lock(&lodzMut);
                if (LISTlodz.size() != 0)
                {
                    std::sort(LISTlodz.begin(), LISTlodz.end());
                    int index = std::distance(LISTlodz.begin(), std::find(LISTlodz.begin(), LISTlodz.end(), rank));
                    if (index == 0)
                    {
                        changeState(LodzTEST);
                        MPI_Send(&kucykACKcount, 1, MPI_INT, rank, 0, SELFCOMM);
                    }
                }
                pthread_mutex_unlock(&lodzMut);
            }

            if (nadzorca == status.MPI_SOURCE && rank != nadzorca) // jeśli brał udział w wycieczce nadawcy
            {
                debug("5. wracam z wycieczki, zwalniam stroj kucyka");
                lamportSend(touristsId, RELkucyk, &lamportClock, packetOut); // zwalnia kucyka
                changeState(Inactive);                                       // zmienia stan na poczatkowy
                MPI_Send(&kucykACKcount, 1, MPI_INT, rank, 0, SELFCOMM);
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

            // debug("!!!odebralem od %d: %s, stan LISTlodz: %s", status.MPI_SOURCE, stringArray(odplywajace, liczbaodplywajacych).c_str(), stringLIST(LISTlodz).c_str());
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
                turysciStan[odplywajace[i]] = 0;
                LISTlodz.erase(std::remove(LISTlodz.begin(), LISTlodz.end(), odplywajace[i]), LISTlodz.end());
            }

            if (LISTlodz.size() != 0)
            {
                std::sort(LISTlodz.begin(), LISTlodz.end());
                int index = std::distance(LISTlodz.begin(), std::find(LISTlodz.begin(), LISTlodz.end(), rank));
                if (index == 0)
                {
                    // debug("ZMIENIAM STAN NA LODZTEST (FULLLODZ)");
                    changeState(LodzTEST);
                    MPI_Send(&kucykACKcount, 1, MPI_INT, rank, 0, SELFCOMM);
                }
            }

            // debug("!!!usunalem, stan LISTlodz: %s, nadzorca: %d, turysciWycieczka: %d", stringLIST(LISTlodz).c_str(), nadzorca, turysciWycieczka);
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
