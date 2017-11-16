
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

#define MATRIX_SIZE 1000

/*
 * Argument 1: the number of threads
 */
int main(int argc, char** argv)
{
    matrix a, b, c;

    init_matrix(&a, MATRIX_SIZE, MATRIX_SIZE);
    init_matrix(&b, MATRIX_SIZE, MATRIX_SIZE);
    init_matrix(&c, MATRIX_SIZE, MATRIX_SIZE);

    clock_t begin_time = clock();
    mul_matrix(&a, &b, &c);
    clock_t end_time = clock();

    destroy_matrix(&a);
    destroy_matrix(&b);
    destroy_matrix(&c);

    printf("time: %f\n", (double)(end_time - begin_time) / CLOCKS_PER_SEC);

    return 0;
}