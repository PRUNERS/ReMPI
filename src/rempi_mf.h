#ifndef __REMPI_MF_H__
#define __REMPI_MF_H__

#include "rempi_type.h"

int rempi_mpi_isend(mpi_const void *buf,
		    int count,
		    MPI_Datatype datatype,
		    int dest,
		    int tag,
		    MPI_Comm comm,
		    MPI_Request *request,
		    int send_function_type);

int rempi_mpi_mf(int incount,
	     MPI_Request array_of_requests[],
	     int *outcount,
	     int array_of_indices[],
	     MPI_Status array_of_statuses[],
	     int matching_function_type);

int rempi_mpi_get_matched_count(int incount, int *outcount, int nullcount, int matching_function_type);

int rempi_mpi_pf(int source,
             int tag,
             MPI_Comm comm,
             int *flag,
             MPI_Status *status,
             int prove_function_type);

#endif
