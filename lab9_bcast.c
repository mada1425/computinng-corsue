// lab9_bcast.c
#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

int main(int argc, char *argv[]) {
  MPI_Init(&argc, &argv);
  int rank, size; MPI_Comm_rank(MPI_COMM_WORLD, &rank); MPI_Comm_size(MPI_COMM_WORLD, &size);

  int N = (argc > 1) ? atoi(argv[1]) : 1024*1024; // default 1 MB
  char *buf = (char*)malloc(N);
  if (rank == 0) for (int i = 0; i < N; i++) buf[i] = (char)(i % 251);

  MPI_Barrier(MPI_COMM_WORLD);
  double t0 = MPI_Wtime();
  MPI_Bcast(buf, N, MPI_CHAR, 0, MPI_COMM_WORLD);
  double t1 = MPI_Wtime();
  double local = t1 - t0, worst = 0.0;

  MPI_Reduce(&local, &worst, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
  if (rank == 0) printf("MPI_Bcast | bytes=%d | p=%d | time=%f s\n", N, size, worst);

  free(buf); MPI_Finalize(); return 0;
}