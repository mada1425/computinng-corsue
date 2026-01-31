#include <stdio.h>
#include <omp.h>

#define NTHREADS 4
#define NITERATIONS 100000000

// Pad the array to ensure each counter is on its own cache line.
// Assumes a 64-byte cache line and 8-byte long long.
#define CACHE_LINE_SIZE 64
#define PADDING_SIZE (CACHE_LINE_SIZE / sizeof(long long))
long long counters[NTHREADS][PADDING_SIZE];

int main() {
    double start_time, end_time;

    // Reset counters to 0
    for(int i = 0; i < NTHREADS; ++i) {
        counters[i][0] = 0;
    }
    
    start_time = omp_get_wtime();
    
    #pragma omp parallel num_threads(NTHREADS)
    {
        int id = omp_get_thread_num();
        for (int i = 0; i < NITERATIONS; i++) {
            counters[id][0]++; // Only access the first element of each row
        }
    }
    
    end_time = omp_get_wtime();
    
    printf("Mitigation time: %f seconds\n", end_time - start_time);
    return 0;
}