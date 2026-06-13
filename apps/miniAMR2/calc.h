#include "params.h"

#ifndef _CALC_INCLUDE_
#define _CALC_INCLUDE_

// Macro to access imbalances with three indices
#define IMBALANCES(i, j, k) imbalances[y_block_size * z_block_size * (i) + z_block_size * (j) + (k)]

// Return minimum of two numbers
#define MIN(x, y) (((x) < (y)) ? (x) : (y))

// Precalculate imbalances
extern int* imbalances;

// Execute calculation
void calculation(void);

// Initialize calculation
void init_calculation(void);

// Finalize calculation
void fin_calculation(void);

#endif
