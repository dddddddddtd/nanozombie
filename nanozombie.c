#include <mpi.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include "vector.h"

#define ROOT 0

//mozliwe ze to niepotrzebne
#define INIT 1
#define MESSAGE_TAG 2

#define MESSAGE_SIZE 16

int getRandom(int lower, int upper)
{
    return rand() % (upper - lower + 1) + lower;
}

void printArray(int *rank, int array[], int *count, char title[])
{
    char result[100] = "";
    char test[60];
    sprintf(test, "%d: %s : [", *rank, title);
    strcat(result, test);

    for (int i = 0; i < *count; i++)
    {
        if (i != *count - 1)
        {
            sprintf(test, "%d, ", array[i]);
            strcat(result, test);
        }
        else
        {
            sprintf(test, "%d]", array[i]);
            strcat(result, test);
        }
    }
    printf("%s\n", result);
}

// rzeczy lamporta
int max(int a, int b)
{
    return a > b ? a : b;
}

MPI_Datatype mpiLamportPacket;

typedef struct lamportPacket_s
{
    int clock;
    char message[MESSAGE_SIZE];
} lamportPacket;

int lamportSend(int clock, int src, int dest, char *messageOut)
{
    lamportPacket packetOut;
    packetOut.clock = clock + 1;
    strcpy(packetOut.message, messageOut);
    MPI_Send(&packetOut, 1, mpiLamportPacket, dest, MESSAGE_TAG, MPI_COMM_WORLD);
    return packetOut.clock;
}

lamportPacket lamportReceive(int clock, int src, int dest)
{
    MPI_Status status;
    lamportPacket packetIn;
    MPI_Recv(&packetIn, 1, mpiLamportPacket, src, MESSAGE_TAG, MPI_COMM_WORLD, &status);
    packetIn.clock = max(clock, packetIn.clock) + 1;
    return packetIn;
}

//program uruchamiany
//mpirun -np <liczba turystów> --oversubscribe a.out <liczba strojow kucyka> <liczba lodzi podwodnych> /
// <minimum turysty> <maksimum> <minimum lodzi> <maksimum lodzi>
int main(int argc, char **argv)
{
    MPI_Init(&argc, &argv);

    // konfiguracja structa dla MPI
    MPI_Datatype types[2] = {MPI_INT, MPI_CHAR};
    int blocklengths[2] = {1, MESSAGE_SIZE};
    MPI_Aint offsets[2];
    offsets[0] = offsetof(lamportPacket, clock);
    offsets[1] = offsetof(lamportPacket, message);
    MPI_Type_create_struct(2, blocklengths, offsets, types, &mpiLamportPacket);
    MPI_Type_commit(&mpiLamportPacket);

    int size, rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Status status;

    int touristCount = size;
    int ponyCostumes, submarineCount, touristRangeFrom, touristRangeTo, submarineRangeFrom, submarineRangeTo;
    if (argc != 7)
    {
        // dałem turystów na 20
        ponyCostumes = 10;
        submarineCount = 5;
        touristRangeFrom = 1;
        touristRangeTo = 3;
        submarineRangeFrom = 3;
        submarineRangeTo = 10;
    }
    else
    {
        ponyCostumes = atoi(argv[1]);
        submarineCount = atoi(argv[2]);
        touristRangeFrom = atoi(argv[3]);
        touristRangeTo = atoi(argv[4]);
        submarineRangeFrom = atoi(argv[5]);
        submarineRangeTo = atoi(argv[6]);
    }
    // int touristCount = size;
    int tourists[touristCount];
    int submarines[submarineCount];

    if (rank == ROOT)
    {
        srand(time(0));
        //inicjalizacja wszystkiego
        //wysłanie danych do pozostałych procesów
        //może też powinien wysyłać sam do siebie,
        //to wtedy po tym ifie wszystko wspolne dla procesow lacznie z nim
        //było coś o tym wysyłaniu sam do siebie w opisie zegaru lamporta chyba

        printf("tourists: %d\nponyCostumes: %d\nsubmarines: %d\n", touristCount, ponyCostumes, submarineCount);
        printf("tourist range: %d-%d\n", touristRangeFrom, touristRangeTo);
        printf("submarine range: %d-%d\n", submarineRangeFrom, submarineRangeTo);

        //inicjalizacja turystów
        for (int i = 0; i < touristCount; i++)
        {
            tourists[i] = getRandom(touristRangeFrom, touristRangeTo);
        }

        //inicjalizacja łodzi
        for (int i = 0; i < submarineCount; i++)
        {
            submarines[i] = getRandom(submarineRangeFrom, submarineRangeTo);
        }

        //wysłanie danych do wszystkich procesów
        for (int i = 0; i < size; i++)
        {
            MPI_Send(tourists, touristCount, MPI_INT, i, INIT, MPI_COMM_WORLD);
            MPI_Send(submarines, submarineCount, MPI_INT, i, INIT, MPI_COMM_WORLD);
        }
    }

    //każdy proces odbiera dane
    MPI_Recv(tourists, touristCount, MPI_INT, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
    MPI_Recv(submarines, submarineCount, MPI_INT, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

    printArray(&rank, tourists, &touristCount, "turysci");
    printArray(&rank, submarines, &submarineCount, "lodzie");

    //tu cala robota procesow

    // testowanie komunikacja lamporta - root wysyła do (siebie + 1) i +1 odbiera i printuje msg i clock
    // teraz zakomentowane - możesz sobie przetestować jak chcesz
    // if (rank == ROOT)
    // {
    //     char *msg = "ACK";
    //     int clock = 0;
    //     clock = lamportSend(clock, ROOT, ROOT + 1, msg);
    // }
    // if (rank == ROOT + 1)
    // {
    //     int clock = 0;
    //     lamportPacket receivePacket;
    //     receivePacket = lamportReceive(clock, ROOT, ROOT + 1);
    //     printf("zegar odbiorcy: %d\n", receivePacket.clock);
    //     printf("wiadomośc od nadawcy: %s\n", receivePacket.message);
    // }

    // deklaracja zmiennych lokalnych procesu
    struct vector listKucykOk;
    struct vector listKucykHalt;
    struct vector listLodz;
    init_vector(&listKucykOk);
    init_vector(&listKucykHalt);
    init_vector(&listLodz);
    
    MPI_Finalize();


    
    return 0;
}
