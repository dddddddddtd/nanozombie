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
                receivers = std::vector<int>(1, status.MPI_SOURCE);
                packetOut.answerto = req.lamportClock;
                pthread_mutex_lock(&lamportMut);
                lamportSend(receivers, ACKkucyk, &lamportClock, packetOut); // odesłanie do nadawcy potwierdzenia ACKkucyk
                pthread_mutex_unlock(&lamportMut);
            }
            else
            {
                // debug("moje: [%d, %d], %d: [%d, %d]\nnie wysylam ACK do %d", kucyk.processid, kucyk.lamportClock, status.MPI_SOURCE, req.processid, req.lamportClock, status.MPI_SOURCE);
                LISTkucykHALT.push_back(req);
            }

            break;

        // obsługa ACKkucyk
        case ACKkucyk:
            if (stan == Inactive && packet.answerto == kucyk.lamportClock)
            {
                kucykACKcount++; // zwiększenie liczby potwierdzeń dotyczących stroju kucyka
                debug("dostalem ACKkucyk %d/%d od %d z answerto: %d", kucykACKcount, (touristCount - ponyCostumes + 1), status.MPI_SOURCE, packet.answerto);
                if (kucykACKcount >= touristCount - ponyCostumes + 1) // w momencie uzyskania potwierdzeń od wszystkich turystów
                {
                    // debug("wszystkie potwierdzenia, KucykQ");
                    changeState(KucykQ); //zmiana stanu na KucykQ
                }
            }

            // debug("odebralem %d/%d ACKkucyk od %d", kucykACKcount, touristCount, status.MPI_SOURCE);

            break;

        // obsługa RELkucyk
        case RELkucyk:
            if (status.MPI_SOURCE == rank)
            {
                kucyk.processid = -1;
                kucyk.lamportClock = -1;
                pthread_mutex_lock(&lamportMut);
                lamportSendRequest(LISTkucykHALT, ACKkucyk, &lamportClock, packetOut);
                pthread_mutex_unlock(&lamportMut);
                LISTkucykHALT.clear();
                changeState(Inactive);
            }

            break;
        case REQlodz:
            req = Request(status.MPI_SOURCE, packet.lamportClock);

            if (status.MPI_SOURCE == rank || req < lodz)
            {
                // debug("wysylam ACK do %d", status.MPI_SOURCE);
                // LISTlodzOK.push_back(req);
                receivers = std::vector<int>(1, status.MPI_SOURCE);
                packetOut.answerto = req.lamportClock;
                pthread_mutex_lock(&lamportMut);
                lamportSend(receivers, ACKlodz, &lamportClock, packetOut); // odesłanie do nadawcy potwierdzenia ACKkucyk
                pthread_mutex_unlock(&lamportMut);
            }
            else
            {
                // debug("moje: [%d, %d], %d: [%d, %d]\nnie wysylam ACK do %d", kucyk.processid, kucyk.lamportClock, status.MPI_SOURCE, req.processid, req.lamportClock, status.MPI_SOURCE);
                LISTlodzHALT.push_back(req);
            }

            break;

        case ACKlodz:
            if (stan == KucykQ && packet.answerto == lodz.lamportClock)
            {
                lodzACKcount++; // zwiększenie liczby potwierdzeń dotyczących stroju kucyka
                debug("dostalem ACKlodz %d/%d od %d z answerto: %d", lodzACKcount, touristCount, status.MPI_SOURCE, packet.answerto);
                if (lodzACKcount == touristCount) // w momencie uzyskania potwierdzeń od wszystkich turystów
                {
                    // debug("wszystkie potwierdzenia, KucykQ");
                    changeState(LodzQ); //zmiana stanu na KucykQ
                }
            }

            // debug("odebralem %d/%d ACKkucyk od %d", kucykACKcount, touristCount, status.MPI_SOURCE);

            break;
        case FULLlodz:
        {

            int liczbaOdplywajacych = packet.count;
            lodzieStan[packet.lodz] = 0; // zmiana stanu łodzi na wypłyniętą
            int odplywajace[liczbaOdplywajacych];

            MPI_Recv(odplywajace, liczbaOdplywajacych, MPI_INT, status.MPI_SOURCE, DATA, MPI_COMM_WORLD, &status);

            debug("dostalem FULLlodz od %d: %s", status.MPI_SOURCE, stringArray(odplywajace, liczbaOdplywajacych).c_str());

            if (checkIfInArray(odplywajace, liczbaOdplywajacych, rank)) // jeśli turysta odpływa tą łodzią
            {
                debug("bede wysylac ACKlodz do: %s", stringLIST(LISTlodzHALT).c_str());
                nadzorca = status.MPI_SOURCE; // ustawienie nadzorcy na nadawcę
                lodz.processid = -1;
                lodz.lamportClock = -1;
                changeState(Wycieczka);
                pthread_mutex_lock(&lamportMut);
                lamportSendRequest(LISTlodzHALT, ACKlodz, &lamportClock, packetOut);
                pthread_mutex_unlock(&lamportMut);
                LISTlodzHALT.clear();
            }
            break;
        }

        case RELlodz:
            lodzieStan[packet.lodz] = 1; // zmiana stanu łodzi na wypłyniętą

            if (nadzorca == status.MPI_SOURCE) // jeśli brał udział w wycieczce nadawcy   && rank != nadzorca
            {
                nadzorca = -1; // ustawia id nadzorcy na -1
                pthread_mutex_lock(&lamportMut);
                lamportSend(touristsId, RELkucyk, &lamportClock, packetOut); // zwalnia kucyka
                pthread_mutex_unlock(&lamportMut);
            }
            break;
        }
    }
    return EXIT_SUCCESS;
}
