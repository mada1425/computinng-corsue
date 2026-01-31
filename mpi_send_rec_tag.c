#include <stdio.h>
#include <mpi.h>

int main(int argc, char **argv)
{
    int rank, size;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    int value = 10;
    MPI_Status status;

    if (rank == 0) {
        // Send to process 1 with tag = 5
        printf("Rank 0 sending value %d with tag 5\n", value);
        MPI_Send(&value, 1, MPI_INT, 1, 5, MPI_COMM_WORLD);
    } 
    else if (rank == 1) {
        // Receive from process 0 with the same tag = 5
        printf("Rank 1 waiting for message with tag 5\n");
        MPI_Recv(&value, 1, MPI_INT, 0, 5, MPI_COMM_WORLD, &status);
        printf("Rank 1 received value %d from Rank %d (tag=%d)\n",
               value, status.MPI_SOURCE, status.MPI_TAG);
    }

    MPI_Finalize();
    return 0;
}