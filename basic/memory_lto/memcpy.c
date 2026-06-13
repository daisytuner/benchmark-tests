#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define N 16384

__attribute__((noinline))
void init(double* ptr)
{
    void* internal = malloc(N * sizeof(double));
    for (int i = 0; i < N; i++) {
        ((double*)internal)[i] = i;
    }
    memcpy(ptr, internal, N * sizeof(double));
    free(internal);
}

__attribute__((noinline))
void square(double* ptr)
{
    void* internal = malloc(N * sizeof(double));
    memcpy(internal, ptr, N * sizeof(double));
    for (int i = 0; i < N; i++) {
        ((double*)internal)[i] *= ((double*)internal)[i];
    }
    memcpy(ptr, internal, N * sizeof(double));
    free(internal);
}

int main() {

    double* ptr1 = (double*) malloc(N * sizeof(double));

    init(ptr1);
    square(ptr1);

    if (ptr1[1234] != 1234.0 * 1234.0) {
        printf("Error: %f != %f\n", ptr1[1234], 1234.0 * 1234.0);
        exit(1);
    } else {
        printf("Success: %f == %f\n", ptr1[1234], 1234.0 * 1234.0);
    }

    free(ptr1);

    return 0;
}
