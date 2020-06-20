#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <vector>
#include <string>

int max(int a, int b)
{
    return a > b ? a : b;
}

double getRandomTime() // losowanie czasu z zakresu <3, 7>
{
    return (((double)rand() / (RAND_MAX)) * 4.0) + 3.0;
}

int getRandom(int lower, int upper) // losowanie inta z zakresu <lower, upper>
{
    return rand() % (upper - lower + 1) + lower;
}

void waitFor(const char *text) // oczekiwanie losowego czasu z wypisaniem tekstu przekazanego jako argument
{
    double t1, t2;
    t1 = MPI_Wtime();
    t2 = MPI_Wtime();
    double waittime = getRandomTime();
    debug("%s - wylosowalem czas: %f", text, waittime);
    while (t2 - t1 < waittime)
    {
        t2 = MPI_Wtime();
    }
}

std::string stringVector(std::vector<int> vec)
{
    std::string result = "[";
    for (int i = 0; i < vec.size(); i++)
    {
        result += std::to_string(vec[i]) + ", ";
    }
    result = result.substr(0, result.size() - 2);
    result += "]";
    return result;
}

std::string stringArray(int a[], int count)
{
    std::string result = "[";
    for (int i = 0; i < count; i++)
    {
        result += std::to_string(a[i]) + ", ";
    }
    result = result.substr(0, result.size() - 2);
    result += "]";
    return result;
}


void printArray(int *rank, int array[], int count, std::string title) // funkcja pomocnicza do tworzenia stringa z tablicy intów
{
    std::string result = "";
    result += std::to_string(*rank) + ": " + title + " : [";

    for (int i = 0; i < count; i++)
    {
        if (i != count - 1)
        {
            result += std::to_string(array[i]) + ", ";
        }
        else
        {
            result += std::to_string(array[i]) + "]\n";
        }
    }
    std::cout << result;
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