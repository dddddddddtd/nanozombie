#include "main.h"

void mainLoop()
{
    while (1)
    {
        if (stan == Inactive) // stan nieaktywny
        {
            waitFor("zanim zaczne ubiegac sie o kucyka"); // oczekiwanie przez wylosowany czas i wypisanie komunikatu

            debug("Ubiegam się o kostium kucyka");
            kucykACKcount = 0;
            changeState(KucykWait);
            lamportPacket packetOut;
            lamportSend(touristsId, REQkucyk, &lamportClock, packetOut);
        }

        if (stan == KucykWait) // pętla oczekująca, aż odebrane zostaną wszystkie zgody ACKkucyk
        {
        }

        if (stan == KucykQ) // stan oczekiwania, aż proces będzie wśród ponyCostumes pierwszych procesów w kolejce dotyczącej kostiumu kucyka
        {
            pthread_mutex_lock(&kucykMut);
            std::sort(LISTkucyk.begin(), LISTkucyk.end());
            int index = std::distance(LISTkucyk.begin(), std::find(LISTkucyk.begin(), LISTkucyk.end(), rank));
            pthread_mutex_unlock(&kucykMut);

            if (index < ponyCostumes)
            {
                debug("biore stroj kucyka i ubiegam sie o łódź z kolejka KUCYK: %s", stringLIST(LISTkucyk).c_str());
                lodzACKcount = 0;
                changeState(LodzWait); // zmiana stanu na LodzWait
                lamportPacket packetOut;
                lamportSend(touristsId, REQlodz, &lamportClock, packetOut); // wysłanie żądania REQlodz
            }
        }

        if (stan == LodzWait) // pętla oczekująca, aż odebrane zostaną wszystkie zgody ACKlodz
        {
        }

        if (stan == LodzQ)
        {
            if (nadzorca == -1)
            {
                pthread_mutex_lock(&lodzMut);
                std::sort(LISTlodz.begin(), LISTlodz.end());
                int index = std::distance(LISTlodz.begin(), std::find(LISTlodz.begin(), LISTlodz.end(), rank));
                pthread_mutex_unlock(&lodzMut);

                if (index == 0) // jeżeli proces znajduje się na pierwszym miejscu kolejki żądań o łódź
                {

                    if (lodzieStan[wybieranaLodz] != 0) // jeżeli wybierana lodz jest dostepna
                    {

                        if (LISTlodz.size() + turysciWycieczka >= ponyCostumes) // sprawdzenie, czy wszystkie kostiumy kucyka są zajęte
                        {
                            debug("NADZORUJE TEST (%d): %s", turysciWycieczka, stringLIST(LISTlodz).c_str());
                            wycieczka.clear();
                            int suma = 0;
                            for (int i = 0; i < LISTlodz.size(); i++)
                            {
                                suma += tourists[LISTlodz[i].processid];

                                if (suma <= lodziePojemnosc[wybieranaLodz])
                                {
                                    wycieczka.push_back(LISTlodz[i].processid); // dodanie turysty do wektora turystów wypływających, jeśli mieści się on w łodzi
                                }

                                if (suma > lodziePojemnosc[wybieranaLodz])
                                {
                                    suma -= tourists[LISTlodz[i].processid];
                                    continue;
                                }
                            }

                            lamportPacket packetOut;
                            packetOut.count = wycieczka.size(); // liczba turystów wypływających
                            packetOut.lodz = wybieranaLodz;     // indeks łodzi wypływającej
                            debug("wyplywam na wycieczke w lodzi: %d, stan LISTlodz: %s", packetOut.lodz, stringLIST(LISTlodz).c_str());
                            lamportSend(touristsId, FULLlodz, &lamportClock, packetOut); // wysłanie komunikatu FULLłódź do wszystkich turystów
                            for (int i = 0; i < size; i++)                               // przesłanie tablicy z wypływającymi turystami bez zwiększenia zegaru Lamporta (jako część jednej wiadomości wraz z FULLłódź)
                            {
                                MPI_Send(wycieczka.data(), (int)wycieczka.size(), MPI_INT, i, DATA, MPI_COMM_WORLD);
                            }

                            nadzorca = rank; // nadzorca to turysta, który wysyła komunikat FULLłódź i będzie wysyłać komunikat RELłódź
                            changeState(Wycieczka);
                        }
                    }
                    else
                    {
                        wybieranaLodz = (wybieranaLodz + 1) % lodzCount; // zmiana numeru wybieranej łodzi, jeśli obecnie wybierana nie jest w stanie oczekiwania
                    }
                }
            }
        }

        if (stan == Wycieczka)
        {
            if (nadzorca == rank) // jeśli turysta wysyłał komunikat FULLłódź
            {
                debug("wycieczka z nadzorca %d", nadzorca);
                std::string test = "jestem na wycieczce [";
                for (int i = 0; i < wycieczka.size(); i++)
                {
                    test += std::to_string(wycieczka[i]) + ", ";
                }
                test = test.substr(0, test.size() - 2);
                test += "]";
                waitFor(test.c_str()); // odczekanie losowo wyznaczonego czasu i wypisaniu komunikatu, kto z tym turystą płynie
                lamportPacket packetOut;
                packetOut.lodz = wybieranaLodz;
                debug("wracam z wycieczki, zwalniam stroj kucyka");
                lamportSend(touristsId, RELlodz, &lamportClock, packetOut);  // po odczekaniu, wysłanie komunikatu RELlodz
                lamportSend(touristsId, RELkucyk, &lamportClock, packetOut); // wysłanie komunikatu RELkucyk
                changeState(Inactive);                                       // powrot do pierwszego stanu
                nadzorca = -1;
            }
        }

        if (stan == Ending) // zakończenie main_loop
        {
            debug("koncze");
            break;
        }
    }
}
