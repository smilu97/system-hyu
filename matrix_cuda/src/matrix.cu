
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
     lld * arr = (lld*)malloc(sizeof(lld)*cap);
     if(arr == NULL) {
         fprintf(stderr, "Failed to allocate memory to load matrix\n");
         fclose(fd);
         return NULL;
     }

     int col_size = -1;
     int tmp;
     while(~fscanf(fd, "%d", &tmp)) {
         if(size >= cap) {
             lld * narr = (lld*)malloc(sizeof(lld)*cap*2);
             if(narr == NULL) {
                 fprintf(stderr, "Matrix is too big!\n");
                 fclose(fd);
                 free(arr);
                 return NULL;
             }
             memcpy(narr, arr, sizeof(float)*size);
             free(arr);
             arr = narr;
             cap <<= 1;
         }
         arr[size++] = (lld)tmp;
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
     memcpy(ret->v, arr, sizeof(lld)*size);
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
    ret->v = (lld*)malloc(sizeof(lld)*m*n);
    if(ret->v == NULL) {
        fprintf(stderr, "Toob big too create: create_mat()");
        free(ret);
        return NULL;
    }
    ret->m = m;
    ret->n = n;
    
    return ret;
}

void save_mat(matrix * mat)
{
    FILE * fd = fopen("result.txt", "w");

    for(int i=0; i<mat->m; ++i) {
        for(int j=0; j<mat->n; ++j) {
            fprintf(fd, "%lld ", (lld)mat->v[i*mat->n+j]);
        }
        fprintf(fd, "\n");
    }

    fclose(fd);
}

long long cuda_matmul(lld * A, lld * B, lld * C, int m, int n, int l)
{
    lld *cA, *cB, *cC;

    cudaMalloc(&cA, sizeof(lld)*m*n);
    cudaMalloc(&cB, sizeof(lld)*n*l);
    cudaMalloc(&cC, sizeof(lld)*m*l);

    cudaMemcpy(cA, A, sizeof(lld)*m*n, cudaMemcpyHostToDevice);
    cudaMemcpy(cB, B, sizeof(lld)*n*l, cudaMemcpyHostToDevice);

    int dbx = m > 16 ? 16 : m;
    int dby = l > 16 ? 16 : l;
    int dgx = m > 16 ? m/16 : 1;
    int dgy = l > 16 ? l/16 : 1;
    dim3 dimBlock(dbx,dby);
    dim3 dimGrid(dgx,dgy);

    cuda_matmul_unit<<<dimGrid,dimBlock>>>(cA,cB,cC,n,l);

    cudaMemcpy(C, cC, sizeof(lld)*m*l, cudaMemcpyDeviceToHost);

    cudaFree(cA);
    cudaFree(cB);
    cudaFree(cC);

    long long ret = 0;
    for(int i=0; i<m*l; ++i) ret += C[i];

    return ret;
}

__global__ void cuda_matmul_unit(lld * A, lld * B, lld * C, int n, int l)
{
    int tx = blockIdx.x * blockDim.x + threadIdx.x;
    int ty = blockIdx.y * blockDim.y + threadIdx.y;

    lld ret = 0;

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

