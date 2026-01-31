#include <stdio.h>
#include <stdlib.h>
#include <omp.h>

#define ORDER 2000
#define AVAL 3.0
#define BVAL 5.0

int main()
{
    int N = ORDER, P = ORDER, M = ORDER;
    int i, j, k;
    double *A, *B, *C, tmp;
    double start, end;

    A = (double*) malloc(N * P * sizeof(double));
    B = (double*) malloc(P * M * sizeof(double));
    C = (double*) malloc(N * M * sizeof(double));

    for (i = 0; i < N; i++)
        for (j = 0; j < P; j++)
            A[i * N + j] = AVAL;

    for (i = 0; i < P; i++)
        for (j = 0; j < M; j++)
            B[i * P + j] = BVAL;

    start = omp_get_wtime();

    for (i = 0; i < N; i++)
        for (j = 0; j < M; j++) {
            tmp = 0.0;
            for (k = 0; k < P; k++)
                tmp += A[i * N + k] * B[k * P + j];
            C[i * N + j] = tmp;
        }

    end = omp_get_wtime();

    printf("Serial time: %f seconds\n", end - start);

    free(A);
    free(B);
    free(C);

    return 0;
}
