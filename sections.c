#include <stdio.h>
#include <omp.h>
#include <unistd.h>   // for sleep()

// Simulated workloads
void compute_matrix_inverse() {
    sleep(1);
    printf("Matrix inversion done by thread %d\n", omp_get_thread_num());
}

void compute_determinant() {
    sleep(1);
    printf("Determinant computation done by thread %d\n", omp_get_thread_num());
}

void compute_transpose() {
    sleep(1);
    printf("Matrix transpose done by thread %d\n", omp_get_thread_num());
}

int main() {
    double start, end;

    printf("=== OpenMP Parallel Sections Example ===\n");

    start = omp_get_wtime();

    #pragma omp parallel sections
    {
        #pragma omp section
        {
            compute_matrix_inverse();
        }

        #pragma omp section
        {
            compute_determinant();
        }

        #pragma omp section
        {
            compute_transpose();
        }
    }

    end = omp_get_wtime();

    printf("All computations completed in %.3f seconds\n", end - start);
    return 0;
}