#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "params.h"
#include "profile.h"

// From header
long long x_block_size, y_block_size, z_block_size;
int num_refine;
long long max_num_blocks;
int num_objects, num_tsteps, checksum_freq, refine_freq, stages_per_ts;
char plot, summary;
int imbalance, calc_factor, calc_mode, init_blocks, calc_tiling;
object* objects;

// Assign default values to all parameters
void set_default_values(void) {
    x_block_size = y_block_size = z_block_size = 8;
    num_refine = 4;
    max_num_blocks = 4000;
    num_tsteps = 20;
    checksum_freq = 4;
    refine_freq = 5;
    stages_per_ts = 8;
    plot = summary = 0;
    imbalance = calc_factor = calc_mode = init_blocks = 0;
    calc_tiling = 1;
    // hard code objects for now: two moving spheres
    num_objects = 2;
    objects = (object*) malloc(num_objects * sizeof(object));
    objects[0].type    = 2;
    objects[0].bounce  = 0;
    objects[0].cen[0]  = -1.1;
    objects[0].cen[1]  = -1.1;
    objects[0].cen[2]  = -1.1;
    objects[0].move[0] = 0.03;
    objects[0].move[1] = 0.03;
    objects[0].move[2] = 0.03;
    objects[0].size[0] = 1.5;
    objects[0].size[1] = 1.5;
    objects[0].size[2] = 1.5;
    objects[0].inc[0]  = 0.0;
    objects[0].inc[1]  = 0.0;
    objects[0].inc[2]  = 0.0;
    objects[1].type    = 2;
    objects[1].bounce  = 0;
    objects[1].cen[0]  = 0.5;
    objects[1].cen[1]  = 0.5;
    objects[1].cen[2]  = 1.76;
    objects[1].move[0] = 0.0;
    objects[1].move[1] = 0.0;
    objects[1].move[2] = -0.025;
    objects[1].size[0] = 0.75;
    objects[1].size[1] = 0.75;
    objects[1].size[2] = 0.75;
    objects[1].inc[0]  = 0.0;
    objects[1].inc[1]  = 0.0;
    objects[1].inc[2]  = 0.0;
}

// Helper for reading in paramters
#define read_param_i(param, var) else if(strcmp(argv[i], param) == 0) var = atoi(argv[++i])
#define read_param_b(param, var) else if(strcmp(argv[i], param) == 0) var = 1

// Prints the help page
void print_help(void) {
    printf("OVERVIEW: Copy of miniAMR without MPI and extended by different Shared Memory parallelization strategies.\n");
    printf("\n");
    printf("USAGE: miniAMR.x [options]\n");
    printf("\n");
    printf("OPTIONS:\n");
    printf("  --nx              block size in x\n");
    printf("  --ny              block size in y\n");
    printf("  --nz              block size in z\n");
    printf("                    These control the number of cells in the blocks in the mesh.  All of these need to be even and greater than zero.  The default is 8 for each variable.\n");
    printf("  --num_refine      number of levels of refinement\n");
    printf("                    This is the number of levels of refinement that blocks which are refined will be refined to.  If it is zero then the mesh will not be refined. The default is 4 levels of refinement.\n");
    printf("  --max_blocks      maximum number of blocks per processor\n");
    printf("                    The maximum number of blocks used per processor. This is the number of blocks that will be allocated at the start of the run and the code will fail if this number is exceeded. The default is 4000 blocks.\n");
    printf("  --init_blocks     exponent that determines the initial number of blocks\n");
    printf("                    A non-negative integer is expected. It is used as exponent over 8. Therefore, the initial number of blocks is 1, 8, 64, 512, .... The default value of the parameter is 0.\n");
    printf("  --num_tsteps      number of time steps (> 0)\n");
    printf("                    The number of time steps for which the simulation will be run.  The default is 20. This is the default method for determining how long the simulation will run.\n");
    printf("  --refine_freq     frequency (time steps) of refinement (0 for none)\n");
    printf("                    This determines how frequently (in time steps) the mesh is checked and refinement is done. The default is every 5 time steps.\n");
    printf("  --stages_per_ts   number of calc stages per time step\n");
    printf("                    The number of calculate stages per time step. The default is 8.\n");
    printf("  --plot            whether to plot the calculation\n");
    printf("                    If this option is turned on, data files will be generated which can then be plotted with the plot tool.\n");
    printf("  --imbalance       determines the imbalance strategy\n");
    printf("                    0: No imbalance strategy\n");
    printf("                    1: Increasing imbalance\n");
    printf("                    2: Decreasing imbalance\n");
    printf("                    3: Random imbalance (with fixed seed)\n");
    printf("                    The default is 0.\n");
    printf("  --calc_factor     determines the intensity of each calculation step\n");
    printf("                    Depending on the imbalance strategy each calculation step gets more computational intensive.\n");
    printf("  --calc_mode       calculation mode\n");
    printf("                    0: 7-point stencil\n");
    printf("                    1: 27-point stencil\n");
    printf("                    2: wavefront\n");
    printf("                    3: reduction\n");
    printf("                    4: cubic wavefront\n");
    printf("                    5: stepwise wavefront\n");
    printf("                    The default is 0.\n");
    printf("  --calc_tiling     Factor for tiling the calculation in every dimension\n");
    printf("                    The default is 1 (no tiling).\n");
    printf("                    Only affects 21x, 30x, 31x.\n");
    printf("  --summary         whether to print summary at the end\n");
    printf("                    The summary at the end is in CSV format. It contains the following information:\n");
    printf("                     0: x block size\n");
    printf("                     1: y block size\n");
    printf("                     2: z block size\n");
    printf("                     3: refinement levels\n");
    printf("                     4: timesteps\n");
    printf("                     5: refinement frequency\n");
    printf("                     6: stages per timestep\n");
    printf("                     7: parallel strategy\n");
    printf("                     8: parallel strategy level\n");
    printf("                     9: imbalance strategy\n");
    printf("                    10: calculation factor\n");
    printf("                    11: calculation mode\n");
    printf("                    12: compiler\n");
    printf("                    13: number of threads\n");
    printf("                    14: calculation time\n");
    printf("                    15: kernel time\n");
    printf("                    16: calculation Flops\n");
    printf("                    17: calculation GFlops/s\n");
    printf("                    18: kernel GFlops/s\n");
    printf("                    19: devicer number\n");
    printf("                    20: pure calculation time\n");
    printf("                    21: calculation tiling\n");
}

// From header
void read_params(int argc, char** argv) {
    set_default_values();
    for(int i = 1; i < argc; ++i) {
        if(strcmp(argv[i], "--help") == 0) {
            print_help();
            free(objects);
            exit(0);
        } else if(strcmp(argv[i], "--version") == 0) {
            printf("miniAMR 2\nVersion %s\n", MINIAMR2_VERSION);
            free(objects);
            exit(0);
        }
        read_param_i("--nx", x_block_size);
        read_param_i("--ny", y_block_size);
        read_param_i("--nz", z_block_size);
        read_param_i("--num_refine", num_refine);
        read_param_i("--max_blocks", max_num_blocks);
        read_param_i("--init_blocks", init_blocks);
        read_param_i("--num_tsteps", num_tsteps);
        read_param_i("--checksum_freq", checksum_freq);
        read_param_i("--refine_freq", refine_freq);
        read_param_i("--stages_per_ts", stages_per_ts);
        read_param_b("--plot", plot);
        read_param_i("--imbalance", imbalance);
        read_param_i("--calc_factor", calc_factor);
        read_param_i("--calc_mode", calc_mode);
        read_param_i("--calc_tiling", calc_tiling);
        read_param_b("--summary", summary);
        else {
            printf("Error: Unknown input parameter %s\n", argv[i]);
            free(objects);
            exit(1);
        }
    }
}
