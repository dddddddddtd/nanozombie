#include "main.h"

void printLISTkucyk(int index)
{
    std::string res = "";
    for (int i = 0; i < LISTkucyk.size(); i++)
    {
        res += "[" + std::to_string(LISTkucyk[i].processid) + ", " + std::to_string(LISTkucyk[i].lamportClock) + "] ";
    }
    debug("%s %d", res.c_str(), index);
}

void mainLoop()
{
    while (1)
    {
        if (stan == Inactive)
        {
            waitFor(0, 15, "zanim zaczne ubiegac sie o kucyka");

            debug("Ubiegam się o kostium kucyka: moj lamport = %d", lamportClock + 1);
            ponyACKcount = 0;
            changeState(PonyWait);

            std::vector<int> receivers;
            for (int i = 0; i < size; i++)
                receivers.push_back(i);

            lamportSend(receivers, REQkucyk, &lamportClock);

            debug("Czekam na zgody na kucyka");
        }
        if (stan == PonyWait)
        {
        }

        if (stan == PonyQ)
        {
            //
            std::sort(LISTkucyk.begin(), LISTkucyk.end());
            int index = std::distance(LISTkucyk.begin(), std::find(LISTkucyk.begin(), LISTkucyk.end(), rank));
            if (index < ponyCostumes - 1)
            {
                printLISTkucyk(index);
                changeState(Pony);
            }
            //
        }

        if (stan == Pony)
        {
            waitFor(2, 10, "mam kucyka");

            std::vector<int> receivers;
            for (int i = 0; i < size; i++)
                receivers.push_back(i);

            lamportSend(receivers, RELkucyk, &lamportClock);
        }
    }
}
