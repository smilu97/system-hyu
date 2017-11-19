
#include <time.h>
#include "matrix.h"

int main(int argc, char** argv)
{
    if(argc < 3) {
        printf("syntax: %s <mat_A.txt> <mat_B.txt>\n", argv[0]);
        return 0;
    }

    matrix *A, *B, *C;
    struct timespec begin_t, end_t;

    A = read_mat(argv[1]);
    if(A == NULL) {
        printf("Failed to read mat: %s\n", argv[1]);
        return -1;
    }
    printf("read A(%d, %d), sum: %lld\n", A->m, A->n, mat_sum_element(A));
    B = read_mat(argv[2]);
    if(B == NULL) {
        printf("Failed to read mat: %s\n", argv[2]);
        return -1;
    }
    printf("read B(%d, %d), sum: %lld\n", B->m, B->n, mat_sum_element(B));
    C = create_mat(A->m, B->n);
    if(C == NULL) {
        printf("Failed to create mat_C\n");
        return -1;
    }
   
    clock_gettime(CLOCK_MONOTONIC, &begin_t);
    long long sum = cuda_matmul(A->v, B->v, C->v, A->m, A->n, B->n);
    clock_gettime(CLOCK_MONOTONIC, &end_t);

    float ellapse = (end_t.tv_sec - begin_t.tv_sec);
    ellapse += (end_t.tv_nsec - begin_t.tv_nsec) / 1000000000.0f;

    printf("sum: %lld\n", sum);
    printf("ellapse: %f\n", ellapse);

    save_mat(C);

    return 0;
}

