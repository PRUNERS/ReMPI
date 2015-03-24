#include <stdio.h>
#include <mpi.h>
#include <sys/time.h>

#include "rempi_test_util.h"

#define NUM_KV_PER_RANK (2)
#define MAX_VAL (10)
#define MAX_MESG_PASS (4)


double start, end, overall_end;
int recv_msg_count[NUM_KV_PER_RANK], send_msg_count[NUM_KV_PER_RANK];

struct key_val{
  int key;
  int val;
};

double get_runtime()
{
  return MPI_Wtime() - start;
}

int is_finished(){
  int i;
  for (i = 0; i < NUM_KV_PER_RANK; i++) {
    if (send_msg_count[i] < MAX_MESG_PASS || recv_msg_count[i] < MAX_MESG_PASS) return 0;
  }
  return 1;
}

int main(int argc, char *argv[])
{

  int my_rank;
  int size;
  int seed;
  int i;

  struct key_val *my_kv, *final_kv;
  struct key_val recv_kv[NUM_KV_PER_RANK];

  MPI_Status status[NUM_KV_PER_RANK];
  MPI_Request request[NUM_KV_PER_RANK];

  /* Init */
  MPI_Init(&argc, &argv);
  start = MPI_Wtime();
  MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);

  seed = my_rank;
  init_rand(seed);

  my_kv = (struct key_val*)malloc(sizeof(struct key_val) * NUM_KV_PER_RANK);
  for (i = 0; i < NUM_KV_PER_RANK; i++) {
    my_kv[i].key = my_rank * NUM_KV_PER_RANK + i;
    my_kv[i].val = get_rand(MAX_VAL);
    recv_msg_count[i] = 0;
    send_msg_count[i] = 0;
  }


  for (i = 0; i < NUM_KV_PER_RANK; i++) { 
    //    fprintf(stderr, ":%d: App: req:%p, s:%d, t:%d\n", my_rank, &recv_kv[i], (my_rank + size - i)        % size, MPI_ANY_TAG);
    //    MPI_Irecv(&recv_kv[i], 2, MPI_INT, (my_rank + size - i)        % size, MPI_ANY_TAG, MPI_COMM_WORLD, &request[i]);
    MPI_Irecv(&recv_kv[i], 8, MPI_CHAR, (my_rank + size - i)        % size, MPI_ANY_TAG, MPI_COMM_WORLD, &request[i]);
  }

  for (i = 0; i < NUM_KV_PER_RANK; i++) { 
    //    MPI_Send(&my_kv[i], 2, MPI_INT, (my_rank + i)        % size, 0, MPI_COMM_WORLD);
    MPI_Send(&my_kv[i], 8, MPI_CHAR, (my_rank + i)        % size, 0, MPI_COMM_WORLD);
    send_msg_count[i]++;
  }
  
  while (!is_finished()) {
    int testsome_outcount;
    int testsome_array_of_indices[NUM_KV_PER_RANK];

    testsome_outcount = 0;    
    memset(status, 0, sizeof(MPI_Status) * NUM_KV_PER_RANK);
    MPI_Testsome(NUM_KV_PER_RANK, request, 
		 &testsome_outcount, testsome_array_of_indices, status);

    for(i = 0; i < testsome_outcount; i++) {
      struct key_val sendrecv_kv;
      int recv_index;
      int send_dest;
      recv_index = testsome_array_of_indices[i];
#if 1
      if (my_rank == 2) {
	int count = -1;
	MPI_Get_count(&status[i], MPI_CHAR, &count);
	/* fprintf(stderr, "i    : rank:%d: Receved source:%d tag:%d outcount:%d i    :%d count:%d\n",  */
	/* 	my_rank, status[i].MPI_SOURCE, status[i].MPI_TAG, testsome_outcount, i, count); */
	/* fprintf(stderr, "index: rank:%d: Receved source:%d tag:%d outcount:%d index:%d\n",  */
	/* 	      my_rank, status[recv_index].MPI_SOURCE, status[recv_index].MPI_TAG, testsome_outcount, recv_index); */
      }
#endif
      memcpy(&sendrecv_kv, &recv_kv[recv_index], sizeof(struct key_val));
      //memset(&sendrecv_kv, 0, sizeof(struct key_val));
      recv_msg_count[recv_index]++;
      if (recv_msg_count[recv_index] < MAX_MESG_PASS) {
	//	MPI_Irecv(&recv_kv[recv_index], 2, MPI_INT, (my_rank + size - recv_index) % size, MPI_ANY_TAG, MPI_COMM_WORLD, &request[recv_index]);
	MPI_Irecv(&recv_kv[recv_index], 8, MPI_CHAR, (my_rank + size - recv_index) % size, MPI_ANY_TAG, MPI_COMM_WORLD, &request[recv_index]);
      }

      if (send_msg_count[recv_index] < MAX_MESG_PASS) {
	sendrecv_kv.val = get_hash(sendrecv_kv.val, MAX_VAL);
	//	MPI_Send(&sendrecv_kv, 2, MPI_INT, (my_rank + recv_index) % size, 0, MPI_COMM_WORLD);
	MPI_Send(&sendrecv_kv, 8, MPI_CHAR, (my_rank + recv_index) % size, 0, MPI_COMM_WORLD);
	send_msg_count[recv_index]++;
      }
    }
  }


  /* for (i = 0; i < NUM_KV_PER_RANK; i++) { */
  /*   fprintf(stderr, "rank %d: recv: %d send: %d\n", my_rank, recv_msg_count[i], send_msg_count[i]); */
  /* } */
  /* for (i = 0; i < NUM_KV_PER_RANK; i++) { */
  /*   fprintf(stderr, "rank: %d key: %d val: %d\n", my_rank, recv_kv[i].key, recv_kv[i].val); */
  /* } */

  final_kv = (struct key_val*)malloc(sizeof(struct key_val) * NUM_KV_PER_RANK * size);  
  memset(final_kv, 0, sizeof(final_kv));
  MPI_Gather(recv_kv, 2 * NUM_KV_PER_RANK, MPI_INT, final_kv, 2 * NUM_KV_PER_RANK, MPI_INT, 0, MPI_COMM_WORLD);
  int hash = 1;
  if (my_rank == 0) {

    for (i = 0; i < size * NUM_KV_PER_RANK; i++) {
      hash = get_hash(hash * final_kv[i].val + final_kv[i].key, 1000000);
    }
  
#if 0    
    int print_key = 0;
    while(print_key < size * NUM_KV_PER_RANK) {
      for (i = 0; i < size * NUM_KV_PER_RANK; i++) {
    	if (final_kv[i].key == print_key) {
    	  fprintf(stdout, "rank: %d, val: %d\n", final_kv[i].key, final_kv[i].val);
    	  print_key++;
    	}
      }
    }

    for (i = 0; i < size * NUM_KV_PER_RANK; i++) {
      fprintf(stdout, "key: %d, val: %d\n", final_kv[i].key, final_kv[i].val);
    }
#endif
  }
  free(my_kv);
  free(final_kv);

  end = MPI_Wtime();
  MPI_Finalize();
  overall_end = MPI_Wtime();
  if (my_rank == 0) {
    fprintf(stdout, "Hash %d, Time (Main loop): %f, Time (Overall): %f\n", hash, end - start, overall_end - start);
  }
  return 0;



  //----------

  //  MPI_Send(&my_rank, 1, MPI_INT, 0, i, MPI_COMM_WORLD); 
  //  MPI_Irecv(&res, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &request);
  //  MPI_Test(&request, &flag, &status);
}
