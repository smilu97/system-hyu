
#include "matrix.h"

void matcpy(matrix * dest, matrix * src, int sz, int offset)
{
    for(int i=0; i<sz; ++i) {
        for(int j=0; j<sz; ++j) {
            dest->v[i*dest->n + j] = src->v[offset + i*src->n + j];
        }
    }
}
void matadd(matrix * dest, matrix * src, int sz, int offset)
{
    for(int i=0; i<sz; ++i) {
        for(int j=0; j<sz; ++j) {
            dest->v[i*dest->n + j] += src->v[offset + i*src->n + j];
        }
    }
}
void matsub(matrix * dest, matrix * src, int sz, int offset)
{
    for(int i=0; i<sz; ++i) {
        for(int j=0; j<sz; ++j) {
            dest->v[i*dest->n + j] -= src->v[offset + i*src->n + j];
        }
    }
}



matrix new_matrix(int n)
{
    matrix ret;
    ret.v = (lld*)malloc(sizeof(lld)*n*n);
    ret.n = n;
    memset(ret.v, 0, sizeof(lld)*n*n);
    return ret;
}

matrix matmul(matrix a, matrix b)
{
    int n = a.n;
    int n2 = n/2;
    matrix ret = new_matrix(n);
    matrix m[7];

    if(n == 32) {
        for(int i=0; i<n; ++i) for(int j=0; j<n; ++j) {
            lld tmp = 0;
            for(int k=0; k<n; ++k) {
                tmp += a.v[i*n+k] * b.v[k*n+j];
            }
            ret.v[i*n+j] = tmp;
        }
        return ret;
    }

    matrix q = new_matrix(n/2);
    matrix w = new_matrix(n/2);

    // M1
    matcpy(&q, &a, n2, 0);
    matadd(&q, &a, n2, (n*n+n)/2);
    matcpy(&w, &b, n2, 0);
    matadd(&w, &b, n2, (n*n+n)/2);
    m[0] = matmul(q, w);

    //M2
    matcpy(&q, &a, n2, n*n/2);
    matadd(&q, &a, n2, (n*n+n)/2);
    matcpy(&w, &b, n2, 0);
    m[1] = matmul(q, w);

    //M3
    matcpy(&q, &a, n2, 0);
    matcpy(&w, &b, n2, n2);
    matsub(&w, &b, n2, (n*n+n)/2);
    m[2] = matmul(q, w);

    //M4
    matcpy(&q, &a, n2, (n*n+n)/2);
    matcpy(&w, &b, n2, n*n/2);
    matsub(&w, &b, n2, 0);
    m[3] = matmul(q, w);

    //M5
    matcpy(&q, &a, n2, 0);
    matadd(&q, &a, n2, n2);
    matcpy(&w, &b, n2, (n*n+n)/2);
    m[4] = matmul(q, w);
    
    //M6
    matcpy(&q, &a, n2, n*n/2);
    matsub(&q, &a, n2, 0);
    matcpy(&w, &b, n2, 0);
    matadd(&w, &b, n2, n2);
    m[5] = matmul(q, w);

    //M7
    matcpy(&q, &a, n2, n2);
    matsub(&q, &a, n2, (n*n+n)/2);
    matcpy(&w, &b, n2, n*n/2);
    matadd(&w, &b, n2, (n*n+n)/2);
    m[6] = matmul(q, w);

    // C11
    matcpy(&q, m, n2, 0);
    matadd(&q, m+3, n2, 0);
    matsub(&q, m+4, n2, 0);
    matadd(&q, m+6, n2, 0);
    matcpy(&ret, &q, n2, 0);

    // C12
    matcpy(&q, m+2, n2, 0);
    matadd(&q, m+4, n2, 0);
    ret.v += n2;
    matcpy(&ret, &q, n2, 0);
    ret.v -= n2;

    // C21
    matcpy(&q, m+1, n2, 0);
    matadd(&q, m+3, n2, 0);
    ret.v += n*n/2;
    matcpy(&ret, &q, n2, 0);
    
    // C22
    matcpy(&q, m, n2, 0);
    matsub(&q, m+1, n2, 0);
    matadd(&q, m+2, n2, 0);
    matadd(&q, m+5, n2, 0);
    ret.v += n2;
    matcpy(&ret, &q, n2, 0);
    ret.v -= (n*n+n)/2;

    free(q.v);
    free(w.v);
    for(int i=0; i<7; ++i) free(m[i].v);

    return ret;
}

void * matmul_8_work(void * varg)
{
    matmul_8_arg * arg = (matmul_8_arg*)varg;

    int n = arg->a.n;
    int n2 = n/2;

    matrix na = new_matrix(n2);
    matrix nb = new_matrix(n2);
    matcpy(&na, &(arg->a), n2, 0);
    matcpy(&nb, &(arg->b), n2, 0);

    matrix k = matmul(na, nb);
    matadd(&(arg->c), &k, n2, 0);

    free(k.v);
    free(na.v);
    free(nb.v);

    return 0;
}

matrix matmul_8(matrix a, matrix b)
{
    matmul_8_arg args[8];
    pthread_t threads[8];
    int th_idx = 0;

    int n = a.n;
    int n2 = n/2;
    int nn2 = n*n/2;

    matrix c = new_matrix(n);

    for(int i=0; i<2; ++i) {
        for(int j=0; j<2; ++j) {
            for(int k=0; k<2; ++k) {
                args[th_idx].a = a;
                args[th_idx].a.v += i * nn2 + k * n2;
                args[th_idx].b = b;
                args[th_idx].b.v += k * nn2 + j * n2;
                args[th_idx].c = c;
                args[th_idx].c.v += i * nn2 + j * n2;
                pthread_create(threads+th_idx, NULL, matmul_8_work, args+th_idx);
                ++th_idx;
            }
        }
    }

    for(int i=0; i<8; ++i) {
        pthread_join(threads[i], NULL);
    }

    return c;
}
