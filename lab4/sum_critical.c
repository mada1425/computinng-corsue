// sum_critical.c
#include <stdio.h>
#include <omp.h>

int main() {
    const int N = 10000000;
    double sum = 0.0, sumSq = 0.0;

    double t0 = omp_get_wtime();
    #pragma omp parallel for
    for (int i = 0; i < N; ++i) {
        double x = 1.0 / (i + 1);
        #pragma omp critical
        {
            sum   += x;
            sumSq += x * x;
        }
    }
    double t1 = omp_get_wtime();

    printf("[critical ] sum=%.10f sumSq=%.10f time=%.3f s\n", sum, sumSq, t1 - t0);
    return 0;
}