
#ifndef __MATRIX_H__
#define __MATRIX_H__

#include <stdio.h>
#include <string.h>

struct matrix
{
    float * v;
    int m;
    int n;
};

matrix * read_mat(const char * filepath);
matrix * create_mat(int m, int n);
long long cuda_matmul(float * A, float * B, float * C, int m, int n, int l);
__global__ void cuda_matmul_unit(float * A, float * B, float * C, int n, int l);
long long mat_sum_element(matrix * mat);

#endif

