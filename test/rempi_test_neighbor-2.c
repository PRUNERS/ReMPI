#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <sys/time.h>
#include <time.h>
#include <signal.h>

#include "rempi_test_util.h"

#define NUM_ITE (2)
#define NUM_KV_PER_RANK (3)
#define MAX_VAL (10)
#define MAX_MESG_PASS (4)


double start, end, overall_end;
int recv_msg_count[NUM_KV_PER_RANK], send_msg_count[NUM_KV_PER_RANK];
int my_rank;
int size;

struct key_val{
  int key;
  int val;
};

MPI_Comm reduction_comm;
MPI_Request reduction_recv_req, reduction_send_req;
int reduction_val = 0;

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


int bin_reduction_start()
{
  int parent = (my_rank - 1) / 2;
  int last_child_rank = (my_rank + 1) * 2;
  int num_children;
  if (last_child_rank < size) {
    num_children = 2;
  } else if (last_child_rank == size ) {
    num_children = 1;
  } else {
    num_children = 0;
  }


  if (num_children > 0) {
    //    fprintf(stderr, "rank %d: 1. >>>>>>>>> communicator: %p <<<<<<<<<<<\n", my_rank, reduction_comm);
    MPI_Irecv(&reduction_val, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, reduction_comm, &reduction_recv_req);
  }

  if (my_rank != 0) {
    MPI_Isend(&my_rank,     1, MPI_INT, parent,           0, reduction_comm, &reduction_send_req);
    //    fprintf(stderr, "Isend: rank %d: send_request: %p\n", my_rank, reduction_send_req);
  }
  
  //  fprintf(stderr, "rank %d: 1. >>>>>>>>> communicator: %p <<<<<<<<<<<\n", my_rank, reduction_comm);

  return 0;
}

int bin_reduction_end()
{
  MPI_Status recv_status, send_status;
  int last_child_rank = (my_rank + 1) * 2;
  int num_children = 0 ;
  int recv_count = 0;
  int flag = 0;
  
  if (last_child_rank < size) {
    num_children = 2;
  } else if (last_child_rank == size ) {
    num_children = 1;
  } else {
    num_children = 0;
  }

  //  fprintf(stderr, "my_rank: %3d\n", my_rank);

  while (recv_count < num_children) {
    MPI_Test(&reduction_recv_req, &flag, &recv_status);
    if (flag) {
      recv_count++;
      //      fprintf(stderr, "rank %d: 2. >>>>>>>>> communicator: %p <<<<<<<<<<< recv_count: %d, num_children: %d\n",
      //      	      my_rank, reduction_comm, recv_count, num_children);
      if (recv_count < num_children) MPI_Irecv(&reduction_val, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, reduction_comm, &reduction_recv_req);
      //if (recv_count < num_children) MPI_Irecv(&reduction_val, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &reduction_recv_req);
    }
    //    fprintf(stderr, "my_rank: %3d, recv_count: %3d, num_children: %d\n", my_rank, recv_count, num_children);
  }
  //  fprintf(stderr, "my_rank: %3d: complete\n", my_rank);

  //  MPI_Cancel(&reduction_recv_req);
  if (my_rank != 0) {
    //    fprintf(stderr, "Send Test : rank %d: send_request: %p\n", my_rank, reduction_send_req);
    int flag_a = 0;
    while (flag_a == 0) {
      //      fprintf(stderr, "rank %d: 3. >>>>>>>>> communicator: %p <<<<<<<<<<< recv_count: %d, num_children: %d\n", 
      //	      my_rank, reduction_comm, recv_count, num_children);
      MPI_Test(&reduction_send_req, &flag_a, &send_status);
    }
    //    fprintf(stderr, "Send Test End : rank %d: send_request: %p\n", my_rank, reduction_send_req);
    //    MPI_Wait(&reduction_send_req, &status);
  }


  
  //  fprintf(stderr, "my_rank: %3d: complete finalize\n", my_rank);

  /* usleep(my_rank * 10000); */
  /* printf("my_rank: %3d, parent: %3d, num_children: %3d\n", my_rank, parent, num_children); */
  /* exit(0); */
  return 0;
}

int main(int argc, char *argv[])
{

  int seed;
  int i, k;

  struct key_val *my_kv, *final_kv;
  struct key_val recv_kv[NUM_KV_PER_RANK];

  MPI_Status status[NUM_KV_PER_RANK];
  MPI_Request request[NUM_KV_PER_RANK];

  /* Init */
  MPI_Init(&argc, &argv);
  signal(SIGSEGV, SIG_DFL);
  start = MPI_Wtime();
  MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);

  seed = my_rank;//time(0);//my_rank;
  init_rand(seed);

  my_kv = (struct key_val*)malloc(sizeof(struct key_val) * NUM_KV_PER_RANK);
  for (i = 0; i < NUM_KV_PER_RANK; i++) {
    my_kv[i].key = my_rank * NUM_KV_PER_RANK + i;
    my_kv[i].val = my_rank;//get_rand(MAX_VAL);
    recv_msg_count[i] = 0;
    send_msg_count[i] = 0;
  }

  MPI_Comm_dup(MPI_COMM_WORLD, &reduction_comm);

  for (k = 0; k < NUM_ITE; ++k) {
    /* ======================== */
    /* 1. Send: binary tree reduction */
    /* ======================== */
    bin_reduction_start();

    /* ======================== */
    /* 2.  neighbor exchange    */
    /* ======================== */
#if 0
    for (i = 0; i < NUM_KV_PER_RANK; i++) {
      //    printf("rank %d: recv: %d\n", my_rank, (my_rank + size - (i + 1))        % size);
      MPI_Irecv(&recv_kv[i], 8, MPI_CHAR, (my_rank + size - (i + 1))        % size, MPI_ANY_TAG, MPI_COMM_WORLD, &request[i]);
    }

    for (i = 0; i < NUM_KV_PER_RANK; i++) {
      //    printf("rank %d: send: %d\n", my_rank, (my_rank + (i + 1))        % size);
      MPI_Send(&my_kv[i], 8, MPI_CHAR, (my_rank + (i + 1))        % size, 0, MPI_COMM_WORLD);
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
#if 0
	if (my_rank == 2) {
	  int count = -1;
	  MPI_Get_count(&status[i], MPI_CHAR, &count);
	  fprintf(stderr, "i    : rank:%d: Receved source:%d tag:%d outcount:%d i    :%d count:%d\n",
		  my_rank, status[i].MPI_SOURCE, status[i].MPI_TAG, testsome_outcount, i, count);
	  fprintf(stderr, "index: rank:%d: Receved source:%d tag:%d outcount:%d index:%d\n",
		  my_rank, status[recv_index].MPI_SOURCE, status[recv_index].MPI_TAG, testsome_outcount, recv_index);
	}
#endif
	memcpy(&sendrecv_kv, &recv_kv[recv_index], sizeof(struct key_val));
	//memset(&sendrecv_kv, 0, sizeof(struct key_val));
	recv_msg_count[recv_index]++;
	if (recv_msg_count[recv_index] < MAX_MESG_PASS) {
	  MPI_Irecv(&recv_kv[recv_index], 8, MPI_CHAR, (my_rank + size - (recv_index + 1)) % size, MPI_ANY_TAG, MPI_COMM_WORLD, &request[recv_index]);
	} else {
	  MPI_Irecv(&recv_kv[recv_index], 8, MPI_CHAR, (my_rank + size - (recv_index + 1)) % size, MPI_ANY_TAG, MPI_COMM_WORLD, &request[recv_index]);
	}

	if (send_msg_count[recv_index] < MAX_MESG_PASS) {
	  sendrecv_kv.val = get_hash(sendrecv_kv.val, MAX_VAL);
	  //	MPI_Send(&sendrecv_kv, 2, MPI_INT, (my_rank + recv_index) % size, 0, MPI_COMM_WORLD);
	  usleep((rand() % 2) * 100000);
	  /* MPI_Request req; */
	  /* MPI_Status stat; */
	  /* MPI_Isend(&sendrecv_kv, 8, MPI_CHAR, (my_rank + recv_index) % size, 0, MPI_COMM_WORLD, &req); */
	  /* MPI_Wait(&req, &stat); */
	  MPI_Send(&sendrecv_kv, 8, MPI_CHAR, (my_rank + (recv_index + 1)) % size, 0, MPI_COMM_WORLD);
	  send_msg_count[recv_index]++;
	}
      } // end: for
    } // end: while
#endif
    /* ======================== */
    /* 3. End: binary tree reduction */
    /* ======================== */
    bin_reduction_end();
  } // end: for

  MPI_Comm_free(&reduction_comm);


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
