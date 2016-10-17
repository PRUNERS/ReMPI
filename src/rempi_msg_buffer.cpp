#include <list>
#include <unordered_set>
#include <unordered_map>

#include "rempi_msg_buffer.h"
#include "rempi_request_mg.h"
#include "rempi_err.h"
#include "rempi_request_mg.h"
#include "rempi_event.h"
#include "rempi_event_list.h"
#include "rempi_cp.h"
#include "clmpi.h"

#define REMPI_MSGB_REQUEST_TYPE_PASSIVE (0) /* Activated by application */
#define REMPI_MSGB_REQUEST_TYPE_ACTIVE (1)  /* Activated by rempi_msb_buffer */

class rempi_msgb_request {
public:
  int type;
  rempi_reqmg_recv_args *recv_args;
  MPI_Status status;
  MPI_Request app_request;
  rempi_msgb_request(rempi_reqmg_recv_args *recv_args, MPI_Request *request, MPI_Status *stat, int type)
    : recv_args(recv_args)
    , type(type)
  {
    if (stat != MPI_STATUS_IGNORE) status = *stat;
    if (*request != MPI_REQUEST_NULL) app_request = *request;
  }

  ~rempi_msgb_request() {
    delete recv_args;
  }
};


static int rempi_msgb_next_recv_request_id = 1;
static int rempi_msgb_next_send_request_id = 1;

/* Send queue to encoder */
static rempi_event_list<rempi_event*> *send_event_queue;
/* Recv queue from encoder */
static rempi_event_list<rempi_event*> *recv_event_queue;

/* MPI_Irecv is posted. Needs to be tested by matching functions */
static list<rempi_msgb_request*> active_recv_list;
/* MPI_Irecv not is posted. Needs to be probed by probing functions */
static list<rempi_msgb_request*> inactive_recv_list;


unordered_set<int> probed_message_source_set;
unordered_set<int> recved_message_source_set;
unordered_map<int, size_t> max_recved_clock_umap;


static int is_matched(int requested_rank, int requested_tag, MPI_Comm requested_comm, int actual_rank, int actual_tag, MPI_Comm actual_comm)
{
  if (requested_comm != actual_comm) return 0;
  if (requested_tag != MPI_ANY_TAG && requested_tag != actual_tag) return 0;
  if (requested_rank != MPI_ANY_SOURCE && requested_rank != actual_rank) return 0;
  return 1;
}

static int activate_recv(int count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm, MPI_Request *request, int matching_set_id, int request_type)
{
  int datatype_size;
  void* pooled_buf;
  rempi_reqmg_recv_args *recv_args;
  rempi_msgb_request *msgb_request;
  MPI_Request real_request;
  

  PMPI_Type_size(datatype, &datatype_size);
  pooled_buf = rempi_malloc(datatype_size * count);
  PMPI_Irecv(pooled_buf, count, datatype, source, tag, comm, &real_request);
  recv_args = new rempi_reqmg_recv_args(pooled_buf, count, datatype, source, tag, comm, real_request, 
					REMPI_REQMG_MPI_CALL_ID_UNKNOWN, 
					matching_set_id);
  msgb_request = new rempi_msgb_request(recv_args, request, MPI_STATUS_IGNORE, request_type);  
  active_recv_list.push_back(msgb_request);
  //  REMPI_DBGI(0, "request: %p (null: %p)", real_request, MPI_REQUEST_NULL);  

  return 0;
}

static int update_max_recved_clock(int source, size_t clock) 
{
  size_t old_clock;
  if (max_recved_clock_umap.find(source) == 
      max_recved_clock_umap.end()) {
    old_clock = 0;
  } else {
    old_clock = max_recved_clock_umap.at(source);
  }

  if (old_clock < clock) {
    max_recved_clock_umap[source]  = clock;
  } 
  return 0;
}

static int progress_inactive_recv()
{
  size_t clock;
  int flag;
  rempi_msgb_request *msgb_request;
  rempi_reqmg_recv_args *recv_args;
  list<rempi_msgb_request*>::iterator it, it_end;
  MPI_Status status;
  int count;

  for (it = inactive_recv_list.begin(); it != inactive_recv_list.end(); it++) {
    msgb_request = *it;
    recv_args = msgb_request->recv_args;
    flag = 0;

    CLMPI_clock_control(0);

    PMPI_Iprobe(recv_args->source, recv_args->tag, recv_args->comm, &flag, &status);
    //    REMPI_DBG("Probe: source:%d, tag:%d => flag: %d", recv_args->source, recv_args->tag, flag);
    //    sleep(1);
    CLMPI_clock_control(1);
    if (flag) {
      //      REMPI_DBG("size: %lu", inactive_recv_list.size());
      PMPI_Get_count(&status, recv_args->datatype, &count);
      if (recv_args->count != count) {
	REMPI_ERR("Recieve count is inconsistent: %d and %d", recv_args->count, count);
      }
      activate_recv(recv_args->count, recv_args->datatype, recv_args->source, recv_args->tag, recv_args->comm, 
		    &(msgb_request->app_request), recv_args->matching_set_id, REMPI_MSGB_REQUEST_TYPE_ACTIVE);
    }
  }
  return 0;
}

static int progress_active_recv()
{
  size_t clock;
  int flag;
  rempi_msgb_request *msgb_request;
  rempi_reqmg_recv_args *recv_args;
  list<rempi_msgb_request*>::iterator it, it_end;
  MPI_Status status;
  rempi_event *event;

  recved_message_source_set.clear();
  for (it = active_recv_list.begin(); it != active_recv_list.end();) {
    msgb_request = *it;
    recv_args = msgb_request->recv_args;
    flag = 0;

    CLMPI_register_recv_clocks(&clock, 1);
    CLMPI_clock_control(0);
    //    REMPI_DBGI(0, "test request: %p (soruct: %d)", recv_args->request, recv_args->source);
    PMPI_Test(&recv_args->request, &flag, &status);
    CLMPI_clock_control(1);
    if (flag) {
#ifdef REMPI_DBG_REPLAY      
      REMPI_DBGI(-1, "A->     : (source: %d, tag: %d, clock: %d, msid: %d)",
       		 status.MPI_SOURCE, status.MPI_TAG, clock, recv_args->matching_set_id);
      // REMPI_DBGI(REMPI_DBG_REPLAY, "A->     : (source: %d, tag: %d, clock: %d, msid: %d)",
      //  		 status.MPI_SOURCE, status.MPI_TAG, clock, recv_args->matching_set_id);
#endif
      
      msgb_request->status = status;
      inactive_recv_list.push_back(msgb_request);
      it = active_recv_list.erase(it);
      rempi_cp_record_recv(status.MPI_SOURCE, 0);
      recved_message_source_set.insert(status.MPI_SOURCE);
      event =  rempi_event::create_test_event(1,
					      1,
					      status.MPI_SOURCE,
					      REMPI_MPI_EVENT_INPUT_IGNORE,
					      REMPI_MPI_EVENT_INPUT_IGNORE,
					      clock,
					      recv_args->matching_set_id);
      update_max_recved_clock(status.MPI_SOURCE, clock);
      send_event_queue->enqueue_replay(event, recv_args->matching_set_id);
    } else {
      it++;
    }
  }
  return 0;
}

static int probe_msg()
{
  size_t pred_ranks_length;
  int *pred_ranks;
  size_t comm_length = 1;
  MPI_Comm comm[1] = {MPI_COMM_WORLD};
  int flag;
  
  probed_message_source_set.clear();
  pred_ranks = rempi_cp_get_pred_ranks(&pred_ranks_length);
  for (int i = 0; i < pred_ranks_length; i++) {
    for (int j = 0; j < comm_length; j ++) {
      PMPI_Iprobe(pred_ranks[j], MPI_ANY_TAG, comm[j], &flag, MPI_STATUS_IGNORE);
      if (flag) probed_message_source_set.insert(pred_ranks[j]);
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
  return MPI_REQUEST_NULL;
}

int rempi_msgb_register_recv(void *buf, int count, MPI_Datatype datatype, int source,
                                 int tag, MPI_Comm comm, MPI_Request *request, int matching_set_id)
{
  activate_recv(count, datatype, source, tag, comm, request, matching_set_id, REMPI_MSGB_REQUEST_TYPE_PASSIVE);
  return 0;
}

int rempi_msgb_progress_recv()
{
  progress_active_recv();
  progress_inactive_recv();
  probe_msg();
  return 0;
}

int rempi_msgb_cancel_request(MPI_Request *request)
{
  /* 
     Regardless of how canceling recv request, ReMPI replay message order as recorded.
     So this routin do not anything.
  */
  return 0;
}

int rempi_msgb_recv_msg(void* dest_buffer, int replayed_rank, int requested_tag, MPI_Comm  requested_comm, MPI_Status *replaying_status)
{
  list<rempi_msgb_request*>::iterator it;
  rempi_msgb_request* inactive_request;
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


int rempi_msgb_has_recved_msg(int source) 
{
  return (recved_message_source_set.find(source) != recved_message_source_set.end())? 1:0;
}

int rempi_msgb_has_probed_msg(int source)
{
  return (probed_message_source_set.find(source) != probed_message_source_set.end())? 1:0;
}

size_t rempi_msgb_get_max_recved_clock(int source)
{
  return (max_recved_clock_umap.find(source) != max_recved_clock_umap.end())? max_recved_clock_umap.at(source):0;
}




