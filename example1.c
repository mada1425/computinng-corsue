#include <mpi.h>
#include <stdio.h>

int main(int argc, char **argv) {
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    int N = 100000;
    double data[N], local_sum = 0.0, global_sum = 0.0;

    if (rank == 0)
        for (int i = 0; i < N; i++) data[i] = (double)i;

    MPI_Barrier(MPI_COMM_WORLD);          // Synchronize before timing
    double start = MPI_Wtime();

    MPI_Bcast(data, N, MPI_DOUBLE, 0, MPI_COMM_WORLD);          // One-to-all
    for (int i = rank; i < N; i += size) local_sum += data[i];  // Local work
    MPI_Reduce(&local_sum, &global_sum, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD); // All-to-one

    MPI_Barrier(MPI_COMM_WORLD);
    double end = MPI_Wtime();

    if (rank == 0)
        printf("Global sum = %.2f, elapsed = %.6f s (%d ranks)\n",
               global_sum, end - start, size);

    MPI_Finalize();
    return 0;
}
