#ifndef _PROFILE_INCLUDE_
#define _PROFILE_INCLUDE_

#define MINIAMR2_VERSION "1.2.1"

// Time needed for executing whole calculation
extern double calc_time, calc_time_min, calc_time_max;

// Pure time needed for executing calculation
extern double pure_calc_time;

// Operations needed for executing calculation
extern unsigned long long calc_ops, calc_ops_min, calc_ops_max;

// Time needed for executing complete kernel
extern double kernel_time;

// Get the current time
double timer(void);

// Start the timer for the pure calculation time
void start_pure_calc_timer(void);

// Stop the timer for the pure calculation time
void stop_pure_calc_timer(void);

// Outputs a few profiling values at the end
void profile(void);

#endif
