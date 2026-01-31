#include <stdio.h>
#include <omp.h>

#define NTHREADS 4
#define NITERATIONS 100000000

// All counters are tightly packed in memory, likely on the same cache line.
long long counters[NTHREADS];

int main() {
    double start_time, end_time;

    // Reset counters to 0
    for(int i = 0; i < NTHREADS; ++i) {
        counters[i] = 0;
    }
    
    start_time = omp_get_wtime();
    
    #pragma omp parallel num_threads(NTHREADS)
    {
        int id = omp_get_thread_num();
        for (int i = 0; i < NITERATIONS; i++) {
            counters[id]++;
        }
    }

    end_time = omp_get_wtime();
    
    printf("False sharing time: %f seconds\n", end_time - start_time);
    return 0;
}