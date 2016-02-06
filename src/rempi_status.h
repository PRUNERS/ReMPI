#ifndef __REMPI_STATUS_H__
#define __REMPI_STATUS_H__

#include <mpi.h>

MPI_Status *rempi_status_allocate(int incount, MPI_Status *statuses, int *flag);
void rempi_status_free(MPI_Status *statuses);

#endif
