#include <stddef.h>

#ifndef _PARAMS_INCLUDE_
#define _PARAMS_INCLUDE_

// dimensions
extern long long x_block_size, y_block_size, z_block_size;

// number of levels of refinement
extern int num_refine;

// maximum numbers of blocks
extern long long max_num_blocks;

// number of timesteps
extern int num_tsteps;

// numer of stages between checksums
extern int checksum_freq;

// frequency (timesteps) of refinement
extern int refine_freq;

// numer of comm/calc stages per timestamp
extern int stages_per_ts;

// whether to generate a plot or not
extern char plot;

// imbalance strategy
extern int imbalance;

// intesity of each calculation step
extern int calc_factor;

// determines the calculation mode
extern int calc_mode;

// whether to print summary at the end
extern char summary;

// objects (hard coded)
extern int num_objects;
typedef struct {
    int type;
    int bounce;
    double cen[3];
    //double orig_cen[3];
    double move[3];
    //double orig_move[3];
    double size[3];
    //double orig_size[3];
    double inc[3];
} object;
extern object* objects;

// exponent that determines the initial number of blocks
extern int init_blocks;

// factor for tiling the calculation in every dimension
extern int calc_tiling;

// Read the command line parameters and assign them to the values
void read_params(int, char**);

#endif
