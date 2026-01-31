#include <stdio.h>
#include <omp.h>
#define SIZE 100

int main() {
    int data[SIZE];
    int results[SIZE];

    #pragma omp parallel
    {
        // Phase 1: Initialize data in parallel
        #pragma omp for
        for (int i = 0; i < SIZE; i++) {
            data[i] = i * 2;
        }

        // Explicit barrier: All threads must finish initializing data
        // before any thread proceeds to the next phase.
        #pragma omp barrier

        // Phase 2: Use the initialized data to compute results
        #pragma omp for
        for (int i = 0; i < SIZE; i++) {
            results[i] = data[i] + 1;
        }
    }

    // Print a few results to verify
    printf("Result at index 10: %d\n", results[10]); // Should be 21
    return 0;
}