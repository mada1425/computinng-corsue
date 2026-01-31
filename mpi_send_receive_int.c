#include <stdio.h>
#include <mpi.h> // For MPI functions

int main(int argc, char **argv)
{
    int size; // Number of processes
    int rank; // Process rank

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if (rank == 0)
    { 
        // Rank 0 sends an integer to each other process
        int i;
        int value = 2;

        for (i = 1; i < size; i++)
        {
            value = value * i;
            MPI_Send(&value, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
            printf("Rank %d sent value %d to Rank %d\n", rank, value, i);
        }
    }
    else
    {
        // All other processes receive one number from process 0
        int value;
        MPI_Status status;

        MPI_Recv(&value, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);
        printf("Rank %d received value %d from Rank 0\n", rank, value);
    }

    MPI_Finalize();
    return 0;
}