#ifndef __REMPI_SEND_H__
#define __REMPI_SEND_H__

#include <mpi.h>

#include "rempi_type.h"

#define REMPI_SEND_TYPE_ISEND (0)
#define REMPI_SEND_TYPE_IBSEND (1)
#define REMPI_SEND_TYPE_IRSEND (2)
#define REMPI_SEND_TYPE_ISSEND (3)


int rempi_ixsend(mpi_const void *buf,
	       int count,
	       MPI_Datatype datatype,
	       int dest,
	       int tag,
	       MPI_Comm comm,
	       MPI_Request *request,
	       int mpi_send_type);


#endif
