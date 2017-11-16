/*
 * Structure and Functions about matrices
 */

#include "matrix.h"

void init_matrix(matrix * p_mat, int m, int n)
{
    p_mat->v = (m_element**)malloc(sizeof(m_element*) * m);
    m_element * cont = (m_element*)malloc(sizeof(m_element) * m * n);
    for(int i=0; i<m; ++i) {
        p_mat->v[i] = cont + (n * i);
    }
    p_mat->m = m;
    p_mat->n = n;
}

void destroy_matrix(matrix * p_mat)
{
    free(p_mat->v[0]);
    free(p_mat->v);
}

void mul_matrix(matrix * p_arg1, matrix * p_arg2, matrix * p_dest)
{
    // Not multiplicat-able
    if(p_arg1->n != p_arg2->m) return;

    int m = p_arg1->m;
    int n = p_arg1->n;
    int l = p_arg2->n;

    for(int i=0; i<m; ++i) {
        for(int j=0; j<l; ++j) {
            p_dest->v[i][j] = 0;
            for(int k=0; k<n; ++k) {
                p_dest->v[i][j] += p_arg1->v[i][k] * p_arg2->v[k][j];
            }
        }
    }
}

void mul_add(matrix * p_arg1, matrix * p_arg2, matrix * p_dest)
{
    if(p_arg1->m != p_arg2->m) return;
    if(p_arg2->n != p_arg2->n) return;
    if(p_arg1->m != p_dest->m) return;
    if(p_arg1->n != p_dest->n) return;

    int m = p_arg1->m, n = p_arg2->n;

    for(int i=0; i<m; ++i) {
        for(int j=0; j<n; ++j) {
            p_dest->v[i][j] = p_arg1->v[i][j] + p_arg2->v[i][j];
        }
    }
}
