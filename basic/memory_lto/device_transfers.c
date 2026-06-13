#include <stdlib.h>
#include <stdio.h>

#define N 16384

extern void init(double* ptr);

extern void square(double* ptr);

int main() {

    double* ptr1 = (double*) malloc(N * sizeof(double));

    init(ptr1);
    square(ptr1);

    if (ptr1[100] != 10000.0) {
        fprintf(stderr, "Error: Expected 10000.0, got %f\n", ptr1[100]);
        free(ptr1);
        return 1;
    } else {
        printf("Success: ptr1[100] = %f\n", ptr1[100]);
    }

    free(ptr1);

    return 0;
}
