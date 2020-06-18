#include "main.h"

void mainLoop()
{
    while (1)
    {
        if (stan == Inactive)
        {
            waitFor(0, 10, "zanim zaczne ubiegac sie o kucyka");

            debug("Ubiegam się o kostium kucyka");
            kucykACKcount = 0;
            changeState(KucykWait);
            lamportPacket packetOut;
            lamportSend(touristsId, REQkucyk, &lamportClock, packetOut);
        }

        if (stan == KucykWait)
        {
        }

        if (stan == KucykQ)
        {
            //
            std::sort(LISTkucyk.begin(), LISTkucyk.end());
            int index = std::distance(LISTkucyk.begin(), std::find(LISTkucyk.begin(), LISTkucyk.end(), rank));
            if (index < ponyCostumes)
            {
                debug("biore stroj kucyka");
                changeState(Kucyk);
            }
            //
        }

        if (stan == Kucyk)
        {
            debug("ubiegam się o łódź");
            lodzACKcount = 0;
            changeState(LodzWait);
            lamportPacket packetOut;
            lamportSend(touristsId, REQlodz, &lamportClock, packetOut);
        }

        if (stan == LodzWait)
        {
        }

        if (stan == LodzQ)
        {
            if (LISTlodz.size() >= ponyCostumes)
            {
                if (lodzieStan[wybieranaLodz] != 0)
                {
                    std::sort(LISTlodz.begin(), LISTlodz.end());
                    int index = std::distance(LISTlodz.begin(), std::find(LISTlodz.begin(), LISTlodz.end(), rank));
                    if (index == 0)
                    {
                        debug("jestem pierwszy w kolejce do lodzi");
                        wycieczka.clear();
                        int suma = 0;
                        for (int i = 0; i < LISTlodz.size(); i++)
                        {
                            suma += tourists[LISTlodz[i].processid];

                            if (suma <= lodziePojemnosc[wybieranaLodz])
                            {
                                wycieczka.push_back(LISTlodz[i].processid);
                            }

                            if (suma > lodziePojemnosc[wybieranaLodz])
                            {
                                suma -= tourists[LISTlodz[i].processid];
                                continue;
                            }
                        }

                        lamportPacket packetOut;
                        packetOut.count = wycieczka.size();
                        packetOut.lodz = wybieranaLodz;
                        debug("wysylam komunikat FULLlodz");
                        lamportSend(touristsId, FULLlodz, &lamportClock, packetOut);
                        for (int i = 0; i < size; i++)
                        {   
                            MPI_Send(wycieczka.data(), (int) wycieczka.size(), MPI_INT, i, DATA, MPI_COMM_WORLD);
                        }
                        debug("zmieniam stan na wycieczke");
                        nadzorca=rank;
                        changeState(Wycieczka);
                    }
                }
                else{
                    wybieranaLodz = (wybieranaLodz + 1) % lodzCount;
                }
            }
        }

        if (stan == Wycieczka)
        {
            if (nadzorca == rank)
            {
                std::string test = "wyplywam razem z [";
                for (int i = 0; i < wycieczka.size(); i++)
                {
                    test += std::to_string(wycieczka[i]) + ", ";
                }
                test = test.substr(0, test.size() - 2);
                test += "]";
                waitFor(5, 10, test.c_str());
                lamportPacket packetOut;
                debug("wysylam RELlodz: %d size: %d", RELlodz, (int) touristsId.size());
                lamportSend(touristsId, RELlodz, &lamportClock, packetOut);
                changeState(Inactive);
            }
        }
    }
}
