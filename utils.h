#include <mpi.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <vector>

int max(int a, int b)
{
    return a > b ? a : b;
}

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

void check_thread_support(int provided)
{
    printf("THREAD SUPPORT: %d\n", provided);
    switch (provided)
    {
    case MPI_THREAD_SINGLE:
        printf("Brak wsparcia dla wątków, kończę\n");
        MPI_Finalize();
        exit(-1);
        break;
    case MPI_THREAD_FUNNELED:
        printf("tylko te wątki, ktore wykonaly mpi_init_thread mogą wykonać wołania do biblioteki mpi\n");
        break;
    case MPI_THREAD_SERIALIZED:
        printf("tylko jeden watek naraz może wykonać wołania do biblioteki MPI\n");
        break;
    case MPI_THREAD_MULTIPLE:
        printf("Pełne wsparcie dla wątków\n");
        break;
    default:
        printf("Nikt nic nie wie\n");
    }
}