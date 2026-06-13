//
// Created by ramon on 15.07.25.
//

#pragma once

void spmvCPU(float *y, float *A, int *IA, int *JA, int M, const float *x);

void printArray(const float* arr, int l);

void printArray(const int* arr, int l);

bool areEqualRMSE(const float *a, const float *b, int N);

void printRMSE(const float *a, const float *b, int N);

void printSpMatrix(const float* A, const int* IA, const int* JA, int M, int N);

void generateSquareSpMatrix(float **A_p, int **IA_p, int **JA_p, int *NNZ_p, int N, double p_diag, double p_nondiag);

void resizeSpMatrixArraysAndCopy(float **A_temp_p, int **JA_temp_p, int *bufferSize_p, double RESIZE_FACTOR);

float getRandomFloat(float min, float max);

void fillDenseVector(float* v, int N);