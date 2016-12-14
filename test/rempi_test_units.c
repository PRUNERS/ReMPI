#include <stdio.h>
#include <mpi.h>
#include <sys/time.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/mman.h>

#include "rempi_test_util.h"

#define FUNC_NAME_LEN (32)
//#define NUM_TEST_MSG  (1024)
//#define NUM_TEST_MSG  (100000) 
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

#define MPI_Status_NULL_id   (701)
#define MPI_Status_ignore_id (702)


#define TEST_MATCHING_FUNC
#define TEST_PROBE_FUNC
#define TEST_ISEND_FUNC
#define TEST_INIT_SENDRECV_FUNC
#define TEST_START_FUNC
#define TEST_NULL_STATUS
#define TEST_SENDRECV_REQ
#define TEST_COMM_DUP


#define rempi_test_dbg_print(format, ...)				\
  do {									\
    fprintf(stderr, "REMPI(test):%3d: " format " (%s:%d)\n", my_rank, ## __VA_ARGS__, __FILE__, __LINE__); \
  } while (0)

#define rempi_test_dbgi_print(rank, format, ...)			\
  do {									\
    if (rank == 0) {							\
      fprintf(stderr, "REMPI(test):%3d: " format " (%s:%d)\n", my_rank, ## __VA_ARGS__, __FILE__, __LINE__); \
    }									\
  } while (0)


int my_rank=-1;
int size;
static double start, end, overall_end;

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
  MPI_Iprobe_id
};

int isend_ids[] = {
  MPI_Isend_id,
  //   MPI_Ibsend_id, /*MPI_Ibsend does not work in MPI*/
  MPI_Irsend_id,
  MPI_Issend_id
};

int send_init_ids[] = {
  MPI_Send_init_id,
  //  MPI_Bsend_init_id /*MPI_Bsend_init does not work in MPI*/
  MPI_Rsend_init_id,
  //  MPI_Ssend_init_id /*MPI_Ssend_init does not work in MPI*/
};

int start_ids[] = {
  MPI_Start_id,
  MPI_Startall_id
};

int status_ids[] = {
  //  MPI_Status_NULL_id, // it failes in mvapich2-intel-2.2
  MPI_Status_ignore_id
};

int sendrecv_req_ids[] = {
  MPI_Test_id,
  MPI_Testany_id,
  MPI_Testsome_id,
  MPI_Testall_id,
  MPI_Wait_id,
  MPI_Waitany_id,
  MPI_Waitsome_id,
  MPI_Waitall_id
};

int request_null_ids[] = {
  MPI_Test_id,
  MPI_Testany_id,
  MPI_Testsome_id,
  MPI_Testall_id,
  MPI_Wait_id,
  MPI_Waitany_id,
  MPI_Waitsome_id,
  MPI_Waitall_id
};

int zero_incount_ids[] = {
  MPI_Testany_id,
  MPI_Testsome_id,
  MPI_Testall_id,
  MPI_Waitany_id,
  MPI_Waitsome_id,
  MPI_Waitall_id
};


static   size_t ag;  
static   size_t addr;

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

void rempi_test_mpi_sends_with_random_sleep(MPI_Comm comm)
{
  int i;

  for (i = 0; i < NUM_TEST_MSG; i++) {
    //    rempi_test_randome_sleep();
    //    rempi_test_dbg_print("sending to 0");
    MPI_Send(&i, 1, MPI_INT, 0, my_rank, comm);
    //    rempi_test_dbg_print("sent to 0");
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
  return;
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
    rempi_test_mpi_sends_with_random_sleep(MPI_COMM_WORLD);
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

/* Test: matching */
void rempi_test_mpi_send_and_nonblocking_recvs(int matching_type)
{


  int i, j;
  if (my_rank != 0) {
    rempi_test_mpi_sends_with_random_sleep(MPI_COMM_WORLD);
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
	//	rempi_test_dbg_print("            SOURCE: %d, TAG: %d, index: %d, val: %d", statuses[i].MPI_SOURCE, statuses[i].MPI_TAG, matched_index, recv_vals[matched_index]);
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
  default:
    rempi_test_dbg_print("Invalid matching_type: %d", matching_type);
    exit(1);    
  };

  for (i = 0; i < num_sender; i++) {
    MPI_Cancel(&requests[i]);
  }

  free(requests);
  free(statuses);
  free(recv_vals);
  free(matched_indices);

}


void rempi_test_null_status(int status_type)
{
  int i, j;
  if (my_rank != 0) {
    rempi_test_mpi_sends_with_random_sleep(MPI_COMM_WORLD);
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
  if (status_type == MPI_Status_NULL_id) {
    statuses   = NULL;
  } else if (status_type == MPI_Status_ignore_id) {
    statuses   = MPI_STATUS_IGNORE;
  } else {
    rempi_test_dbg_print("no such status_type");
    exit(1);
  } 

  recv_vals  = (int*)malloc(sizeof(int) * (num_sender));
  matched_indices = (int*)malloc(sizeof(int) * (num_sender));
  for (i = 0; i < num_sender; i++) {
    MPI_Irecv(&recv_vals[i], 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &requests[i]);
  }


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

  for (i = 0; i < num_sender; i++) {
    MPI_Cancel(&requests[i]);
  }

  free(requests);
  free(recv_vals);
  free(matched_indices);

  return;
}


#define SENDRECV_REQ_NUM_MSG (4)
void rempi_test_sendrecv_req(int matching_type)
{
  int i;
  int num_msgs = SENDRECV_REQ_NUM_MSG;
  MPI_Request requests[SENDRECV_REQ_NUM_MSG];
  MPI_Status  statuses[SENDRECV_REQ_NUM_MSG];
  int         matched_indices[SENDRECV_REQ_NUM_MSG];
  MPI_Status  status;
  int recv_vals[SENDRECV_REQ_NUM_MSG/2];
  int index = 0;
  int size;
  int flag;
  int matched_index;
  int matched_count;
  int recv_msg_count;
  
  MPI_Comm_size(MPI_COMM_WORLD, &size);
  
  for (i = 0; i < num_msgs/2; i++) {
    int dest = (my_rank + 1 + i) % size;
    MPI_Isend(&i, 1, MPI_INT, dest, i, MPI_COMM_WORLD, &requests[index]);
    statuses[index].MPI_SOURCE = -1;
    statuses[index].MPI_TAG = -1;
    index++;
  }

  for (i = 0; i < num_msgs/2; i++) {
    int src = (my_rank - 1 -i + size) % size;
    MPI_Irecv(&recv_vals[i], 1, MPI_INT, src, i, MPI_COMM_WORLD, &requests[index]);
    statuses[index].MPI_SOURCE = -2;
    statuses[index].MPI_TAG = -2;
    index++;
  }

  //  rempi_test_dbg_print("req: %p, status: %p", requests, statuses);

  switch(matching_type) {
  case MPI_Test_id:
    for (i = 0; i < num_msgs; i++) {
      while (1) {
	MPI_Test(&requests[i], &flag, &status);
	if (flag==1) {
	  flag = 0;
	  break;
	}
      }
    }
    break;
  case MPI_Testany_id:
    for (i = 0; i < num_msgs; i++) {
      while (1) {
	MPI_Testany(num_msgs, requests, &matched_index, &flag, &status);
	if (flag==1) {
	  flag = 0;
	  break;
	}
      }
    }
    break;
  case MPI_Testsome_id:
    recv_msg_count = 0;
    while (recv_msg_count < num_msgs) {
      while (1) {
	matched_count = 0;
	MPI_Testsome(num_msgs, requests, &matched_count, matched_indices, statuses);
	if (matched_count > 0) {
	  recv_msg_count += matched_count;
	  /* for (i = 0; i < matched_count; i++) { */
	  /*   rempi_test_dbg_print("index: %d, rank: %d, tag: %d", matched_indices[i], statuses[i].MPI_SOURCE, statuses[i].MPI_TAG); */
	  /* } */
	  break;
	}
      }
    }
    break;
  case MPI_Testall_id:
    while (1) {
      flag = 0;
      MPI_Testall(num_msgs, requests, &flag, statuses);
      if (flag == 1) {
	break;
      }
    }
    break;
  case MPI_Wait_id:
    for (i = 0; i < num_msgs; i++) {
      MPI_Wait(&requests[i], &status);
    }
    break;
  case MPI_Waitany_id:
    for (i = 0; i < num_msgs; i++) {
      MPI_Waitany(num_msgs, requests, &matched_index, &status);
    }
    break;
  case MPI_Waitsome_id:
    recv_msg_count = 0;
    while (recv_msg_count < num_msgs) {
      MPI_Waitsome(num_msgs, requests, &matched_count, matched_indices, statuses);
      recv_msg_count += matched_count;
    }
    break;
  case MPI_Waitall_id:
    MPI_Waitall(num_msgs, requests, statuses);
    /* for (i = 0; i < num_msgs; i++) { */
    /*   rempi_test_dbg_print("source: %d, tag: %d", statuses[i].MPI_SOURCE, statuses[i].MPI_TAG); */
    /* } */
    break;
  };


  return;
}


void rempi_test_comm_dup()
{
  MPI_Comm comm_dup;
  int num_sender = size - 1;
  int num_send_msgs = num_sender * NUM_TEST_MSG;
  int recv_msg_count = 0, matched_count = 0;
  MPI_Request *requests;
  MPI_Status *statuses;
  int *recv_vals;
  int *matched_indices;
  int matched_index;
  int i;
  

  MPI_Comm_dup(MPI_COMM_WORLD, &comm_dup);
  if (my_rank != 0) {
    rempi_test_mpi_sends_with_random_sleep(comm_dup);
    rempi_test_mpi_sends_with_random_sleep(MPI_COMM_WORLD);
  } else {

    requests   = (MPI_Request*)malloc(sizeof(MPI_Request) * (num_sender));
    statuses   = (MPI_Status*)malloc(sizeof(MPI_Status) * (num_sender));
    recv_vals  = (int*)malloc(sizeof(int) * (num_sender));
    matched_indices = (int*)malloc(sizeof(int) * (num_sender));

    for (i = 0; i < num_sender; i++) {
      MPI_Irecv(&recv_vals[i], 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &requests[i]);
    }
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

    for (i = 0; i < num_sender; i++) {
      MPI_Cancel(&requests[i]);
    }

    for (i = 0; i < num_sender; i++) {
      MPI_Irecv(&recv_vals[i], 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, comm_dup, &requests[i]);
    }
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
	MPI_Irecv(&recv_vals[matched_index], 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, comm_dup, &requests[matched_index]);
      }
    }

    for (i = 0; i < num_sender; i++) {
      MPI_Cancel(&requests[i]);
    }


    
    free(requests);
    free(statuses);
    free(recv_vals);
    free(matched_indices);
    
  }

}



#define REQUEST_NULL_LEN (4)
void rempi_test_request_null(int matching_type)
{
  int i;
  int flag;
  int matched_count;
  int matched_index;
  int matched_indices[REQUEST_NULL_LEN];
  MPI_Status status;
  MPI_Status statuses[REQUEST_NULL_LEN];
  MPI_Request request = MPI_REQUEST_NULL;
  MPI_Request requests[REQUEST_NULL_LEN] = {
    MPI_REQUEST_NULL, 
    MPI_REQUEST_NULL,
    MPI_REQUEST_NULL,
    MPI_REQUEST_NULL};

  if (my_rank != 0) return;



  switch(matching_type) {
  case MPI_Test_id:
    memset(&status, 0, sizeof(MPI_Status));
    MPI_Test(&request, &flag, &status);
    if (flag != 1 || status.MPI_SOURCE != MPI_ANY_SOURCE || status.MPI_TAG != MPI_ANY_TAG) {
      rempi_test_dbg_print("MPI_Test: flag: %d", flag);      
      exit(1); 
    }
    break;
  case MPI_Testany_id:
    memset(&status, 0, sizeof(MPI_Status));
    MPI_Testany(REQUEST_NULL_LEN, requests, &matched_index, &flag, &status);
    if (matched_index != MPI_UNDEFINED || flag != 1 || 
	status.MPI_SOURCE != MPI_ANY_SOURCE || status.MPI_TAG != MPI_ANY_TAG) {
      rempi_test_dbg_print("MPI_Testany: matched_index: %d, flag: %d (source: %d, tag: %d)", 
			   matched_index, flag, status.MPI_SOURCE, status.MPI_TAG);
      exit(1);
    }
    break;
  case MPI_Testsome_id:
    memset(statuses, 0, sizeof(MPI_Status) * REQUEST_NULL_LEN);
    MPI_Testsome(REQUEST_NULL_LEN, requests, &matched_count, matched_indices, statuses);
    if (matched_count != MPI_UNDEFINED) {
      rempi_test_dbg_print("MPI_Testsome: matched_count: %d", matched_count);
      exit(1);
    }
    for (i = 0; i < REQUEST_NULL_LEN; i++) {
      if (statuses[i].MPI_SOURCE != 0 || statuses[i].MPI_TAG != 0) {
	rempi_test_dbg_print("MPI_Testsome: index: %d (source: %d, tag: %d)", i,  statuses[i].MPI_SOURCE, statuses[i].MPI_TAG);
	exit(1);
      }
    }
    break;
  case MPI_Testall_id:
    memset(statuses, 0, sizeof(MPI_Status) * REQUEST_NULL_LEN);
    MPI_Testall(REQUEST_NULL_LEN, requests, &flag, statuses);
    if (flag != 1) {
      rempi_test_dbg_print("MPI_Testall: flag: %d", flag);
      exit(1);
    }

    for (i = 0; i < REQUEST_NULL_LEN; i++) {
      if (statuses[i].MPI_SOURCE != MPI_ANY_SOURCE || statuses[i].MPI_TAG != MPI_ANY_TAG) {
	rempi_test_dbg_print("MPI_Testall: index: %d (source: %d, tag: %d)", i,  statuses[i].MPI_SOURCE, statuses[i].MPI_TAG);
	exit(1);
      }
    }

    break;
  case MPI_Wait_id:
    memset(&status, 0, sizeof(MPI_Status));
    MPI_Wait(&request, &status);
    if (status.MPI_SOURCE != MPI_ANY_SOURCE || status.MPI_TAG != MPI_ANY_TAG) {
      rempi_test_dbg_print("MPI_Wait: (source: %d, tag: %d)", status.MPI_SOURCE, status.MPI_TAG);
      exit(1);
    }
    break;
  case MPI_Waitany_id:
    memset(&status, 0, sizeof(MPI_Status));
    MPI_Waitany(REQUEST_NULL_LEN, requests, &matched_index, &status);
    if (matched_index != MPI_UNDEFINED || status.MPI_SOURCE != MPI_ANY_SOURCE || status.MPI_TAG != MPI_ANY_TAG) {
      rempi_test_dbg_print("MPI_Waitany: matched_index: %d (source: %d, tag: %d)", 
			   matched_index, status.MPI_SOURCE, status.MPI_TAG);
      exit(1);
    }
    break;
  case MPI_Waitsome_id:
    memset(statuses, 0, sizeof(MPI_Status) * REQUEST_NULL_LEN);
    MPI_Waitsome(REQUEST_NULL_LEN, requests, &matched_count, matched_indices, statuses);
    if (matched_count != MPI_UNDEFINED) {
      rempi_test_dbg_print("MPI_Waitsome: matched_count: %d", matched_count);
      exit(1);
    }
    for (i = 0; i < REQUEST_NULL_LEN; i++) {
      if (statuses[i].MPI_SOURCE != 0 || statuses[i].MPI_TAG != 0) {
	rempi_test_dbg_print("MPI_Testsome: index: %d (source: %d, tag: %d)", i,  statuses[i].MPI_SOURCE, statuses[i].MPI_TAG);
	exit(1);
      }
    }
    break;
  case MPI_Waitall_id:
    memset(statuses, 0, sizeof(MPI_Status) * REQUEST_NULL_LEN);
    MPI_Waitall(REQUEST_NULL_LEN, requests, statuses);
    for (i = 0; i < REQUEST_NULL_LEN; i++) {
      if (statuses[i].MPI_SOURCE != MPI_ANY_SOURCE || statuses[i].MPI_TAG != MPI_ANY_TAG) {
	rempi_test_dbg_print("MPI_Waitall: index: %d (source: %d, tag: %d)", i,  statuses[i].MPI_SOURCE, statuses[i].MPI_TAG);
	exit(1);
      }
    }
    break;
  default:
    rempi_test_dbg_print("Invalid matching_type: %d", matching_type);
    exit(1);    
  };

  return;
}

void rempi_test_zero_incount(int matching_type)
{
  int i;
  int flag;
  int matched_count;
  int matched_index;
  int matched_indices[REQUEST_NULL_LEN];
  MPI_Status status;
  MPI_Status statuses[REQUEST_NULL_LEN];
  MPI_Request request = MPI_REQUEST_NULL;
  MPI_Request requests[REQUEST_NULL_LEN] = {
    MPI_REQUEST_NULL, 
    MPI_REQUEST_NULL,
    MPI_REQUEST_NULL,
    MPI_REQUEST_NULL};

  if (my_rank != 0) return;



  switch(matching_type) {
  case MPI_Testany_id:
    memset(&status, 0, sizeof(MPI_Status));
    MPI_Testany(0, requests, &matched_index, &flag, &status);
    if (matched_index != MPI_UNDEFINED || flag != 1 || 
    	status.MPI_SOURCE != MPI_ANY_SOURCE || status.MPI_TAG != MPI_ANY_TAG) {
      rempi_test_dbg_print("MPI_Testany: matched_index: %d, flag: %d (source: %d, tag: %d)", 
			   matched_index, flag, status.MPI_SOURCE, status.MPI_TAG);
      exit(1);
    }
    break;
  case MPI_Testsome_id:
    memset(statuses, 0, sizeof(MPI_Status) * REQUEST_NULL_LEN);
    MPI_Testsome(0, requests, &matched_count, matched_indices, statuses);
    if (matched_count != MPI_UNDEFINED) {
      rempi_test_dbg_print("MPI_Testsome: matched_count: %d", matched_count);
      exit(1);
    }
    for (i = 0; i < REQUEST_NULL_LEN; i++) {
      if (statuses[i].MPI_SOURCE != 0 || statuses[i].MPI_TAG != 0) {
	rempi_test_dbg_print("MPI_Testsome: index: %d (source: %d, tag: %d)", i,  statuses[i].MPI_SOURCE, statuses[i].MPI_TAG);
      	exit(1);
      }
    }
    break;
  case MPI_Testall_id:
    memset(statuses, 0, sizeof(MPI_Status) * REQUEST_NULL_LEN);
    MPI_Testall(0, requests, &flag, statuses);
    if (flag != 1) {
      rempi_test_dbg_print("MPI_Testall: flag: %d", flag);
      exit(1);
    }

    for (i = 0; i < REQUEST_NULL_LEN; i++) {
      if (statuses[i].MPI_SOURCE != 0 || statuses[i].MPI_TAG != 0) {
	rempi_test_dbg_print("MPI_Testall: index: %d (source: %d, tag: %d)", i,  statuses[i].MPI_SOURCE, statuses[i].MPI_TAG);
      	exit(1);
      }
    }

    break;
  case MPI_Waitany_id:
    memset(&status, 0, sizeof(MPI_Status));
    MPI_Waitany(0, requests, &matched_index, &status);
    if (matched_index != MPI_UNDEFINED || status.MPI_SOURCE != MPI_ANY_SOURCE || status.MPI_TAG != MPI_ANY_TAG) {
      rempi_test_dbg_print("MPI_Waitany: matched_index: %d (source: %d, tag: %d)", 
			   matched_index, status.MPI_SOURCE, status.MPI_TAG);
      exit(1);
    }
    break;
  case MPI_Waitsome_id:
    memset(statuses, 0, sizeof(MPI_Status) * REQUEST_NULL_LEN);
    MPI_Waitsome(0, requests, &matched_count, matched_indices, statuses);
    if (matched_count != MPI_UNDEFINED) {
      rempi_test_dbg_print("MPI_Waitsome: matched_count: %d", matched_count);
      exit(1);
    }
    for (i = 0; i < REQUEST_NULL_LEN; i++) {
      if (statuses[i].MPI_SOURCE != 0 || statuses[i].MPI_TAG != 0) {
	rempi_test_dbg_print("MPI_Testsome: index: %d (source: %d, tag: %d)", i,  statuses[i].MPI_SOURCE, statuses[i].MPI_TAG);
      	exit(1);
      }
    }
    break;
  case MPI_Waitall_id:
    memset(statuses, 0, sizeof(MPI_Status) * REQUEST_NULL_LEN);
    MPI_Waitall(0, requests, statuses);
    for (i = 0; i < REQUEST_NULL_LEN; i++) {
      if (statuses[i].MPI_SOURCE != 0 || statuses[i].MPI_TAG != 0) {
	rempi_test_dbg_print("MPI_Waitall: index: %d (source: %d, tag: %d)", i,  statuses[i].MPI_SOURCE, statuses[i].MPI_TAG);
      	exit(1);
      }
    }
    break;
  default:
    rempi_test_dbg_print("Invalid matching_type: %d", matching_type);
    exit(1);    
  };

  return;
}

void rempi_test_late_irecv()
{
  int val = my_rank;
  MPI_Request requests[2];

  if (my_rank == 0) {
    sleep(1);
    MPI_Recv(&val, 1, MPI_INT, 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    MPI_Recv(&val, 1, MPI_INT, 1, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
  } else if (my_rank == 1) {
    MPI_Isend(&val, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &requests[0]);
    MPI_Isend(&val, 1, MPI_INT, 0, 1, MPI_COMM_WORLD, &requests[1]);
    MPI_Waitall(2, requests, MPI_STATUS_IGNORE);
  }

  if (my_rank == 0) {
    sleep(1);
    MPI_Recv(&val, 1, MPI_INT, 1, 2, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    MPI_Recv(&val, 1, MPI_INT, 1, 3, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
  } else if (my_rank == 1) {
    MPI_Isend(&val, 1, MPI_INT, 0, 2, MPI_COMM_WORLD, &requests[0]);
    MPI_Isend(&val, 1, MPI_INT, 0, 3, MPI_COMM_WORLD, &requests[1]);
    MPI_Waitall(2, requests, MPI_STATUS_IGNORE);
  }
  return;

}


void rempi_test_clock_wait()
{
  int val = my_rank;
  MPI_Request request;

  if (my_rank == 2) {
    MPI_Send(&val, 1, MPI_INT, 1, 0, MPI_COMM_WORLD);
  } else if (my_rank == 1) {
    MPI_Recv(&val, 1, MPI_INT, 2, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    MPI_Send(&val, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
    MPI_Recv(&val, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
  } else if (my_rank == 0) {
    MPI_Recv(&val, 1, MPI_INT, 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    MPI_Send(&val, 1, MPI_INT, 1, 0, MPI_COMM_WORLD);
  }

  return;

}


int main(int argc, char *argv[])
{
  int i, j, k;
  int is_all = 0;
  char *test_name;
  /* Init */
  MPI_Init(&argc, &argv);
  signal(SIGSEGV, SIG_DFL);
  MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);

  init_ndrand();
  start = MPI_Wtime();
  


  for (k = 0; k < argc; k++) {
    if (argc == 1) {
      is_all = 1;
      test_name = "all";
    } else {
      k++;
      test_name = argv[k];
    }


    if (!strcmp(test_name, "matching") || is_all) {
#if defined(TEST_MATCHING_FUNC)
      if (my_rank == 0) fprintf(stdout, "Start testing matching functions ... \n"); fflush(stdout);
      for (i = 0; i < sizeof(matching_ids)/ sizeof(int); i++) {
	rempi_test_mpi_send_and_nonblocking_recvs(matching_ids[i]);
	MPI_Barrier(MPI_COMM_WORLD);
      }
      if (my_rank == 0) fprintf(stdout, "Done\n"); fflush(stdout);
#endif
    } 

    if (!strcmp(test_name, "probe")  || is_all) {
#if defined(TEST_PROBE_FUNC)
      if (my_rank == 0) fprintf(stdout, "Start testing probe functions ... \n"); fflush(stdout);
      for (i = 0; i < sizeof(probe_ids) / sizeof(int); i++) {
	rempi_test_probe(probe_ids[i]);
	MPI_Barrier(MPI_COMM_WORLD);
      }
      if (my_rank == 0) fprintf(stdout, "Done\n"); fflush(stdout);
#endif
    } 

    if (!strcmp(test_name, "isend") || is_all) {
#if defined(TEST_ISEND_FUNC)
      if (my_rank == 0) fprintf(stdout, "Start testing isend functions ... \n"); fflush(stdout);
      for (i = 0; i < sizeof(isend_ids) / sizeof(int); i++) {
	rempi_test_nonblocking_sends(isend_ids[i]);
	MPI_Barrier(MPI_COMM_WORLD);
      }
      if (my_rank == 0) fprintf(stdout, "Done\n"); fflush(stdout);
#endif
    } 

    if (!strcmp(test_name, "init_sendrecv") || is_all) {
#if defined(TEST_INIT_SENDRECV_FUNC)
      if (my_rank == 0) fprintf(stdout, "Start testing init send/recv functions ... \n"); fflush(stdout);
      for (i = 0; i < sizeof(send_init_ids)/sizeof(int); i++) {
	rempi_test_init_sendrecv(send_init_ids[i], MPI_Start_id, MPI_Start_id);
	MPI_Barrier(MPI_COMM_WORLD);
      }
      if (my_rank == 0) fprintf(stdout, "Done\n"); fflush(stdout);
#endif
    } 

    if (!strcmp(test_name, "start") || is_all) {
#if defined(TEST_START_FUNC)
      if (my_rank == 0) fprintf(stdout, "Start testing start/startall functions ... \n"); fflush(stdout);
      for (i = 0; i < sizeof(start_ids)/sizeof(int); i++) {
	for (j = 0; j < sizeof(start_ids)/sizeof(int); j++) {
	  rempi_test_init_sendrecv(MPI_Send_init_id, start_ids[i], start_ids[j]);
	  MPI_Barrier(MPI_COMM_WORLD);
	}
      }
      if (my_rank == 0) fprintf(stdout, "Done\n"); fflush(stdout);
#endif
    } 

    if (!strcmp(test_name, "null_status") || is_all) {

#if defined(TEST_NULL_STATUS)
      if (my_rank == 0) fprintf(stdout, "Start testing null status ... \n"); fflush(stdout);
      for (i = 0; i < sizeof(status_ids)/sizeof(int); i++) {
	rempi_test_null_status(status_ids[i]);
	MPI_Barrier(MPI_COMM_WORLD);
      }
      if (my_rank == 0) fprintf(stdout, "Done\n"); fflush(stdout);
#endif
    } 


    if (!strcmp(test_name, "sendrecv_req") || is_all) {
#if defined(TEST_SENDRECV_REQ)
      if (my_rank == 0) fprintf(stdout, "Start testing sendrecv req ... \n"); fflush(stdout);
      for (i = 0; i < sizeof(sendrecv_req_ids)/sizeof(int); i++) {
	rempi_test_sendrecv_req(sendrecv_req_ids[i]);
	MPI_Barrier(MPI_COMM_WORLD);
      }
      if (my_rank == 0) fprintf(stdout, "Done\n"); fflush(stdout);
#endif
    }

    if (!strcmp(test_name, "comm_dup") || is_all) {
#if defined(TEST_COMM_DUP)
      if (my_rank == 0) fprintf(stdout, "Start testing comm dup ... \n"); fflush(stdout);
      rempi_test_comm_dup();
      MPI_Barrier(MPI_COMM_WORLD);
      if (my_rank == 0) fprintf(stdout, "Done\n"); fflush(stdout);
#endif
    }

    if (!strcmp(test_name, "request_null") || is_all) {
#if defined(TEST_COMM_DUP)
      if (my_rank == 0) fprintf(stdout, "Start testing request null ... \n"); fflush(stdout);
      for (i = 0; i < sizeof(request_null_ids)/sizeof(int); i++) {
	rempi_test_request_null(request_null_ids[i]);
	MPI_Barrier(MPI_COMM_WORLD);
      }
      if (my_rank == 0) fprintf(stdout, "Done\n"); fflush(stdout);
#endif
    }

    if (!strcmp(test_name, "zero_incount") || is_all) {
#if defined(TEST_COMM_DUP)
      if (my_rank == 0) fprintf(stdout, "Start testing zero incount ... \n"); fflush(stdout);
      for (i = 0; i < sizeof(zero_incount_ids)/sizeof(int); i++) {
	rempi_test_zero_incount(zero_incount_ids[i]);
	MPI_Barrier(MPI_COMM_WORLD);
      }
      if (my_rank == 0) fprintf(stdout, "Done\n"); fflush(stdout);
#endif
    }

    if (!strcmp(test_name, "late_irecv") || is_all) {
#if defined(TEST_COMM_DUP)
      if (my_rank == 0) fprintf(stdout, "Start testing late irecv ... \n"); fflush(stdout);
      rempi_test_late_irecv();
      MPI_Barrier(MPI_COMM_WORLD);
      if (my_rank == 0) fprintf(stdout, "Done\n"); fflush(stdout);
#endif
    }

    if (!strcmp(test_name, "clock_wait") || is_all) {
#if defined(TEST_COMM_DUP)
      if (my_rank == 0) fprintf(stdout, "Start testing clock wait ... \n"); fflush(stdout);
      rempi_test_clock_wait();
      MPI_Barrier(MPI_COMM_WORLD);
      if (my_rank == 0) fprintf(stdout, "Done\n"); fflush(stdout);
#endif
    }

  }



  end = MPI_Wtime();

  MPI_Finalize();
  if (my_rank == 0) {
    rempi_test_dbg_print("Time to complete tests: %f", end - start);
  }
  return 0;
}
