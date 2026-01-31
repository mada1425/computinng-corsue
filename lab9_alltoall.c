// lab9_alltoall.c
#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

int main(int argc, char *argv[]) {
  MPI_Init(&argc, &argv);
  int rank, size; MPI_Comm_rank(MPI_COMM_WORLD, &rank); MPI_Comm_size(MPI_COMM_WORLD, &size);

  int N = (argc > 1) ? atoi(argv[1]) : 1024; // ints per destination
  int *sendbuf = (int*)malloc(N * size * sizeof(int));
  int *recvbuf = (int*)malloc(N * size * sizeof(int));
  for (int i = 0; i < N * size; i++) sendbuf[i] = rank;

  MPI_Barrier(MPI_COMM_WORLD);
  double t0 = MPI_Wtime();
  MPI_Alltoall(sendbuf, N, MPI_INT, recvbuf, N, MPI_INT, MPI_COMM_WORLD);
  double t1 = MPI_Wtime(), local = t1 - t0, worst = 0.0;

  MPI_Reduce(&local, &worst, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
  if (rank == 0) printf("MPI_Alltoall | elems/dest=%d | p=%d | time=%f s\n", N, size, worst);

  free(sendbuf); free(recvbuf);
  MPI_Finalize(); return 0;
}