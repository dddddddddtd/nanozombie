#include "main.h"
// #include "utils.h"
void mainLoop()
{
    while (1)
    {
        if (stan == Inactive)
        {
            int perc = random() % 100;

            if (perc < STATE_CHANGE_PROB)
            {
                if (LISTkucykOK.size() < ponyCostumes)
                {
                    debug("Ubiegam się o kostium kucyka");
                    ponyACKcount = 0;
                    changeState(PonyQ);
                    ponyQclock = lamportClock;

                    for (int i = 0; i < size; i++)
                    {
                        lamportSend(rank, i, REQkucyk, &lamportClock);
                    }
                    debug("Czekam na zgody na kucyka");
                }
            }
        }

        if (stan == PonyQ) 
        {
        }

        if (stan == Pony)
        {
            sleep(SEC_IN_STATE*2000);
            for (int i = 0; i < size; i++)
            {
                lamportSend(rank, i, RELkucyk, &lamportClock);
            }

            for(int i=0; i<LISTkucykHALT.size(); i++){
                lamportSend(rank, LISTkucykHALT[i], ACKkucyk, &lamportClock);
            }

            debug("Zwalniam stroj kucyka");
            changeState(Inactive);
        }
        
        if(stan==SubQ){

        }
    }
}