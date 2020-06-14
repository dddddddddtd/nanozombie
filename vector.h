// https://stackoverflow.com/a/47292911
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct vector
{
    int len;
    int allocated;
    int step;
    int *data;
};

#define INIT_SIZE 1

void init_vector(struct vector *v)
{
    v->len = 0;
    v->allocated = 0;
    v->step = 2;
    v->data = NULL;
}

int append(struct vector *v, int item)
{
    if (!v->data)
    {
        v->data = malloc(INIT_SIZE * sizeof(int));

        if (!v->data)
            return -1;

        v->allocated = INIT_SIZE;
    }
    else if (v->len >= v->allocated)
    {
        int *tmp = realloc(v->data,
                           v->allocated * v->step * sizeof(int));

        if (!tmp)
            return -1;

        v->data = tmp;
        v->allocated *= v->step;
    }

    v->data[v->len] = item;
    v->len++;

    return 0;
}

int delete (struct vector *v, int index)
{
    if (index < 0 || index >= v->len)
        return -1;

    memmove(v->data + index, v->data + index + 1,
            (v->len - index - 1) * sizeof(int));
    v->len--;

    return 0;
}

void print(const struct vector *v)
{
    printf("Array:\n");

    for (int i = 0; i < v->len; i++)
        printf("%d ", v->data[i]);

    printf("\n");
}