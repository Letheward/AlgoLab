#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define n 1024

double A[n][n];
double B[n][n];
double C[n][n];

struct timespec start, end;

int main() {

    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            A[i][j] = (double) rand() / (double) RAND_MAX;
            B[i][j] = (double) rand() / (double) RAND_MAX;
            C[i][j] = 0;
        }
    }
    
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    #pragma omp parallel for
    for (int i = 0; i < n; i++) {
        for (int k = 0; k < n; k++) {
            for (int j = 0; j < n; j++) {
                C[i][j] += A[i][k] * B[k][j];
            }
        }
    }

    clock_gettime(CLOCK_MONOTONIC, &end);
    printf("%fs\n", (end.tv_sec - start.tv_sec) + 1e-9 * (end.tv_nsec - start.tv_nsec));
    
    return 0;
}

