/*
 * Structure and Functions about matrices
 */

#include <stdlib.h>

typedef int m_element;

typedef struct matrix {
    m_element ** v;
    int m;
    int n;
} matrix;

void init_matrix(matrix * p_mat, int m, int n);
void destroy_matrix(matrix * p_mat);
void mul_matrix(matrix * p_arg1, matrix * p_arg2, matrix * p_dest);
void mul_add(matrix * p_arg1, matrix * p_arg2, matrix * p_dest);
