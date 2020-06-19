#include "main.h"
#include "utils.h"
#include "watek_komunikacyjny.h"
#include "watek_glowny.h"

// zmienne extern
int rank, size, touristCount, ponyCostumes, lodzCount, touristRangeFrom, touristRangeTo, submarineRangeFrom, submarineRangeTo, lamportClock = 0;
int kucykACKcount, lodzACKcount;
int wybieranaLodz;
int nadzorca;
int turysciWycieczka = 0;

std::vector<Request> LISTkucyk, LISTlodz;
std::vector<int> tourists, lodziePojemnosc, touristsId, wycieczka, lodzieStan;

// wątek komunikacyjny, mutexy i stan
pthread_t threadKom;
pthread_mutex_t stateMut = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t lamportMut = PTHREAD_MUTEX_INITIALIZER;
state_t stan = Inactive;

// lamport
MPI_Datatype mpiLamportPacket;

// wysyłanie komunikatów
void lamportSend(std::vector<int> receivers, int tag, int *lamportClock, lamportPacket packetOut)
{
    //zwiększenie wartości zegaru Lamporta i wpisanie jej do pakietu
    pthread_mutex_lock(&lamportMut);
    (*lamportClock)++;
    packetOut.lamportClock = *lamportClock;
    pthread_mutex_unlock(&lamportMut);

    // wysłanie wiadomości do wszystkich procesów, których id znajduje się w wektorze receivers
    for (int i = 0; i < receivers.size(); i++)
    {
        MPI_Send(&packetOut, 1, mpiLamportPacket, receivers[i], tag, MPI_COMM_WORLD);
    }
}

// odbieranie komunikatów
int lamportReceive(lamportPacket *packetIn, int src, int tag, MPI_Status *status, int *lamportClock)
{
    int result = MPI_Recv(packetIn, 1, mpiLamportPacket, src, tag, MPI_COMM_WORLD, status);

    //zwiększenie wartości zegaru Lamporta po odebraniu
    pthread_mutex_lock(&lamportMut);
    *lamportClock = max(*lamportClock, packetIn->lamportClock) + 1;
    pthread_mutex_unlock(&lamportMut);
    return result;
}

// zmiana stanu
void changeState(state_t newState)
{
    pthread_mutex_lock(&stateMut);
    stan = newState;
    pthread_mutex_unlock(&stateMut);
}

// funkcja pomocnicza do wypisywania zawartości kolejek
std::string stringLIST(std::vector<Request> LIST)
{
    std::string res = "";
    for (int i = 0; i < LIST.size(); i++)
    {
        res += "[" + std::to_string(LIST[i].processid) + ", " + std::to_string(LIST[i].lamportClock) + "] ";
    }
    return res;
}

// funkcja pomocnicza sprawdzająca, czy id danego procesu znajduje sie w tablicy a o rozmiarze size
bool checkIfInArray(int a[], int size, int val)
{
    for (int i = 0; i < size; i++)
    {
        if (a[i] == val)
            return true;
    }
    return false;
}

void inicjuj(int argc, char **argv)
{
    int provided;
    MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
    // check_thread_support(provided);

    // konfiguracja structa dla MPI
    MPI_Datatype types[4] = {MPI_INT, MPI_INT, MPI_INT, MPI_INT};
    int blocklengths[4] = {
        1,
        1,
        1,
        1,
    };
    MPI_Aint offsets[4];
    offsets[0] = offsetof(lamportPacket, lamportClock);
    offsets[1] = offsetof(lamportPacket, src);
    offsets[2] = offsetof(lamportPacket, count);
    offsets[3] = offsetof(lamportPacket, lodz);
    MPI_Type_create_struct(4, blocklengths, offsets, types, &mpiLamportPacket);
    MPI_Type_commit(&mpiLamportPacket);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Status status;

    srand(time(NULL) * rank);

    touristCount = size;
    if (argc != 7) // w razie brakujących argumentów - wartości domyślne
    {
        ponyCostumes = 2;
        lodzCount = 2;
        touristRangeFrom = 3;
        touristRangeTo = 3;
        submarineRangeFrom = 3;
        submarineRangeTo = 4;
    }
    else
    {
        ponyCostumes = atoi(argv[1]);
        lodzCount = atoi(argv[2]);
        touristRangeFrom = atoi(argv[3]);
        touristRangeTo = atoi(argv[4]);
        submarineRangeFrom = atoi(argv[5]);
        submarineRangeTo = atoi(argv[6]);
    }

    if (rank == ROOT)
    {
        // wypisanie ustalonych wartości
        printf("tourists: %d\nponyCostumes: %d\nsubmarines: %d\n", touristCount, ponyCostumes, lodzCount);
        printf("tourist range: %d-%d\n", touristRangeFrom, touristRangeTo);
        printf("submarine range: %d-%d\n", submarineRangeFrom, submarineRangeTo);

        // inicjalizacja turystów
        for (int i = 0; i < touristCount; i++)
        {
            tourists.push_back(getRandom(touristRangeFrom, touristRangeTo));
        }

        // inicjalizacja łodzi
        for (int i = 0; i < lodzCount; i++)
        {
            lodziePojemnosc.push_back(getRandom(submarineRangeFrom, submarineRangeTo));
        }

        // wysłanie danych do wszystkich procesów
        for (int i = 1; i < size; i++)
        {
            MPI_Send(tourists.data(), touristCount, MPI_INT, i, DATA, MPI_COMM_WORLD);
            MPI_Send(lodziePojemnosc.data(), lodzCount, MPI_INT, i, DATA, MPI_COMM_WORLD);
        }

        printArray(&rank, tourists.data(), touristCount, "turysci");
        printArray(&rank, lodziePojemnosc.data(), lodzCount, "lodzie");
    }

    // każdy proces odbiera dane
    if (rank != ROOT)
    {
        int touristsArray[touristCount];
        int submarinesArray[lodzCount];

        MPI_Recv(touristsArray, touristCount, MPI_INT, 0, DATA, MPI_COMM_WORLD, &status);
        MPI_Recv(submarinesArray, lodzCount, MPI_INT, 0, DATA, MPI_COMM_WORLD, &status);

        tourists = std::vector<int>(touristsArray, touristsArray + touristCount);
        lodziePojemnosc = std::vector<int>(submarinesArray, submarinesArray + lodzCount);
    }

    // utworzenie wątku komunikacyjnego
    pthread_create(&threadKom, NULL, startKomWatek, NULL);

    // wypełnienie wektora touristsId identyfikatorami procesów
    for (int i = 0; i < size; i++)
    {
        touristsId.push_back(i);
    }

    lodzieStan = std::vector<int>(lodzCount, 1); // ustawienie stanu wszystkich łodzi na oczekujące
    wybieranaLodz = 0;                           // ustawienie id wybieranej łodzi
    nadzorca = -1;
}

void finalizuj()
{
    //tu jakoś się musi dostać, mainLoop musi zostać przerwany i trzeba wysłać komunikat do wątku komunikacyjnego
    //najlepiej obsłużyć sygnał ctrl+c

    pthread_mutex_destroy(&stateMut);
    pthread_mutex_destroy(&lamportMut);

    pthread_join(threadKom, NULL);
    MPI_Type_free(&mpiLamportPacket);
    MPI_Finalize();
}

//program uruchamiany
//mpirun -np <liczba turystów> --oversubscribe a.out <liczba strojow kucyka> <liczba lodzi podwodnych> <minimum turysty> <maksimum> <minimum lodzi> <maksimum lodzi>
int main(int argc, char **argv)
{
    inicjuj(argc, argv);
    mainLoop();
    finalizuj();

    return 0;
}
