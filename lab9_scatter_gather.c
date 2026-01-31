// lab9_scatter_gather.c
#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

int main(int argc, char *argv[]) {
  MPI_Init(&argc, &argv);
  int rank, size; MPI_Comm_rank(MPI_COMM_WORLD, &rank); MPI_Comm_size(MPI_COMM_WORLD, &size);

  int N = (argc > 1) ? atoi(argv[1]) : 1024*1024; // total ints
  int chunk = N / size;

  int *sendbuf = NULL;
  if (rank == 0) {
    sendbuf = (int*)malloc(N * sizeof(int));
    for (int i = 0; i < N; i++) sendbuf[i] = i;
  }
  int *recvbuf = (int*)malloc(chunk * sizeof(int));

  MPI_Barrier(MPI_COMM_WORLD);
  double t0 = MPI_Wtime();

  MPI_Scatter(sendbuf, chunk, MPI_INT, recvbuf, chunk, MPI_INT, 0, MPI_COMM_WORLD);

  for (int i = 0; i < chunk; i++) recvbuf[i] *= 2; // dummy local work

  MPI_Gather(recvbuf, chunk, MPI_INT, sendbuf, chunk, MPI_INT, 0, MPI_COMM_WORLD);

  double t1 = MPI_Wtime(), local = t1 - t0, worst = 0.0;
  MPI_Reduce(&local, &worst, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);

  if (rank == 0) printf("Scatter+Compute+Gather | N=%d ints | p=%d | time=%f s\n", N, size, worst);

  free(recvbuf); if (rank == 0) free(sendbuf);
  MPI_Finalize(); return 0;
}