#include <stddef.h>

#include "params.h"

#ifndef _BLOCK_INCLUDE_
#define _BLOCK_INCLUDE_

// Macro to ease access to block sizes
#define BLOCK_SIZE (x_block_size * y_block_size * z_block_size)
#define BLOCK_DATA_SIZE ((x_block_size + 2) * (y_block_size + 2) * (z_block_size + 2))

// Macro to access block array with three indices
#define BLOCKARRAY_READ(block, i, j, k) blockarrays[max_num_blocks * BLOCK_DATA_SIZE * blockarray_switch + BLOCK_DATA_SIZE * (block) + (y_block_size + 2) * (z_block_size + 2) * (i) + (z_block_size + 2) * (j) + (k)]
#define BLOCKARRAY_WRITE(block, i, j, k) blockarrays[max_num_blocks * BLOCK_DATA_SIZE * (1 - blockarray_switch) + BLOCK_DATA_SIZE * (block) + (y_block_size + 2) * (z_block_size + 2) * (i) + (z_block_size + 2) * (j) + (k)]

// switches between two versions of block values
extern int blockarray_switch;

// list of blocks
typedef struct {
    int number;
    int refine;
    int level;
    int parent;
    int child_number;
    int nei_refine[6];
    int nei_level[6];
    int nei[6][2][2];
    int cen[3];
} block;
extern block* blocks;

// list of block values
extern double* blockarrays;

// length of list of block values
extern size_t blockarrays_length;

// list of parent blocks
typedef struct {
    int number;
    int refine;
    int level;
    int parent;
    int child_number;
    int cen[3];
    int child[8];
} parent;
extern parent* parents;

// active block with highest id
extern size_t max_active_block;

// active parent with highest id
extern int max_active_parent;

// number of active blocks
extern int num_active;

// sorted permutation of block list
typedef struct {
    int number;
    int n;
} sorted_block;
extern sorted_block* sorted_list;
extern int* sorted_index;

// number of blocks
extern int* num_blocks;

// size of the mesh
extern size_t mesh_size[3];

// list with power of 2
extern int* p2;

// initial number of blocks
extern long num_blocks_init;

// Add block to the sorted list
void add_sorted_list(int, int, int);

// Split blocks that are marked to be refined into 8 new blocks
void split_blocks(int);

// Combine blocks that are marked to be coarsened
void consolidate_blocks(int);

// Switch the two blockarrays
void switch_blockarrays(void);

#endif
