#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
// I got help from ChatGPT to fix syntax issues in this code.
// I used it to more understand 
// I checked and tested the final version myself.

#define N 10000

int main() {
    unsigned long long A[N];
    A[0] = 1;
    double start, end;

     start = omp_get_wtime();
    for (int i = 1; i < N; ++i) {
        A[i] = A[i-1] * 2 + i;
    }
     end = omp_get_wtime();

    printf("Serial time = %.6f s\n", end - start);
    for (int i = 0; i < 10; i++)
        printf("%lld ", A[i]);
    printf("\n");

    return 0;
}
