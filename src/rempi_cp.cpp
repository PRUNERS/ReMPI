#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h> 
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <unordered_map>

#include "mpi.h"
#include "rempi_cp.h"
#include "rempi_err.h"
#include "rempi_config.h"
#include "rempi_mem.h"

#if !defined(REMPI_LITE)
#include "clmpi.h"


#define REMPI_RI_GATHER_TAG (1512)
#define REMPI_RI_SCATTER_TAG (1513)

using namespace std;

static int my_rank;
static int comm_size;
static MPI_Comm rempi_cp_comm;
static MPI_Win rempi_cp_win;
static int rempi_is_initialized = 0;

static struct rempi_cp_prop_clock *rempi_cp_gather_pc;
static size_t rempi_cp_scatter_clock = 0;
static struct rempi_cp_prop_clock *rempi_cp_scatter_pc;


static size_t rempi_pred_rank_count;
static int *rempi_pred_ranks, *rempi_pred_indices;
unordered_map<int, int> rempi_pred_ranks_indices_umap;
static size_t *rempi_recv_counts;

static size_t rempi_succ_rank_count;
static int *rempi_succ_ranks, *rempi_succ_indices;
unordered_map<int, int> rempi_succ_ranks_indices_umap;




double get_dtime(void)
{
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return ((double)(tv.tv_sec) + (double)(tv.tv_usec) * 0.001 * 0.001);
}

static void print_array(int length, int *values)
{
  for (int i = 0; i < length; i++) {
    fprintf(stderr, "Rank %d: val[%d]: %d\n", my_rank, i, values[i]);
  }  
}

static void print_array2(int length, int *values1, int *values2)
{
  for (int i = 0; i < length; i++) {
    fprintf(stderr, "Rank %d: val[%d] of Rank %d\n", my_rank, values1[i], values2[i]);
  }  
}

static int compare(const void *val1, const void *val2)
{
  return *(int*)val1 > *(int*)val2;
}




/*
 Remote indexing algorithm: 
    Collective algirithm for gathering indeices of arraies distributed across other ranks.
    A rank can know which index the rank should read from each remote array.

 Example:
    Rank 0 wants read remote values on rank 1, 2
    Rank 1 wants read remote values on rank 2
    Rank 2 wants read remote values on rank 0, 1, 2


  Step 1: Allreduce(SUM) to how many ranks want read my values
    Rank 0: [0, 1, 1] => [1,2,3]
    Rank 1: [0, 0, 1] => [1,2,3]
    Rank 2: [1, 1, 1] => [1,2,3]

  Step 2: Multi-MPI_Recv(any_source)/Send for ranks reading my values
    Rank 0: one recv   & two sends to rank 1,2           => rank 2   want to read
    Rank 1: two recv   & one send to rank 2              => rank 0,2   want to read
    Rank 2: three recv & three sends to rank 0, 1 and 2  => rank 0,1,2 want to read

  Step 3: Compute indices
    Rank 0: [X=rank2]
    Rank 1: [X=rank0, Y=rank2]
    Rank 2: [X=rank0, Y=rank1, Z=rank2]

  Step 4: Replay indices
    Rank 0: Send 0 to rank 2
    Rank 1: Send 0 to rank 0, 1 to rank 2
    Rank 2: Send 0 to rank 0, 1 to rank 1, 2 to rank 2

  Step 5: Compute remote indices

    Rank 0: Read array[0] of rank1, array[0] of rank2
    Rank 1: Read array[1] of rank2
    Rank 2: Read array[0] of rank0, array[1] of rank1 and array[2] of rank2

 Input: 
    int length (e.g., Rank0: 2)
    int input_pred_ranks (e.g., Rank0: [1,2])
 Output
    int ouput_remote_indices (e.g., Rank0: [0, 0])
*/

/*TODO: 
 COMM_WORLD => REMPI_COMM_WORLD */
static void rempi_cp_remote_indexing(int input_length, int* input_pred_ranks, int* output_pred_indices, 
				     size_t* output_succ_rank_count, int** output_succ_ranks, int** output_succ_indices)
{
  int comm_world_size;
  int *remote_rank_flags; 
  int *succ_rank_counts;
  int *succ_ranks;
  int *succ_indices;
  int succ_rank_count;
  int sum = 0;
  MPI_Request *recv_requests, *send_requests;


  PNMPIMOD_clock_control(0);


  /* Step 1 */
  PMPI_Comm_size(MPI_COMM_WORLD, &comm_world_size);
  remote_rank_flags = (int*)malloc(sizeof(int) * comm_world_size);
  succ_rank_counts = (int*)malloc(sizeof(int) * comm_world_size);
  memset(remote_rank_flags, 0, sizeof(int) * comm_world_size);
  memset(succ_rank_counts, 0, sizeof(int) * comm_world_size);

  for (int i = 0; i < input_length; i++) {
    remote_rank_flags[input_pred_ranks[i]] = 1;
  }
  PMPI_Allreduce(remote_rank_flags, succ_rank_counts, comm_world_size, MPI_INT, MPI_SUM, MPI_COMM_WORLD);


  /* Step 2 */
  succ_ranks = remote_rank_flags; /* Reusing buffer */
  succ_rank_count = succ_rank_counts[my_rank];
  send_requests = (input_length > 0)? (MPI_Request*) malloc(sizeof(MPI_Request) * input_length):NULL;
  for (int i = 0; i < input_length; i++) {
    PMPI_Isend(&my_rank, 1, MPI_INT, input_pred_ranks[i], REMPI_RI_GATHER_TAG, MPI_COMM_WORLD, &send_requests[i]);
  }
  recv_requests = (MPI_Request*) malloc(sizeof(MPI_Request) * succ_rank_count);
  for (int i = 0; i < succ_rank_count; i++) {
    PMPI_Irecv(&succ_ranks[i], 1, MPI_INT, MPI_ANY_SOURCE, REMPI_RI_GATHER_TAG, MPI_COMM_WORLD, &recv_requests[i]);
  }
  PMPI_Waitall(input_length, send_requests, MPI_STATUSES_IGNORE);
  PMPI_Waitall(succ_rank_count, recv_requests, MPI_STATUSES_IGNORE);

  /* Step 3: The indices are determied in order of source ranks, i.e., succ_ranks values. */
  qsort(succ_ranks, succ_rank_count, sizeof(int), compare);

  /* Step 4 */
  succ_indices = succ_rank_counts; /* Resuing buffer */
  for (int local_index = 0; local_index < succ_rank_count; local_index++) {
    succ_indices[local_index] = local_index;
    PMPI_Isend(&succ_indices[local_index], 1, MPI_INT, succ_ranks[local_index], REMPI_RI_SCATTER_TAG, MPI_COMM_WORLD, &recv_requests[local_index]);
  }

  for (int i = 0; i < input_length; i++) {
    PMPI_Irecv(&output_pred_indices[i], 1, MPI_INT, input_pred_ranks[i], REMPI_RI_SCATTER_TAG, MPI_COMM_WORLD, &send_requests[i]);
  }
  PMPI_Waitall(succ_rank_count, recv_requests, MPI_STATUSES_IGNORE);
  PMPI_Waitall(input_length, send_requests, MPI_STATUSES_IGNORE);


  /* Step 5: Results are already in output_pred_indices */
  //  print_array2(input_length, output_pred_indices, input_pred_ranks);

  PNMPIMOD_clock_control(1);

  /* Return results */
  *output_succ_rank_count = succ_rank_count;
  *output_succ_ranks      = succ_ranks;
  *output_succ_indices    = succ_indices;
  
  
  free(send_requests);
  free(recv_requests);

  return;
}

static void rempi_cp_init_pred_ranks(const char* path)
{
  int fd;
  size_t chunk_size = 1;
  size_t count = 1;

  fd = open(path, O_RDONLY);
  while(chunk_size != 0 && count != 0) {
    count = read(fd, &chunk_size, sizeof(size_t));
    lseek(fd, chunk_size, SEEK_CUR);
  }

  //  sleep(my_rank);
  //  REMPI_DBG("separator: %d", chunk_size);
  if (chunk_size == 0) {
    count = read(fd, &rempi_pred_rank_count, sizeof(size_t));
    //    REMPI_DBG("rank_count: %d, read_count: %lu", rempi_pred_rank_count, count);
    if (rempi_pred_rank_count > 0) {
      rempi_pred_ranks = (int*)rempi_malloc(rempi_pred_rank_count * sizeof(int));
      count = read(fd, rempi_pred_ranks, rempi_pred_rank_count * sizeof(int));
      if (count != rempi_pred_rank_count * sizeof(int)) {
	REMPI_ERR("Incocnsistent size to actual data size");
      }
    }
  }  

  // int a, b;
  // count = read(fd, &a, sizeof(int));
  // REMPI_DBG("rest size: %lu %d", count, a);
  // count = read(fd, &b, sizeof(int));
  // REMPI_DBG("rest size: %lu %d", count, b);

  close(fd);


  return;
}

//void rempi_cp_init(int input_length, int* input_pred_ranks)
void rempi_cp_init(const char* path)
{
  int ret;

  PMPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
  PMPI_Comm_size(MPI_COMM_WORLD, &comm_size);


  rempi_cp_init_pred_ranks(path);
  rempi_pred_indices    = (rempi_pred_rank_count > 0)? (int*)malloc(sizeof(int) * rempi_pred_rank_count):NULL;


  rempi_cp_remote_indexing(rempi_pred_rank_count, rempi_pred_ranks, rempi_pred_indices, 
			   &rempi_succ_rank_count, &rempi_succ_ranks, &rempi_succ_indices);

  /* == Init Window for one-sided communication for clock propagation == */
  PMPI_Comm_dup(MPI_COMM_WORLD, &rempi_cp_comm);

  ret = PMPI_Win_allocate(sizeof(struct rempi_cp_prop_clock) * rempi_succ_rank_count, sizeof(struct rempi_cp_prop_clock), MPI_INFO_NULL, rempi_cp_comm, &rempi_cp_scatter_pc, &rempi_cp_win);
  if (ret != MPI_SUCCESS) {
    fprintf(stderr, "PMPI_Win_allocate failed\n");
  }
  memset(rempi_cp_scatter_pc, 0, sizeof(struct rempi_cp_prop_clock) * rempi_succ_rank_count);
  PMPI_Win_lock_all(MPI_MODE_NOCHECK, rempi_cp_win);
  /* Init gather buffer */
  rempi_cp_gather_pc = (struct rempi_cp_prop_clock*)malloc(sizeof(struct rempi_cp_prop_clock) * rempi_pred_rank_count);
  /* Init pred_rank to index*/
  rempi_recv_counts = (size_t*)malloc(sizeof(size_t) * rempi_pred_rank_count);
  for (int i = 0; i < rempi_pred_rank_count; i++) {
    //    REMPI_DBGI(0, "rempi_pred_ranks[%d] = %d", rempi_pred_ranks[i], i);
    rempi_pred_ranks_indices_umap[rempi_pred_ranks[i]] = i;
    rempi_recv_counts[i] = 0;
  }

  for (int i = 0; i < rempi_succ_rank_count; i++) {
    rempi_succ_ranks_indices_umap[rempi_succ_ranks[i]] = i;
  }

  rempi_is_initialized = 1;


  return;
}

int rempi_cp_initialized()
{
  return rempi_is_initialized;
}

void rempi_cp_finalize()
{
  PMPI_Win_unlock_all(rempi_cp_win);
  PMPI_Win_free(&rempi_cp_win);
  return;
}

void rempi_cp_gather_clocks()
{
  int ret;
  
  for (int i = 0; i < rempi_pred_rank_count; ++i) {
    ret = PMPI_Get(&rempi_cp_gather_pc[i], sizeof(struct rempi_cp_prop_clock), MPI_BYTE, 
		       rempi_pred_ranks[i], rempi_pred_indices[i], sizeof(struct rempi_cp_prop_clock), MPI_BYTE, rempi_cp_win);
    if (ret != MPI_SUCCESS) {
      fprintf(stderr, "PMPI_Get failed\n");
    }
  }

  if (rempi_pred_rank_count > 0) {
    if ((ret = PMPI_Win_flush_local_all(rempi_cp_win)) != MPI_SUCCESS) {
      //    REMPI_DBG("PMPI_Win_flush_local_all failed");
      fprintf(stderr, "PMPI_Win_flush_local_all failed\n");
    }
  }

  // for (int i = 0; i < 2; i++) {
  //   REMPI_DBGI(0, "rempi_cp_gather_pc[%d]: %d", i, rempi_cp_gather_pc[i]);
  // }


  return;
}

int rempi_cp_has_in_flight_msgs(int source_rank)
{
  int index;
  int has;
  if (rempi_pred_ranks_indices_umap.find(source_rank) == 
      rempi_pred_ranks_indices_umap.end()) {
    REMPI_ERR("No such pred rank: %d", source_rank);
  }
  index = rempi_pred_ranks_indices_umap.at(source_rank);
  if (rempi_cp_gather_pc[index].send_count <= rempi_recv_counts[index]) {
    /* rempi_recv_counts may be incremented before updated rempi_cp_gather_pc[index].send_count 
     so I use "<=" instead of "==" */
    // REMPI_DBGI(0, "NO: Rank %d (send_count: %d) ==> Rank %d (recv_count: %d)",
    // 	      source_rank, rempi_cp_gather_pc[index].send_count, 
    // 	      my_rank, rempi_recv_counts[index]
    // 	      );
    has = 0;
  } else {
    // REMPI_DBGI(0, "YES: Rank %d (send_count: %d) ==> Rank %d (recv_count: %d)",
    // 	      source_rank, rempi_cp_gather_pc[index].send_count, 
    // 	      my_rank, rempi_recv_counts[index]
    // 	      );
    has = 1;
  }
  return has;
}

void rempi_cp_record_recv(int source_rank, size_t clock)
{
  int index;
  if (rempi_pred_ranks_indices_umap.find(source_rank) == 
      rempi_pred_ranks_indices_umap.end()) {
    REMPI_ERR("Rank %d does not exist", source_rank);
  }
  index = rempi_pred_ranks_indices_umap.at(source_rank);
  rempi_recv_counts[index]++;
  return;
}

void rempi_cp_record_send(int dest_rank, size_t clock)
{
  int index;
  if (rempi_succ_ranks_indices_umap.find(dest_rank) == 
      rempi_succ_ranks_indices_umap.end()) {

    for (unordered_map<int, int>::iterator it = rempi_succ_ranks_indices_umap.begin(),
    	   it_end = rempi_succ_ranks_indices_umap.end();
         it != it_end; it++) {
      REMPI_DBG("Succ rank: %d", it->first);
    }
    REMPI_ERR("Rank %d does not exist in Succ map: size: %lu", dest_rank, rempi_succ_ranks_indices_umap.size());
  }
  index = rempi_succ_ranks_indices_umap.at(dest_rank);
  rempi_cp_scatter_pc[index].send_count++;
  return;
}



void rempi_cp_set_scatter_clock(size_t clock)
{
  rempi_cp_scatter_clock = clock;
  for (int i = 0; i < rempi_succ_rank_count; i++) {
    rempi_cp_scatter_pc[i].clock = clock;
  }
  return;
}

size_t rempi_cp_get_scatter_clock(void)
{
  return rempi_cp_scatter_clock;
}

size_t rempi_cp_get_gather_clock(int source_rank)
{
  int index;
  if (rempi_pred_ranks_indices_umap.find(source_rank) == 
      rempi_pred_ranks_indices_umap.end()) {
    REMPI_ERR("Rank %d does not exist", source_rank);
  }
  index = rempi_pred_ranks_indices_umap.at(source_rank);
  return rempi_cp_gather_pc[index].clock;
}

size_t rempi_cp_get_gather_send_count(int source_rank)
{
  int index;
  if (rempi_pred_ranks_indices_umap.find(source_rank) == 
      rempi_pred_ranks_indices_umap.end()) {
    REMPI_ERR("Rank %d does not exist", source_rank);
  }
  index = rempi_pred_ranks_indices_umap.at(source_rank);
  return rempi_cp_gather_pc[index].send_count;
}

int *rempi_cp_get_pred_ranks(size_t *length) 
{
  *length = rempi_pred_rank_count;
  return rempi_pred_ranks;
}



#endif

