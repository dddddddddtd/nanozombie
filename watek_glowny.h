#include "main.h"

void mainLoop()
{
    int tmp;
    std::string stanstring;
    MPI_Status status;
    while (1)
    {
        // debug("blok");
        if (stan != Inactive)
        {
            MPI_Recv(&tmp, 1, MPI_INT, rank, MPI_ANY_TAG, SELFCOMM, &status);
        }

        switch (stan)
        {
        case Inactive:
            stanstring = "Inactive";
            break;
        case KucykWait:
            stanstring = "KucykWait";
            break;
        case KucykQ:
            stanstring = "KucykQ";
            break;
        case LodzQ:
            stanstring = "LodzQ";
            break;
        case Wycieczka:
            stanstring = "Wycieczka";
            break;
        case LodzWait:
            stanstring = "LodzWait";
            break;
        case Ending:
            stanstring = "Ending";
            break;
        case LodzTEST:
            stanstring = "LodzTEST";
            break;
        case Wait:
            stanstring = "Wait";
            break;
        default:
            break;
        }

        // debug("zmienilem stan na: %s, LISTlodz: %s", stanstring.c_str(), stringLIST(LISTlodz).c_str());

        if (stan == Inactive) // stan nieaktywny
        {

            waitFor("0. zanim zaczne ubiegac sie o kucyka"); // oczekiwanie przez wylosowany czas i wypisanie komunikatu
            debug("1. ubiegam się o kostium kucyka");
            LISTkucyk.push_back(Request(rank, lamportClock + 1)); // dodanie żądania do kolejki związanej ze strojami kucyka
            kucykACKcount = 0;
            lamportPacket packetOut;
            lamportSend(touristsId, REQkucyk, &lamportClock, packetOut);
            changeState(Wait);
        }

        // if (stan == KucykWait) // pętla oczekująca, aż odebrane zostaną wszystkie zgody ACKkucyk
        // {
        // }

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
                changeState(Wait); // zmiana stanu na LodzWait
                lamportPacket packetOut;
                lamportSend(touristsId, REQlodz, &lamportClock, packetOut); // wysłanie żądania REQlodz
            }
        }

        if (stan == LodzTEST)
        {
            while (lodzieStan[wybieranaLodz] == 0)
            {
                wybieranaLodz = (wybieranaLodz + 1) % lodzCount; // zmiana numeru wybieranej łodzi, jeśli obecnie wybierana nie jest w stanie oczekiwania
            }

            while (LISTlodz.size() + turysciWycieczka < ponyCostumes)
            {
            }

            // debug("NADZORUJE TEST (%d): %s", turysciWycieczka, stringLIST(LISTlodz).c_str());
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
            debug("3. wyplywam na wycieczke w lodzi: %d, wycieczka to turysci: %s", packetOut.lodz, stringVector(wycieczka).c_str());
            lamportSend(touristsId, FULLlodz, &lamportClock, packetOut); // wysłanie komunikatu FULLłódź do wszystkich turystów
            for (int i = 0; i < size; i++)                               // przesłanie tablicy z wypływającymi turystami bez zwiększenia zegaru Lamporta (jako część jednej wiadomości wraz z FULLłódź)
            {
                MPI_Send(wycieczka.data(), (int)wycieczka.size(), MPI_INT, i, DATA, MPI_COMM_WORLD);
            }

            nadzorca = rank; // nadzorca to turysta, który wysyła komunikat FULLłódź i będzie wysyłać komunikat RELłódź
            changeState(Wycieczka);

            std::string test = "4. jestem na wycieczce z " + stringVector(wycieczka);

            waitFor(test.c_str()); // odczekanie losowo wyznaczonego czasu i wypisaniu komunikatu, kto z tym turystą płynie
            packetOut.lodz = wybieranaLodz;
            debug("5. wracam z wycieczki, zwalniam stroj kucyka");
            lamportSend(touristsId, RELlodz, &lamportClock, packetOut);  // po odczekaniu, wysłanie komunikatu RELlodz
            lamportSend(touristsId, RELkucyk, &lamportClock, packetOut); // wysłanie komunikatu RELkucyk
            changeState(Inactive);                                       // powrot do pierwszego stanu
            nadzorca = -1;
        }

        // if (stan == Wycieczka)
        // {
        //     if (nadzorca == rank) // jeśli turysta wysyłał komunikat FULLłódź
        //     {
        //         std::string test = "4. jestem na wycieczce z " + stringVector(wycieczka);

        //         waitFor(test.c_str()); // odczekanie losowo wyznaczonego czasu i wypisaniu komunikatu, kto z tym turystą płynie
        //         lamportPacket packetOut;
        //         packetOut.lodz = wybieranaLodz;
        //         debug("5. wracam z wycieczki, zwalniam stroj kucyka");
        //         lamportSend(touristsId, RELlodz, &lamportClock, packetOut);  // po odczekaniu, wysłanie komunikatu RELlodz
        //         lamportSend(touristsId, RELkucyk, &lamportClock, packetOut); // wysłanie komunikatu RELkucyk
        //         changeState(Inactive);                                       // powrot do pierwszego stanu
        //         nadzorca = -1;
        //     }
        // }

        if (stan == Ending) // zakończenie main_loop
        {
            debug("koncze");
            break;
        }
    }
}
