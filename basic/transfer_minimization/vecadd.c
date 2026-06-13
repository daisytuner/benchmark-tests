#include <stdlib.h>
#include <string.h>

#define N 100

float* vecadd_alloc_0(float* x, float* y, float* z) {
    return (float*) malloc(N * sizeof(float));
}

float* vecadd_in_0(float* x, float* y, float* z, float* d_x) {
    memcpy(d_x, x, N * sizeof(float));
    return d_x;
}

float* vecadd_alloc_1(float* x, float* y, float* z) {
    return (float*) malloc(N * sizeof(float));
}

float* vecadd_in_1(float*x, float* y, float* z, float* d_y) {
    memcpy(d_y, y, N * sizeof(float));
    return d_y;
}

float* vecadd_alloc_2(float*x, float*y, float* z) {
    return (float*) malloc(N * sizeof(float));
}

void vecadd_kernel(float* x, float* y, float* z, float* d_x, float* d_y, float* d_z) {
    for (int i = 0; i < N; i++) {
        d_z[i] = d_x[i] + d_y[i];
    }
}

float* vecadd_free_0(float* x, float* y, float* z, float* d_x) {
    free(d_x);
    return d_x;
}

float* vecadd_free_1(float* x, float* y, float* z, float* d_y) {
    free(d_y);
    return d_y;
}

void vecadd_out_2(float* x, float* y, float* z, float* d_z) {
    memcpy(z, d_z, N * sizeof(float));
}

float* vecadd_free_2(float* x, float* y, float* z, float* d_z) {
    free(d_z);
    return d_z;
}

void vecadd(float* x, float* y, float* z) {
    float* d_x = vecadd_alloc_0(x, y, z);
    d_x = vecadd_in_0(x, y, z, d_x);
    float* d_y = vecadd_alloc_1(x, y, z);
    d_y = vecadd_in_1(x, y, z, d_y);
    float* d_z = vecadd_alloc_2(x, y, z);
    vecadd_kernel(x, y, z, d_x, d_y, d_z);
    d_x = vecadd_free_0(x, y, z, d_x);
    d_y = vecadd_free_1(x, y, z, d_y);
    vecadd_out_2(x, y, z, d_z);
    d_z = vecadd_free_2(x, y, z, d_z);
}