#ifndef __REMPI_REQUEST_MG_H__
#define __REMPI_REQUEST_MG_H__

#include <stdlib.h>
#include <string.h>

#include <mpi.h>

#include "rempi_err.h"
#include "rempi_mem.h"
#include "rempi_config.h"
#include "rempi_util.h"

#define REMPI_SEND_REQUEST (151)
#define REMPI_RECV_REQUEST (152)
#define REMPI_NULL_REQUEST (153)
#define REMPI_IGNR_REQUEST (154)

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

int rempi_reqmg_register_recv_request(MPI_Request *request, int source, int tag, int comm_id);
int rempi_reqmg_deregister_recv_request(MPI_Request *request);
int rempi_reqmg_register_send_request(MPI_Request *request, int source, int tag, int comm_id);
int rempi_reqmg_deregister_send_request(MPI_Request *request);

int rempi_reqmg_get_test_id(MPI_Request *request, int count);
void rempi_reqmg_get_request_info(int incount, MPI_Request *requests, int *sendcount, int *recvcount, int *nullcount, int *request_info);
void rempi_reqmg_store_send_statuses(int incount, MPI_Request *requests, int *request_info, MPI_Status *statuses);
/*TODO: remove the below two functions*/
int rempi_reqmg_get_recv_request_count(int incount, MPI_Request *requests);
int rempi_reqmg_get_null_request_count(int incount, MPI_Request *requests);

#endif
