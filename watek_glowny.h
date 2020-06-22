#include "main.h"

void mainLoop()
{
    int licznik = 0;
    while (1)
    {
        // licznik++;
        // while (licznik > 4)
        // {
        // }

        // debug("%d", stan);
        if (stan == Inactive) // stan nieaktywny
        {
            // waitFor("zanim zaczne ubiegac sie o kucyka"); // oczekiwanie przez wylosowany czas i wypisanie komunikatu
            debug("1. ubiegam się o kostium kucyka");
            // debug("1. ubiegam się o kostium kucyka");
            kucykACKcount = 0;

            pthread_mutex_lock(&lamportMut);
            lamportPacket packetOut;
            kucyk = Request(rank, lamportClock + 1);
            lamportSend(touristsId, REQkucyk, &lamportClock, packetOut);
            pthread_mutex_unlock(&lamportMut);

            while (stan == Inactive)
            {
                //czekam na
            }
        }

        if (stan == KucykQ) // stan oczekiwania, aż proces będzie wśród ponyCostumes pierwszych procesów w kolejce dotyczącej kostiumu kucyka
        {

            debug("2. biore stroj kucyka  i ubiega sie o lodz");
            // waitFor("3. czekam przed zwolnieniem stroju");
            // waitFor("3. czekam jeszcze dluzej");

            lodzACKcount = 0;

            pthread_mutex_lock(&lamportMut);
            lamportPacket packetOut;
            lodz = Request(rank, lamportClock + 1);
            lamportSend(touristsId, REQlodz, &lamportClock, packetOut);
            pthread_mutex_unlock(&lamportMut);

            while (stan == KucykQ)
            {
            }
        }

        if (stan == LodzQ)
        {
            debug("czekam na pozostalych turystow");
            int licz = 0;
            while (LISTlodzHALT.size() < ponyCostumes - 1) // czekam, az wszyscy turysci w strojach kucyka beda ubiegac sie o lodz
            {   
                // waitFor("czekam na wypelnienie LISTlodzHALT");
                // debug("licz %d: %d < %d", licz, (int)LISTlodzHALT.size(), ponyCostumes - 1);
                // licz++;
            }
            debug("czekam na wolna lodz");

            while (lodzieStan[wybieranaLodz] == 0) // znajduje wolna lodz
            {

                wybieranaLodz = (wybieranaLodz + 1) % lodzCount;
            }

            debug("wybralem lodz");

            wycieczka.clear();

            int suma = tourists[rank];
            wycieczka.push_back(rank);

            for (int i = 0; i < LISTlodzHALT.size(); i++)
            {
                suma += tourists[LISTlodzHALT[i].processid];

                if (suma <= lodziePojemnosc[wybieranaLodz])
                {
                    wycieczka.push_back(LISTlodzHALT[i].processid); // dodanie turysty do wektora turystów wypływających, jeśli mieści się on w łodzi
                }

                if (suma > lodziePojemnosc[wybieranaLodz])
                {
                    suma -= tourists[LISTlodzHALT[i].processid];
                    continue;
                }
            }

            debug("bede jechac na wycieczke z %s w lodzi %d", stringVector(wycieczka).c_str(), wybieranaLodz);
            lamportPacket packetOut;
            packetOut.count = wycieczka.size(); // liczba turystów wypływających
            packetOut.lodz = wybieranaLodz;     // indeks łodzi wypływającej
            pthread_mutex_lock(&lamportMut);
            lamportSend(touristsId, FULLlodz, &lamportClock, packetOut); // wysłanie komunikatu FULLłódź do wszystkich turystów
            pthread_mutex_unlock(&lamportMut);
            for (int i = 0; i < size; i++) // przesłanie tablicy z wypływającymi turystami bez zwiększenia zegaru Lamporta (jako część jednej wiadomości wraz z FULLłódź)
            {
                MPI_Send(wycieczka.data(), (int)wycieczka.size(), MPI_INT, i, DATA, MPI_COMM_WORLD);
            }

            while (stan == LodzQ)
            {
            }
        }

        if (stan == Wycieczka)
        {
            debug("jestem na wycieczce z %d", nadzorca) if (nadzorca == rank)
            {
                pthread_mutex_lock(&lamportMut);
                lamportPacket packetOut;
                debug("wysylam RELlodz");
                lamportSend(touristsId, RELlodz, &lamportClock, packetOut); //na sam koniec
                pthread_mutex_unlock(&lamportMut);
            }

            while (stan == Wycieczka)
            {
            }
            debug("koncze wycieczke");
        }
    }
}
