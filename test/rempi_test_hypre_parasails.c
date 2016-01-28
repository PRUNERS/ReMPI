#include <stdio.h>
#include <mpi.h>
#include <sys/time.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "rempi_test_util.h"
#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <string.h>

typedef int HYPRE_Int;
typedef double HYPRE_Real;

#define HYPRE_MPI_INT  MPI_INT
#define hypre_MPI_DOUBLE  MPI_DOUBLE
#define hypre_MPI_SUM MPI_SUM
#define hypre_MPI_ANY_SOURCE MPI_ANY_SOURCE
#define hypre_MPI_SOURCE MPI_SOURCE
#define hypre_MPI_Request MPI_Request
#define hypre_MPI_Status MPI_Status
#define hypre_MPI_Probe MPI_Probe
#define hypre_MPI_Wtime MPI_Wtime
#define hypre_MPI_Allreduce MPI_Allreduce
#define hypre_MPI_Allgather MPI_Allgather
#define hypre_MPI_Comm_rank MPI_Comm_rank
#define hypre_MPI_Comm_size MPI_Comm_size
#define hypre_MPI_Isend MPI_Isend
#define hypre_MPI_Request_free MPI_Request_free
#define hypre_MPI_Get_count MPI_Get_count
#define hypre_MPI_Recv MPI_Recv
#define hypre_MPI_Waitall MPI_Waitall


#define ROW_REQ_TAG        222
#define ROW_REPI_TAG       223
#define ROW_REPV_TAG       224

#define TEST_COMM_SIZE (64)
#define TEST_MAX_ROWS (32)

/* HYPRE_Int dest_list_pruned[][TEST_MAX_ROWS] = { */
/*   {0, -1}, */
/*   {0, -1}, */
/*   {0, -1}, */
/*   {0, -1}, */
/*   {0, -1}, */
/*   {0, -1}, */
/*   {0, -1}, */
/*   {0, -1}, */
/*   {0, -1}, */
/*   {0, -1}, */
/*   {0, -1}, */
/*   {0, -1}, */
/*   {0, -1}, */
/*   {0, -1}, */
/*   {0, -1}, */
/*   {0, -1}, */
/*   {0, -1}, */
/*   {0, -1}, */
/*   {0, -1}, */
/*   {0, -1}, */
/*   {0, -1}, */
/*   {0, -1}, */
/*   {0, -1}, */
/*   {0, -1}, */
/*   {0, -1}, */
/*   {0, -1}, */
/*   {0, -1}, */
/*   {0, -1}, */
/*   {0, -1}, */
/*   {0, -1}, */
/*   {0, -1}, */
/*   {0, -1}, */
/*   {0, -1}, */
/*   {0, -1}, */
/*   {0, -1}, */
/*   {0, -1}, */
/*   {0, -1}, */
/*   {0, -1}, */
/*   {0, -1}, */
/*   {0, -1}, */
/*   {0, -1}, */
/*   {0, -1}, */
/*   {0, -1}, */
/*   {0, -1}, */
/*   {0, -1}, */
/*   {0, -1}, */
/*   {0, -1}, */
/*   {0, -1}, */
/*   {0, -1}, */
/*   {0, -1}, */
/*   {0, -1}, */
/*   {0, -1}, */
/*   {0, -1}, */
/*   {0, -1}, */
/*   {0, -1}, */
/*   {0, -1}, */
/*   {0, -1}, */
/*   {0, -1}, */
/*   {0, -1}, */
/*   {0, -1}, */
/*   {0, -1}, */
/*   {0, -1}, */
/*   {0, -1}, */
/*   {0, -1} */
/* }; */

HYPRE_Int dest_list_pruned[][TEST_MAX_ROWS] = {
  {1, 4, 7, 8, 10, 12, 13, 14, 16, 17, 20, 21, 23, -1},
  {0, 3, 4, 5, 7, 10, 12, 13, -1},
  {-1},
  {1, 4, 13, 14, 32, 43, -1},
  {0, 1, 3, 5, 6, 7, 12, 13, 17, 18, 19, 20, 21, 22, 23, -1},
  {1, 4, 6, 7, 12, 13, 17, 18, 20, 28, 29, -1},
  {4, 5, 7, 18, 28, 29, -1},
  {0, 1, 4, 5, 6, 17, 18, 19, 20, 21, 22, 23, 28, 29, -1},
  {0, 10, 17, -1},
  {-1},
  {0, 1, 8, 13, 14, 16, 17, -1},
  {-1},
  {0, 1, 4, 5, 13, 14, 16, 17, 20, 28, 32, 33, 42, 43, 45, -1},
  {0, 1, 3, 4, 5, 10, 12, 14, 32, 43, -1},
  {0, 3, 10, 12, 13, 16, 17, 20, 32, 33, 42, 43, 45, -1},
  {-1},
  {0, 10, 12, 14, 17, 20, 30, 32, 38, 45, 50, 54, -1},
  {0, 4, 5, 7, 8, 10, 12, 14, 16, 18, 19, 20, 21, 22, 30, -1},
  {4, 5, 6, 7, 17, 19, 29, -1},
  {4, 7, 17, 18, 20, 22, -1},
  {0, 4, 5, 7, 12, 14, 16, 17, 19, 21, 22, 23, 28, 30, 31, 38, 45, -1},
  {0, 4, 7, 17, 20, 22, 23, 30, 31, -1},
  {4, 7, 17, 19, 20, 21, 23, -1},
  {0, 4, 7, 20, 21, 22, -1},
  {25, 26, 27, -1},
  {24, 26, 27, 28, 29, 44, 47, -1},
  {24, 25, 27, 28, 29, 30, 31, 56, 57, 58, -1},
  {24, 25, 26, 28, 31, -1},
  {5, 6, 7, 12, 20, 25, 26, 27, 29, 30, 31, 44, 45, 50, 51, 56, 58, -1},
  {5, 6, 7, 18, 25, 26, 28, 36, 44, 45, 47, 56, 57, 58, 62, -1},
  {16, 17, 20, 21, 26, 28, 31, 38, 45, 50, 54, 56, 58, -1},
  {20, 21, 26, 27, 28, 30, -1},
  {3, 12, 13, 14, 16, 33, 40, 41, 42, 43, 45, 46, 47, 51, -1},
  {12, 14, 32, 34, 36, 38, 40, 42, 43, 44, 45, 46, 51, -1},
  {33, 36, 42, 45, 46, -1},
  {-1},
  {29, 33, 34, 38, 44, 45, 46, 47, 56, 62, -1},
  {-1},
  {16, 20, 30, 33, 36, 44, 45, 46, 50, 51, 56, 62, -1},
  {-1},
  {32, 33, 41, 42, 43, 46, 47, -1},
  {32, 40, 43, -1},
  {12, 14, 32, 33, 34, 40, 43, 45, 46, -1},
  {3, 12, 13, 14, 32, 33, 40, 41, 42, -1},
  {25, 28, 29, 33, 36, 38, 45, 46, 47, 62, -1},
  {12, 14, 16, 20, 28, 29, 30, 32, 33, 34, 36, 38, 42, 44, 46, 50, 51, 56, -1},
  {32, 33, 34, 36, 38, 40, 42, 44, 45, 47, -1},
  {25, 29, 32, 36, 40, 44, 46, -1},
  {-1},
  {-1},
  {16, 28, 30, 38, 45, 51, 54, 56, 58, 62, -1},
  {28, 32, 33, 38, 45, 50, 56, -1},
  {-1},
  {-1},
  {16, 30, 50, -1},
  {-1},
  {26, 28, 29, 30, 36, 38, 45, 50, 51, 57, 58, 62, -1},
  {26, 29, 56, 58, -1},
  {26, 28, 29, 30, 50, 56, 57, -1},
  {-1},
  {-1},
  {-1},
  {29, 36, 38, 44, 50, 56, -1},
  {-1}
};


HYPRE_Int dest_list_stored[][TEST_MAX_ROWS] = {
  {-1},
  {0, -1},
  {-1},
  {1, -1},
  {-1},
  {0, 1, 4, -1},
  {1, 4, 5, -1},
  {0, 1, 4, 5, 6, -1},
  {0, -1},
  {-1},
  {0, 1, 8, -1},
  {-1},
  {0, 1, 4, 5, -1},
  {0, 1, 3, 4, 5, 10, 12, -1},
  {0, 1, 3, 10, 12, 13, -1},
  {-1},
  {0, 10, 12, 13, 14, -1},
  {0, 1, 4, 5, 7, 8, 10, 12, 14, 16, -1},
  {4, 5, 6, 7, 17, -1},
  {4, 7, 17, 18, -1},
  {0, 1, 4, 5, 7, 12, 13, 14, 16, 17, 19, -1},
  {0, 4, 7, 17, 20, -1},
  {0, 4, 7, 17, 19, 20, 21, -1},
  {0, 4, 7, 17, 20, 21, 22, -1},
  {-1},
  {24, -1},
  {21, 24, 25, -1},
  {24, 25, 26, -1},
  {1, 4, 5, 6, 7, 12, 20, 21, 25, 26, 27, -1},
  {5, 6, 7, 12, 18, 25, 26, 28, -1},
  {0, 14, 16, 17, 20, 21, 26, 28, -1},
  {20, 21, 26, 27, 28, 30, -1},
  {3, 12, 13, 14, 16, -1},
  {12, 13, 14, 16, 32, -1},
  {32, 33, -1},
  {-1},
  {25, 28, 29, 33, 34, -1},
  {-1},
  {12, 14, 16, 20, 29, 30, 33, 36, -1},
  {-1},
  {32, 33, -1},
  {32, 40, -1},
  {12, 13, 14, 32, 33, 34, 40, -1},
  {3, 12, 13, 14, 32, 33, 40, 41, 42, -1},
  {25, 28, 29, 32, 33, 34, 36, 38, 42, -1},
  {12, 13, 14, 16, 20, 26, 28, 29, 30, 32, 33, 34, 36, 38, 40, 42, 44, -1},
  {32, 33, 34, 36, 38, 40, 42, 44, 45, -1},
  {25, 29, 32, 36, 40, 44, 46, -1},
  {-1},
  {-1},
  {16, 20, 26, 28, 29, 30, 36, 38, 45, -1},
  {28, 32, 33, 38, 45, 50, -1},
  {-1},
  {-1},
  {16, 30, 50, -1},
  {-1},
  {20, 26, 28, 29, 30, 36, 38, 44, 45, 50, 51, -1},
  {26, 29, 56, -1},
  {26, 28, 29, 30, 50, 56, 57, -1},
  {-1},
  {-1},
  {-1},
  {29, 36, 38, 44, 50, 56, -1},
  {-1}
};

int my_rank;
int comm_size;

HYPRE_Int FindNumReplies(MPI_Comm comm, HYPRE_Int *replies_list)
{
    HYPRE_Int num_replies;
    HYPRE_Int npes, mype;
    HYPRE_Int *replies_list2;

    hypre_MPI_Comm_rank(comm, &mype);
    hypre_MPI_Comm_size(comm, &npes);

    replies_list2 = (HYPRE_Int *) malloc(npes * sizeof(HYPRE_Int));

    hypre_MPI_Allreduce(replies_list, replies_list2, npes, HYPRE_MPI_INT, hypre_MPI_SUM, comm);
    num_replies = replies_list2[mype];

    free(replies_list2);

    return num_replies;
}


static void SendRequests(MPI_Comm comm, HYPRE_Int reqlen, HYPRE_Int *reqind,
			 HYPRE_Int *num_requests, HYPRE_Int *replies_list, HYPRE_Int dest_list[][TEST_MAX_ROWS])
{
    hypre_MPI_Request request;
    HYPRE_Int i, j, this_pe;
    HYPRE_Int val = 0;

    *num_requests = 0;

    for (i=0; dest_list[my_rank % TEST_COMM_SIZE][i] >= 0; i++) /* j is set below */
    {
      /* The processor that owns the row with index reqind[i] */
      this_pe = dest_list[my_rank % TEST_COMM_SIZE][i];
      if (this_pe >= comm_size) continue;
      /* Request rows in reqind[i..j-1] */
      
      //      if (my_rank == 63) sleep(1);

      hypre_MPI_Isend(&val, 1, HYPRE_MPI_INT, this_pe, ROW_REQ_TAG,
		      comm, &request);
      hypre_MPI_Request_free(&request);
      (*num_requests)++;
      
      if (replies_list != NULL)
	replies_list[this_pe] = 1;
    }
}


static void ReceiveRequest(MPI_Comm comm, HYPRE_Int *source, HYPRE_Int **buffer,
  HYPRE_Int *buflen, HYPRE_Int *count)
{
    hypre_MPI_Status status;

    hypre_MPI_Probe(hypre_MPI_ANY_SOURCE, ROW_REQ_TAG, comm, &status);
    *source = status.hypre_MPI_SOURCE;
    hypre_MPI_Get_count(&status, HYPRE_MPI_INT, count);

    if (*count > *buflen)
    {
        free(*buffer);
        *buflen = *count;
        *buffer = (HYPRE_Int *) malloc(*buflen * sizeof(HYPRE_Int));
    }


    hypre_MPI_Recv(*buffer, *count, HYPRE_MPI_INT, *source, ROW_REQ_TAG, comm, &status);
}


static void SendReplyPrunedRows(MPI_Comm comm,  HYPRE_Int dest, 
				HYPRE_Int *buffer, HYPRE_Int count, hypre_MPI_Request *request)
{
    HYPRE_Int sendbacksize, j;
    HYPRE_Int len, *ind, *indbuf, *indbufp;
    HYPRE_Int temp;
    HYPRE_Int val = 0;

    hypre_MPI_Isend(&val, 1, HYPRE_MPI_INT, dest, ROW_REPI_TAG, comm, request);
}


static void ReceiveReplyPrunedRows(MPI_Comm comm)
{
    hypre_MPI_Status status;
    HYPRE_Int source, count;
    HYPRE_Int len, *ind, num_rows, *row_nums, j;
    HYPRE_Int val;

    /* Don't know the size of reply, so use probe and get count */
    hypre_MPI_Probe(hypre_MPI_ANY_SOURCE, ROW_REPI_TAG, comm, &status);
    source = status.hypre_MPI_SOURCE;
    hypre_MPI_Get_count(&status, HYPRE_MPI_INT, &count);
    hypre_MPI_Recv(&val, 1, HYPRE_MPI_INT, source, ROW_REPI_TAG, comm, &status);
}


static void SendReplyStoredRows(MPI_Comm comm, HYPRE_Int dest, 
				HYPRE_Int *buffer, HYPRE_Int count, hypre_MPI_Request *request)
{
    HYPRE_Int val;

    hypre_MPI_Isend(&val, 1, HYPRE_MPI_INT, dest, ROW_REPI_TAG,
        comm, request);

    hypre_MPI_Request_free(request);

    hypre_MPI_Isend(&val, 1, HYPRE_MPI_INT, dest, ROW_REPV_TAG,
        comm, request);
}


static void ReceiveReplyStoredRows(MPI_Comm comm)
{
    hypre_MPI_Status status;
    HYPRE_Int source, count;
    HYPRE_Int val;

    /* Don't know the size of reply, so use probe and get count */
    hypre_MPI_Probe(hypre_MPI_ANY_SOURCE, ROW_REPI_TAG, comm, &status);
    source = status.hypre_MPI_SOURCE;
    hypre_MPI_Get_count(&status, HYPRE_MPI_INT, &count);

    /* Allocate space in stored rows data structure */
    hypre_MPI_Recv(&val, 1, HYPRE_MPI_INT, source, ROW_REPI_TAG, comm, &status);
    hypre_MPI_Recv(&val, 1, HYPRE_MPI_INT, source, ROW_REPV_TAG, comm, &status);
}


static void ExchangePrunedRows(MPI_Comm comm, HYPRE_Int num_levels)
{
    HYPRE_Int row, len, *ind;

    HYPRE_Int num_requests;
    HYPRE_Int source;

    HYPRE_Int bufferlen;
    HYPRE_Int *buffer;

    HYPRE_Int level;

    HYPRE_Int i;
    HYPRE_Int count;
    hypre_MPI_Request *requests;
    hypre_MPI_Status *statuses;
    HYPRE_Int npes;
    HYPRE_Int num_replies, *replies_list;


    hypre_MPI_Comm_size(comm, &npes);
    requests = (hypre_MPI_Request *) malloc(npes * sizeof(hypre_MPI_Request));
    statuses = (hypre_MPI_Status *) malloc(npes * sizeof(hypre_MPI_Status));

    /* Loop to construct pattern of pruned rows on this processor */
    bufferlen = 10; /* size will grow if get a long msg */
    buffer = (HYPRE_Int *) malloc(bufferlen * sizeof(HYPRE_Int));

    for (level=1; level<=num_levels; level++)
    {

        replies_list = (HYPRE_Int *) calloc(npes, sizeof(HYPRE_Int));

        SendRequests(comm, len, ind, &num_requests, replies_list, dest_list_pruned);


        num_replies = FindNumReplies(comm, replies_list);
        free(replies_list);

        for (i=0; i<num_replies; i++)
        {
            /* Receive count indices stored in buffer */
            ReceiveRequest(comm, &source, &buffer, &bufferlen, &count);
	    //    rempi_test_dbg_print("source: %d", source);
            SendReplyPrunedRows(comm, source, buffer, count, &requests[i]);
                
        }

        for (i=0; i<num_requests; i++)
        {
            /* Will also merge the pattern of received rows into "patt" */
	  ReceiveReplyPrunedRows(comm);
        }

        hypre_MPI_Waitall(num_replies, requests, statuses);
    }

    free(buffer);
    free(requests);
    free(statuses);
}


static void ExchangeStoredRows(MPI_Comm comm)
{
    HYPRE_Int row, len, *ind;
    HYPRE_Real *val;

    HYPRE_Int num_requests;
    HYPRE_Int source;

    HYPRE_Int bufferlen;
    HYPRE_Int *buffer;

    HYPRE_Int i;
    HYPRE_Int count;
    hypre_MPI_Request *requests = NULL;
    hypre_MPI_Status *statuses = NULL;
    HYPRE_Int npes;
    HYPRE_Int num_replies, *replies_list;

    hypre_MPI_Comm_size(comm, &npes);

    replies_list = (HYPRE_Int *) calloc(npes, sizeof(HYPRE_Int));

    SendRequests(comm, len, ind, &num_requests, replies_list, dest_list_stored);

    num_replies = FindNumReplies(comm, replies_list);
    free(replies_list);

    if (num_replies)
    {
        requests = (hypre_MPI_Request *) malloc(num_replies * sizeof(hypre_MPI_Request));
        statuses = (hypre_MPI_Status *) malloc(num_replies * sizeof(hypre_MPI_Status));
    }

    bufferlen = 10; /* size will grow if get a long msg */
    buffer = (HYPRE_Int *) malloc(bufferlen * sizeof(HYPRE_Int));

    for (i=0; i<num_replies; i++)
    {
        /* Receive count indices stored in buffer */
        ReceiveRequest(comm, &source, &buffer, &bufferlen, &count);
        SendReplyStoredRows(comm, source, buffer, count, &requests[i]);
    }

    for (i=0; i<num_requests; i++)
    {
      ReceiveReplyStoredRows(comm);
    }

    hypre_MPI_Waitall(num_replies, requests, statuses);

    /* Free all send buffers */
    free(buffer);
    free(requests);
    free(statuses);
}


static HYPRE_Real SelectThresh(MPI_Comm comm, HYPRE_Real param)
{
    HYPRE_Int row, len, *ind, i, npes;
    HYPRE_Real *val;
    HYPRE_Real localsum = 0.0, sum;
    HYPRE_Real temp;

    /* Find the average across all processors */
    hypre_MPI_Allreduce(&localsum, &sum, 1, hypre_MPI_DOUBLE, hypre_MPI_SUM, comm);
    hypre_MPI_Comm_size(comm, &npes);

    return sum;
}

static HYPRE_Real SelectFilter(MPI_Comm comm, HYPRE_Real param, HYPRE_Int symmetric)
{
    HYPRE_Int row, len, *ind, i, npes;
    HYPRE_Real *val;
    HYPRE_Real localsum = 0.0, sum;

    /* Find the average across all processors */
    hypre_MPI_Allreduce(&localsum, &sum, 1, hypre_MPI_DOUBLE, hypre_MPI_SUM, comm);
    hypre_MPI_Comm_size(comm, &npes);

    return sum;
}


void ParaSailsCreate(MPI_Comm comm, HYPRE_Int beg_row, HYPRE_Int end_row, HYPRE_Int sym)
{
  HYPRE_Int npes;
  HYPRE_Int *beg_rows;
  HYPRE_Int *end_rows;

  hypre_MPI_Comm_size(comm, &npes);
  
  beg_rows = (HYPRE_Int *) malloc(npes * sizeof(HYPRE_Int));
  end_rows = (HYPRE_Int *) malloc(npes * sizeof(HYPRE_Int));
  
  hypre_MPI_Allgather(&beg_row, 1, HYPRE_MPI_INT, beg_rows, 1, HYPRE_MPI_INT, comm);
  hypre_MPI_Allgather(&end_row, 1, HYPRE_MPI_INT, end_rows, 1, HYPRE_MPI_INT, comm);
  
  return;
}

void ParaSailsDestroy()
{
  return;
}

void ParaSailsSetupPattern(HYPRE_Real thresh, HYPRE_Int num_levels)
{
  HYPRE_Real time0, time1, time;

  time0 = hypre_MPI_Wtime();
  SelectThresh(MPI_COMM_WORLD, 0);
  ExchangePrunedRows(MPI_COMM_WORLD, num_levels);
  time1 = hypre_MPI_Wtime();
  time = time1 - time0;
  return;
}


HYPRE_Int ParaSailsSetupValues(HYPRE_Real filter)
{
    HYPRE_Int row, len, *ind;
    HYPRE_Real *val;
    HYPRE_Int i;
    HYPRE_Real time0, time1, time;
    MPI_Comm comm = MPI_COMM_WORLD;
    HYPRE_Int error = 0, error_sum;

    time0 = hypre_MPI_Wtime();


    ExchangeStoredRows(comm);

    time1 = hypre_MPI_Wtime();
    time  = time1 - time0;

    /* check if there was an error in computing the approximate inverse */
    hypre_MPI_Allreduce(&error, &error_sum, 1, HYPRE_MPI_INT, hypre_MPI_SUM, comm);
    SelectFilter(comm, 0, 0);

    return 0;
}


int main(int argc, char* argv[])
{

  MPI_Init(&argc, &argv);
  signal(SIGSEGV, SIG_DFL);
  hypre_MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
  hypre_MPI_Comm_size(MPI_COMM_WORLD, &comm_size);

  int pid = getpid();
  char path[32];
  int fd;
  /* //  if (my_rank == 0) { */
  /*   sprintf(path, "parasails-%d.log", pid); */
  /*   fd = open(path, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR); */
  /*   //  } */
  
  int i;
  //  for (i = 0; ;i++) {
  int print_rank = 0;
  for (i = 0; i < 400 ;i++) {
    ParaSailsSetupPattern(0, 1);
    ParaSailsSetupValues(0);
    if (i % 200 == 0) {
      if (my_rank == print_rank++ % comm_size) {
	//	dprintf(fd, "loop %d\n", i);
	//	write(fd, "loop\n", 5);
    	rempi_test_dbg_print("loop %d", i);
      }
    }
  }
  //  if (my_rank == 0) {
  //    close(fd);
    //  }

  MPI_Finalize();
  return 0;
}
