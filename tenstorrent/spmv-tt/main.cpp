

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <sys/time.h>

#include "util.h"
#include "kernel.h"

#define BLOCK_SIZE 64

int main(int argc, char **argv) {
    // Set CUDA to prefer L1 cache over shared memory
    // Will set L1 cache to 48K and shared memory to 16K if possible

    // Do gettimeofday timing for estimating execution time of main
    struct timeval t1_main, t2_main, k_start, k_end; //, t1, t2;
    double elapsedMain;
    gettimeofday(&t1_main, nullptr);

    printf("Running main using spmvStrawberry with block size %i\n", BLOCK_SIZE);

    // PARAMETERS
    double p_diag = 0.9;
    double p_nondiag = 0.1;
    float *A_cpu, *A_gpu, *x_cpu, *x_gpu, *y_cpu, *y_gpu, *y_correct;
    int *IA_cpu, *IA_gpu, *JA_cpu, *JA_gpu;
    int NNZ;

    int expMmin = 10;
    int expMmax = 15;
    int Mmin = (1 << expMmin);
    int Mmax = (1 << expMmax);
    int L = expMmax - expMmin + 1; //printf("L = %i\n", L);
    float *t_arr = (float *) malloc(sizeof(float) * L);
    int *M_arr = (int *) malloc(sizeof(float) * L);
    int i;
    int idx = 0;
    for (i = 0; i < L; ++i)
        M_arr[i] = (1 << (i + expMmin));

    // seed random number generator
    time_t t;
    srand((unsigned) time(&t));

    const int NUM_ITERS = 1;

    int M, N, iter;
    double elapsed;
    for (M = Mmin; M <= Mmax; M = M * 2) {
        elapsed = 0;
        for (iter = 0; iter < NUM_ITERS; ++iter) {
            // Create sparse matrix
            generateSquareSpMatrix(&A_cpu, &IA_cpu, &JA_cpu, &NNZ, M, p_diag, p_nondiag); // allocates!
            N = M; // due to generation of square sparse matrix

            // Generate dense vector x
            x_cpu = (float *) malloc(((sizeof(float) * N) + 4095)/4096 * 4096); // allocate aligned memory
            fillDenseVector(x_cpu, N);

            // Define output vector y and y_correct
            y_cpu = (float *) malloc(((sizeof(float) * M) + 4095)/4096 * 4096);
            y_correct = (float *) malloc(((sizeof(float) * M) + 4095)/4096 * 4096);


            // Do CPU timing
            auto kstart = std::chrono::high_resolution_clock::now();

            // CUDA Strawberry SpMV Kernel
            kernel_spmv(y_cpu, A_cpu, IA_cpu, JA_cpu, N, x_cpu);

            // Record timing result
            auto kend = std::chrono::high_resolution_clock::now();
            auto ktime = kend - kstart;

            elapsed += static_cast<double>(std::chrono::duration_cast<std::chrono::milliseconds>(ktime).count());

            // Test correctness of CUDA kernel vs "golden" cpu spmv function
            auto cpu_ref_start = std::chrono::high_resolution_clock::now();
            spmvCPU(y_correct, A_cpu, IA_cpu, JA_cpu, N, x_cpu);
            auto cpu_ref_end = std::chrono::high_resolution_clock::now();

            auto cpu_ref_time = static_cast<double>(std::chrono::duration_cast<std::chrono::milliseconds>(cpu_ref_end - cpu_ref_start).count());

            std::cout << "CPU SpMV time: " << cpu_ref_time << " ms" << std::endl;

            if (!areEqualRMSE(y_correct, y_cpu, N)) {
                printf("Not correct result for a (%ix%i)*(%ix1) spmv multiplication\n", M, N, N);
                printRMSE(y_correct, y_cpu, N);
            }

            // Free memory
            free(A_cpu);
            free(IA_cpu);
            free(JA_cpu);
            free(x_cpu);
            free(y_cpu);
            free(y_correct);
        }
        t_arr[idx++] = (float) elapsed / NUM_ITERS;
    }

    printf("Results averaged over %i iterations with time in ms:\n", NUM_ITERS);
    printf("M = ");
    printArray(M_arr, L);
    printf("t = ");
    printArray(t_arr, L);

    free(t_arr);
    free(M_arr);

    gettimeofday(&t2_main, nullptr);
    //elapsed += (t2.tv_sec-t1.tv_sec)*1000.0 + (t2.tv_usec-t1.tv_usec)/1000.0; // in ms
    elapsedMain = (t2_main.tv_sec - t1_main.tv_sec) + (t2_main.tv_usec - t1_main.tv_usec) / 1000000.0;
    printf("Total time to execute main() : %g seconds\n", elapsedMain);
    return 0;
}
