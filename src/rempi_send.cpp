
#include "rempi_send.h"
#include "rempi_err.h"
#include "rempi_config.h"
#include "rempi_type.h"

#define REMPI_SIsend (0)
#define REMPI_Ibsend (1)
#define REMPI_Irsend (2)
#define REMPI_Issend (3)


int rempi_ixsend(mpi_const void *buf,
	       int count,
	       MPI_Datatype datatype,
	       int dest,
	       int tag,
	       MPI_Comm comm,
	       MPI_Request *request,
		 int mpi_send_type) {
  int ret;
  switch(mpi_send_type) {
  case REMPI_SEND_TYPE_ISEND:
    ret = PMPI_Isend(buf, count, datatype, dest, tag, comm, request);
    break;
  case REMPI_SEND_TYPE_IBSEND:
    ret = PMPI_Ibsend(buf, count, datatype, dest, tag, comm, request);
    break;
  case REMPI_SEND_TYPE_IRSEND:
    ret = PMPI_Irsend(buf, count, datatype, dest, tag, comm, request);
    break;
  case REMPI_SEND_TYPE_ISSEND:
    ret = PMPI_Issend(buf, count, datatype, dest, tag, comm, request);
    break;
  default:
    REMPI_ERR("No such send type: %d", mpi_send_type);
    break;
  }
  return ret;
}


