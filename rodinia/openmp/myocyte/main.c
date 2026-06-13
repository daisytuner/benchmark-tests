//====================================================================================================100
//		UPDATE
//====================================================================================================100

// Lukasz G. Szafaryn 24 JAN 09

//====================================================================================================100
//		DESCRIPTION
//====================================================================================================100

// Myocyte application models cardiac myocyte (heart muscle cell) and simulates its behavior according to the work by Saucerman and Bers [8]. The model integrates 
// cardiac myocyte electrical activity with the calcineurin pathway, which is a key aspect of the development of heart failure. The model spans large number of temporal 
// scales to reflect how changes in heart rate as observed during exercise or stress contribute to calcineurin pathway activation, which ultimately leads to the expression 
// of numerous genes that remodel the heart�s structure. It can be used to identify potential therapeutic targets that may be useful for the treatment of heart failure. 
// Biochemical reactions, ion transport and electrical activity in the cell are modeled with 91 ordinary differential equations (ODEs) that are determined by more than 200 
// experimentally validated parameters. The model is simulated by solving this group of ODEs for a specified time interval. The process of ODE solving is based on the 
// causal relationship between values of ODEs at different time steps, thus it is mostly sequential. At every dynamically determined time step, the solver evaluates the 
// model consisting of a set of 91 ODEs and 480 supporting equations to determine behavior of the system at that particular time instance. If evaluation results are not 
// within the expected tolerance at a given time step (usually as a result of incorrect determination of the time step), another calculation attempt is made at a modified 
// (usually reduced) time step. Since the ODEs are stiff (exhibit fast rate of change within short time intervals), they need to be simulated at small time scales with an 
// adaptive step size solver. 

//	1) The original version of the current solver code was obtained from: Mathematics Source Library (http://mymathlib.webtrellis.net/index.html). The solver has been 
//      somewhat modified to tailor it to our needs. However, it can be reverted back to original form or modified to suit other simulations.
// 2) This solver and particular solving algorithm used with it (embedded_fehlberg_7_8) were adapted to work with a set of equations, not just one like in original version.
//	3) In order for solver to provide deterministic number of steps (needed for particular amount of memore previousely allocated for results), every next step is 
//      incremented by 1 time unit (h_init).
//	4) Function assumes that simulation starts at some point of time (whatever time the initial values are provided for) and runs for the number of miliseconds (xmax) 
//      specified by the uses as a parameter on command line.
// 5) The appropriate amount of memory is previousely allocated for that range (y).
//	6) This setup in 3) - 5) allows solver to adjust the step ony from current time instance to current time instance + 0.9. The next time instance is current time instance + 1;
//	7) The original solver cannot handle cases when equations return NAN and INF values due to discontinuities and /0. That is why equations provided by user need to 
//      make sure that no NAN and INF are returned.
// 8) Application reads initial data and parameters from text files: y.txt and params.txt respectively that need to be located in the same folder as source files. 
//     For simplicity and testing purposes only, when multiple number of simulation instances is specified, application still reads initial data from the same input files. That 
//     can be modified in this source code.

//====================================================================================================100
//		IMPLEMENTATION-SPECIFIC DESCRIPTION (OPEN MP)
//====================================================================================================100

// This is the OpenMP version of Myocyte code.

// The original single-threaded code was written in MATLAB and used MATLAB ode45 ODE solver. In the process of accelerating this code, we arrived with the 
// intermediate versions that used single-threaded Sundials CVODE solver which evaluated model parallelized with OpenMP at each time step. In order to convert entire 
// solver to OpenMP code (to remove some of the operational overheads such as thread launches in OpenMP) we used a simpler solver, from Mathematics Source 
// Library, and tailored it to our needs. The parallelism in the cardiac myocyte model is on a very fine-grained level, close to that of ILP, therefore it is very hard to exploit 
// as DLP or TLB in OpenMP code. We were able to divide the model into 4 individual groups that run in parallel. However, even that is not enough work to compensate 
// for some of the OpenMP thread launch overheads, which resulted in performance worse than that of single-threaded C code. Speedup in this code could 
// be achieved only if a customizable accelerator such as FPGA was used for evaluation of the model itself. We also approached the application from another angle and 
// allowed it to run several concurrent simulations, thus turning it into an embarrassingly parallel problem. This version of the code is also useful for scientists who want to 
// run the same simulation with different sets of input parameters. OpenMP version of this code provides constant speedup of about 3.64x regardless of the number of 
// concurrent simulations.

// Speedup numbers reported in the description of this application were obtained on the machine with: Intel Quad Core CPU, 4GB of RAM, Nvidia GTX280 GPU.  

// 1) When running with parallelization inside each simulation instance (value of 3rd command line parameter equal to 0), performance is bad because:
// a) thread launch overhead
// b) small amount of work for each forked thread
// 2) When running with parallelization across simulation instances, code gets continues speedup with the increasing number of simulation insances which saturates
//     around 4 instances on Quad Core CPU (roughly corresponding to the number of multiprocessorsXprocessors in GTX280), with the speedup of around 3.5x compared
//     to serial C version of code, as expected.

// The following are the command parameters to the application:
// 1) Simulation time interval which is the number of miliseconds to simulate. Needs to be integer > 0
// 2) Number of instances of simulation to run. Needs to be integer > 0.
// 3) Method of parallelization. Need to be 0 for parallelization inside each simulation instance, or 1 for parallelization across instances.
// 4) Number of threads to use. Needs to be integer > 0.
// Example:
// a.out 100 100 1 4

//====================================================================================================100
//	DEFINE / INCLUDE
//====================================================================================================100

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include <omp.h>

#include "define.h"
#include "solver.h"
#include "file.h"

//====================================================================================================100
//	MAIN FUNCTION
//====================================================================================================100

int main(int argc, char *argv []){

	//================================================================================80
	//		VARIABLES
	//================================================================================80

	//============================================================60
	//		TIME
	//============================================================60

	//============================================================60
	//		COUNTERS
	//============================================================60

	long long memory;
	int i,j;
	int status;

	//============================================================60
	//		SOLVER PARAMETERS
	//============================================================60

	long workload;
	long xmin;
	long xmax;
	fp h;
	fp tolerance;

	//============================================================60
	//		DATA
	//============================================================60

	fp*** y;
	fp** x;
	fp** params;

	//================================================================================80
	// 	GET INPUT PARAMETERS
	//================================================================================80

	//============================================================60
	//		CHECK NUMBER OF ARGUMENTS
	//============================================================60

	if(argc!=3){
		printf("ERROR: %d is the incorrect number of arguments, the number of arguments must be 4\n", argc-1);
		return 0;
	}

	//============================================================60
	//		GET AND CHECK PARTICULAR ARGUMENTS
	//============================================================60

	else{

		//========================================40
		//		SPAN
		//========================================40

		xmax = atoi(argv[1]);
		if(xmax<0){
			printf("ERROR: %d is the incorrect end of simulation interval, use numbers > 0\n", xmax);
			return 0;
		}

		//========================================40
		//		WORKLOAD
		//========================================40

		workload = atoi(argv[2]);
		if(workload<0){
			printf("ERROR: %d is the incorrect number of instances of simulation, use numbers > 0\n", workload);
			return 0;
		}
	}

	//================================================================================80
	// 	ALLOCATE MEMORY
	//================================================================================80

	//============================================================60
	//		MEMORY CHECK
	//============================================================60

	memory = workload*(xmax+1)*EQUATIONS*4;
	if(memory>1000000000){
		printf("ERROR: trying to allocate more than 1.0GB of memory, decrease workload and span parameters or change memory parameter\n");
		return 0;
	}

	//============================================================60
	// 	ALLOCATE ARRAYS
	//============================================================60

	y = (fp ***) malloc(workload* sizeof(fp **));
	for(i=0; i<workload; i++){
		y[i] = (fp**)malloc((1+xmax)*sizeof(fp*));
		for(j=0; j<(1+xmax); j++){
			y[i][j]= (fp *) malloc(EQUATIONS* sizeof(fp));
		}
	}

	x = (fp **) malloc(workload * sizeof(fp *));
	for (i= 0; i<workload; i++){
		x[i]= (fp *)malloc((1+xmax) *sizeof(fp));
	}

	params = (fp **) malloc(workload * sizeof(fp *));
	for (i= 0; i<workload; i++){
		params[i]= (fp *)malloc(PARAMETERS * sizeof(fp));
	}

	//================================================================================80
	// 	INITIAL VALUES
	//================================================================================80

	// y
	for(i=0; i<workload; i++){
		read(	"rodinia/data/myocyte/y.txt.data",
					y[i][0],
					91,
					1,
					0);
	}

	// params
	for(i=0; i<workload; i++){
		read(	"rodinia/data/myocyte/params.txt.data",
					params[i],
					16,
					1,
					0);
	}

	//================================================================================80
	//	EXECUTION
	//================================================================================80

	double start_time = omp_get_wtime();
	for(i=0; i<workload; i++){

		status = solver(	y[i],
									x[i],
									xmax,
									params[i]);


	}
	double end_time = omp_get_wtime();

	// print results
#ifdef RODINIA_DUMP_ARRAYS
	int k;
	printf("BEGIN DUMP OUTPUT\n");
	for(i=0; i<workload; i++){
		for(j=0; j<(xmax+1); j++){
			for(k=0; k<EQUATIONS; k++){
				printf("%f\n", y[i][j][k]);
			}
		}
	}
	printf("END DUMP OUTPUT\n");
#endif
#ifdef RODINIA_TIME
	printf("%f\n", end_time - start_time);
#endif

	//================================================================================80
	//	DEALLOCATION
	//================================================================================80

	// y values
	for (i= 0; i< workload; i++){
		for (j= 0; j< (1+xmax); j++){
			free(y[i][j]);
		}
		free(y[i]);
	}
	free(y);

	// x values
	for (i= 0; i< workload; i++){
		free(x[i]);
	}
	free(x);

	// parameters
	for (i= 0; i< workload; i++){
		free(params[i]);
	}
	free(params);

//====================================================================================================100
//	END OF FILE
//====================================================================================================100

	return 0;
}
