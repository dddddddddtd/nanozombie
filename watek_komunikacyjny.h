#include "main.h"

/* wątek komunikacyjny; zajmuje się odbiorem i reakcją na komunikaty */
void *startKomWatek(void *)
{

    MPI_Status status;

    lamportPacket packet;
    lamportPacket packetOut;
    Request req = Request(-1, -1);

    std::string test;
    std::vector<int> receivers;

    /* Obrazuje pętlę odbierającą pakiety o różnych typach */
    while (1)
    {
        // odbiór dowolnej wiadomości
        lamportReceive(&packet, MPI_ANY_SOURCE, MPI_ANY_TAG, &status, &lamportClock);
        // debug("ODEBRALEM (%d) od %d", status.MPI_TAG, status.MPI_SOURCE);

        // obsługa wiadomości o różnych tagach
        switch (status.MPI_TAG)
        {
        // obsługa REQkucyk
        case REQkucyk:
            // debug("REQkucyk od %d: lamport = %d", status.MPI_SOURCE, packet.lamportClock);
            req = Request(status.MPI_SOURCE, packet.lamportClock);

            if (status.MPI_SOURCE == rank || req < kucyk)
            {
                // debug("wysylam ACK do %d", status.MPI_SOURCE);
                LISTkucykOK.push_back(status.MPI_SOURCE);
                receivers = std::vector<int>(1, status.MPI_SOURCE);
                pthread_mutex_lock(&lamportMut);
                lamportSend(receivers, ACKkucyk, &lamportClock, packetOut); // odesłanie do nadawcy potwierdzenia ACKkucyk
                pthread_mutex_unlock(&lamportMut);
            }
            else
            {
                // debug("moje: [%d, %d], %d: [%d, %d]", kucyk.processid, kucyk.lamportClock, status.MPI_SOURCE, req.processid, req.lamportClock);
                // debug("nie wysylam ACK do %d", status.MPI_SOURCE);
                LISTkucykHALT.push_back(status.MPI_SOURCE);
            }

            break;

        // obsługa ACKkucyk
        case ACKkucyk:
            kucykACKcount++; // zwiększenie liczby potwierdzeń dotyczących stroju kucyka
            // debug("odebralem %d/%d ACKkucyk od %d", kucykACKcount, touristCount, status.MPI_SOURCE);
            if (kucykACKcount == touristCount) // w momencie uzyskania potwierdzeń od wszystkich turystów
            {
                // debug("wszystkie potwierdzenia, KucykQ");
                changeState(KucykQ); //zmiana stanu na KucykQ
            }
            break;

        // obsługa RELkucyk
        case RELkucyk:
            LISTkucykOK.erase(std::remove(LISTkucykOK.begin(), LISTkucykOK.end(), status.MPI_SOURCE), LISTkucykOK.end());
            if (status.MPI_SOURCE == rank)
            {
                kucyk.processid = -1;
                pthread_mutex_lock(&lamportMut);
                lamportSend(LISTkucykHALT, ACKkucyk, &lamportClock, packetOut);
                pthread_mutex_unlock(&lamportMut);
                LISTkucykHALT.clear();
                changeState(Inactive);
            }

            break;
        }
    }
    return EXIT_SUCCESS;
}
