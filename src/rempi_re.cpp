#include <stdlib.h>
#include <string.h>

#include <mpi.h>

#include "rempi_re.h"
#include "rempi_err.h"
#include "rempi_mem.h"
#include "rempi_config.h"
#include "rempi_recorder.h"

using namespace std;


int rempi_re::init_after_pmpi_init(int *argc, char ***argv)
{
  char comm_id[REMPI_COMM_ID_LENGTH];
  comm_id[0] = 0;
  PMPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
  PMPI_Comm_set_name(MPI_COMM_WORLD, comm_id);

  rempi_err_init(my_rank);
  rempi_set_configuration(argc, argv);

  return 0;
}

#ifdef REVERT1
int rempi_re::get_test_id()
{
  return 0; /*rempi_re does not distingish different test/testsome from different call stacks */
}
#else
int rempi_re::get_test_id(MPI_Request *requests)
{
  return 0; /*rempi_re does not distingish different test/testsome from different call stacks */
}
#endif


int rempi_re::re_init(int *argc, char ***argv)
{
  int ret;
  ret = PMPI_Init(argc, argv);
  return ret;
}

int rempi_re::re_init_thread(
			     int *argc, char ***argv,
			     int required, int *provided)
{
  int ret;
  ret = PMPI_Init_thread(argc, argv, required, provided);
  return ret;
}

int rempi_re::re_isend(
		     void *buf,
		     int count,
		     MPI_Datatype datatype,
		     int dest,
		     int tag,
		     MPI_Comm comm,
		     MPI_Request *request)
{
  int ret;
  ret = PMPI_Isend(buf, count, datatype, dest, tag, comm, request);
  return ret;
}

int rempi_re::re_irecv(
		 void *buf,
		 int count,
		 MPI_Datatype datatype,
		 int source,
		 int tag,
		 MPI_Comm comm,
		 MPI_Request *request)
{
  int ret;
  ret = PMPI_Irecv(buf, count, datatype, source, tag, comm, request);
  return ret;
}

int rempi_re::re_cancel(MPI_Request *request)
{
  int ret;
  ret = PMPI_Cancel(request);
  return ret;
}
  
int rempi_re::re_test(
		    MPI_Request *request, 
		    int *flag, 
		    MPI_Status *status)
{
  int ret;
  ret = PMPI_Test(request, flag, status);
  return ret;
}
  
int rempi_re::re_testsome(
			  int incount, 
			  MPI_Request array_of_requests[],
			  int *outcount, 
			  int array_of_indices[], 
			  MPI_Status array_of_statuses[])
{
  int ret;
  ret = PMPI_Testsome(incount, array_of_requests, outcount, array_of_indices, array_of_statuses);
  return ret;
}

int rempi_re::re_waitall(
			  int incount, 
			  MPI_Request array_of_requests[],
			  MPI_Status array_of_statuses[])
{
  int ret;
  ret = PMPI_Waitall(incount, array_of_requests, array_of_statuses);
  return ret;
}


int rempi_re::re_comm_split(MPI_Comm arg_0, int arg_1, int arg_2, MPI_Comm *arg_3)
{
  int ret;
  ret = PMPI_Comm_split(arg_0, arg_1, arg_2, arg_3);
  return ret;
}

int rempi_re::re_comm_create(MPI_Comm arg_0, MPI_Group arg_1, MPI_Comm *arg_2)
{
  int ret;
  ret = PMPI_Comm_create(arg_0, arg_1, arg_2);
  return ret;
}

int rempi_re::re_comm_dup(MPI_Comm arg_0, MPI_Comm *arg_2)
{
  int ret;
  ret = PMPI_Comm_dup(arg_0, arg_2);
  return ret;
}


int rempi_re::re_allreduce(const void *arg_0, void *arg_1, int arg_2, MPI_Datatype arg_3, MPI_Op arg_4, MPI_Comm arg_5) {
  return PMPI_Allreduce(arg_0, arg_1, arg_2, arg_3, arg_4, arg_5);
}

int rempi_re::re_reduce(const void *arg_0, void *arg_1, int arg_2, MPI_Datatype arg_3, MPI_Op arg_4, int arg_5, MPI_Comm arg_6)
{
  return PMPI_Reduce(arg_0, arg_1, arg_2, arg_3, arg_4, arg_5, arg_6);
}

int rempi_re::re_scan(const void *arg_0, void *arg_1, int arg_2, MPI_Datatype arg_3, MPI_Op arg_4, MPI_Comm arg_5)
{
  return PMPI_Scan(arg_0, arg_1, arg_2, arg_3, arg_4, arg_5);
}

int rempi_re::re_allgather(const void *arg_0, int arg_1, MPI_Datatype arg_2, void *arg_3, int arg_4, MPI_Datatype arg_5, MPI_Comm arg_6)
{
  return PMPI_Allgather(arg_0, arg_1, arg_2, arg_3, arg_4, arg_5, arg_6);
}

int rempi_re::re_gatherv(const void *arg_0, int arg_1, MPI_Datatype arg_2, void *arg_3, const int *arg_4, const int *arg_5, MPI_Datatype arg_6, int arg_7, MPI_Comm arg_8)
{
  return PMPI_Gatherv(arg_0, arg_1, arg_2, arg_3, arg_4, arg_5, arg_6, arg_7, arg_8);
}

int rempi_re::re_reduce_scatter(const void *arg_0, void *arg_1, const int *arg_2, MPI_Datatype arg_3, MPI_Op arg_4, MPI_Comm arg_5)
{
  return PMPI_Reduce_scatter(arg_0, arg_1, arg_2, arg_3, arg_4, arg_5);
}

int rempi_re::re_scatterv(const void *arg_0, const int *arg_1, const int *arg_2, MPI_Datatype arg_3, void *arg_4, int arg_5, MPI_Datatype arg_6, int arg_7, MPI_Comm arg_8)
{
  return PMPI_Scatterv(arg_0, arg_1, arg_2, arg_3, arg_4, arg_5, arg_6, arg_7, arg_8);
}

int rempi_re::re_allgatherv(const void *arg_0, int arg_1, MPI_Datatype arg_2, void *arg_3, const int *arg_4, const int *arg_5, MPI_Datatype arg_6, MPI_Comm arg_7)
{
  return PMPI_Allgatherv(arg_0, arg_1, arg_2, arg_3, arg_4, arg_5, arg_6, arg_7);
}

int rempi_re::re_scatter(const void *arg_0, int arg_1, MPI_Datatype arg_2, void *arg_3, int arg_4, MPI_Datatype arg_5, int arg_6, MPI_Comm arg_7)
{
  return PMPI_Scatter(arg_0, arg_1, arg_2, arg_3, arg_4, arg_5, arg_6, arg_7);
}

int rempi_re::re_bcast(void *arg_0, int arg_1, MPI_Datatype arg_2, int arg_3, MPI_Comm arg_4)
{
  return PMPI_Bcast(arg_0, arg_1, arg_2, arg_3, arg_4);
}

int rempi_re::re_alltoall(const void *arg_0, int arg_1, MPI_Datatype arg_2, void *arg_3, int arg_4, MPI_Datatype arg_5, MPI_Comm arg_6)
{
  return PMPI_Alltoall(arg_0, arg_1, arg_2, arg_3, arg_4, arg_5, arg_6);
}

int rempi_re::re_gather(const void *arg_0, int arg_1, MPI_Datatype arg_2, void *arg_3, int arg_4, MPI_Datatype arg_5, int arg_6, MPI_Comm arg_7)
{
  return PMPI_Gather(arg_0, arg_1, arg_2, arg_3, arg_4, arg_5, arg_6, arg_7);
}

int rempi_re::re_barrier(MPI_Comm arg_0)
{
  return PMPI_Barrier(arg_0);
}




int rempi_re::re_finalize()
{
  int ret;
  ret = PMPI_Finalize();
  return ret;
}



