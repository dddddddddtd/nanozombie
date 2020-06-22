#include "main.h"

void mainLoop()
{
    int licznik = 0;
    while (1)
    {
        // debug("licznik: %d, stan: %d", licznik, stan);
        licznik++;
        while (licznik > 4)
        {
        }

        // debug("%d", stan);
        if (stan == Inactive) // stan nieaktywny
        {
            // waitFor("zanim zaczne ubiegac sie o kucyka"); // oczekiwanie przez wylosowany czas i wypisanie komunikatu

            debug("1. ubiegam się o kostium kucyka");
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

            debug("2. biore stroj kucyka");
            // waitFor("zanim zwolnie kucyka");
            debug("3. zwracam stroj kucyka");
            lamportPacket packetOut;
            pthread_mutex_lock(&lamportMut);
            lamportSend(touristsId, RELkucyk, &lamportClock, packetOut);
            pthread_mutex_unlock(&lamportMut);

            while (stan == KucykQ)
            {
            }
        }
        if (stan == Ending)
        {
            debug("koncze");
            break;
        }
    }
}
