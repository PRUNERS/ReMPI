#include <stdio.h>
#include <mpi.h>
#include <sys/time.h>
#include <signal.h>

#include "rempi_test_util.h"

#define NUM_KV_PER_RANK (3)
#define MAX_VAL (10)
#define MAX_MESG_PASS (4)


int my_rank;
int size;


void one_way_send_recv() 
{
  int i;

  int buff;
  MPI_Request req;
  MPI_Status stat;

  int partner;
  MPI_Request recv_requests[NUM_KV_PER_RANK];
  MPI_Request send_requests[NUM_KV_PER_RANK];
  MPI_Status status;
  int recv_buff[NUM_KV_PER_RANK];
  int send_buff[NUM_KV_PER_RANK];
  int flag = 0;


  if (my_rank % 2 == 0) {
    partner = my_rank + 1;
  } else {
    partner = my_rank - 1;
  }

  if (partner >= size) return;

  if (my_rank == 0) {
    MPI_Irecv(&buff, 1, MPI_INT, size - 1, MPI_ANY_TAG, MPI_COMM_WORLD, &req);
  }
  
  for (i = 0; i < NUM_KV_PER_RANK; i++) { 
    MPI_Irecv(&recv_buff[i], 1, MPI_INT, partner, MPI_ANY_TAG, MPI_COMM_WORLD, &recv_requests[i]);
  }

  for (i = 0; i < NUM_KV_PER_RANK; i++) { 
    MPI_Isend(&send_buff[i], 1, MPI_INT, partner,           0, MPI_COMM_WORLD, &send_requests[i]);
  }
  
  usleep(100000);

  if (my_rank == 0) {
    while (!flag) {
      MPI_Test(&req, &flag, &stat);
    }
  }
  
  for (i = 0; i < NUM_KV_PER_RANK; i++) { 
    flag = 0;
    while (!flag) {
      MPI_Test(&send_requests[i], &flag, &status);
    }
  }

  for (i = 0; i < NUM_KV_PER_RANK; i++) { 
    flag = 0;
    while (!flag) {
      MPI_Test(&recv_requests[i], &flag, &status);
    }
  }
}

int main(int argc, char *argv[])
{


  /* Init */
  MPI_Init(&argc, &argv);
  signal(SIGSEGV, SIG_DFL);
  MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);




  if (my_rank == size - 1) {
    int buff;
    MPI_Request req;
    MPI_Status stat;
    MPI_Send(&buff, 1, MPI_INT,        0,           0, MPI_COMM_WORLD);
  }
  


  if (my_rank < 2) {
    one_way_send_recv();
  }
  
  MPI_Barrier(MPI_COMM_WORLD);

  if (my_rank == size - 1) {
    int buff;
    MPI_Request req;
    MPI_Status stat;
    MPI_Send(&buff, 1, MPI_INT,        0,           0, MPI_COMM_WORLD);
  }
  one_way_send_recv();
  
  MPI_Finalize();

}
