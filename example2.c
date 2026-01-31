#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    int N = 16;   // total data elements
    int data[N], result[N];

    // Root process initializes data
    if (rank == 0) {
        for (int i = 0; i < N; i++)
            data[i] = i + 1;
    }

    // --- Safety check: N must be divisible by size ---
    if (N % size != 0) {
        if (rank == 0)
            fprintf(stderr,
                    "Error: N (%d) must be divisible by number of processes (%d)\n",
                    N, size);
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    int local_size = N / size;
    int local[local_size];

    // --- Synchronize before timing ---
    MPI_Barrier(MPI_COMM_WORLD);
    double start = MPI_Wtime();

    // --- Step 1: Scatter the data ---
    MPI_Scatter(data, local_size, MPI_INT,
                local, local_size, MPI_INT,
                0, MPI_COMM_WORLD);

    // --- Step 2: Each process performs computation ---
    for (int i = 0; i < local_size; i++)
        local[i] = local[i] * local[i];  // square each element

    // --- Step 3: Gather results back to root ---
    MPI_Gather(local, local_size, MPI_INT,
               result, local_size, MPI_INT,
               0, MPI_COMM_WORLD);

    // --- Synchronize again and record elapsed time ---
    MPI_Barrier(MPI_COMM_WORLD);
    double end = MPI_Wtime();

    // --- Root prints final results ---
    if (rank == 0) {
        printf("\n===== MPI Scatter/Gather Example =====\n");
        printf("Total processes: %d\n", size);
        printf("Squared results: ");
        for (int i = 0; i < N; i++)
            printf("%d ", result[i]);
        printf("\nElapsed time = %.6f seconds\n", end - start);
        printf("======================================\n");
    }

    MPI_Finalize();
    return 0;
}
