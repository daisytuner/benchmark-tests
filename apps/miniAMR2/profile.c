#include <stdio.h>
#include <omp.h>
#include <time.h>

#include "profile.h"
#include "params.h"
#include "block.h"

// From header
double calc_time, calc_time_min, calc_time_max, pure_calc_time;
unsigned long long calc_ops, calc_ops_min, calc_ops_max;
double kernel_time;

// Temporary starting point for pure calculation time
double pure_calc_time_start;

// From header
double timer(void) {
    return omp_get_wtime();
}

// From header
void start_pure_calc_timer(void) {
    pure_calc_time_start = timer();
}

// From header
void stop_pure_calc_timer(void) {
    pure_calc_time += (timer() - pure_calc_time_start);
}

// From header
void profile(void) {
    printf("miniAMR 2\nVersion %s\n\n", MINIAMR2_VERSION);
    char imbalanceStr[11];
    switch(imbalance) {
    case 0:
        sprintf(imbalanceStr, "None");
        break;
    case 1:
        sprintf(imbalanceStr, "Increasing");
        break;
    case 2:
        sprintf(imbalanceStr, "Decreasing");
        break;
    case 3:
        sprintf(imbalanceStr, "Random");
        break;
    default:
        sprintf(imbalanceStr, "Unknown");
        break;
    }
    char calcModeStr[19];
    switch(calc_mode) {
    case 0:
        sprintf(calcModeStr, "7-point stencil");
        break;
    case 1:
        sprintf(calcModeStr, "27-point stencil");
        break;
    case 2:
        sprintf(calcModeStr, "Wavefront");
        break;
    case 3:
        sprintf(calcModeStr, "Reduction");
        break;
    case 4:
        sprintf(calcModeStr, "Cubic wavefront");
        break;
    case 5:
        sprintf(calcModeStr, "Stepwise wavefront");
        break;
    default:
        sprintf(calcModeStr, "Unknown");
        break;
    }
    printf("Block size:               %zu x %zu x %zu\n", x_block_size, y_block_size, z_block_size);
    printf("Initial number of blocks: %li\n", num_blocks_init);
    printf("Refinement levels:        %i\n", num_refine);
    printf("Timesteps:                %i\n", num_tsteps);
    printf("Refine frequency:         %i\n", refine_freq);
    printf("Stages per timestep:      %i\n", stages_per_ts);
    printf("Imbalance strategy:       %s\n", imbalanceStr);
    printf("Calculation factor:       %i\n", calc_factor);
    printf("Calculation mode:         %s\n", calcModeStr);
    printf("Calculation tiling:       %i\n", calc_tiling);
    char compiler[20];
    #if defined(__clang__)
    sprintf(compiler, "Clang %s", __clang_version__);
    #elif defined(__ICC)
    sprintf(compiler, "Intel C %d.%d", __INTEL_COMPILER, __INTEL_COMPILER_UPDATE);
    #elif defined(__GNUC__)
    sprintf(compiler, "GCC %d.%d.%d", __GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__);
    #endif
    printf("Compiled with:            %s\n\n", compiler);
    printf("Pure calculation time: %12f\n", pure_calc_time);
    printf("Calculation time:      %12f (min: %f, max: %f, avg: %f)\n", calc_time, calc_time_min, calc_time_max, (calc_time / (num_tsteps * stages_per_ts)));
    printf("Kernel time:           %12f\n", kernel_time);
    printf("Calculation Flops:     %12lli (min: %lli, max: %lli, avg: %lli)\n", calc_ops, calc_ops_min, calc_ops_max, (calc_ops / (num_tsteps * stages_per_ts)));
    printf("Calculation GFlops/s:  %12f\n", (calc_ops / calc_time) / 1000000000);
    printf("Kernel GFlops/s:       %12f\n", (calc_ops / kernel_time) / 1000000000);
    if(summary)
        printf("\n%zu,%zu,%zu,%i,%i,%i,%i,%i,%i,%i,%s,%f,%f,%lli,%f,%f,%f,%i\n", x_block_size, y_block_size, z_block_size, num_refine, num_tsteps, refine_freq, stages_per_ts, imbalance, calc_factor, calc_mode, compiler, calc_time, kernel_time, calc_ops, (calc_ops / calc_time) / 1000000000, (calc_ops / kernel_time) / 1000000000, pure_calc_time, calc_tiling);
}
