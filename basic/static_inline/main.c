#include "mul.h"

#include <stdio.h>
#include <stdlib.h>

double cube(double x);

double square(double x);

int main(int argc, char** argv) {
    if (argc < 2) return 1;

    double val = atof(argv[1]);
    double sq = square(val);
    double cb = cube(val);

    printf("Square: %f\n", sq);
    printf("Cube: %f\n", cb);

    return 0;
}
