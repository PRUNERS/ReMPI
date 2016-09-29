
#include <mpi.h>

#include "rempi_config.h"
#include "rempi_err.h"

int rempi_mpi_mf(int incount,
		 MPI_Request array_of_requests[],
		 int *outcount,
		 int array_of_indices[],
		 MPI_Status array_of_statuses[],
		 int matching_function_type)
{
  switch(matching_function_type) {
  case REMPI_MPI_TEST: 
    return PMPI_Test(array_of_requests, outcount, array_of_statuses);
    break;
  case REMPI_MPI_TESTANY:
    //    int i;
    // for (i = 0; i < incount; i++) {
    //   REMPI_DBGI(3, "incount: %d, requests: %p, indices: %p, statuses: %p", 
    // 		 incount, array_of_requests[i], *array_of_indices, *array_of_statuses);
    //    }
    return PMPI_Testany(incount, array_of_requests, array_of_indices, outcount, array_of_statuses);
    break;
  case REMPI_MPI_TESTSOME:
    return PMPI_Testsome(incount, array_of_requests, outcount, array_of_indices, array_of_statuses);
    break;
  case REMPI_MPI_TESTALL: 
    return PMPI_Testall(incount, array_of_requests, outcount, array_of_statuses);
    break;
  case REMPI_MPI_WAIT:
    return PMPI_Wait(array_of_requests, array_of_statuses);
    break;
  case REMPI_MPI_WAITANY:
    return PMPI_Waitany(incount, array_of_requests, array_of_indices, array_of_statuses);
    break;
  case REMPI_MPI_WAITSOME:
    return PMPI_Waitsome(incount, array_of_requests, outcount, array_of_indices, array_of_statuses);
    break;
  case REMPI_MPI_WAITALL:
    return PMPI_Waitall(incount, array_of_requests, array_of_statuses);
    break;
  default:
    REMPI_ERR("No such MF: %d", matching_function_type);
  }
  return MPI_ERR_UNKNOWN;
}


int rempi_mpi_pf(int source,
		 int tag,
		 MPI_Comm comm,
		 int *flag,
		 MPI_Status *status,
		 int probe_function_type)
{
  int ret;
  switch(probe_function_type) {
  case REMPI_MPI_PROBE:
    ret = PMPI_Probe(source, tag, comm, status);
    break;
  case REMPI_MPI_IPROBE:
    ret = PMPI_Iprobe(source, tag, comm, flag, status);
    break;
  default:
    REMPI_ERR("No such PF: %d", probe_function_type);
  }
  return ret; 
}
