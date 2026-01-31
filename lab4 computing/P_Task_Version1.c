#include <stdio.h>
#include <omp.h>
#include <stdlib.h>
// I got help from ChatGPT to fix syntax issues in this code.
// I used it to more understand 
// I checked and tested the final version myself.

#define N 10000

int main() {
    unsigned long long A[N];
    A[0] = 1;
double start ,end;
     start = omp_get_wtime();

    #pragma omp parallel
    {
        #pragma omp single
        {
            for (int i = 1; i < N; i++) {
                #pragma omp task firstprivate(i) 
                {
                            A[i] = A[i-1] * 2 + i;
                }
            }
            #pragma omp taskwait
        }
    }

     end = omp_get_wtime();

    printf("Tasks time = %.6f s\n", end - start);
    for (int i = 0; i < 10; i++) {
        printf("%llu ", A[i]);
    }
    printf("\n");

    return 0;
}
