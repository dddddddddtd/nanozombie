#include "main.h"
#include "utils.h"
#include "watek_komunikacyjny.h"
#include "watek_glowny.h"

// zmienne extern
int rank, size, touristCount, ponyCostumes, lodzCount, touristRangeFrom, touristRangeTo, submarineRangeFrom, submarineRangeTo, lamportClock = 0;
int kucykACKcount, lodzACKcount;
int wybieranaLodz;
int nadzorca;

std::vector<Request> LISTkucyk, LISTlodz;
std::vector<int> tourists, lodziePojemnosc, touristsId, wycieczka, lodzieStan;

// mutex stan
pthread_t threadKom;
pthread_mutex_t stateMut = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t lamportMut = PTHREAD_MUTEX_INITIALIZER;
state_t stan = Inactive;

// rzeczy lamporta
MPI_Datatype mpiLamportPacket;

void lamportSend(std::vector<int> receivers, int tag, int *lamportClock, lamportPacket packetOut)
{    
    packetOut.lamportClock = *lamportClock + 1;
    pthread_mutex_lock(&lamportMut);
    *lamportClock++;
    pthread_mutex_unlock(&lamportMut);

    for (int i = 0; i < receivers.size(); i++)
    {
        MPI_Send(&packetOut, 1, mpiLamportPacket, receivers[i], tag, MPI_COMM_WORLD);
    }
}

int lamportReceive(lamportPacket *packetIn, int src, int tag, MPI_Status *status, int *lamportClock)
{
    int result = MPI_Recv(packetIn, 1, mpiLamportPacket, src, tag, MPI_COMM_WORLD, status);
    pthread_mutex_lock(&lamportMut);
    *lamportClock = max(*lamportClock, packetIn->lamportClock) + 1;
    pthread_mutex_unlock(&lamportMut);
    return result;
}

void changeState(state_t newState)
{
    pthread_mutex_lock(&stateMut);
    stan = newState;
    pthread_mutex_unlock(&stateMut);
}

std::string stringLIST(std::vector<Request> LIST)
{
    std::string res = "";
    for (int i = 0; i < LIST.size(); i++)
    {
        res += "[" + std::to_string(LIST[i].processid) + ", " + std::to_string(LIST[i].lamportClock) + "] ";
    }
    return res;
}

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
    int blocklengths[4] = {1, 1, 1, 1,};
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

    srand(rank);

    touristCount = size;
    if (argc != 7)
    {
        ponyCostumes = 4;
        lodzCount = 1;
        touristRangeFrom = 1;
        touristRangeTo = 3;
        submarineRangeFrom = 3;
        submarineRangeTo = 8;
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
    // int touristCount = size;

    if (rank == ROOT)
    {
        //inicjalizacja wszystkiego
        printf("tourists: %d\nponyCostumes: %d\nsubmarines: %d\n", touristCount, ponyCostumes, lodzCount);
        printf("tourist range: %d-%d\n", touristRangeFrom, touristRangeTo);
        printf("submarine range: %d-%d\n", submarineRangeFrom, submarineRangeTo);

        //inicjalizacja turystów
        for (int i = 0; i < touristCount; i++)
        {
            tourists.push_back(getRandom(touristRangeFrom, touristRangeTo));
        }

        //inicjalizacja łodzi
        for (int i = 0; i < lodzCount; i++)
        {
            lodziePojemnosc.push_back(getRandom(submarineRangeFrom, submarineRangeTo));
        }

        //wysłanie danych do wszystkich procesów
        for (int i = 1; i < size; i++)
        {
            // double* touristsArray = &v[0];
            MPI_Send(tourists.data(), touristCount, MPI_INT, i, DATA, MPI_COMM_WORLD);
            MPI_Send(lodziePojemnosc.data(), lodzCount, MPI_INT, i, DATA, MPI_COMM_WORLD);
        }
        // debug("jestem");
    }

    //każdy proces odbiera dane
    if (rank != ROOT)
    {
        int touristsArray[touristCount];
        int submarinesArray[lodzCount];

        MPI_Recv(touristsArray, touristCount, MPI_INT, 0, DATA, MPI_COMM_WORLD, &status);
        MPI_Recv(submarinesArray, lodzCount, MPI_INT, 0, DATA, MPI_COMM_WORLD, &status);

        tourists = std::vector<int>(touristsArray, touristsArray + touristCount);
        lodziePojemnosc = std::vector<int>(submarinesArray, submarinesArray + lodzCount);

        // tourists.insert(tourists.begin(), std::begin(touristsArray), std::end(touristsArray));
        // submarines.insert(sumbarines.begin(), std::begin(submarinesArray), std::end(submarinesArray));
        printArray(&rank, tourists.data(), touristCount, "turysci");
        printArray(&rank, lodziePojemnosc.data(), lodzCount, "lodzie");
    }

    pthread_create(&threadKom, NULL, startKomWatek, NULL);

    for (int i = 0; i < size; i++)
    {
        touristsId.push_back(i);
    }

    lodzieStan = std::vector<int>(lodzCount, 1);
    wybieranaLodz = 0;
    nadzorca = -1;
}

void finalizuj()
{
    //niszczenie mutexów
    //łączenie wątków

    // pthread_mutex_destroy( &stateMut);
    // /* Czekamy, aż wątek potomny się zakończy */
    // println("czekam na wątek \"komunikacyjny\"\n" );
    // pthread_join(threadKom,NULL);
    // if (rank==0) pthread_join(threadMon,NULL);
    // MPI_Type_free(&MPI_PAKIET_T);
    MPI_Finalize();
}

//program uruchamiany
//mpirun -np <liczba turystów> --oversubscribe a.out <liczba strojow kucyka> <liczba lodzi podwodnych> /
// <minimum turysty> <maksimum> <minimum lodzi> <maksimum lodzi>
int main(int argc, char **argv)
{
    inicjuj(argc, argv);

    mainLoop();
    finalizuj();

    return 0;
}
