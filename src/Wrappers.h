/*
 * Wrappers.h
 *
 *  Created on: Oct 15, 2014
 *      Author: Ignacio Laguna
 *     Contact: ilaguna@llnl.gov
 */

#ifndef WRAPPERS_H_
#define WRAPPERS_H_

#include <mpi.h>
#include <stdio.h>

int MPI_Irecv(void *buf, int count, MPI_Datatype datatype, int source,
              int tag, MPI_Comm comm, MPI_Request *request)
{
	printf("Inside MPI_Irecv");
	fflush(stdout);
	int ret = PMPI_Irecv(buf, count, datatype, source, tag, comm, request);
	return ret;
}

int MPI_Test(MPI_Request *request, int *flag, MPI_Status *status)
{
	printf("Inside MPI_Request");
	fflush(stdout);
	int ret = PMPI_Test(request, flag, status);
	return ret;
}



#endif /* WRAPPERS_H_ */
