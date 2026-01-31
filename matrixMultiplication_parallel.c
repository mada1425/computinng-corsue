#include <stdio.h>
#include <stdlib.h>
#include <omp.h>

#define ORDER 2000
#define AVAL 3.0
#define BVAL 5.0

int main(int argc, char *argv[])
{
    int N = ORDER, P = ORDER, M = ORDER;
    int i, j, k;
    int threads = 1;

    if (argc >= 2)
        threads = atoi(argv[1]);

    omp_set_num_threads(threads);

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

#pragma omp parallel for private(j, k, tmp)
    for (i = 0; i < N; i++)
        for (j = 0; j < M; j++) {
            tmp = 0.0;
            for (k = 0; k < P; k++)
                tmp += A[i * N + k] * B[k * P + j];
            C[i * N + j] = tmp;
        }

    end = omp_get_wtime();

    printf("Threads: %d\n", threads);
    printf("Parallel time: %f seconds\n", end - start);

    free(A);
    free(B);
    free(C);

    return 0;
}
