#ifndef __REMPI_MSGB_H__
#define __REMPI_MSGB_H__

#include <mpi.h>

#include "rempi_type.h"
#include "rempi_event_list.h"

int rempi_msgb_init(rempi_event_list<rempi_event*> *send_eq, rempi_event_list<rempi_event*> *recv_eq);
MPI_Request rempi_msgb_allocate_request(int request_type);
int rempi_msgb_register_recv(void *buf, int count, MPI_Datatype datatype, int source,
			     int tag, MPI_Comm comm, MPI_Request *request, int matching_set_id);
int rempi_msgb_progress_recv();
int rempi_msgb_cancel_request(MPI_Request *request);
int rempi_msgb_recv_msg(void* dest_buffer, int replayed_rank, int requested_tag, int requested_comm, MPI_Status *replaying_status);


#endif

