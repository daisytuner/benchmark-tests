#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "backprop.h"
#include "omp.h"

extern char *strcpy();
extern void exit();

int layer_size = 0;

backprop_face()
{
  BPNN *net;
  int i;
  float out_err, hid_err;
  net = bpnn_create(layer_size, 16, 1); // (16, 1 can not be changed)
  printf("Input layer size : %d\n", layer_size);
  load(net);
  //entering the training kernel, only one iteration
  printf("Starting training kernel\n");
  double start = omp_get_wtime();
  bpnn_train_kernel(net, &out_err, &hid_err);
  double end = omp_get_wtime();
  printf("Training done\n");

#ifdef RODINIA_DUMP_ARRAYS
	printf("BEGIN DUMP OUTPUT\n");
    for (int m = 0; m < net->hidden_n + 1; m++){
      for(int l = 0; l < net->output_n + 1; l++){
        printf("%0.9g ", net->hidden_weights[m][l]);
    }
  }
  printf("\n");
	printf("END DUMP OUTPUT\n");
#endif
#ifdef RODINIA_TIME
  printf("%f\n", (end-start));
#endif
  bpnn_free(net);

}

int setup(argc, argv)
int argc;
char *argv[];
{
  if(argc!=2){
  fprintf(stderr, "usage: backprop <num of input elements>\n");
  exit(0);
  }

  layer_size = atoi(argv[1]);
  
  int seed;

  seed = 7;   
  bpnn_initialize(seed);
  backprop_face();

  exit(0);
}
