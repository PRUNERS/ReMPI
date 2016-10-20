#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <sys/time.h>
#include <time.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>

#include "rempi_test_util.h"

#define NUM_ITE (2)
#define NUM_KV_PER_RANK (3)
#define MAX_VAL (100)
#define MAX_MESG_PASS (10)

//#define USE_MPI_ISEND
#define USE_BIN_REDUCTION
//#define USE_WAITALL
//#define USE_COMM_DUP

#define dbg_printf(format, ...) \
  do { \
  fprintf(stderr, "TEST:%4d: " format " (%s:%d)\n", my_rank, ## __VA_ARGS__, __FILE__, __LINE__); \
  } while (0)

int hash_count = 0;

static double start, end, overall_start, overall_end;
int recv_msg_count[NUM_KV_PER_RANK], send_msg_count[NUM_KV_PER_RANK];
int my_rank;
int size;

struct key_val{
  int key;
  int val;
};

int num_iterations = 0;

// === For binary communication ===
MPI_Comm reduction_comm;
MPI_Request reduction_recv_req, reduction_send_req;
int reduction_val = 0;
int reduction_comm_steps = 0;

// === For neighbor communication ===
struct key_val *my_kv, *final_kv;
struct key_val recv_kv[NUM_KV_PER_RANK];
struct key_val sendrecv_kv[NUM_KV_PER_RANK];

MPI_Status status[NUM_KV_PER_RANK];
MPI_Request request[NUM_KV_PER_RANK];
MPI_Status send_stat;
MPI_Request send_req;
int flag = 0 ;
int neighbor_comm_steps = 0;

double get_runtime()
{
  return MPI_Wtime() - start;
}

int is_finished(){
  int i;
  for (i = 0; i < NUM_KV_PER_RANK; i++) {
    if (send_msg_count[i] < neighbor_comm_steps || recv_msg_count[i] < neighbor_comm_steps) return 0;
  }
  return 1;
}

/* MPI_Request waitall_recv_req[2], waitall_send_req[2]; */
/* int waitall_send_val[2], waitall_recv_val[2]; */
/* void waitall_start()  */
/* { */
/*   waitall_send_val[0] = my_rank; */
/*   waitall_send_val[1] = my_rank; */
/*   MPI_Isend(&waitall_send_val[0], 1, MPI_INT, (my_rank + 1)        % size , 15, MPI_COMM_WORLD, &waitall_send_req[0]); */
/*   MPI_Isend(&waitall_send_val[1], 1, MPI_INT, (my_rank + size - 1) % size , 15, MPI_COMM_WORLD, &waitall_send_req[1]); */
/*   MPI_Irecv(&waitall_recv_val[0], 1, MPI_INT, MPI_ANY_SOURCE              , 15, MPI_COMM_WORLD, &waitall_recv_req[0]); */
/*   MPI_Irecv(&waitall_recv_val[1], 1, MPI_INT, MPI_ANY_SOURCE              , 15, MPI_COMM_WORLD, &waitall_recv_req[1]); */
/*   return; */
/* } */


/* void waitall_end()  */
/* { */

/* #if 0 */
/*   MPI_Status status; */
/*   int flag; */
/*   while(!flag) MPI_Test(&waitall_send_req[0], &flag, &status); */
/*   while(!flag) MPI_Test(&waitall_send_req[1], &flag, &status); */
/*   while(!flag) MPI_Test(&waitall_recv_req[0], &flag, &status); */
/*   while(!flag) MPI_Test(&waitall_recv_req[1], &flag, &status); */
/* #else */
/*   MPI_Status status[2]; */
/*   MPI_Waitall(2, waitall_send_req, status); */
/*   MPI_Waitall(2, waitall_recv_req, status); */

/* #endif */
/*   return; */
/* } */

void bin_reduction_init()
{
  if (reduction_comm_steps == 0) return;
#ifdef USE_COMM_DUP
  MPI_Comm_dup(MPI_COMM_WORLD, &reduction_comm);
#else
  reduction_comm = MPI_COMM_WORLD;
#endif
  return;
}

int bin_reduction_start()
{
  if (reduction_comm_steps == 0) return 0;
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
    MPI_Irecv(&reduction_val, 1, MPI_INT, MPI_ANY_SOURCE, 22, reduction_comm, &reduction_recv_req);
  }
  if (my_rank != 0) {
    MPI_Isend(&my_rank,     1, MPI_INT, parent,           22, reduction_comm, &reduction_send_req);
  }
  return 0;
}

int bin_reduction_end()
{
  if (reduction_comm_steps == 0) return 0;

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

  while (recv_count < num_children) {
    MPI_Test(&reduction_recv_req, &flag, &recv_status);
    if (flag) {
      recv_count++;
      MPI_Irecv(&reduction_val, 1, MPI_INT, MPI_ANY_SOURCE, 22, reduction_comm, &reduction_recv_req);
    }
  }

  if (num_children > 0) {
    MPI_Cancel(&reduction_recv_req);
  }
  
  if (my_rank != 0) {
    int flag_a = 0;
    while (flag_a == 0) {
      MPI_Test(&reduction_send_req, &flag_a, &send_status);
    }
  }  
  //  usleep(my_rank * 10000);
  //  printf("my_rank: %3d, num_children: %3d\n", my_rank, num_children); 
  /* exit(0); */
  return 0;
}

void bin_reduction_finalize()
{
  if (reduction_comm_steps == 0) return;
#ifdef USE_COMM_DUP
  MPI_Comm_free(&reduction_comm);
#endif
  return;
}

void do_work()
{
  usleep(100);
  return;
}


void neighbor_communication_init()
{
  int i;
  int seed;
  seed = time(0);//my_rank;
  init_rand(seed);
  my_kv = (struct key_val*)malloc(sizeof(struct key_val) * NUM_KV_PER_RANK);
  for (i = 0; i < NUM_KV_PER_RANK; i++) {
    sendrecv_kv[i].key = my_rank * NUM_KV_PER_RANK + i;
    sendrecv_kv[i].val = my_rank;//get_rand(MAX_VAL);
  }
  return;
}

void neighbor_communication()
{
  int i;
  for (i = 0; i < NUM_KV_PER_RANK; i++) {
    recv_msg_count[i] = 0;
    send_msg_count[i] = 0;
  }


  for (i = 0; i < NUM_KV_PER_RANK; i++) {
    MPI_Irecv(&recv_kv[i], 8, MPI_CHAR, (my_rank + size - (i + 1))        % size, 0, MPI_COMM_WORLD, &request[i]);
  }

  for (i = 0; i < NUM_KV_PER_RANK; i++) {
    MPI_Isend(&sendrecv_kv[i], 8, MPI_CHAR, (my_rank + (i + 1))        % size, 0, MPI_COMM_WORLD, &send_req);
    flag = 0;
    while (flag == 0) {
      MPI_Test(&send_req, &flag, &send_stat); 
    }
    send_msg_count[i]++;
  }
  
  while (!is_finished()) {
    int testsome_outcount;
    int testsome_array_of_indices[NUM_KV_PER_RANK];

    do_work();

    testsome_outcount = 0;
    memset(status, 0, sizeof(MPI_Status) * NUM_KV_PER_RANK);
    MPI_Testsome(NUM_KV_PER_RANK, request,
		 &testsome_outcount, testsome_array_of_indices, status);

    if (testsome_outcount == 0) {
      hash_count++;
      //	if (my_rank == 0) fprintf(stdout, "hash_count %d -> %d\n", hash_count - 1, hash_count);
    }

    for(i = 0; i < testsome_outcount; i++) {
      int recv_index;
      int send_dest;
      recv_index = testsome_array_of_indices[i];
      memcpy(&sendrecv_kv[recv_index], &recv_kv[recv_index], sizeof(struct key_val));
      recv_msg_count[recv_index]++;
      if (recv_msg_count[recv_index] < neighbor_comm_steps) {
	MPI_Irecv(&recv_kv[recv_index], 8, MPI_CHAR, (my_rank + size - (recv_index + 1)) % size, 0, MPI_COMM_WORLD, &request[recv_index]);
      }


      if (send_msg_count[recv_index] < neighbor_comm_steps) {
	sendrecv_kv[recv_index].val = get_hash(sendrecv_kv[recv_index].val + hash_count++, MAX_VAL);
	MPI_Isend(&sendrecv_kv[recv_index], 8, MPI_CHAR, (my_rank + (recv_index + 1)) % size, 0, MPI_COMM_WORLD, &send_req);
	flag = 0;
	while (flag == 0) {
	  MPI_Test(&send_req, &flag, &send_stat);
	}
	send_msg_count[recv_index]++;
      }

    } // end: for(outcount)

  } // end: while

  return;
}

void neighbor_communication_finalize()
{
  free(my_kv);
  return;
}



int compute_global_hash()
{
  int i;
  int hash = -1;

  final_kv = (struct key_val*)malloc(sizeof(struct key_val) * NUM_KV_PER_RANK * size);
  memset(final_kv, 0, sizeof(final_kv));
  MPI_Gather(recv_kv, 2 * NUM_KV_PER_RANK, MPI_INT, final_kv, 2 * NUM_KV_PER_RANK, MPI_INT, 0, MPI_COMM_WORLD);


  if (my_rank == 0) {
#if 0
    int print_key = 0;
    while(print_key < size * NUM_KV_PER_RANK) {
      for (i = 0; i < size * NUM_KV_PER_RANK; i++) {
   	if (final_kv[i].key == print_key) {
    	  fprintf(stdout, "key: %d, val: %d\n", final_kv[i].key, final_kv[i].val);
    	  print_key++;
    	}
      }
    }
#endif
    for (i = 0; i < size * NUM_KV_PER_RANK; i++) {
      hash = get_hash(hash * final_kv[i].val + final_kv[i].key + hash_count++, 1000000);
      //      fprintf(stdout, "hash: %d (hash_count: %d)\n", hash, hash_count);
    }
  }
  free(final_kv);
  return hash;
}

void print_usage(char *bin)
{
  if (my_rank == 0) {
    fprintf(stderr, "./rempi_test_mini_mcb <# of iterations> <Enable binary communication> <# of neighbor communication steps>\n");
  }
  return;
}

int main(int argc, char *argv[])
{
  int i, k;
  int hash;

  /* Init */
  MPI_Init(&argc, &argv);
  start = MPI_Wtime();
  overall_start = get_time();
  MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);

  if (argc != 4) {
    print_usage(argv[0]);
    exit(0);
  }

  num_iterations       = atoi(argv[1]);
  reduction_comm_steps = atoi(argv[2]);
  neighbor_comm_steps  = atoi(argv[3]);

  if (my_rank == 0) {
    rempi_test_dbg_print("=====================================");
    rempi_test_dbg_print("# of iterations                    : %d", num_iterations);
    rempi_test_dbg_print("Enable binary communication steps  : %d", reduction_comm_steps);
    rempi_test_dbg_print("# of neighbor communication steps  : %d", neighbor_comm_steps);
    rempi_test_dbg_print("=====================================");
  }
  
  neighbor_communication_init();
  bin_reduction_init();
  MPI_Barrier(MPI_COMM_WORLD);

  for (k = 0; k < num_iterations; ++k) {
    if (my_rank == 0) rempi_test_dbgi_print(0, " Step %d", k);
    /* ======================== */
    /* 1. Send: binary tree reduction */
    /* ======================== */
#ifdef USE_BIN_REDUCTION
    bin_reduction_start();
#endif
    /* ======================== */
    /* 2.  neighbor exchange    */
    /* ======================== */
    neighbor_communication();
    MPI_Barrier(MPI_COMM_WORLD);
    /* ======================== */
    /* 3. End: binary tree reduction */
    /* ======================== */
#ifdef USE_BIN_REDUCTION
    bin_reduction_end();
#endif
    rempi_test_dbg_print("====== Finalizing: %d ========", k);
    MPI_Barrier(MPI_COMM_WORLD); /*<= korehazushitemo daizyoubuni shi ro !!! MPI_Cancel problem*/
    if (my_rank == 0) rempi_test_dbgi_print(0, " Step %d -- end", k);
  } // end: for

  //  exit(1);
  //  rempi_test_dbgi_print(0, " Finalizing ");

  MPI_Barrier(MPI_COMM_WORLD);
  hash = compute_global_hash();

  bin_reduction_finalize();
  neighbor_communication_finalize();

  end = MPI_Wtime();
  MPI_Finalize();
  overall_end = get_time();

  if (my_rank == 0) {
    fprintf(stdout, "Hash %u, Time (Main loop): %f\n", (unsigned int)hash, end - start);
  }

  return 0;
}
