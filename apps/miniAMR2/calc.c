#include <stdio.h>
#include <stdlib.h>

#include "calc.h"
#include "params.h"
#include "block.h"
#include "profile.h"

#include "stencil.h"

// seed for random imbalance
#define RAND_IMBALANCE_SEED 1319

// From header
int* imbalances;

// operations per step
int op_step;

// Get the imbalance value for specifiec stencil point
int get_imbalance(int x, int y, int z) {
    switch(imbalance) {
    case 1:
    case 3:
        return calc_factor * (x + 1) * (y + 1) * (z + 1);
    case 2:
        return calc_factor * (x_block_size - x) * (y_block_size - y) * (z_block_size - z);
    default:
        return calc_factor;
    }
}

// Shuffle imbalance values for random mode
void shuffle_imbalance(void) {
    srand(RAND_IMBALANCE_SEED);
    for(size_t i = 0; i < x_block_size - 1; ++i) {
        for(size_t j = 0; j < y_block_size - 1; ++j) {
            for(size_t k = 0; k < z_block_size - 1; ++k) {
                int si = (int) (rand() / (RAND_MAX + 1.0) * x_block_size);
                int sj = (int) (rand() / (RAND_MAX + 1.0) * y_block_size);
                int sk = (int) (rand() / (RAND_MAX + 1.0) * z_block_size);
                int tmp = IMBALANCES(i, j, k);
                IMBALANCES(i, j, k) = IMBALANCES(si, sj, sk);
                IMBALANCES(si, sj, sk) = tmp;
            }
        }
    }
}

// From header
void calculation(void) {
    double start = timer();
    switch(calc_mode) {
    case 0:
        stencil_7();
        break;
    case 1:
        stencil_27();
        break;
    default:
        printf("Error: No such calculation mode (%i)\n", calc_mode);
        exit(12);
    }
    double time = timer() - start;
    calc_time += time;
    if(time < calc_time_min)
        calc_time_min = time;
    if(time > calc_time_max)
        calc_time_max = time;
    unsigned long long ops = 0;
    for(size_t i = 0; i < x_block_size; ++i)
        for(size_t j = 0; j < y_block_size; ++j)
            for(size_t k = 0; k < z_block_size; ++k)
                ops += op_step + (op_step + 2) * IMBALANCES(i, j, k);
    ops *= sorted_index[num_refine + 1];
    calc_ops += ops;
    if(ops < calc_ops_min)
        calc_ops_min = ops;
    if(ops > calc_ops_max)
        calc_ops_max = ops;
}

// From header
void init_calculation(void) {
    switch(calc_mode) {
    case 1:
        op_step = 27;
        break;
    case 2:
        op_step = 8;
        break;
    case 3:
        op_step = 2;
        break;
    default:
        op_step = 7;
        break;
    }
    for(size_t i = 0; i < x_block_size; ++i)
        for(size_t j = 0; j < y_block_size; ++j)
            for(size_t k = 0; k < z_block_size; ++k)
                IMBALANCES(i, j, k) = get_imbalance(i, j, k);
    if(imbalance == 3)
        shuffle_imbalance();
}

// From header
void fin_calculation(void) {}
