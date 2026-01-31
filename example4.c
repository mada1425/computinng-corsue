#include <mpi.h>
#include <stdio.h>

int main(int argc, char **argv) {
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    int sendbuf[8], recvbuf[8];

    // Each process fills its send buffer with identifiable data
    for (int i = 0; i < size; i++)
        sendbuf[i] = rank * 10 + i;

    MPI_Barrier(MPI_COMM_WORLD);
    double start = MPI_Wtime();

    MPI_Alltoall(sendbuf, 1, MPI_INT, recvbuf, 1, MPI_INT, MPI_COMM_WORLD);

    MPI_Barrier(MPI_COMM_WORLD);
    double end = MPI_Wtime();

    // Print what each process sent and received
    printf("\n--- Rank %d ---\n", rank);

    printf("Sent to:   ");
    for (int i = 0; i < size; i++)
        printf("[to %d: %d] ", i, sendbuf[i]);

    printf("\nReceived from: ");
    for (int i = 0; i < size; i++)
        printf("[from %d: %d] ", i, recvbuf[i]);

    printf("\nElapsed time = %.6f s\n", end - start);

    MPI_Finalize();
    return 0;
}
