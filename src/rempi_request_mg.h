#ifndef __REMPI_REQUEST_MG_H__
#define __REMPI_REQUEST_MG_H__

#include <stdlib.h>
#include <string.h>
#include <vector>

#include <mpi.h>

#include "rempi_err.h"
#include "rempi_mem.h"
#include "rempi_config.h"
#include "rempi_util.h"
#include "rempi_type.h"

#define REMPI_SEND_REQUEST (151)
#define REMPI_RECV_REQUEST (152)
#define REMPI_NULL_REQUEST (153)
#define REMPI_IGNR_REQUEST (154)

#define REMPI_REQMG_MATCHING_SET_ID_UNKNOWN   (-1)
#define REMPI_REQMG_MPI_CALL_ID_UNKNOWN   (-1)

using namespace std;

/*TODO: 
 get_matching_set_id, get_recv_id, get_test_id should be in different source file 
*/
class rempi_matching_id 
{
public:
  int source;
  int tag;
  int comm_id;
  rempi_matching_id(int source, int tag, int comm_id)
    : source(source)
    , tag(tag)
    , comm_id(comm_id)
  {};
  int has_intersection_in(vector<rempi_matching_id*> *vec) {
    for (vector<rempi_matching_id*>::const_iterator cit = vec->cbegin(),
         cit_end = vec->cend();
	 cit != cit_end;
	 cit++) {
      rempi_matching_id *mid = *cit;
      if (this->has_intersection_with(*mid)) return 1;
    }
    return 0;
  };

  int has_intersection_with(rempi_matching_id &input) {
    if (this->comm_id != input.comm_id) return 0;

    if (this->source == MPI_ANY_SOURCE) {
      if (input.tag == MPI_ANY_TAG ||
	  input.tag == this->tag) {
	return 1;
      }
    } else if (this->tag == MPI_ANY_TAG) {
      if (input.source == MPI_ANY_SOURCE ||
	  input.source == this->source) {
	return 1;
      }
    } else {
      if (this->source == input.source && 
	  this->tag    == input.tag) {
	return 1;
      }
    }
    return 0;
  };

};

class rempi_reqmg_send_args
{
 public:
  int dest;
  size_t clock;
 rempi_reqmg_send_args(int dest, size_t clock)
    : dest(dest)
    , clock(clock) {}
};



class rempi_reqmg_recv_args
{

 public:
  void* buffer;
  int count;
  MPI_Datatype datatype;
  int source;
  int tag;
  MPI_Comm comm;
  MPI_Request request;
  int mpi_call_id;
  int matching_set_id;

  
    

  rempi_reqmg_recv_args(void *buf, int count, MPI_Datatype datatype, int source,
			int tag, MPI_Comm comm, MPI_Request request, int mpi_call_id, int matching_set_id)
    : buffer(buf)
    , count(count)
    , datatype(datatype)
    , source(source)
    , tag(tag)
    , comm(comm)
    , request(request)
    , mpi_call_id(mpi_call_id)
    , matching_set_id(matching_set_id) {}
};

/* =========
   Wheneve MPI_Request is initialized, 
   rempi_reqmg_register_request must be called.
   
    - MPI_Isend/Ibsend/Irsend/Issend
    - MPI_Send_init/Bsend_init/Rsend_init/Ssend_init
    - MPI_Recv_init

   And, whenever MPI_Request is freed,
   rempi_reqmg_deregister_request must be called.
   But if the MPI_Request is from MPI_Recv/Send..._Init, the MPI_Request
   is reused by MPI so you do not have to call this function.
   
    - MPI_Wait/Waitany/Waitsome/Waitall
    - MPI_Test/Testany/Testsome/Testall (only when matched)
    - MPI_Cancel
    - MPI_Request_free   
*/
int rempi_reqmg_register_request(mpi_const void *buf, int count, MPI_Datatype datatype, int source,
				 int tag, MPI_Comm comm, MPI_Request *request, int request_type, int *matching_set_id);
int rempi_reqmg_deregister_request(MPI_Request *request, int request_type);
/* =========== */


/* Check and progress receiving messages */
int rempi_reqmg_progress_recv(int matching_set_id, int incount, MPI_Request array_of_requests[]);

/* Return test_id */
int rempi_reqmg_get_test_id(MPI_Request *request, int count);

/* */
void rempi_reqmg_get_request_info(int incount, MPI_Request *requests, int *sendcount, int *recvcount, int *nullcount, int *request_info, int *is_record_and_replay, int *matching_set_id);
void rempi_reqmg_get_request_type(MPI_Request *request, int *request_type);
void rempi_reqmg_store_send_statuses(int incount, MPI_Request *requests, int *request_info, MPI_Status *statuses);

/* Get arguments */
int rempi_reqmg_get_matching_id(MPI_Request *request, int *rank, int *tag, MPI_Comm *comm);
int rempi_reqmg_get_buffer(MPI_Request *request, void** buffer);

/* Get/Set matching set ids */
int rempi_reqmg_get_matching_set_id(MPI_Request *requet);
int rempi_reqmg_get_matching_set_id_map(int **mpi_call_ids, int **matching_set_ids, int *length);
int rempi_reqmg_set_matching_set_id_map(int *mpi_call_ids, int *matching_set_ids, int length);

rempi_reqmg_recv_args* rempi_reqmg_get_recv_args(MPI_Request *request);
size_t rempi_reqmg_get_send_request_clock(MPI_Request *request);

/*TODO: remove the below two functions*/
int rempi_reqmg_get_recv_request_count(int incount, MPI_Request *requests);
int rempi_reqmg_get_null_request_count(int incount, MPI_Request *requests);

#endif
