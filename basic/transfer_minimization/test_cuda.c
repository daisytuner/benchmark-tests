#include <stdio.h>
#include <alloca.h>

#define N 100

int main(void) {
    printf("N = %d\n", N);

    float a[N];
    float b[N];
    float c[N];
    float d[N];
    float e[N];

    // init
    for (int i = 0; i < N; i++) {
        a[i] = 6.0f;
        b[i] = 2.0f;
        d[i] = 3.0f;
    }

    // vecadd 1
    for (int i = 0; i < N; i++) {
        c[i] = a[i] + b[i];
    }

    // vecadd 2
    for (int i = 0; i < N; i++) {
        e[i] = c[i] + d[i];
    }

    // check
    for (int i = 0; i < N; i++) {
        if (e[i] != 11.0f) {
            printf("Expected 33.0 at position %d, but got %f\n", i, c[i]);
            return 1;
        }
    }

    printf("Correct\n");
    return 0;
}