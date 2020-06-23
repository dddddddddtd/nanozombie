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

        // obsługa wiadomości o różnych tagach
        switch (status.MPI_TAG)
        {
        // obsługa REQkucyk - otrzymanie prośby na dostęp do stroju kucyka
        case REQkucyk:
            // request nadawcy
            req = Request(status.MPI_SOURCE, packet.lamportClock);
            // jeśli nadawca to ja lub nadawca ma wcześniejszy request 
            if (status.MPI_SOURCE == rank || req < kucyk)
            {
                receivers = std::vector<int>(1, status.MPI_SOURCE); //nadawca
                packetOut.answerto = req.lamportClock;
                pthread_mutex_lock(&lamportMut);
                // odesłanie do nadawcy potwierdzenia ACKkucyk
                lamportSend(receivers, ACKkucyk, &lamportClock, packetOut); 
                pthread_mutex_unlock(&lamportMut);
            }
            // jeśli nadawca ma późniejszy priorytet do dodaje go do listy oczekujących
            else
            {
                LISTkucykHALT.push_back(req);
            }
            break;

        // obsługa ACKkucyk - otzymanie zgody na dostęp do stroju kucyka
        case ACKkucyk:
            if (stan == Inactive && packet.answerto == kucyk.lamportClock)
            {
                // zwiększenie liczby potwierdzeń dotyczących stroju kucyka
                kucykACKcount++; 
                debug("dostalem ACKkucyk %d/%d od %d z answerto: %d", kucykACKcount, (touristCount - ponyCostumes + 1), status.MPI_SOURCE, packet.answerto);
                // jeśli uzyskałem już wystarczającą liczbę zgód
                if (kucykACKcount >= touristCount - ponyCostumes + 1) 
                {
                     //zmiana stanu na KucykQ
                    changeState(KucykQ);
                }
            }
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

        // obsługa REQlodz - otrzymanie prośby na dostęp do łodzi
        case REQlodz:
            // stworzenie requesta nadawcy 
            req = Request(status.MPI_SOURCE, packet.lamportClock);
            // jeśli nadawca to ja lub nadawca ma wcześniejszy request 
            if (status.MPI_SOURCE == rank || req < lodz)
            {
                receivers = std::vector<int>(1, status.MPI_SOURCE);
                packetOut.answerto = req.lamportClock;
                pthread_mutex_lock(&lamportMut);
                 // odesłanie do nadawcy potwierdzenia ACKlodz
                lamportSend(receivers, ACKlodz, &lamportClock, packetOut);
                pthread_mutex_unlock(&lamportMut);
            }
            // jeśli nadawca ma późniejszy priorytet do dodaje go do listy oczekujących na łódź
            else
            {
                LISTlodzHALT.push_back(req);
            }

            break;
        // obsługa ACKlodz - otrzymanie zgody na dostęp do łodzi
        case ACKlodz:
            // jeśli jestem w KucykQ (stan ubiegania się o łódź) i wiadomość jest do mnie (answerto)
            if (stan == KucykQ && packet.answerto == lodz.lamportClock)
            {
                // zwiększenie liczby potwierdzeń dostępu do łódzi
                lodzACKcount++; 
                debug("dostalem ACKlodz %d/%d od %d z answerto: %d", lodzACKcount, touristCount, status.MPI_SOURCE, packet.answerto);
                 // jeśli otrzymał zgody od wszystkich turystów
                if (lodzACKcount == touristCount)
                {
                    changeState(LodzQ); //zmiana stanu na lodzQ
                }
            }
            break;
        // obsługa FULLlodz - info że łódz wypływa
        case FULLlodz:
        {
            int liczbaOdplywajacych = packet.count;
            // zmiana stanu łodzi na wypłyniętą
            lodzieStan[packet.lodz] = 0; 
            int odplywajace[liczbaOdplywajacych];

            MPI_Recv(odplywajace, liczbaOdplywajacych, MPI_INT, status.MPI_SOURCE, DATA, MPI_COMM_WORLD, &status);

            // debug("dostalem FULLlodz od %d: %s", status.MPI_SOURCE, stringArray(odplywajace, liczbaOdplywajacych).c_str());
            // jeśli turysta odpływa tą łodzią
            if (checkIfInArray(odplywajace, liczbaOdplywajacych, rank))
            {
                debug("bede wysylac ACKlodz do: %s", stringLIST(LISTlodzHALT).c_str());
                // ustawienie nadzorcy na nadawcę
                nadzorca = status.MPI_SOURCE; 
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
