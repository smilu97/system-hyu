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

void read_matrix(matrix * p_mat, const char * filepath)
{
    FILE * fd = fopen(filepath, "r");

    int cap = 16000000;
    int * arr = (int*)malloc(sizeof(int)*cap);
    int idx = 0, tmp;
    int m = -1;

    while(~fscanf(fd, "%d", &tmp)) {
        arr[idx++] = tmp;
        if(idx >= cap) {
            int * new_arr = (int*)malloc(sizeof(int)*cap*2);
            memcpy(new_arr, arr, sizeof(int)*cap);
            cap <<= 1;
            free(arr);
            arr = new_arr;
        }
        if(m == -1 && fgetc(fd) == '\n') {
            m = idx;
        }
    }

    fclose(fd);

    int n = idx / m;

    init_matrix(p_mat, m, n);
    idx = 0;
    for(int i=0; i<m; ++i) {
        for(int j=0; j<n; ++j) {
            p_mat->v[i][j] = arr[idx++];
        }
    }

#ifdef PRINT_DEBUG
    printf("read: %s(m: %d, n: %d)\n", filepath, m, n);
#endif

    free(arr);
}

/*
 * This function multiplicate two matrix with multi threading
 * 
 * It divides works. Each thread calculate partial rows of dest matrix.
 * 
 * For example, The matrix size is 4, and num_threads is 4,
 *      Each thread calculate only one row of dest matrix
 */
long long mul_matrix_mt(matrix * p_arg1, matrix * p_arg2, matrix * p_dest, int num_threads)
{
    // Not multiplicat-able
    if(p_arg1->n != p_arg2->m) return 0;
    if(p_dest->m != p_arg1->m) return 0;
    if(p_dest->m != p_arg2->n) return 0;

    int m = p_arg1->m;
    int n = p_arg1->n;
    int l = p_arg2->n;

    int batch_size = m / num_threads;
    int one_more_threshold = num_threads - (m % num_threads) + 1;

    pthread_t * threads = (pthread_t*)malloc(sizeof(pthread_t) * num_threads);
    mat_mul_arg * args = (mat_mul_arg*)malloc(sizeof(mat_mul_arg) * num_threads);

    pthread_barrier_t bar;
    pthread_barrier_init(&bar, NULL, num_threads);
    
    int begin_cur = 0;
    for(int i=0; i<num_threads; ++i) {
        if(i == one_more_threshold) ++batch_size;

        args[i].r_begin = begin_cur;
        args[i].r_end = begin_cur + batch_size;
        args[i].p_arg1 = p_arg1;
        args[i].p_arg2 = p_arg2;
        args[i].p_dest = p_dest;
        args[i].bar = &bar;

        begin_cur += batch_size;

        pthread_create(threads + i, NULL, mul_matrix_mt_thread, args + i);
    }

    long long sum = 0;

    for(int i=0; i<num_threads; ++i) {
        pthread_join(threads[i], NULL);
        sum += args[i].row_sum;
    }

    free(threads);
    free(args);

    return sum;
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

    long long row_sum = 0;

    for(int i=r_begin; i<r_end; ++i) {
        for(int j=0; j<l; ++j) {
            p_dest->v[i][j] = 0;
            for(int k=0; k<n; ++k) {
                p_dest->v[i][j] += p_arg1->v[i][k] * p_arg2->v[k][j];
            }
            row_sum += p_dest->v[i][j];
        }
    }

    arg->row_sum = row_sum;

    pthread_barrier_wait(arg->bar);

    return NULL;
}
