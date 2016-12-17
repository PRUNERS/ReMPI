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
#include "rempi_config.h"
#include "rempi_util.h"
#include "rempi_clock.h"

#define REMPI_MSGB_REQUEST_TYPE_USER_REQUESTED   (0) /* Requested by App */
#define REMPI_MSGB_REQUEST_TYPE_REMPI_REQUESTED  (1) /* Requested by ReMPI */

class rempi_msgb_request {
public:
  rempi_reqmg_recv_args *recv_args;
  MPI_Status status;
  MPI_Request app_request;
  int howto_requested;
  size_t clock;
  int recved_count;
  rempi_msgb_request(rempi_reqmg_recv_args *recv_args, MPI_Request *app_request, int howto_requested)
    : recv_args(recv_args)
    , app_request(*app_request)
    , howto_requested(howto_requested)
    , recved_count(-1)
  {}
};

static MPI_Request REMPI_REQUEST_NULL = MPI_REQUEST_NULL;



static int rempi_msgb_next_recv_request_id = 0;
static int rempi_msgb_next_send_request_id = 0;

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



static void print_recv_list() {
#ifdef REMPI_DBG_REPLAY
  rempi_msgb_request *msgb_request;
  rempi_reqmg_recv_args *recv_args;
  list<rempi_msgb_request*>::iterator it, it_end;
  REMPI_DBGI(REMPI_DBG_REPLAY, "Active/Inactive Buffer")
  for (it = active_recv_list.begin(); it != active_recv_list.end(); it++) {
    recv_args = (*it)->recv_args;
    REMPI_DBGI(REMPI_DBG_REPLAY, "  ACT  : source: %d, tag: %d, comm: %p (set_id: %d)", 
	       recv_args->source, recv_args->tag, recv_args->comm, recv_args->matching_set_id);
  }

  for (it = inactive_recv_list.begin(); it != inactive_recv_list.end(); it++) {
    msgb_request = *it;
    recv_args    = msgb_request->recv_args;
    REMPI_DBGI(REMPI_DBG_REPLAY, "  INACT: source: %d, tag: %d, clock: %lu, comm: %p (set_id: %d)", 
	       msgb_request->status.MPI_SOURCE, msgb_request->status.MPI_TAG, msgb_request->clock, recv_args->comm, recv_args->matching_set_id);
  }
#endif
}


int rempi_msgb_is_matched(int requested_rank, int requested_tag, MPI_Comm requested_comm, int actual_rank, int actual_tag, MPI_Comm actual_comm)
{
  int matched_type;
  if (requested_comm != actual_comm) return REMPI_MSGB_REQUEST_MATCHED_TYPE_NOT_MATCHED;
  if (requested_tag != MPI_ANY_TAG && requested_tag != actual_tag) return REMPI_MSGB_REQUEST_MATCHED_TYPE_NOT_MATCHED;
  if (requested_rank != MPI_ANY_SOURCE && requested_rank != actual_rank) return REMPI_MSGB_REQUEST_MATCHED_TYPE_NOT_MATCHED;

  matched_type = (requested_rank == actual_rank && requested_tag == actual_tag)? 
    REMPI_MSGB_REQUEST_MATCHED_TYPE_SAME:REMPI_MSGB_REQUEST_MATCHED_TYPE_MATCHED;
  return matched_type;
}

static int is_activated_request(int count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm, list<rempi_msgb_request*> &recv_list)
{
  rempi_msgb_request *msgb_request;
  rempi_reqmg_recv_args *recv_args;
  list<rempi_msgb_request*>::iterator it, it_end;
  int matched_type;
  int datatype_size_user, datatype_size_rempi ;
  for (it = recv_list.begin(); it != recv_list.end(); it++) {
    msgb_request = *it;
    recv_args = msgb_request->recv_args;
    if (msgb_request->howto_requested == REMPI_MSGB_REQUEST_TYPE_REMPI_REQUESTED) {
      PMPI_Type_size(datatype,            &datatype_size_user);
      PMPI_Type_size(recv_args->datatype, &datatype_size_rempi);
      if (recv_args->count * datatype_size_rempi == count * datatype_size_user &&
	  recv_args->source   == source   &&
	  recv_args->tag      == tag      &&
	  recv_args->comm     == comm) {
	msgb_request->howto_requested = REMPI_MSGB_REQUEST_TYPE_USER_REQUESTED;
	return 1;
      }
    }
  }
  return 0;
}


static int is_activated_request_by_rempi(int count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm)
{
  if (is_activated_request(count, datatype, source, tag, comm, inactive_recv_list)) return 1;
  if (is_activated_request(count, datatype, source, tag, comm,   active_recv_list)) return 1;
  return 0;
}

static int activate_recv(int count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm, MPI_Request *request, int matching_set_id, int howto_requested)
{
  int datatype_size;
  void* pooled_buf;
  rempi_msgb_request *msgb_request;
  rempi_reqmg_recv_args *recv_args;
  MPI_Request real_request;

  //  REMPI_DBGI(0, " ==== Activating: source: %d, tag: %d", source, tag);      
  if (matching_set_id == REMPI_REQMG_MATCHING_SET_ID_UNKNOWN && howto_requested == REMPI_MSGB_REQUEST_TYPE_USER_REQUESTED) {
    /* 
       This request will not be never matched (and canceled by MPI_Cancel).
       So I do not even call MPI_Irecv;
    */
    return 0;
  } else if (matching_set_id == REMPI_REQMG_MATCHING_SET_ID_UNKNOWN && howto_requested == REMPI_MSGB_REQUEST_TYPE_USER_REQUESTED) {
    REMPI_ERR("Unknown matching_set_id");
  }

  if (is_activated_request_by_rempi(count, datatype, source, tag, comm) && howto_requested == REMPI_MSGB_REQUEST_TYPE_USER_REQUESTED) {
    //  if (is_activated_request_by_rempi(count, datatype, source, tag, comm)) {
    return 0;
  }

  PMPI_Type_size(datatype, &datatype_size);
  // REMPI_DBG("size: %d: datatype: %d", datatype_size, datatype);
  // if (datatype == MPI_FLOAT ||
  //     datatype == MPI_DOUBLE ||
  //     datatype == MPI_INT ||
  //     datatype == MPI_LONG ||
  //     datatype == MPI_LONG_LONG ||
  //     datatype == MPI_UNSIGNED ||
  //     datatype == MPI_COMPLEX ||
  //     datatype == MPI_DOUBLE_COMPLEX) {
    
  // } else {
  //   if (datatype == MPI_BYTE) {
  //     REMPI_DBG("count: %d", count);
  //   } else {
  //     REMPI_ERR("error");
  //   }
  // }
  pooled_buf = rempi_malloc(datatype_size * count);
  PMPI_Irecv(pooled_buf, count, datatype, source, tag, comm, &real_request);
  recv_args = new rempi_reqmg_recv_args(pooled_buf, count, datatype, source, tag, comm, real_request, 
					REMPI_REQMG_MPI_CALL_ID_UNKNOWN, 
					matching_set_id);
  msgb_request = new rempi_msgb_request(recv_args, request, howto_requested);
  active_recv_list.push_back(msgb_request);
  //  REMPI_DBGI(0, " ==== Activated: source: %d, tag: %d", source, tag);
  //  REMPI_DBGI(0, "request: %p (null: %p)", real_request, MPI_REQUEST_NULL);  

  return 1;
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
  int is_progressed = 0;
  REMPI_ERR("Erro");
  for (it = inactive_recv_list.begin(); it != inactive_recv_list.end(); it++) {
    msgb_request = *it;
    recv_args = msgb_request->recv_args;
    flag = 0;

    PMPI_Iprobe(recv_args->source, recv_args->tag, recv_args->comm, &flag, &status);
    if (flag) {
      PMPI_Get_count(&status, recv_args->datatype, &count);
      activate_recv(count, recv_args->datatype, recv_args->source, recv_args->tag, recv_args->comm, &msgb_request->app_request, 
		    recv_args->matching_set_id, REMPI_MSGB_REQUEST_TYPE_REMPI_REQUESTED);
      is_progressed = 1;
    }
  }
  return is_progressed;
}

static int push_to_send_event_queue(int source, size_t clock, int matching_set_id)
{
  rempi_event *event;
  event =  rempi_event::create_test_event(1,
					  1,
					  source,
					  REMPI_MPI_EVENT_INPUT_IGNORE,
					  REMPI_MPI_EVENT_INPUT_IGNORE,
					  clock,
					  matching_set_id);
  update_max_recved_clock(source, clock);
  send_event_queue->enqueue_replay(event, matching_set_id);
  //  REMPI_DBGI(0, "send_event_queue: rank: %d, clock: %lu", source, clock);
  return 0;
}

static int progress_active_recv_in_inactive_unknown_recv(rempi_msgb_request *msgb_request_active)
{
  rempi_msgb_request *msgb_request_inactive;
  rempi_reqmg_recv_args *recv_args_inactive, *recv_args_active;
  list<rempi_msgb_request*>::iterator it, it_end;
  int matched;
  
  REMPI_ERR("Erro");

  recv_args_active   = msgb_request_active->recv_args;
  if (recv_args_active->matching_set_id == REMPI_REQMG_MATCHING_SET_ID_UNKNOWN) return 0;

  for (it = inactive_recv_list.begin(); it != inactive_recv_list.end(); it++) {
    msgb_request_inactive = *it;
    recv_args_inactive = msgb_request_inactive->recv_args;
    if (recv_args_inactive->matching_set_id != REMPI_REQMG_MATCHING_SET_ID_UNKNOWN) continue;


    matched = rempi_msgb_is_matched(recv_args_active->source,   recv_args_active->tag,   recv_args_active->comm,
			 msgb_request_inactive->status.MPI_SOURCE, msgb_request_inactive->status.MPI_TAG, recv_args_inactive->comm);
    if (matched) {
      recv_args_inactive->matching_set_id = recv_args_active->matching_set_id;
      push_to_send_event_queue(msgb_request_inactive->status.MPI_SOURCE, msgb_request_inactive->clock, recv_args_inactive->matching_set_id);
      PMPI_Cancel(&recv_args_active->request);
      return 1;
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
  int is_progressed  = 0;

  recved_message_source_set.clear();
  for (it = active_recv_list.begin(); it != active_recv_list.end();) {
    msgb_request = *it;
    recv_args = msgb_request->recv_args;
    flag = 0;

    rempi_clock_register_recv_clocks(&clock, 1);
    //    REMPI_DBG("count: %d, datatype: %d", recv_args->count, recv_args->datatype);
    PMPI_Test(&recv_args->request, &flag, &status);
    if (flag) {
#ifdef REMPI_DBG_REPLAY      
      REMPI_DBGI(REMPI_DBG_REPLAY, "A->     : (source: %d, tag: %d, clock: %d, msid: %d)",
      		 status.MPI_SOURCE, status.MPI_TAG, clock, recv_args->matching_set_id);
#endif
      msgb_request->status = status;
      PMPI_Get_count(&(msgb_request->status), recv_args->datatype, &(msgb_request->recved_count));//kento
      msgb_request->clock  = clock;
      inactive_recv_list.push_back(msgb_request);
      it = active_recv_list.erase(it);
      rempi_cp_record_recv(status.MPI_SOURCE, 0);
      recved_message_source_set.insert(status.MPI_SOURCE);
      if (recv_args->matching_set_id != REMPI_REQMG_MATCHING_SET_ID_UNKNOWN) {
	push_to_send_event_queue(status.MPI_SOURCE, msgb_request->clock, recv_args->matching_set_id);
      } else {
	REMPI_ERR("Unknown matching_set_id");
      }
      is_progressed = 1;
    } else 
#if 0
      if (progress_active_recv_in_inactive_unknown_recv(msgb_request)) {
	it = active_recv_list.erase(it);
	is_progressed = 1;
      } else 
#endif
	{
	  it++;
	}
  }
  return is_progressed;
}

static int probe_msg()
{
  size_t pred_ranks_length;
  int *pred_ranks;
  size_t comm_length = 1;
  MPI_Comm comm[1] = {MPI_COMM_WORLD};
  int flag;
  MPI_Status status;
  

  probed_message_source_set.clear();
  pred_ranks = rempi_cp_get_pred_ranks(&pred_ranks_length);
  for (int i = 0; i < pred_ranks_length; i++) {
    for (int j = 0; j < comm_length; j ++) {
      //      REMPI_DBG("Probing: rank %d, pred_ranks: %d", pred_ranks[j], pred_ranks_length);
      PMPI_Iprobe(pred_ranks[i], MPI_ANY_TAG, comm[j], &flag, &status);
      if (flag) { 
	int count;
	int matching_set_id;
	PMPI_Get_count(&status, MPI_BYTE, &count);
	matching_set_id = rempi_reqmg_get_matching_set_id(status.MPI_TAG, comm[j]);
	activate_recv(count, MPI_BYTE, status.MPI_SOURCE, status.MPI_TAG, comm[j], &REMPI_REQUEST_NULL,
		      matching_set_id, REMPI_MSGB_REQUEST_TYPE_REMPI_REQUESTED);
	probed_message_source_set.insert(pred_ranks[i]);
      }
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
  activate_recv(count, datatype, source, tag, comm, request, matching_set_id, REMPI_MSGB_REQUEST_TYPE_USER_REQUESTED);
  return 0;
}

int rempi_msgb_progress_recv()
{
  int is_active_recv_progressed = 0, is_inactive_recv_progressed = 0;
  int is_progressed = 0;

  /* To avoid out-of-order message receive, 
     if a request is matched, check all requests */
  do {
    is_active_recv_progressed = progress_active_recv();
    //is_inactive_recv_progressed = progress_inactive_recv();
    probe_msg();
    //    if (is_active_recv_progressed || is_inactive_recv_progressed) is_progressed = 1;
    if (is_active_recv_progressed) is_progressed = 1;
    //  } while(is_active_recv_progressed || is_inactive_recv_progressed);
  } while(is_active_recv_progressed);

  if (is_progressed) {
    print_recv_list(); 
  }
  return 0;
}

int rempi_msgb_cancel_request(MPI_Request *request)
{
  *request = MPI_REQUEST_NULL;
  /* 
     Regardless of how canceling recv request, ReMPI replay message order as recorded.
     So this routin do not anything.
  */

  return 0;
}


int rempi_msgb_recv_msg(void* dest_buffer, int replayed_rank, int requested_tag, MPI_Comm  requested_comm, size_t clock, int matching_set_id, MPI_Status *replaying_status)
{
  list<rempi_msgb_request*>::iterator it;
  rempi_msgb_request* inactive_request;
  rempi_reqmg_recv_args* recv_args;;
  int matched;
  int datatype_size, count;

  for (it = inactive_recv_list.begin(); it != inactive_recv_list.end(); it++) {
    inactive_request = *it;
    recv_args = inactive_request->recv_args;
    // REMPI_DBGI(4, "RecvBF: replaying rank: %d, replaying tag: %d, replaying clock: %lu, replaying msid: %d, "
    // 	      "inactive request rank: %d, inactive request tag: %d, inactive request clock: %lu, inactive request msid: %d",
    // 	      replayed_rank, requested_tag, clock, matching_set_id, 
    // 	      inactive_request->status.MPI_SOURCE, inactive_request->status.MPI_TAG, inactive_request->clock, recv_args->matching_set_id);
#ifdef REMPI_DBG_REPLAY
    // REMPI_DBGI(REMPI_DBG_REPLAY, "Recv: replaying rank: %d, replaying tag: %d, replaying clock: %lu, replaying msid: %d, "
    // 	      "inactive request rank: %d, inactive request tag: %d, inactive request clock: %lu, inactive request msid: %d",
    // 	      replayed_rank, requested_tag, clock, matching_set_id, 
    // 	      inactive_request->status.MPI_SOURCE, inactive_request->status.MPI_TAG, inactive_request->clock, recv_args->matching_set_id);
#endif
    if (matching_set_id != recv_args->matching_set_id) continue;
    matched = rempi_msgb_is_matched(replayed_rank, requested_tag, requested_comm,
			    inactive_request->status.MPI_SOURCE, inactive_request->status.MPI_TAG, recv_args->comm);
    if (matched) {
      if (recv_args->matching_set_id == REMPI_REQMG_MATCHING_SET_ID_UNKNOWN) {
	REMPI_ERR("Unknown matching_set_id: source: %d tag: %d clock: %d", 
		  inactive_request->status.MPI_SOURCE, inactive_request->status.MPI_TAG, inactive_request->clock);
      }
      if (clock != inactive_request->clock) { 
	REMPI_ERR("Different clock error: replaying rank: %d, replaying tag: %d, replaying clock: %lu, replaying msid: %d, "
		  "inactive request rank: %d, inactive request tag: %d, inactive request clock: %lu, inactive request msid: %d",
		  replayed_rank, requested_tag, clock, matching_set_id, 
		  inactive_request->status.MPI_SOURCE, inactive_request->status.MPI_TAG, inactive_request->clock, recv_args->matching_set_id);
      }

      PMPI_Type_size(recv_args->datatype, &datatype_size);
      PMPI_Get_count(&(inactive_request->status), recv_args->datatype, &count);
      if (count != inactive_request->recved_count) REMPI_ERR("Count is different");
      //      memcpy(recv_args->buffer, dest_buffer, datatype_size * count);
      //      REMPI_DBG("datatype_size: %d, count: %d", datatype_size, count);
      int byte;
      PMPI_Get_count(&(inactive_request->status), MPI_BYTE, &byte);

      memcpy(dest_buffer, recv_args->buffer, datatype_size * count);
#ifdef REMPI_DBG_REPLAY
      REMPI_DBGI(REMPI_DBG_REPLAY, "Replay: dest_buff: %d (byte: %d)", rempi_compute_hash(dest_buffer, datatype_size * count), byte);
#endif
      *replaying_status = inactive_request->status;
      replaying_status->MPI_ERROR = 0;
      inactive_recv_list.erase(it);
      rempi_free(recv_args->buffer);
      delete recv_args;
      delete inactive_request;
      return 0;
    }
  }
  REMPI_ASSERT(0);
  REMPI_ERR("There is not any matched request");
  return 0;
}

int rempi_msgb_probe_msg(int source, int tag, MPI_Comm comm, int *flag, MPI_Status *status)
{
  rempi_msgb_request *msgb_request;
  list<rempi_msgb_request*>::iterator it, it_end;
  int is_matched;

  rempi_msgb_progress_recv();
  for (it = inactive_recv_list.begin(); it != inactive_recv_list.end(); it++) {
    msgb_request = *it;
    REMPI_DBGI(0, "(source: %d, tag: %d) and (msgb_source: %d, msgb_tag: %d): type: %d", 
    	       source, tag, msgb_request->status.MPI_SOURCE, msgb_request->status.MPI_TAG,
    	       msgb_request->howto_requested);
    if (msgb_request->howto_requested == REMPI_MSGB_REQUEST_TYPE_REMPI_REQUESTED) {
      is_matched = rempi_msgb_is_matched(source, tag, comm, 
					 msgb_request->status.MPI_SOURCE, msgb_request->status.MPI_TAG, msgb_request->recv_args->comm);
      if (is_matched != REMPI_MSGB_REQUEST_MATCHED_TYPE_NOT_MATCHED) { 
	*status = msgb_request->status;
	*flag = 1;
	return MPI_SUCCESS;
      }
    }
  }
  *flag = 0;
  return MPI_SUCCESS;
}

int rempi_msgb_get_tag_comm(int matched_rank, size_t matched_clock, int *output_tag, MPI_Comm *output_comm)
{
  rempi_msgb_request *msgb_request;
  list<rempi_msgb_request*>::iterator it, it_end;
  for (it = inactive_recv_list.begin(); it != inactive_recv_list.end(); it++) {
    msgb_request = *it;
    if (msgb_request->status.MPI_SOURCE == matched_rank && 
	msgb_request->clock == matched_clock) {
      *output_tag  = msgb_request->status.MPI_TAG;
      *output_comm = msgb_request->recv_args->comm;
      return 1;
    }
  }
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




