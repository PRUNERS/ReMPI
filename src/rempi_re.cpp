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

int rempi_re::get_test_id()
{
  return 0; /*rempi_re does not distingish different test/testsome from different call stacks */
}


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


int rempi_re::re_finalize()
{
  int ret;
  ret = PMPI_Finalize();
  return ret;
}



