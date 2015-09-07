#include <stdio.h>
#include <mpi.h>
#include <sys/time.h>
#include <signal.h>

#include "rempi_test_util.h"


double start, end, overall_end;

int main(int argc, char *argv[])
{
  int my_rank;
  int size;
  char val;
  int right, left;
  int flag = 0;
  MPI_Status status;
  MPI_Request send_request, recv_request;

  /* Init */
  MPI_Init(&argc, &argv);
  signal(SIGSEGV, SIG_DFL);

  start = MPI_Wtime();
  MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);

  right = (my_rank + 1) % size;
  left  = (my_rank + size - 1) % size;

  MPI_Irecv(&val,     1, MPI_CHAR,  left, 0, MPI_COMM_WORLD, &recv_request);
  //MPI_Send(&my_rank,  1, MPI_INT, right, 0, MPI_COMM_WORLD); 
  MPI_Isend(&my_rank, 1, MPI_CHAR, right, 0, MPI_COMM_WORLD, &send_request); 
  while (!flag)   MPI_Test(&send_request, &flag, &status);
  flag = 0;
  //  fprintf(stderr, "rank %d: using request: %p", my_rank, recv_request);
  while (!flag)   MPI_Test(&recv_request, &flag, &status);

  end = MPI_Wtime();
  MPI_Finalize();
  //  overall_end = MPI_Wtime();
  if (my_rank == 0) {
    fprintf(stdout, "Time (Main loop): %f, Time (Overall): %f\n", end - start, overall_end - start);
  }
  return 0;
}
