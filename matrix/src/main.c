
/*
 * Author: smilu97, 2016025241
 * 
 * Matrix multiplication with multi threads
 * 
 * This program is made for testing speed of matrix multiplication
 * The main goal is learning how to make program faster with multi threading
 * 
 * All copyright is reserved to Hanyang Univ.
 */

#include <stdio.h>
#include <time.h>

#include "matrix.h"

/*
 * Argument 1: The size of matrix
 * Argument 2: The number of threads to use
 */
int main(int argc, char** argv)
{
    if(argc < 3) {
        printf("syntax: %s <size of matrix> <num of threads>\n", argv[0]);
        exit(1);
    }

    int matrix_size = atoi(argv[1]);
    int num_thread  = atoi(argv[2]);

    struct timespec begin, end;

    matrix a, b, c;

    init_matrix(&a, matrix_size, matrix_size);
    init_matrix(&b, matrix_size, matrix_size);
    init_matrix(&c, matrix_size, matrix_size);

    clock_gettime(CLOCK_MONOTONIC, &begin);
    mul_matrix_mt(&a, &b, &c, num_thread);
    clock_gettime(CLOCK_MONOTONIC, &end);

    destroy_matrix(&a);
    destroy_matrix(&b);
    destroy_matrix(&c);

    double elapsed = (end.tv_sec - begin.tv_sec);
    elapsed += (end.tv_nsec - begin.tv_nsec) / 1000000000.0;

    printf("elapsed: %f\n", elapsed);

    return 0;
}