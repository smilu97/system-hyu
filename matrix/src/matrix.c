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

/*
 * This function multiplicate two matrix with multi threading
 * 
 * It divides works. Each thread calculate partial rows of dest matrix.
 * 
 * For example, The matrix size is 4, and num_threads is 4,
 *      Each thread calculate only one row of dest matrix
 */
void mul_matrix_mt(matrix * p_arg1, matrix * p_arg2, matrix * p_dest, int num_threads)
{
    // Not multiplicat-able
    if(p_arg1->n != p_arg2->m) return;
    if(p_dest->m != p_arg1->m) return;
    if(p_dest->m != p_arg2->n) return;

    int m = p_arg1->m;
    int n = p_arg1->n;
    int l = p_arg2->n;

    int batch_size = m / num_threads;
    int one_more_threshold = num_threads - (m % num_threads) + 1;

    mat_mul_arg * args = (mat_mul_arg*)malloc(sizeof(mat_mul_arg) * num_threads);
    
    int begin_cur = 0;
    for(int i=0; i<num_threads; ++i) {
        if(i == one_more_threshold) ++batch_size;

        args[i].r_begin = begin_cur;
        args[i].r_end = begin_cur + batch_size;
        args[i].p_arg1 = p_arg1;
        args[i].p_arg2 = p_arg2;
        args[i].p_dest = p_dest;

        begin_cur += batch_size;
    }

    pthread_t * threads = (pthread_t*)malloc(sizeof(pthread_t) * num_threads);
    for(int i=0; i<num_threads; ++i) {
        pthread_create(threads + i, NULL, mul_matrix_mt_thread, args + i);
    }
    for(int i=0; i<num_threads; ++i) {
        pthread_join(threads[i], NULL);
    }

    free(threads);
    free(args);
}

void * mul_matrix_mt_thread(void * p_arg)
{
    mat_mul_arg * arg = (mat_mul_arg*)p_arg;
    
    matrix * p_arg1 = arg->p_arg1;
    matrix * p_arg2 = arg->p_arg2;
    matrix * p_dest = arg->p_dest;
    int r_begin = arg->r_begin, r_end = arg->r_end;

    int n = p_arg1->n;
    int l = p_arg2->n;

    for(int i=r_begin; i<r_end; ++i) {
        for(int j=0; j<l; ++j) {
            p_dest->v[i][j] = 0;
            for(int k=0; k<n; ++k) {
                p_dest->v[i][j] += p_arg1->v[i][k] * p_arg2->v[k][j];
            }
        }
    }

    return NULL;
}
