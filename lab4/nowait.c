#include <stdio.h>
#include <omp.h>
#define SIZE 10000

int main() {
    int a[SIZE], b[SIZE];
    double start_time, end_time;

    start_time = omp_get_wtime();
    #pragma omp parallel
    {
        // Loop 1: Perform calculations on array 'a'
        #pragma omp for nowait
        for (int i = 0; i < SIZE; i++) {
            a[i] = i * 2;
        }

        // Loop 2: Perform calculations on array 'b'
        #pragma omp for
        for (int i = 0; i < SIZE; i++) {
            b[i] = i * 3;
        }
    }
    end_time = omp_get_wtime();
    printf("Processing complete for both arrays using nowait. Time =%f \n", end_time - start_time);
    return 0;
}
