
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

typedef long long lld;

typedef struct matrix
{
    lld * v;
    int w;
    int n;
} matrix;

void matcpy(matrix * dest, matrix * src, int sz, int offset);
void matadd(matrix * dest, matrix * src, int sz, int offset);
void matsub(matrix * dest, matrix * src, int sz, int offset);

matrix new_matrix(int n);
matrix matmul(matrix a, matrix b);

typedef struct matmul_8_arg
{
    matrix a, b, c;
} matmul_8_arg;

void * matmul_8_work(void * varg);
matrix matmul_8(matrix a, matrix b);