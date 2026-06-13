#include <stdio.h>
#include <stdlib.h>

#include <cblas.h>


#ifdef DATA_TYPE_IS_d
    #define DATA_TYPE double
    #define DATA_TYPE_LITERAL(a) a
    #define DATA_TYPE_CBLAS_PREFIX(a) cblas_d##a
	#define DATA_TYPE_PARSE(s, f) (strtod(s, f))
#else
    #define DATA_TYPE float
    #define DATA_TYPE_LITERAL(a) a##f
    #define DATA_TYPE_CBLAS_PREFIX(a) cblas_s##a
	#define DATA_TYPE_PARSE(s, f) (strtof(s, f))
#endif

int main(int argc, char** argv) {

    char* fail = NULL;

    DATA_TYPE alpha = DATA_TYPE_LITERAL(1.0);
    DATA_TYPE beta = DATA_TYPE_LITERAL(0.0);

    // Parse optional command-line arguments: alpha, beta, M, N, K
    if (argc > 1) {
        alpha = DATA_TYPE_PARSE(argv[1], &fail);
    }
    if (argc > 2) {
        beta = DATA_TYPE_PARSE(argv[2], &fail);
    }

#ifndef M_STATIC
    int M = 66;
	if (argc > 3) M = strtol(argv[3], &fail, 10);
#else
    const int M = M_STATIC;
#endif

#ifndef N_STATIC
	int N = 66;
	if (argc > 4) N = strtol(argv[4], &fail, 10);
#else
    const int N = N_STATIC;
#endif

#ifndef K_STATIC
	int K = 66;
    if (argc > 5) K = strtol(argv[5], &fail, 10);
#else
    const int K = K_STATIC;
#endif

    printf("Running with M=%d, N=%d, K=%d, alpha=%f, beta=%f\n", M, N, K, alpha, beta);

    DATA_TYPE A[M * K];
    for (int i = 0; i < M; ++i) {
        for (int j = 0; j < K; ++j) {
            long tile = (i/32) * (K/32) + (j/32);
            A[i * K + j] = (DATA_TYPE)((j&0x1F) * (tile+1));
        }
    }

#ifdef PRINT_INPUTS
    printf("==BEGIN A==\n");
    for (int i = 0; i < M; ++i) {
        for (int j = 0; j < K; ++j) {
            printf("%12.5f ", A[i * K + j]);
        }
        printf("\n");
    }
    printf("==END   A==\n");
#endif

    DATA_TYPE B[K * N];
    for (int i = 0; i < K; ++i) {
        for (int j = 0; j < N; ++j) {
            long tile = (i/32) * (N/32) + (j/32);
            B[i * N + j] = ((DATA_TYPE)(i&0x1F)*(tile+1))/DATA_TYPE_LITERAL(32.0);
        }
    }

#ifdef PRINT_INPUTS
    printf("==BEGIN B==\n");
    for (int i = 0; i < K; ++i) {
        for (int j = 0; j < N; ++j) {
            printf("%12.5f ", B[i * N + j]);
        }
        printf("\n");
    }
    printf("==END   B==\n");
#endif

    DATA_TYPE C[M * N];
    for (int i = 0; i < M * N; ++i) {
        C[i] = DATA_TYPE_LITERAL(20.0);
    }

    DATA_TYPE_CBLAS_PREFIX(gemm)(
        CblasRowMajor,
        CblasNoTrans,
        CblasNoTrans,
        M,
        N,
        K,
        alpha,
        A,
        K,
        B,
        N,
        beta,
        C,
        N
    );

    for (int i = 0; i < M; ++i) {
        for (int j = 0; j < N; ++j) {
            C[i * N + j] += DATA_TYPE_LITERAL(1.0);
        }
    }

    // Verify the result
    printf("==BEGIN DUMP_ARRAYS==\n");
    for (int i = 0; i < M; ++i) {
        for (int j = 0; j < N; ++j) {
            printf("%10.3f ", C[i * N + j]);
        }
        printf("\n");
    }
    printf("==END   DUMP_ARRAYS==\n");

    return 0;
}
