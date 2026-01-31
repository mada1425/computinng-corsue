#include <stdio.h>
#include <stdlib.h>
#include <omp.h>

#define BINS 100
#define NUM_VALUES 10000000

int main() {
    int histogram[BINS] = {0};
    int *values = malloc(NUM_VALUES * sizeof(int));
    double start_time, end_time;

    // Fill values array with random data
    for (int i = 0; i < NUM_VALUES; i++) {
        values[i] = rand() % BINS;
    }
// --- Critical version (Less performant) ---
    // Reset histogram for comparison
    for(int i = 0; i < BINS; i++) histogram[i] = 0;
    start_time = omp_get_wtime();
    #pragma omp parallel for
    for (int i = 0; i < NUM_VALUES; i++) {
        int bin_index = values[i];
        #pragma omp critical
        {
            histogram[bin_index]++;
        }
    }
    end_time = omp_get_wtime();
    printf("Critical Time (for histogram): %f seconds\n\n", end_time - start_time);

    free(values);
    return 0;
}