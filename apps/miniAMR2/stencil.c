#include <stdio.h>
#include <stdlib.h>

#include "stencil.h"
#include "calc.h"
#include "params.h"
#include "block.h"
#include "profile.h"

// Calculates a single 7 point stencil step
double stencil_7_step(long long x, long long y, long long z, int block, long long x_block_size, long long y_block_size, long long z_block_size, long long max_num_blocks, int blockarray_switch, int* imbalances, double* blockarrays) {
    double res = (BLOCKARRAY_READ(block, x - 1, y    , z    ) +
                  BLOCKARRAY_READ(block, x    , y - 1, z    ) +
                  BLOCKARRAY_READ(block, x    , y    , z - 1) +
                  BLOCKARRAY_READ(block, x    , y    , z    ) +
                  BLOCKARRAY_READ(block, x    , y    , z + 1) +
                  BLOCKARRAY_READ(block, x    , y + 1, z    ) +
                  BLOCKARRAY_READ(block, x + 1, y    , z    )) / 7.0;
    for(int l = 1; l <= IMBALANCES(x - 1, y - 1, z - 1); ++l)
        res += ((BLOCKARRAY_READ(block, x - 1, y    , z    ) +
                 BLOCKARRAY_READ(block, x    , y - 1, z    ) +
                 BLOCKARRAY_READ(block, x    , y    , z - 1) +
                 BLOCKARRAY_READ(block, x    , y    , z    ) +
                 BLOCKARRAY_READ(block, x    , y    , z + 1) +
                 BLOCKARRAY_READ(block, x    , y + 1, z    ) +
                 BLOCKARRAY_READ(block, x + 1, y    , z    )) / 7.0) / ((double) l);
    return res;
}

// Calculates a single 27 point stencil step
double stencil_27_step(long long x, long long y, long long z, int block, long long x_block_size, long long y_block_size, long long z_block_size, long long max_num_blocks, int blockarray_switch, int* imbalances, double* blockarrays) {
    double res = (BLOCKARRAY_READ(block, x - 1, y - 1, z - 1) +
                  BLOCKARRAY_READ(block, x - 1, y - 1, z    ) +
                  BLOCKARRAY_READ(block, x - 1, y - 1, z + 1) +
                  BLOCKARRAY_READ(block, x - 1, y    , z - 1) +
                  BLOCKARRAY_READ(block, x - 1, y    , z    ) +
                  BLOCKARRAY_READ(block, x - 1, y    , z + 1) +
                  BLOCKARRAY_READ(block, x - 1, y + 1, z - 1) +
                  BLOCKARRAY_READ(block, x - 1, y + 1, z    ) +
                  BLOCKARRAY_READ(block, x - 1, y + 1, z + 1) +
                  BLOCKARRAY_READ(block, x    , y - 1, z - 1) +
                  BLOCKARRAY_READ(block, x    , y - 1, z    ) +
                  BLOCKARRAY_READ(block, x    , y - 1, z + 1) +
                  BLOCKARRAY_READ(block, x    , y    , z - 1) +
                  BLOCKARRAY_READ(block, x    , y    , z    ) +
                  BLOCKARRAY_READ(block, x    , y    , z + 1) +
                  BLOCKARRAY_READ(block, x    , y + 1, z - 1) +
                  BLOCKARRAY_READ(block, x    , y + 1, z    ) +
                  BLOCKARRAY_READ(block, x    , y + 1, z + 1) +
                  BLOCKARRAY_READ(block, x + 1, y - 1, z - 1) +
                  BLOCKARRAY_READ(block, x + 1, y - 1, z    ) +
                  BLOCKARRAY_READ(block, x + 1, y - 1, z + 1) +
                  BLOCKARRAY_READ(block, x + 1, y    , z - 1) +
                  BLOCKARRAY_READ(block, x + 1, y    , z    ) +
                  BLOCKARRAY_READ(block, x + 1, y    , z + 1) +
                  BLOCKARRAY_READ(block, x + 1, y + 1, z - 1) +
                  BLOCKARRAY_READ(block, x + 1, y + 1, z    ) +
                  BLOCKARRAY_READ(block, x + 1, y + 1, z + 1)) / 27.0;
    for(int l = 1; l <= IMBALANCES(x - 1, y - 1, z - 1); ++l)
        res += ((BLOCKARRAY_READ(block, x - 1, y - 1, z - 1) +
                 BLOCKARRAY_READ(block, x - 1, y - 1, z    ) +
                 BLOCKARRAY_READ(block, x - 1, y - 1, z + 1) +
                 BLOCKARRAY_READ(block, x - 1, y    , z - 1) +
                 BLOCKARRAY_READ(block, x - 1, y    , z    ) +
                 BLOCKARRAY_READ(block, x - 1, y    , z + 1) +
                 BLOCKARRAY_READ(block, x - 1, y + 1, z - 1) +
                 BLOCKARRAY_READ(block, x - 1, y + 1, z    ) +
                 BLOCKARRAY_READ(block, x - 1, y + 1, z + 1) +
                 BLOCKARRAY_READ(block, x    , y - 1, z - 1) +
                 BLOCKARRAY_READ(block, x    , y - 1, z    ) +
                 BLOCKARRAY_READ(block, x    , y - 1, z + 1) +
                 BLOCKARRAY_READ(block, x    , y    , z - 1) +
                 BLOCKARRAY_READ(block, x    , y    , z    ) +
                 BLOCKARRAY_READ(block, x    , y    , z + 1) +
                 BLOCKARRAY_READ(block, x    , y + 1, z - 1) +
                 BLOCKARRAY_READ(block, x    , y + 1, z    ) +
                 BLOCKARRAY_READ(block, x    , y + 1, z + 1) +
                 BLOCKARRAY_READ(block, x + 1, y - 1, z - 1) +
                 BLOCKARRAY_READ(block, x + 1, y - 1, z    ) +
                 BLOCKARRAY_READ(block, x + 1, y - 1, z + 1) +
                 BLOCKARRAY_READ(block, x + 1, y    , z - 1) +
                 BLOCKARRAY_READ(block, x + 1, y    , z    ) +
                 BLOCKARRAY_READ(block, x + 1, y    , z + 1) +
                 BLOCKARRAY_READ(block, x + 1, y + 1, z - 1) +
                 BLOCKARRAY_READ(block, x + 1, y + 1, z    ) +
                 BLOCKARRAY_READ(block, x + 1, y + 1, z + 1)) / 27.0) / ((double) l);
    return res;
}

// From header
void stencil_7(void) {
    start_pure_calc_timer();
    for(int in = 0; in < sorted_index[num_refine + 1]; ++in)
        for(long long i = 1; i <= x_block_size; ++i)
            for(long long j = 1; j <= y_block_size; ++j)
                for(long long k = 1; k <= z_block_size; ++k)
                    BLOCKARRAY_WRITE(sorted_list[in].n, i, j, k) = stencil_7_step(
                        i,
                        j,
                        k,
                        sorted_list[in].n,
                        x_block_size,
                        y_block_size,
                        z_block_size,
                        max_num_blocks,
                        blockarray_switch,
                        imbalances,
                        blockarrays
                    );
    stop_pure_calc_timer();
    switch_blockarrays();
}

// From header
void stencil_27(void) {
    start_pure_calc_timer();
    for(int in = 0; in < sorted_index[num_refine + 1]; ++in)
        for(long long i = 1; i <= x_block_size; ++i)
            for(long long j = 1; j <= y_block_size; ++j)
                for(long long k = 1; k <= z_block_size; ++k)
                    BLOCKARRAY_WRITE(sorted_list[in].n, i, j, k) = stencil_27_step(i, j, k, sorted_list[in].n, x_block_size, y_block_size, z_block_size, max_num_blocks, blockarray_switch, imbalances, blockarrays);
    stop_pure_calc_timer();
    switch_blockarrays();
}
