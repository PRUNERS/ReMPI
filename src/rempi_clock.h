#ifndef __REMPI_CLOCK_H__
#define __REMPI_CLOCK_H__

#include <mpi.h>
#include "rempi_type.h"
#include "rempi_event_list.h"

#define REMPI_CLOCK_INITIAL_CLOCK (10)
#define REMPI_CLOCK_COLLECTIVE_CLOCK (3)

void rempi_clock_init();
int rempi_clock_register_recv_clocks(size_t *clocks, int length);
int rempi_clock_get_local_clock(size_t*);
int rempi_clock_sync_clock(size_t clock, int request_type);
int rempi_clock_sync_clock(int matched_count,
                           int array_of_indices[],
                           size_t *clock, int* request_info,
			   int matching_function_type);


int rempi_clock_collective_sync_clock(MPI_Comm comm);

int rempi_clock_mpi_isend(mpi_const void *buf,
			  int count,
			  MPI_Datatype datatype,
			  int dest,
			  int tag,
			  MPI_Comm comm,
			  MPI_Request *request,
			  int send_function_type);

/* #define REMPI_CLOCK_STOP  (0) */
/* #define REMPI_CLOCK_START (1) */
/* int rempi_clock_control(int); */






#endif

