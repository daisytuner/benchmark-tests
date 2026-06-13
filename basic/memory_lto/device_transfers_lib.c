#include <stdlib.h>
#include <stdio.h>

#define N 16384

__attribute__((noinline))
void init(double* ptr)
{
    for (int i = 0; i < N; i++) {
        ptr[i] = i;
    }
}

__attribute__((noinline))
void square(double* ptr)
{
    for (int i = 0; i < N; i++) {
        ptr[i] *= ptr[i];
    }
}
