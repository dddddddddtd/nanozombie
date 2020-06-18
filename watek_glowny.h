#include "main.h"

void mainLoop()
{
    while (1)
    {
        if (stan == Inactive)
        {
            waitFor(0, 10, "zanim zaczne ubiegac sie o kucyka");

            debug("Ubiegam się o kostium kucyka: moj lamport = %d", lamportClock + 1);
            kucykACKcount = 0;
            changeState(KucykWait);
            lamportSend(touristsId, REQkucyk, &lamportClock);

            debug("Czekam na zgody na kucyka");
        }
        if (stan == KucykWait)
        {
        }

        if (stan == KucykQ)
        {
            //
            std::sort(LISTkucyk.begin(), LISTkucyk.end());
            int index = std::distance(LISTkucyk.begin(), std::find(LISTkucyk.begin(), LISTkucyk.end(), rank));
            if (index < ponyCostumes - 1)
            {
                debug("kucyk: %s", stringLIST(LISTkucyk).c_str());
                changeState(Kucyk);
            }
            //
        }

        if (stan == Kucyk)
        {
            debug("otrzymałem kucyka, ubiegam się o łódź");
            lodzACKcount = 0;
            changeState(LodzWait);
            lamportSend(touristsId, REQlodz, &lamportClock);
            debug("Czekam na potwierdzenia o lodzi kucyka");
        }

        if (stan == LodzWait)
        {
        }

        if (stan == LodzQ)
        {
            if (LISTlodz.size() >= ponyCostumes)
            {
                //if (lodzie[wybieranaLodz]!=0)
                std::sort(LISTlodz.begin(), LISTlodz.end());
                int index = std::distance(LISTlodz.begin(), std::find(LISTlodz.begin(), LISTlodz.end(), rank));
                if (index == 0)
                {
                    std::vector<int> wycieczka;
                    int suma = 0;
                    for (int i = 0; i < LISTlodz.size(); i++)
                    {
                        suma += tourists[LISTlodz[i].processid];

                        if (suma < submarines[wybieranaLodz])
                        {
                            wycieczka.push_back(LISTlodz[i].processid)
                        }

                        if (suma > submarines[wybieranaLodz])
                        {
                            suma -= tourists[LISTlodz[i].processid];
                            continue;
                            // std::vector<int> test(touristsId.cbegin() + 0, touristsId.cbegin() + i - 1 + 1);
                            // printArray(&rank, test.data(), (int)test.size(), "test");
                            // break;
                        }
                    }

                    lamportSend(touristsId, FULLlodz, &lamportClock);
                    for (int i = 0; i < size; i++)
                    {
                        MPI_Send(wycieczka.data(), wycieczka.size(), MPI_INT, i, INIT, MPI_COMM_WORLD);
                    }
                }
            }

            //sprawdzenie czy w turysci w kolejce turystow sa w stanie wypelnic jedna lodz
        }

        if (stan == Ending)
        {
            waitFor(2, 10, "mam kucyka");

            // std::vector<int> receivers;
            // for (int i = 0; i < size; i++)
            //     receivers.push_back(i);

            lamportSend(touristsId, RELkucyk, &lamportClock);
        }
    }
}
