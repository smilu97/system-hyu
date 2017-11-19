
#include "matrix.h"

matrix * read_mat(const char * filepath)
{
     FILE * fd = fopen(filepath, "r");
     if(fd == NULL) {
         fprintf(stderr, "Failed to open matrix file\n");
         return NULL;
     }

     int cap = 16000000;
     int size = 0;
     float * arr = (float*)malloc(sizeof(float)*cap);
     if(arr == NULL) {
         fprintf(stderr, "Failed to allocate memory to load matrix\n");
         fclose(fd);
         return NULL;
     }

     int col_size = -1;
     int tmp;
     while(~fscanf(fd, "%d", &tmp)) {
         if(size >= cap) {
             float * narr = (float*)malloc(sizeof(float)*cap*2);
             if(narr == NULL) {
                 fprintf(stderr, "Matrix is too big!\n");
                 fclose(fd);
                 free(arr);
                 return NULL;
             }
             memcpy(narr, arr, sizeof(float)*size);
             free(arr);
             arr = narr;
         }
         arr[size++] = (float)tmp;
         if(fgetc(fd) == '\n' && col_size == -1) {
             col_size = size;
         }
     }
     fclose(fd);

     matrix * ret = create_mat(size / col_size, col_size);
     if(ret == NULL) {
         fprintf(stderr, "Failed to create_mat: read_mat\n");
         free(arr);
         return NULL;
     }
     memcpy(ret->v, arr, size);
     free(arr);

     return ret;
}

matrix * create_mat(int m, int n)
{
    matrix * ret = (matrix*)malloc(sizeof(matrix));
    if(ret == NULL) {
        fprintf(stderr, "Failed to allocate ret: create_mat\n");
        return NULL;
    }
    ret->v = (float*)malloc(sizeof(float)*m*n);
    if(ret->v == NULL) {
        fprintf(stderr, "Toob big too create: create_mat()");
        free(ret);
        return NULL;
    }
    ret->m = m;
    ret->n = n;
    
    return ret;
}

long long cuda_matmul(float * A, float * B, float * C, int m, int n, int l)
{
    float *cA, *cB, *cC;

    cudaMalloc(&cA, sizeof(float)*m*n);
    cudaMalloc(&cB, sizeof(float)*n*l);
    cudaMalloc(&cC, sizeof(float)*m*l);

    cudaMemcpy(cA, A, sizeof(float)*m*n, cudaMemcpyHostToDevice);
    cudaMemcpy(cB, B, sizeof(float)*n*l, cudaMemcpyHostToDevice);

    dim3 dimBlock(m,l);
    dim3 dimGrid(1,1);

    cuda_matmul_unit<<<dimGrid,dimBlock>>>(cA,cB,cC,n,l);

    cudaMemcpy(C, cC, sizeof(float)*m*l, cudaMemcpyDeviceToHost);

    cudaFree(cA);
    cudaFree(cB);
    cudaFree(cC);

    long long ret = 0;
    for(int i=0; i<m*l; ++i) ret += C[i];

    return ret;
}

__global__ void cuda_matmul_unit(float * A, float * B, float * C, int n, int l)
{
    int tx = threadIdx.x;
    int ty = threadIdx.y;

    float ret = 0;

    for(int i=0; i<n; ++i) {
        ret += A[ty*n+i] * B[i*l+tx];
    }

    C[ty*l+tx] = ret;
}

long long mat_sum_element(matrix * mat)
{
    long long ret = 0;
    int size = mat->m * mat->n;
    for(int i=0; i<size; ++i) {
        ret += mat->v[i];
    }
    return ret;
}

