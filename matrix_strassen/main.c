
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>

typedef long long lld;

typedef struct matrix
{
    lld * v;
    int w;
    int n;
} matrix;

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

int main(int argc, char** argv)
{
    struct timespec begin, end;

    matrix a = new_matrix(4096);
    matrix b = new_matrix(4096);

    FILE * fd = fopen("matrices/sample1.txt", "r");
    for(int i=0; i<4000; ++i) {
        for(int j=0; j<4000; ++j) {
            fscanf(fd, "%lld", a.v + (i*4000 + j));
        }
    }
    fclose(fd);

    fd = fopen("matrices/sample2.txt", "r");
    for(int i=0; i<4000; ++i) {
        for(int j=0; j<4000; ++j) {
            fscanf(fd, "%lld", b.v + (i*4000 + j));
        }
    }
    fclose(fd);

    printf("Read all\n");

    clock_gettime(CLOCK_MONOTONIC, &begin);
    matrix c = matmul(a, b);
    clock_gettime(CLOCK_MONOTONIC, &end);

    double time = (end.tv_sec - begin.tv_sec) + (end.tv_nsec - begin.tv_nsec) / 1000000000.0f;

    printf("time: %f\n", time);

    free(c.v);

    return 0;
}
