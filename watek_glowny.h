#include "main.h"

void mainLoop()
{
    int licznik = 0;
    while (1)
    {
        // stan nieaktywny,  początkowy - ubieganie sie o kostium kucyka
        if (stan == Inactive)
        {
            debug("1. ubiegam się o kostium kucyka");
            // zerowanie liczby zgód na kostium kucyka
            kucykACKcount = 0;

            pthread_mutex_lock(&lamportMut);
            lamportPacket packetOut;
            // stworzenie requesta
            kucyk = Request(rank, lamportClock + 1);
            // wysłanie REQkucyk do wszystkich procesów
            lamportSend(touristsId, REQkucyk, &lamportClock, packetOut);
            pthread_mutex_unlock(&lamportMut);

            while (stan == Inactive)
            {
                //czekam na zmianę stanu tu
            }
        }

        // stan, w którym proces bierze stan kostiym kucyka i zaczyna ubiegać się dostęp do łodzi
        if (stan == KucykQ)
        {
            
            debug("2. biore stroj kucyka  i ubiega sie o lodz");
            // zerowanie liczby zgód na dostęp do łodzi
            lodzACKcount = 0;

            pthread_mutex_lock(&lamportMut);
            lamportPacket packetOut;
            // stworzenie requesta
            lodz = Request(rank, lamportClock + 1);
            // wysłanie REQlodz do wszystkich procesów
            lamportSend(touristsId, REQlodz, &lamportClock, packetOut);
            pthread_mutex_unlock(&lamportMut);

            while (stan == KucykQ)
            {
                // czekam na zmianę stanu tu
            }
        }

        if (stan == LodzQ)
        {
            debug("czekam na pozostalych turystow");
            while (LISTlodzHALT.size() < ponyCostumes - 1)
            { 
                // czekam, az wszyscy turysci w strojach kucyka beda ubiegac sie o lodz 
                // LISTlodzHALT - procesy, które czekają na dostęp do łodzi, ale mają niższy priorytet ode mnie
            }
            debug("czekam na wolna lodz");
            // iterowanie się po łodziach, sprawdzanie czy któraś nie jest dostępna (1)
            while (lodzieStan[wybieranaLodz] == 0)
            {
                wybieranaLodz = (wybieranaLodz + 1) % lodzCount;
            }
            debug("wybralem lodz");

            // tworzenie nowej wycieczki
            wycieczka.clear();

            // suma sumuje rozmiar turystów na łodzi
            // pierwszy dołącza rank
            int suma = tourists[rank];
            wycieczka.push_back(rank);

            // sortowanie procesów
            std::sort(LISTlodzHALT.begin(), LISTlodzHALT.end());

            //iteracja po procesach oczekujących
            for (int i = 0; i < LISTlodzHALT.size(); i++)
            {
                suma += tourists[LISTlodzHALT[i].processid];
                // jesli dany proces-turysta się mieści to jest dodawany
                if (suma <= lodziePojemnosc[wybieranaLodz])
                {
                    // jest dodawany do wycieczki
                    wycieczka.push_back(LISTlodzHALT[i].processid);
                }

                if (suma > lodziePojemnosc[wybieranaLodz])
                {
                    // a jesli nie to jest usuwany
                    suma -= tourists[LISTlodzHALT[i].processid];
                    continue;
                }
            }
            // wycieczka przygotowana
            debug("bede jechac na wycieczke z %s w lodzi %d", stringVector(wycieczka).c_str(), wybieranaLodz);
            lamportPacket packetOut;
            // liczba turystów wypływających
            packetOut.count = wycieczka.size();
            // indeks łodzi wypływającej 
            packetOut.lodz = wybieranaLodz;
            pthread_mutex_lock(&lamportMut);
             // wysłanie komunikatu FULLłódź do wszystkich turystów
            lamportSend(touristsId, FULLlodz, &lamportClock, packetOut);
            pthread_mutex_unlock(&lamportMut);
            // przesłanie tablicy z wypływającymi turystami bez zwiększenia zegaru Lamporta (jako część jednej wiadomości wraz z FULLłódź)
            for (int i = 0; i < size; i++) 
            {
                MPI_Send(wycieczka.data(), (int)wycieczka.size(), MPI_INT, i, DATA, MPI_COMM_WORLD);
            }

            while (stan == LodzQ)
            {
                // czekam tu na zmianę stanu
            }
        }

        if (stan == Wycieczka)
        {
            debug("jestem na wycieczce z %d", nadzorca); 
            if (nadzorca == rank)
            {
                pthread_mutex_lock(&lamportMut);
                lamportPacket packetOut;
                packetOut.lodz=wybieranaLodz;
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
