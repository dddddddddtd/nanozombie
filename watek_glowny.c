#include "main.h"
#include "watek_glowny.h"

void mainLoop()
{
    while (1)
    {
        if (stan == Inactive)
        {
            int perc = random() % 100;

            if (perc < STATE_CHANGE_PROB)
            {
                debug("Ubiegam się o kostium kucyka");
                changeState(PonyQ);

                for (int i = 0; i < size; i++)
                {
                    //nie wiem czy wysyłać do siebie
                    //chyba tak i wtedy w obsłudze requesta trzeba sprawdzac czy wiadomosc od siebie czy nie i jesli od siebie to wysylac ack bez zadnego dalszego sprawdzania

                    //tu wysylac REQpony, wiec chyba w lamportSend nie potrzebujemy tekstu wiadomosci,
                    //a zamiast tego MESSAGE_TAG i wszystko bedzie robione na podstawie clocka, tagu i nadawcy

                    lamportClock = lamportSend(*rank, i, REQkucyk, &lamportClock);
                }

                for (int i = 0; i < size; i++)
                {
                    //na razie byle jak odbior tutaj zamiast w komunikacyjnym
                    //i czekamy na potwierdzenie od kazdego
                    //tutaj dest chyba niepotrzebne w sumie
                    lamporPacket packet;
                    MPI_Status status;
                    lamportReceive(&packet, i, ACKkucyk, &status, &lamportClock);
                    debug("odebralem ack od turysty %d", i);
                }
                changeState(Pony);
            }
        }
        if (stan == Pony)
        {
            sleep(SEC_IN_STATE);
            changeState(Inactive);
        }
    }
}