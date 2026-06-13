#include <stdio.h>
#include <stdlib.h>

#include <cblas.h>


#ifdef DATA_TYPE_IS_d
    #define DATA_TYPE double
    #define DATA_TYPE_LITERAL(a) a
    #define DATA_TYPE_CBLAS_PREFIX(a) cblas_d##a
#else
    #define DATA_TYPE float
    #define DATA_TYPE_LITERAL(a) a##f
    #define DATA_TYPE_CBLAS_PREFIX(a) cblas_s##a
#endif

int main() {
    const int N = 4000;

    DATA_TYPE A[N];
    for (int i = 0; i < N; ++i) {
        int quadrant = i / (N/4);
        A[i] = (DATA_TYPE)(2+quadrant*2);
    }

    DATA_TYPE B[N];
    for (int i = 0; i < N; ++i) {
        B[i] = (DATA_TYPE)0.5;
    }

    DATA_TYPE res = DATA_TYPE_CBLAS_PREFIX(dot)(
        N,
        A,
        1,
        B,
        1
    );

    res = res + DATA_TYPE_LITERAL(1.0);

    for (int i = 0; i < N; ++i) {
        A[i] += res;
    }

    // Verify the result
    printf("==BEGIN DUMP_ARRAYS==\n");
    for (int i = 0; i < N; ++i) {
        printf("%f\n", A[i]);
    }
    printf("==END   DUMP_ARRAYS==\n");

    return 0;
}
