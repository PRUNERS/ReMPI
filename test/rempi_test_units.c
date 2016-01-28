#include <stdio.h>
#include <mpi.h>
#include <sys/time.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

#include "rempi_test_util.h"

#define FUNC_NAME_LEN (32)
//#define NUM_TEST_MSG  (1024)
#define NUM_TEST_MSG  (4)
#define TEST_MSG_CHUNK_SIZE  (2)

#define MPI_Isend_id  (101)
#define MPI_Ibsend_id (102) 
#define MPI_Irsend_id (103)
#define MPI_Issend_id (104)

#define MPI_Send_init_id   (201)
#define MPI_Bsend_init_id  (202)
#define MPI_Rsend_init_id  (203)
#define MPI_Ssend_init_id  (204)

#define MPI_Start_id    (301)
#define MPI_Startall_id (302)

#define MPI_Test_id     (401)
#define MPI_Testany_id  (402)
#define MPI_Testsome_id (403)
#define MPI_Testall_id  (404)
#define MPI_Wait_id     (405)
#define MPI_Waitany_id  (406)
#define MPI_Waitsome_id (407)
#define MPI_Waitall_id  (408)

#define MPI_Probe_id    (501)
#define MPI_Iprobe_id   (502)

#define MPI_Recv_id      (601)
#define MPI_Irecv_id     (602)
#define MPI_Recv_init_id (603)

#define TEST_MATCHING_FUNC
//#define TEST_PROBE_FUNC
//#define TEST_ISEND_FUNC
//#define TEST_INIT_SENDRECV_FUNC
//define TEST_START_FUNC


#define rempi_test_dbg_print(format, ...) \
  do { \
    fprintf(stderr, "REMPI(test):%3d: " format " (%s:%d)\n", my_rank, ## __VA_ARGS__, __FILE__, __LINE__); \
  } while (0)

#define rempi_test_dbgi_print(rank, format, ...) \
  do { \
    if (rank == 0) { \
      fprintf(stderr, "REMPI(test):%3d: " format " (%s:%d)\n", my_rank, ## __VA_ARGS__, __FILE__, __LINE__); \
    } \
  } while (0)


int my_rank=-1;
int size;
double start, end, overall_end;

int matching_ids[] = {
  MPI_Test_id,
  MPI_Testany_id,
  MPI_Testsome_id, 
  MPI_Testall_id,
  MPI_Wait_id,
  MPI_Waitany_id,
  MPI_Waitsome_id,
  MPI_Waitall_id
};

int probe_ids[] = {
  MPI_Probe_id,
  //  MPI_Iprobe_id
};

int isend_ids[] = {
  MPI_Isend_id,
  //  MPI_Ibsend_id, /*MPI_Ibsend does not work*/
  MPI_Irsend_id,
  MPI_Issend_id
};

int send_init_ids[] = {
  MPI_Send_init_id,
  //  MPI_Bsend_init_id /*MPI_Bsend_init does not work*/
  MPI_Rsend_init_id,
  //  MPI_Ssend_init_id /*MPI_Ssend_init does not work*/
};

int start_ids[] = {
  MPI_Start_id,
  MPI_Startall_id
};


static void rempi_test_randome_sleep()
{
  int rand = get_rand(5);
  if (rand > 3) {
    usleep(rand * 1000);
  }
  return;
}

void rempi_test_nonblocking_send_with_random_sleep(int isend_type)
{
  int i;
  MPI_Request request;
  MPI_Status status;
  
  for (i = 0; i < NUM_TEST_MSG; i++) {
    rempi_test_randome_sleep();
    switch(isend_type) {
    case MPI_Isend_id:
      MPI_Isend(&i, 1, MPI_INT, 0, i, MPI_COMM_WORLD, &request);
      break;
    case MPI_Ibsend_id:
      MPI_Ibsend(&i, 1, MPI_INT, 0, i, MPI_COMM_WORLD, &request);
      break;
    case MPI_Irsend_id:
      MPI_Irsend(&i, 1, MPI_INT, 0, i, MPI_COMM_WORLD, &request);
      break;
    case MPI_Issend_id:
      MPI_Issend(&i, 1, MPI_INT, 0, i, MPI_COMM_WORLD, &request);
      break;
    }
    MPI_Wait(&request, &status);
  }
}


void rempi_test_send_init_with_random_sleep(int send_init_type, int start_type)
{
  int i, j;
  MPI_Status statuses[TEST_MSG_CHUNK_SIZE];
  int vals[TEST_MSG_CHUNK_SIZE] = { my_rank, my_rank};
  MPI_Request requests[TEST_MSG_CHUNK_SIZE];

  if (NUM_TEST_MSG % TEST_MSG_CHUNK_SIZE != 0 || NUM_TEST_MSG < TEST_MSG_CHUNK_SIZE) {
    rempi_test_dbg_print("wrong configuration");
    exit(1);
  }
  
  for (i = 0; i < TEST_MSG_CHUNK_SIZE; i++) {
    switch(send_init_type) {
    case MPI_Send_init_id:
      MPI_Send_init(&vals[i], 1, MPI_INT, 0, vals[i], MPI_COMM_WORLD, &requests[i]);
      break;
    case MPI_Bsend_init_id:
      MPI_Bsend_init(&vals[i], 1, MPI_INT, 0, vals[i], MPI_COMM_WORLD, &requests[i]);
      break;
    case MPI_Rsend_init_id:
      MPI_Rsend_init(&vals[i], 1, MPI_INT, 0, vals[i], MPI_COMM_WORLD, &requests[i]);
      break;
    case MPI_Ssend_init_id:
      MPI_Ssend_init(&vals[i], 1, MPI_INT, 0, vals[i], MPI_COMM_WORLD, &requests[i]);
      break;
    default:
      rempi_test_dbg_print("Invalid send_init_type: %d", send_init_type);
      exit(1);
    }  
  }
  
  for (i = 0; i < NUM_TEST_MSG / TEST_MSG_CHUNK_SIZE; i++) {
    switch(start_type) {
    case MPI_Start_id:
      for (j = 0; j < TEST_MSG_CHUNK_SIZE; j++) {
	rempi_test_randome_sleep();
	MPI_Start(&requests[j]);
	MPI_Wait(&requests[j], &statuses[j]);
      }
      break;
    case MPI_Startall_id:
      rempi_test_randome_sleep();
      MPI_Startall(TEST_MSG_CHUNK_SIZE, requests);
      MPI_Waitall(TEST_MSG_CHUNK_SIZE, requests, statuses);
      break;
    default:
      rempi_test_dbg_print("Invalid start_type: %d", start_type);
      exit(1);
    }
  }
}

void rempi_test_mpi_sends_with_random_sleep()
{
  int i;
  for (i = 0; i < NUM_TEST_MSG; i++) {
    rempi_test_randome_sleep();
    MPI_Send(&i, 1, MPI_INT, 0, i, MPI_COMM_WORLD);
  }
}


void rempi_test_init_sendrecv(int send_init_type, int send_start_type, int recv_start_type)
{
  if (my_rank != 0) {
    rempi_test_send_init_with_random_sleep(send_init_type, send_start_type);
    return;
  }

  /*For rank 0*/
  int i, j;
  MPI_Status statuses[TEST_MSG_CHUNK_SIZE];
  int vals[TEST_MSG_CHUNK_SIZE] = { my_rank, my_rank};
  MPI_Request requests[TEST_MSG_CHUNK_SIZE];

  for (i = 0; i < TEST_MSG_CHUNK_SIZE; i++) {
    MPI_Recv_init(&vals[i], 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &requests[i]);
  }

  for (i = 0; i < NUM_TEST_MSG / TEST_MSG_CHUNK_SIZE; i++) {
    switch(recv_start_type) {
    case MPI_Start_id:
      for (j = 0; j < TEST_MSG_CHUNK_SIZE; j++) {
	MPI_Start(&requests[j]);
	MPI_Wait(&requests[j], &statuses[j]);
      }
      break;
    case MPI_Startall_id:
      MPI_Startall(TEST_MSG_CHUNK_SIZE, requests);
      MPI_Waitall(TEST_MSG_CHUNK_SIZE, requests, statuses);
      break;
    default:
      rempi_test_dbg_print("Invalid start_type: %d", recv_start_type);
      exit(1);
    }
  }
}

void rempi_test_nonblocking_sends(int isend_type)
{
  int i, j;
  if (my_rank != 0) {
    rempi_test_nonblocking_send_with_random_sleep(isend_type);
    return;
  }
  
  /*For rank 0*/
  int recv_val;
  int num_send_msgs = (size - 1) * NUM_TEST_MSG;
  MPI_Status status;
  for (i = 0; i < num_send_msgs; i++) {
    MPI_Recv(&recv_val, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
    //    rempi_test_dbg_print("val %d (source: %d)", recv_val, status.MPI_SOURCE);
  }

  return;
}

void rempi_test_probe(int probe_type)
{
  int i, j;
  if (my_rank != 0) {
    rempi_test_mpi_sends_with_random_sleep();
    return;
  }

  /*For rank 0*/
  int flag = 0;
  int recv_val;
  int recv_count;
  int num_send_msgs = (size - 1) * NUM_TEST_MSG;
  MPI_Status status;
  for (i = 0; i < num_send_msgs; i++) {
    switch(probe_type) {
    case MPI_Probe_id:
      MPI_Probe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
      break;
    case MPI_Iprobe_id:
      while(1) {
	flag = 0;
	MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &flag, &status);
	if (flag) {
	  break;
	}
      }
      break;
    default:
      rempi_test_dbg_print("Invalid probe_type: %d", probe_type);
      exit(1);
    }
    MPI_Get_count(&status, MPI_INT, &recv_count);
    MPI_Recv(&recv_val, recv_count, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
    //    MPI_Recv(&recv_val, recv_count, MPI_INT, status.MPI_SOURCE, status.MPI_TAG, MPI_COMM_WORLD, &status);
    //    rempi_test_dbg_print("val %d (source: %d)", recv_val, status.MPI_SOURCE);
  }
}

void rempi_test_mpi_send_and_nonblocking_recvs(int matching_type)
{
  int i, j;
  if (my_rank != 0) {
    rempi_test_mpi_sends_with_random_sleep();
    return;
  }

  /*For rank 0*/
  int *recv_vals;
  int num_sender = size - 1;
  int num_send_msgs = num_sender * NUM_TEST_MSG;
  int recv_msg_count = 0;
  int flag = 0;
  int request_index = 0;
  int matched_index;
  int matched_count = 0;
  MPI_Request *requests;
  MPI_Status  *statuses;
  MPI_Status status;
  int *matched_indices;

  requests   = (MPI_Request*)malloc(sizeof(MPI_Request) * (num_sender));
  statuses   = (MPI_Status*)malloc(sizeof(MPI_Status) * (num_sender));
  recv_vals  = (int*)malloc(sizeof(int) * (num_sender));
  matched_indices = (int*)malloc(sizeof(int) * (num_sender));
  for (i = 0; i < num_sender; i++) {
    MPI_Irecv(&recv_vals[i], 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &requests[i]);
  }
  

  switch(matching_type) {
  case MPI_Test_id:
    for (i = 0; i < num_send_msgs; i++) {
      while (1) {
	MPI_Test(&requests[request_index], &flag, &status);
	if (flag==1) {
	  flag = 0;
	  break;
	}
      }
      MPI_Irecv(&recv_vals[request_index], 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &requests[request_index]);
      request_index = (request_index + 1) % num_sender;
    }
    break;
  case MPI_Testany_id:
    for (i = 0; i < num_send_msgs; i++) {
      while (1) {
	MPI_Testany(num_sender, requests, &matched_index, &flag, &status);
	if (flag==1) {
	  flag = 0;
	  break;
	}
      }
      MPI_Irecv(&recv_vals[matched_index], 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &requests[matched_index]);
    }
    break;
  case MPI_Testsome_id:
    recv_msg_count = 0;
    while (recv_msg_count < num_send_msgs) {
      while (1) {
	matched_count = 0;
	MPI_Testsome(num_sender, requests, &matched_count, matched_indices, statuses);
	if (matched_count > 0) {
	  recv_msg_count += matched_count;
	  break;
	}
      }
      for (i = 0; i < matched_count; i++) {
	matched_index = matched_indices[i];
	MPI_Irecv(&recv_vals[matched_index], 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &requests[matched_index]);
      }
    }
    break;
  case MPI_Testall_id:
    recv_msg_count = 0;
    while (recv_msg_count < num_send_msgs) {
      while (1) {
	flag = 0;
	MPI_Testall(num_sender, requests, &flag, statuses);
	if (flag == 1) {
	  recv_msg_count += num_sender;
	  break;
	}
      }
      for (i = 0; i < num_sender; i++) {
	matched_index = i;
	MPI_Irecv(&recv_vals[matched_index], 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &requests[matched_index]);
      }
    }
    break;
  case MPI_Wait_id:
    for (i = 0; i < num_send_msgs; i++) {
      MPI_Wait(&requests[request_index], &status);
      MPI_Irecv(&recv_vals[request_index], 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &requests[request_index]);
      request_index = (request_index + 1) % num_sender;
    }
    break;
  case MPI_Waitany_id:
    for (i = 0; i < num_send_msgs; i++) {
      MPI_Waitany(num_sender, requests, &matched_index, &status);
      MPI_Irecv(&recv_vals[matched_index], 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &requests[matched_index]);
    }
    break;
  case MPI_Waitsome_id:
    recv_msg_count = 0;
    while (recv_msg_count < num_send_msgs) {
      MPI_Waitsome(num_sender, requests, &matched_count, matched_indices, statuses);
      recv_msg_count += matched_count;
      for (i = 0; i < matched_count; i++) {
	matched_index = matched_indices[i];
	MPI_Irecv(&recv_vals[matched_index], 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &requests[matched_index]);
      }
    }
    break;
  case MPI_Waitall_id:
    recv_msg_count = 0;
    while (recv_msg_count < num_send_msgs) {
      MPI_Waitall(num_sender, requests, statuses);
      recv_msg_count += num_sender;
      for (i = 0; i < num_sender; i++) {
	matched_index = i;
	MPI_Irecv(&recv_vals[matched_index], 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &requests[matched_index]);
      }
    }
    break;
  };

  for (i = 0; i < num_sender; i++) {
    MPI_Cancel(&requests[i]);
  }

  free(requests);
  free(statuses);
  free(recv_vals);
  free(matched_indices);

}


int main(int argc, char *argv[])
{
  int i, j;
  /* Init */
  MPI_Init(&argc, &argv);
  signal(SIGSEGV, SIG_DFL);
  MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);
  
  init_ndrand();

  start = MPI_Wtime();

#if defined(TEST_MATCHING_FUNC)
  if (my_rank == 0) fprintf(stdout, "Start testing matching functions ... "); fflush(stdout);
  for (i = 0; i < sizeof(matching_ids)/ sizeof(int); i++) {
    rempi_test_mpi_send_and_nonblocking_recvs(matching_ids[i]);
    MPI_Barrier(MPI_COMM_WORLD);
  }
  if (my_rank == 0) fprintf(stdout, "Done\n"); fflush(stdout);
#endif
  
#if defined(TEST_PROBE_FUNC)
  if (my_rank == 0) fprintf(stdout, "Start testing probe functions ... "); fflush(stdout);
  for (i = 0; i < sizeof(probe_ids) / sizeof(int); i++) {
    rempi_test_probe(probe_ids[i]);
    MPI_Barrier(MPI_COMM_WORLD);
  }
  if (my_rank == 0) fprintf(stdout, "Done\n"); fflush(stdout);
#endif

#if defined(TEST_ISEND_FUNC)
  if (my_rank == 0) fprintf(stdout, "Start testing isend functions ... "); fflush(stdout);
  for (i = 0; i < sizeof(isend_ids) / sizeof(int); i++) {
    rempi_test_nonblocking_sends(isend_ids[i]);
    MPI_Barrier(MPI_COMM_WORLD);
  }
  if (my_rank == 0) fprintf(stdout, "Done\n"); fflush(stdout);
#endif

#if defined(TEST_INIT_SENDRECV_FUNC)
  if (my_rank == 0) fprintf(stdout, "Start testing init send/recv functions ... "); fflush(stdout);
  for (i = 0; i < sizeof(send_init_ids)/sizeof(int); i++) {
    rempi_test_init_sendrecv(send_init_ids[i], MPI_Start_id, MPI_Start_id);
    MPI_Barrier(MPI_COMM_WORLD);
  }
  if (my_rank == 0) fprintf(stdout, "Done\n"); fflush(stdout);
#endif

#if defined(TEST_START_FUNC)
  if (my_rank == 0) fprintf(stdout, "Start testing start/startall functions ... "); fflush(stdout);
  for (i = 0; i < sizeof(start_ids)/sizeof(int); i++) {
    for (j = 0; j < sizeof(start_ids)/sizeof(int); j++) {
      rempi_test_init_sendrecv(MPI_Send_init_id, start_ids[i], start_ids[j]);
      MPI_Barrier(MPI_COMM_WORLD);
    }
  }
  if (my_rank == 0) fprintf(stdout, "Done\n"); fflush(stdout);
#endif


  end = MPI_Wtime();

  MPI_Finalize();
  if (my_rank == 0) {
    rempi_test_dbg_print("Time to complete tests: %f", end - start);
  }
  return 0;
}
