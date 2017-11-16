/*
 * Structure and Functions about matrices
 */

#include <stdlib.h>
#include <pthread.h>

typedef int m_element;

typedef struct matrix {
    m_element ** v;
    int m;
    int n;
} matrix;

typedef struct mat_mul_arg {
    int r_begin;
    int r_end;
    matrix * p_arg1;
    matrix * p_arg2;
    matrix * p_dest;
} mat_mul_arg;

void init_matrix(matrix * p_mat, int m, int n);
void destroy_matrix(matrix * p_mat);
void mul_matrix_mt(matrix * p_arg1, matrix * p_arg2, matrix * p_dest, int num_threads);
void* mul_matrix_mt_thread(void * p_arg);
