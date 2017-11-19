
#ifndef __MATRIX_H__
#define __MATRIX_H__

#include <stdio.h>
#include <string.h>

typedef long long lld;

struct matrix
{
    lld * v;
    int m;
    int n;
};

matrix * read_mat(const char * filepath);
matrix * create_mat(int m, int n);
void save_mat(matrix * mat);
long long cuda_matmul(lld * A, lld * B, lld * C, int m, int n, int l);
__global__ void cuda_matmul_unit(lld * A, lld * B, lld * C, int n, int l);
long long mat_sum_element(matrix * mat);

#endif

