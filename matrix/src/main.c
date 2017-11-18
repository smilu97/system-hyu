
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
    if(argc < 4) {
        printf("syntax: %s <A matrix file> <B matrix file> <num of threads>\n", argv[0]);
        exit(1);
    }

    int num_thread  = atoi(argv[3]);

    struct timespec begin, end;

    matrix a, b, c;

    read_matrix(&a, argv[1]);
    read_matrix(&b, argv[2]);
    init_matrix(&c, a.m, b.n);

    clock_gettime(CLOCK_MONOTONIC, &begin);
    long long sum = mul_matrix_mt(&a, &b, &c, num_thread);
    clock_gettime(CLOCK_MONOTONIC, &end);

    destroy_matrix(&a);
    destroy_matrix(&b);
    destroy_matrix(&c);

    double elapsed = (end.tv_sec - begin.tv_sec) + (end.tv_nsec - begin.tv_nsec) / 1000000000.0;

    printf("sum: %lld\n", sum);
    printf("elapsed: %f\n", elapsed);

    return 0;
}