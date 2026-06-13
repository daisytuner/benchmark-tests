#include "zero.h"

#include <iostream>

double cube(double x);

double square(double x);

int main(int argc, char** argv) {
    if (argc < 2) return 1;

    double val = atof(argv[1]);
    double sq = square(val);
    double cb = cube(val);

    std::cout << "Square: " << sq << std::endl;
    std::cout << "Cube: " << cb << std::endl;

    return 0;
}
