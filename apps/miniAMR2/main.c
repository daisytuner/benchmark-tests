#include <stdio.h>
#include <stdlib.h>

#include "params.h"
#include "block.h"
#include "calc.h"
#include "refine.h"
#include "plot.h"
#include "profile.h"

// Allocate all needed variables on the heap
void allocate(void) {
    blocks = (block*) malloc(max_num_blocks * sizeof(block));
    blockarrays_length = (size_t) max_num_blocks * (size_t) BLOCK_DATA_SIZE;
    if(calc_mode == 0 || calc_mode == 1)
        blockarrays_length *= 2;
    blockarrays = (double*) malloc(blockarrays_length * sizeof(double));
    parents = (parent*) malloc(max_num_blocks * sizeof(parent));
    sorted_index = (int*) malloc((num_refine + 2) * sizeof(int));
    sorted_list = (sorted_block*) malloc(max_num_blocks * sizeof(sorted_block));
    num_blocks = (int*) malloc((num_refine + 1) * sizeof(int));
    p2 = (int*) malloc((num_refine + 2) * sizeof(int));
    for(size_t i = 0; i < max_num_blocks; ++i)
        blocks[i].number = parents[i].number = -1;
    imbalances = (int*) malloc(BLOCK_SIZE * sizeof(int));
}

// Free all needed variables on the heap
void deallocate(void) {
#ifdef MANTEVO_DUMP_ARRAYS
    // Print results
    fprintf(stdout, "BEGIN DUMP OUTPUT\n");
    for (size_t i = 0; i < max_num_blocks * BLOCK_DATA_SIZE; i++) {
        fprintf(stdout, "%0.9g ", blockarrays[i]);
    }
    fprintf(stdout, "\n");
    fprintf(stdout, "END DUMP OUTPUT\n");
#endif

    free(imbalances);
    free(p2);
    free(num_blocks);
    free(sorted_list);
    free(sorted_index);
    free(parents);
    free(blockarrays);
    free(blocks);
    free(objects);
}

// Initialize all needed variables
void init(void) {
    p2[0] = 1;
    for(int i = 0; i < (num_refine + 1); ++i) {
        p2[i + 1] = p2[i] * 2;
        sorted_index[i] = 0;
        num_blocks[i] = 0;
    }
    sorted_index[num_refine + 1] = 0;
    max_active_parent = 0;
    blockarray_switch = 0;
    int size = p2[num_refine + 1];
    size_t blocks_per_dim = 1;
    for(int i = 0; i < init_blocks; ++i)
        blocks_per_dim *= 2;
    mesh_size[0] = mesh_size[1] = mesh_size[2] = blocks_per_dim * size;
    int n = 0;
    for(size_t x = 0; x < blocks_per_dim; ++x) {
        for(size_t y = 0; y < blocks_per_dim; ++y) {
            for(size_t z = 0; z < blocks_per_dim; ++z) {
                blocks[n].number = n;
                blocks[n].level = 0;
                blocks[n].parent = -1;
                blocks[n].child_number = 0;
                blocks[n].cen[0] = x * size + size / 2;
                blocks[n].cen[1] = y * size + size / 2;
                blocks[n].cen[2] = z * size + size / 2;
                add_sorted_list(n, n, 0);
                BLOCKARRAY_READ(n,                0,                0,                0) = 0.0;
                BLOCKARRAY_READ(n, x_block_size + 1,                0,                0) = 0.0;
                BLOCKARRAY_READ(n,                0, y_block_size + 1,                0) = 0.0;
                BLOCKARRAY_READ(n, x_block_size + 1, y_block_size + 1,                0) = 0.0;
                BLOCKARRAY_READ(n,                0,                0, z_block_size + 1) = 0.0;
                BLOCKARRAY_READ(n, x_block_size + 1,                0, z_block_size + 1) = 0.0;
                BLOCKARRAY_READ(n,                0, y_block_size + 1, z_block_size + 1) = 0.0;
                BLOCKARRAY_READ(n, x_block_size + 1, y_block_size + 1, z_block_size + 1) = 0.0;
                for(size_t i = 1; i <= x_block_size; ++i)
                    for(size_t j = 1; j <= y_block_size; ++j)
                        for(size_t k = 1; k <= z_block_size; ++k)
                            BLOCKARRAY_READ(n, i, j, k) = ((double) rand()) / ((double) RAND_MAX);
                for(int i = 0; i < 6; ++i) {
                    blocks[n].nei_refine[i] = 0;
                    blocks[n].nei_level[i] = -2;
                    blocks[n].nei[i][0][0] = 0;
                }
                ++n;
            }
        }
    }
    num_blocks[0] = max_active_block = num_active = num_blocks_init = n;
    calc_time = calc_time_max = pure_calc_time = kernel_time = 0.0;
    calc_ops = calc_ops_max = 0;
    calc_time_min = __DBL_MAX__;
    calc_ops_min = __LONG_MAX__;
    init_calculation();
}

// Main program routine
int main(int argc, char** argv) {
    read_params(argc, argv);
    allocate();
    init();
    double t = timer();
    if(plot) {
        plot_init();
        plot_update(0);
    }
    if(num_refine)
        refine(0);
    for(int ts = 1; ts <= num_tsteps; ++ts) {
        for(int stage = 0; stage < stages_per_ts; ++stage)
            calculation();
        if(num_refine) {
            move();
            if(refine_freq > 0 && ts % refine_freq == 0)
                refine(ts);
        }
        if(plot)
            plot_update(ts);
    }
    if(plot)
        plot_dump();
    kernel_time = timer() - t;
    profile();
    fin_calculation();
    deallocate();
    return 0;
}
