#include <list>

#include "rempi_msg_buffer.h"
#include "rempi_request_mg.h"
#include "rempi_err.h"
#include "rempi_request_mg.h"
#include "rempi_event.h"
#include "rempi_event_list.h"
#include "rempi_cp.h"
#include "clmpi.h"

class inactive_request {
public:
  rempi_reqmg_recv_args *recv_args;
  MPI_Status status;
  inactive_request(rempi_reqmg_recv_args *recv_args, MPI_Status status)
    : recv_args(recv_args)
    , status(status)
  {}
};


static int rempi_msgb_next_recv_request_id = 0;
static int rempi_msgb_next_send_request_id = 0;

/* Send queue to encoder */
static rempi_event_list<rempi_event*> *send_event_queue;
/* Recv queue from encoder */
static rempi_event_list<rempi_event*> *recv_event_queue;

/* MPI_Irecv is posted. Needs to be tested by matching functions */
static list<rempi_reqmg_recv_args*> active_recv_list;
/* MPI_Irecv not is posted. Needs to be probed by probing functions */
static list<inactive_request*> inactive_recv_list;
/* */
static unordered_map<MPI_Request, void*> request_to_original_buffer;





static int is_matched(int requested_rank, int requested_tag, MPI_Comm requested_comm, int actual_rank, int actual_tag, MPI_Comm actual_comm)
{
  if (requested_comm != actual_comm) return 0;
  if (requested_tag != MPI_ANY_TAG && requested_tag != actual_tag) return 0;
  if (requested_rank != MPI_ANY_SOURCE && requested_rank != actual_rank) return 0;
  return 1;
}

static int activate_recv(int count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm, int matching_set_id)
{
  int datatype_size;
  void* pooled_buf;
  rempi_reqmg_recv_args *recv_args;
  MPI_Request real_request;

  PMPI_Type_size(datatype, &datatype_size);
  pooled_buf = rempi_malloc(datatype_size * count);
  PMPI_Irecv(pooled_buf, count, datatype, source, tag, comm, &real_request);
  recv_args = new rempi_reqmg_recv_args(pooled_buf, count, datatype, source, tag, comm, real_request, 
					REMPI_REQMG_MPI_CALL_ID_UNKNOWN, 
					matching_set_id);
  active_recv_list.push_back(recv_args);
  //  REMPI_DBGI(0, "request: %p (null: %p)", real_request, MPI_REQUEST_NULL);  

  return 0;
}

static int progress_inactive_recv()
{
  size_t clock;
  int flag;
  rempi_reqmg_recv_args *recv_args;
  list<inactive_request*>::iterator it, it_end;
  MPI_Status status;
  int count;

  for (it = inactive_recv_list.begin(); it != inactive_recv_list.end(); it++) {
    recv_args = (*it)->recv_args;
    flag = 0;

    CLMPI_register_recv_clocks(&clock, 1);
    CLMPI_clock_control(0);
    PMPI_Iprobe(recv_args->source, recv_args->tag, recv_args->comm, &flag, &status);
    CLMPI_clock_control(1);
    if (flag) {
      PMPI_Get_count(&status, recv_args->datatype, &count);
      if (recv_args->count != count) {
	REMPI_ERR("Recieve count is inconsistent: %d and %d", recv_args->count, count);
      }
      activate_recv(recv_args->count, recv_args->datatype, recv_args->source, recv_args->tag, recv_args->comm, recv_args->matching_set_id);
    }
  }
  return 0;
}

static int progress_active_recv()
{
  size_t clock;
  int flag;
  rempi_reqmg_recv_args *recv_args;
  list<rempi_reqmg_recv_args*>::iterator it, it_end;
  MPI_Status status;
  rempi_event *event;


  for (it = active_recv_list.begin(); it != active_recv_list.end();) {
    recv_args = *it;
    flag = 0;

    CLMPI_register_recv_clocks(&clock, 1);
    CLMPI_clock_control(0);
    //    REMPI_DBGI(0, "test request: %p (soruct: %d)", recv_args->request, recv_args->source);
    PMPI_Test(&recv_args->request, &flag, &status);
    CLMPI_clock_control(1);
    if (flag) {
      // REMPI_DBGI(0, "A->     : (source: %d, tag: %d, clock: %d, msid: %d)",
      // 		 status.MPI_SOURCE, status.MPI_TAG, clock, recv_args->matching_set_id);
      inactive_recv_list.push_back(new inactive_request(recv_args, status));
      it = active_recv_list.erase(it);
      rempi_cp_record_recv(status.MPI_SOURCE, 0);
      event =  rempi_event::create_test_event(1,
					      1,
					      status.MPI_SOURCE,
					      REMPI_MPI_EVENT_INPUT_IGNORE,
					      REMPI_MPI_EVENT_INPUT_IGNORE,
					      clock,
					      recv_args->matching_set_id);
      send_event_queue->enqueue_replay(event, recv_args->matching_set_id);
    } else {
      it++;
    }
  }
  return 0;
}

int rempi_msgb_init(rempi_event_list<rempi_event*> *send_eq, rempi_event_list<rempi_event*> *recv_eq)
{
  send_event_queue = send_eq;
  recv_event_queue = recv_eq;
  return 0;
}


MPI_Request rempi_msgb_allocate_request(int request_type)
{
  switch(request_type){
  case REMPI_SEND_REQUEST:
    if ((MPI_Request)rempi_msgb_next_send_request_id == MPI_REQUEST_NULL) {
      rempi_msgb_next_send_request_id++;
    }
    return (MPI_Request)rempi_msgb_next_send_request_id++;
  case REMPI_RECV_REQUEST:
    if ((MPI_Request)rempi_msgb_next_recv_request_id == MPI_REQUEST_NULL) {
      rempi_msgb_next_recv_request_id++;
    }
    return (MPI_Request)rempi_msgb_next_recv_request_id++;
  default:
    REMPI_ERR("No such request_type: %d", request_type);
  }

}

int rempi_msgb_register_recv(void *buf, int count, MPI_Datatype datatype, int source,
                                 int tag, MPI_Comm comm, MPI_Request *request, int matching_set_id)
{
  request_to_original_buffer[*request] = buf;
  activate_recv(count, datatype, source, tag, comm, matching_set_id);
  return 0;
}

int rempi_msgb_progress_recv()
{
  progress_active_recv();
  progress_inactive_recv();
}

int rempi_msgb_cancel_request(MPI_Request *request)
{
  
}

int rempi_msgb_recv_msg(void* dest_buffer, int replayed_rank, int requested_tag, int requested_comm, MPI_Status *replaying_status)
{
  list<inactive_request*>::iterator it;
  inactive_request* inactive_request;
  rempi_reqmg_recv_args* recv_args;;
  int matched;
  int datatype_size, count;

  for (it = inactive_recv_list.begin(); it != inactive_recv_list.end(); it++) {
    inactive_request = *it;
    recv_args = inactive_request->recv_args;
    matched = is_matched(replayed_rank, requested_tag, requested_comm,
			    inactive_request->status.MPI_SOURCE, inactive_request->status.MPI_TAG, recv_args->comm);
    if (matched) {
      PMPI_Type_size(recv_args->datatype, &datatype_size);
      PMPI_Get_count(&(inactive_request->status), recv_args->datatype, &count);
      memcpy(recv_args->buffer, dest_buffer, datatype_size * count);
      *replaying_status = inactive_request->status;
      inactive_recv_list.erase(it);
      rempi_free(recv_args->buffer);
      delete recv_args;
      delete inactive_request;
      return 0;
    }
  }
  REMPI_ERR("There is not any matched request");
  return 0;
}





