#include <stdio.h>
#include <mpi.h>
#include <sys/time.h>

#define NUM_MEG_PER_RANK (1)

int main(int argc, char *argv[])
{

  int rank, size;
  MPI_Status status;
  double start, end, overall_end;


  /* Init */
  MPI_Init(&argc, &argv);
  start = MPI_Wtime();
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);

  if (rank != 0) { // Slaves
    int buf;
    int i;
    for (i = 0; i < NUM_MEG_PER_RANK; i++) {
      MPI_Send(&rank, 1, MPI_INT, 0, i, MPI_COMM_WORLD); 
    }
  } else { // Master
    int sum = 0;
    int flag = -1, res;
    MPI_Request request;
    MPI_Status status;
    while (1) { 
      if(flag != 0) {
	MPI_Irecv(&res, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &request);
	flag = 0;
      }
      MPI_Test(&request, &flag, &status);
      sleep(1);

      if (flag != 0) { 
	printf("** Slave ID: %d **\n", status.MPI_SOURCE);
	if (status.MPI_SOURCE != -1) {
	  sum++;
	}
	flag = -1;
      }
      // 
      if (sum == (size - 1) * NUM_MEG_PER_RANK) {
	break;
      }
    }
  }

  end = MPI_Wtime();
  MPI_Finalize();
  overall_end = MPI_Wtime();
  if (rank == 0) {
    fprintf(stdout, "Time: %f\n", end - start);
    fprintf(stdout, "Time: %f\n", overall_end - start);
  }
  return 0;

}
