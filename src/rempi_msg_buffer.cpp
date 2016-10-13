
#include "rempi_msg_buffer.h"


void rempi_msgb_register_recv(mpi_const void *buf, int count, MPI_Datatype datatype, int source,
                                 int tag, MPI_Comm comm, MPI_Request *request, int request_type, int matching_set_id)
{

}

int rempi_msgb_progress_recv()
{
}

void* rempi_msgb_get_original_buffer(MPI_Request *request) 
{
}

void* rempi_msgb_get_message(int source, int tag, MPI_Comm comm)
{
}

void rempi_msgb_deregister_recv(MPI_Request *request) 
{
}





