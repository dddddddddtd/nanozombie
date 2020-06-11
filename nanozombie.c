#include <mpi.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#define ROOT 0

int getRandom(int lower, int upper)
{
    return rand() % (upper - lower + 1) + lower;
}

//program uruchamiany
//mpirun -np <liczba turystów> --oversubscribe a.out <liczba strojow kucyka> <liczba lodzi podwodnych> /
// <minimum turysty> <maksimum> <minimum lodzi> <maksimum lodzi>
int main(int argc, char **argv)
{
    MPI_Init(&argc, &argv);
    int size, rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Status status;

    //moze lepiej zamiast wywalac dac wartosci domyslne
    if (argc < 7)
    {
        MPI_Finalize();
        printf("Za malo argumentow\n");
        exit(0);
    }

    if (rank == ROOT)
    {
        srand(time(0));
        //inicjalizacja wszystkiego
        //wysłanie danych do pozostałych procesów
        //może też powinien wysyłać sam do siebie,
        //to wtedy po tym ifie wszystko wspolne dla procesow lacznie z nim
        //było coś o tym wysyłaniu sam do siebie w opisie zegaru lamporta chyba

        int touristCount = size;
        int ponyCostumes = atoi(argv[1]);
        int submarineCount = atoi(argv[2]);
        int touristRange[] = {atoi(argv[3]), atoi(argv[4])};
        int submarineRange[] = {atoi(argv[5]), atoi(argv[6])};

        printf("tourists: %d\nponyCostumes: %d\nubmarines: %d\n", touristCount, ponyCostumes, submarineCount);
        printf("tourist range: %d-%d\n", touristRange[0], touristRange[1]);
        printf("submarine range: %d-%d\n", submarineRange[0], submarineRange[1]);

        int tourists[touristCount];
        int submarines[submarineCount];

        //initialize tourists
        for (int i = 0; i < touristCount; i++)
        {
            tourists[i] = getRandom(touristRange[0], touristRange[1]);
        }

        //initialize submarines
        for (int i = 0; i < submarineCount; i++)
        {
            submarines[i] = getRandom(submarineRange[0], submarineRange[1]);
        }

        for (int i = 0; i < touristCount; i++)
        {
            printf("turysta %d: %d\n", i, tourists[i]);
        }

        for (int i = 0; i < submarineCount; i++)
        {
            printf("submarine %d: %d\n", i, submarines[i]);
        }
    }

    MPI_Finalize();
    return 0;
}

