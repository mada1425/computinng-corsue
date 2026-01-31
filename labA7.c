#include <stdio.h>
#include <mpi.h>

int main(int argc, char **argv)
{
    int rank, size;
    int value, sum = 0;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (rank != 0) {
        value = rank * rank;
        MPI_Send(&value, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
    } else  {
        sum = 0;
        int root_square = rank * rank;
        sum += root_square;

        MPI_Status status;
        for (int i = 1; i < size; i++) {
            MPI_Recv(&value, 1, MPI_INT, i, 0, MPI_COMM_WORLD, &status);
            printf("the root process received the number %d from process %d\n", value, i);
            sum += value;
        }
        printf("the  square of process  %d square is %d\n", rank, root_square);

        printf("the sum of squares of the ranks of all processes is %d\n", sum);
    }

    MPI_Finalize();
    return 0;
}
