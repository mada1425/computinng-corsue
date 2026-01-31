#include <mpi.h>
#include <stdio.h>

int main(int argc, char **argv) {
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    double local_val = (rank + 1) * 2.5;
    double global_avg;

    MPI_Barrier(MPI_COMM_WORLD);
    double start = MPI_Wtime();

    double global_sum;
    MPI_Allreduce(&local_val, &global_sum, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
    global_avg = global_sum / size;

    MPI_Barrier(MPI_COMM_WORLD);
    double end = MPI_Wtime();

    printf("Rank %d → local = %.2f, global avg = %.2f (time = %.6f s)\n",
           rank, local_val, global_avg, end - start);

    MPI_Finalize();
    return 0;
}
