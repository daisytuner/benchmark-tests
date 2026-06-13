#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>

#include <omp.h>

#include "timer.h"

#define M_SEED 9

#define MIN(a, b) ((a)<=(b) ? (a) : (b))

int main(int argc, char** argv)
{
    int64_t rows, cols;
    int64_t* data;
    int64_t** wall;
    int64_t* result;

	if(argc==3){
		cols = atoi(argv[1]);
		rows = atoi(argv[2]);
	}else{
                printf("Usage: pathfiner width num_of_steps\n");
                exit(0);
        }
	data = new int64_t[rows*cols];
	wall = new int64_t*[rows];
	for(int64_t n=0; n<rows; n++)
		wall[n]=data+cols*n;
	result = new int64_t[cols];
	
	int64_t seed = M_SEED;
	srand(seed);

	for (int64_t i = 0; i < rows; i++)
    {
        for (int64_t j = 0; j < cols; j++)
        {
            wall[i][j] = rand() % 10;
        }
    }
    for (int64_t j = 0; j < cols; j++)
        result[j] = wall[0][j];

    int64_t *src, *dst, *temp;
    int64_t min;

    dst = result;
    src = new int64_t[cols];

    double start_time = omp_get_wtime();
    for (int64_t t = 0; t < rows-1; t++) {
        temp = src;
        src = dst;
        dst = temp;

        for(int64_t n = 0; n < cols; n++){
          min = src[n];
          if (n > 0)
            min = MIN(min, src[n-1]);
          if (n < cols-1)
            min = MIN(min, src[n+1]);
          dst[n] = wall[t+1][n]+min;
        }
    }
    double end_time = omp_get_wtime();

#ifdef RODINIA_DUMP_ARRAYS
	printf("BEGIN DUMP OUTPUT\n");
    for (int i = 0; i < cols; i++)
        printf("%d ",dst[i]) ;
    printf("\n");
	printf("END DUMP OUTPUT\n");
#endif
#ifdef RODINIA_TIME
	printf("%f\n", end_time - start_time);
#endif

    delete [] data;
    delete [] wall;
    delete [] dst;
    delete [] src;
    return EXIT_SUCCESS;
}

