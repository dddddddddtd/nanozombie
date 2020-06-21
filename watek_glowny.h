#include "main.h"

void mainLoop()
{
    while (1)
    {
        // debug("%d", stan);
        if (stan == Inactive) // stan nieaktywny
        {
            waitFor("zanim zaczne ubiegac sie o kucyka"); // oczekiwanie przez wylosowany czas i wypisanie komunikatu

            debug("1. ubiegam się o kostium kucyka");
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
                debug("2. biore stroj kucyka i ubiegam sie o łódź");
                lodzACKcount = 0;
                changeState(LodzWait); // zmiana stanu na LodzWait
                lamportPacket packetOut;
                lamportSend(touristsId, REQlodz, &lamportClock, packetOut); // wysłanie żądania REQlodz
            }
        }

        if (stan == LodzWait) // pętla oczekująca, aż odebrane zostaną wszystkie zgody ACKlodz
        {
        }

        if (stan == LodzTEST)
        {

            while (lodzieStan[wybieranaLodz] == 0)
            {
                wybieranaLodz = (wybieranaLodz + 1) % lodzCount;
            }

            // debug("%d + %d >= %d", (int) LISTlodz.size(), turysciWycieczka, ponyCostumes);
            int test = 0;
            for (int i = 0; i < turysciStan.size(); i++)
            {
                if (turysciStan[i] == 0)
                    test++;
            }
            if (LISTlodz.size() + test >= ponyCostumes) // sprawdzenie, czy wszystkie kostiumy kucyka są zajęte
            {
                debug("wszyscy maja stroj kucyka!");
                // debug("NADZORUJE TEST (%d): %s", turysciWycieczka, stringLIST(LISTlodz).c_str());
                pthread_mutex_lock(&wycieczkaMut);
                wycieczka.clear();
                pthread_mutex_unlock(&wycieczkaMut);
                int suma = 0;
                for (int i = 0; i < LISTlodz.size(); i++)
                {
                    suma += tourists[LISTlodz[i].processid];
                    pthread_mutex_lock(&wycieczkaMut);
                    if (suma <= lodziePojemnosc[wybieranaLodz])
                    {
                        wycieczka.push_back(LISTlodz[i].processid); // dodanie turysty do wektora turystów wypływających, jeśli mieści się on w łodzi
                    }
                    pthread_mutex_unlock(&wycieczkaMut);

                    if (suma > lodziePojemnosc[wybieranaLodz])
                    {
                        suma -= tourists[LISTlodz[i].processid];
                        continue;
                    }
                }

                lamportPacket packetOut;
                packetOut.count = wycieczka.size(); // liczba turystów wypływających
                packetOut.lodz = wybieranaLodz;     // indeks łodzi wypływającej
                debug("3. wyplywam na wycieczke w lodzi: %d, wycieczka to turysci: %s", packetOut.lodz, stringVector(wycieczka).c_str());
                lamportSend(touristsId, FULLlodz, &lamportClock, packetOut); // wysłanie komunikatu FULLłódź do wszystkich turystów
                for (int i = 0; i < size; i++)                               // przesłanie tablicy z wypływającymi turystami bez zwiększenia zegaru Lamporta (jako część jednej wiadomości wraz z FULLłódź)
                {
                    MPI_Send(wycieczka.data(), (int)wycieczka.size(), MPI_INT, i, DATA, MPI_COMM_WORLD);
                }

                nadzorca = rank; // nadzorca to turysta, który wysyła komunikat FULLłódź i będzie wysyłać komunikat RELłódź
                changeState(Wycieczka);
                changeState(Wait);
                std::string test = "4. jestem na wycieczce z " + stringVector(wycieczka);

                waitFor(test.c_str()); // odczekanie losowo wyznaczonego czasu i wypisaniu komunikatu, kto z tym turystą płynie
                // lamportPacket packetOut;
                packetOut.lodz = wybieranaLodz;
                // debug("5. wracam z wycieczki, zwalniam stroj kucyka");
                lamportSend(touristsId, RELlodz, &lamportClock, packetOut); // po odczekaniu, wysłanie komunikatu RELlodz
                // lamportSend(touristsId, RELkucyk, &lamportClock, packetOut); // wysłanie komunikatu RELkucyk
                // powrot do pierwszego stanu
                // nadzorca = -1;
            }
        }

        if (stan == Ending) // zakończenie main_loop
        {
            debug("koncze");
            break;
        }
    }
}
