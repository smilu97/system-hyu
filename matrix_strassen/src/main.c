
#include <stdio.h>
#include <time.h>

#include "matrix.h"

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
    matrix c = matmul_8(a, b);
    clock_gettime(CLOCK_MONOTONIC, &end);

    double time = (end.tv_sec - begin.tv_sec) + (end.tv_nsec - begin.tv_nsec) / 1000000000.0f;

    printf("time: %f\n", time);

    free(c.v);

    return 0;
}
