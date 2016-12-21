#include <mpi.h>

#if MPI_VERSION == 3 && !defined(REMPI_LITE)

#include <stdio.h>
#include <string.h>

#include <unordered_map>
#include <string>


#include "rempi_recorder.h"
#include "rempi_msg_buffer.h"
#include "rempi_event.h"
#include "rempi_event_list.h"
#include "rempi_io_thread.h"
#include "rempi_util.h"
#include "rempi_err.h"
#include "rempi_config.h"
#include "rempi_mem.h"
#include "rempi_request_mg.h"
#include "rempi_cp.h"
#include "rempi_mf.h"
#include "rempi_clock.h"

#define COMPLETE_MF_SEND           (0)
#define COMPLETE_MF_UNMATCHED_RECV (1)
#define COMPLETE_MF_MATCHED_RECV   (2)

static int local_rank = -1;

void rempi_recorder_rep::init(int rank)
{ 
  string id;
  string path;

  local_rank = rank;
  rempi_clock_init();
  replaying_event_list = new rempi_event_list<rempi_event*>(10000000, 100);
  recording_event_list = new rempi_event_list<rempi_event*>(10000000, 100);
  id = std::to_string((long long int)rank);
  path = rempi_record_dir_path + "/rank_" + id + ".rempi";
  mc_encoder = new rempi_encoder_rep(rempi_mode, path);
  

  return;
}


/* 1. Finish all send request */
/* 2. if (local_clock > next recv event clock || blocking_mf) => matched */
/* 3. else (local_clock < next recv event clock && non-blocking): -> unmatched */

int rempi_recorder_rep::dequeue_matched_events(int matching_set_id)
{
  int event_list_status;
  list<rempi_event*> *matched_recv_event_list;
  rempi_event* replaying_event;

  if (matched_recv_event_list_umap.find(matching_set_id) == matched_recv_event_list_umap.end()) {
    matched_recv_event_list_umap[matching_set_id] = new list<rempi_event*>;
  }
  matched_recv_event_list = matched_recv_event_list_umap[matching_set_id];

  while ((replaying_event = replaying_event_list->dequeue_replay(matching_set_id, event_list_status)) != NULL) {
#ifdef REMPI_DBG_REPLAY
    REMPI_DBGI(REMPI_DBG_REPLAY, "RPQ->REPVEC  : (count: %d, with_next: %d, flag: %d, source: %d, clock: %d) matching_set_id: %d",
               replaying_event->get_event_counts(), replaying_event->get_is_testsome(), replaying_event->get_flag(),
               replaying_event->get_source(), replaying_event->get_clock(), matching_set_id);
#endif
    matched_recv_event_list->push_back(replaying_event);
  }

  return 0;
}

int rempi_recorder_rep::complete_mf_send_single(int incount, MPI_Request array_of_requests[], int *request_info,
						int matching_set_id, int matching_function_type, vector<rempi_event*> &replaying_event_vec)
{
  rempi_event *replaying_event;
  int matched_index;
  size_t min_clock = -1;
  int is_completed;
  size_t clock;
  int dest;


  for (int index = 0; index < incount; index++) {
    if (request_info[index] == REMPI_SEND_REQUEST) {
      clock = rempi_reqmg_get_send_request_clock(&array_of_requests[index]);
      if (min_clock == -1 || clock < min_clock) {
	min_clock     = clock;
	matched_index = index;
      }
    }
  }
  
  if (min_clock != -1) {
    dest = rempi_reqmg_get_send_request_dest(&array_of_requests[matched_index]);
    replaying_event = rempi_event::create_test_event(1,
						     1,
						     dest,
						     REMPI_MPI_EVENT_NOT_WITH_NEXT,
						     matched_index,
						     min_clock,
						     matching_set_id);
    replaying_event_vec.push_back(replaying_event);
    is_completed = 1;
  } else {
    is_completed = 0;
  }
  return is_completed;
}

int rempi_recorder_rep::complete_mf_send_all(int incount, MPI_Request array_of_requests[], int *request_info,
					     int matching_set_id, int matching_function_type, vector<rempi_event*> &replaying_event_vec)
{
  rempi_event *replaying_event;
  int is_completed = 0;
  int dest;
  size_t clock;


  for (int index = 0; index < incount; index++) {
    if (request_info[index] == REMPI_SEND_REQUEST) {
      dest  = rempi_reqmg_get_send_request_dest(&array_of_requests[index]);
      clock = rempi_reqmg_get_send_request_clock(&array_of_requests[index]);
      replaying_event = rempi_event::create_test_event(1,
						       1,
						       dest,
						       REMPI_MPI_EVENT_NOT_WITH_NEXT,
						       index,
						       clock,
						       matching_set_id);
      replaying_event_vec.push_back(replaying_event);
      is_completed = 1;
    }
  }
  return is_completed;
}


int rempi_recorder_rep::complete_mf_send(int incount, MPI_Request array_of_requests[], int *request_info,
					 int matching_set_id, int matching_function_type, vector<rempi_event*> &replaying_event_vec)
{
  int is_completed = 0;
  size_t clock;
  switch(matching_function_type) {
  case REMPI_MPI_TESTALL:
  case REMPI_MPI_WAITALL:
    is_completed = complete_mf_send_all(incount, array_of_requests, request_info, matching_set_id, matching_function_type, replaying_event_vec);
    break;
  default:
    is_completed = complete_mf_send_single(incount, array_of_requests, request_info, matching_set_id, matching_function_type, replaying_event_vec);
    break;
  }
  return is_completed;
}


int rempi_recorder_rep::is_behind_time(int matching_set_id)
{
  size_t look_ahead_recv_clock, recved_clock, local_clock;
  int    look_ahead_recv_rank, recved_rank;
  list<rempi_event*> *matched_recv_event_list;
  rempi_event *next_matched_event;
  int compare;

  rempi_clock_get_local_clock(&local_clock);
  matched_recv_event_list = matched_recv_event_list_umap.at(matching_set_id);
  if (matched_recv_event_list->size() > 0) {
    /* 
       Clock of matched_recv_event_list->front() should be minimum "receive" clock.
       So if matched_recv_event_list->size() > 0, we need to check the front() first
    */
    next_matched_event = matched_recv_event_list->front();
    recved_clock       = next_matched_event->get_clock();
    recved_rank        = next_matched_event->get_source();
    //    REMPI_DBGI(9, "local: %lu, recved: %lu, msid: %d", local_clock, recved_clock, matching_set_id);
    compare = compare_clock(local_clock, local_rank, recved_clock, recved_rank);
    if (compare > 0) {
      return 1;
    } else if (compare == 0) {
      REMPI_ERR("Receving same clock <clock:%lu, rank:%d)", local_clock, local_rank);
    } 
    return 0;
  }   
  mc_encoder->compute_look_ahead_recv_clock(&look_ahead_recv_clock, &look_ahead_recv_rank, matching_set_id);    
  compare = compare_clock(local_clock, local_rank, look_ahead_recv_clock, look_ahead_recv_rank);
  // REMPI_DBGI(9, "(clock:%lu, rank:%d, LA_clock: %lu, LA_rank: %d): compare: %d", 
  //  	     local_clock, local_rank, look_ahead_recv_clock, look_ahead_recv_rank, compare);

  // if (compare > 0 || look_ahead_recv_clock != REMPI_CLOCK_COLLECTIVE_CLOCK) {
  if (compare >= 0 || look_ahead_recv_clock == REMPI_CLOCK_COLLECTIVE_CLOCK) {
    return 1;
  } else if (compare == 0) {
    REMPI_ERR("May receve same clock (clock:%lu, rank:%d, LA_clock: %lu, LA_rank: %d)", 
     	      local_clock, local_rank, look_ahead_recv_clock, look_ahead_recv_rank);
  }
  return 0;
}

int rempi_recorder_rep::complete_mf_unmatched_recv_all(int incount, MPI_Request array_of_requests[], int *request_info,
						       int matching_set_id, int matching_function_type, vector<rempi_event*> &replaying_event_vec)
{
  return complete_mf_unmatched_recv_single(incount, array_of_requests, request_info, matching_set_id, matching_function_type, replaying_event_vec);
}

int rempi_recorder_rep::complete_mf_unmatched_recv_single(int incount, MPI_Request array_of_requests[], int *request_info,
							  int matching_set_id, int matching_function_type, vector<rempi_event*> &replaying_event_vec)
{
  int is_completed = 0;
  size_t local_clock;
  rempi_event* replaying_event;
  rempi_event* *matched_event;

  if (is_behind_time(matching_set_id)) {
    replaying_event = rempi_event::create_test_event(1, 0,
  						     REMPI_MPI_EVENT_INPUT_IGNORE,
  						     REMPI_MPI_EVENT_INPUT_IGNORE,
  						     REMPI_MPI_EVENT_INPUT_IGNORE,
  						     REMPI_MPI_EVENT_INPUT_IGNORE,
  						     REMPI_MPI_EVENT_INPUT_IGNORE);
    replaying_event_vec.push_back(replaying_event);
    is_completed = 1;
  } else {
    is_completed = 0;
  }

  // list<rempi_event*> *matched_recv_event_list;
  // matched_recv_event_list = matched_recv_event_list_umap.at(matching_set_id);
  // REMPI_DBG("size: %lu (SID: %d)", matched_recv_event_list->size(), matching_set_id);
  return is_completed;
}

int rempi_recorder_rep::complete_mf_unmatched_recv(int incount, MPI_Request array_of_requests[], int *request_info,
						   int matching_set_id, int matching_function_type, vector<rempi_event*> &replaying_event_vec)
{
  int is_behind;
  int is_completed = 0;
  switch(matching_function_type) {
  case REMPI_MPI_WAIT:
  case REMPI_MPI_WAITANY:
  case REMPI_MPI_WAITSOME:
  case REMPI_MPI_WAITALL:
    is_behind = is_behind_time(matching_set_id);
    if (is_behind && rempi_encode == REMPI_ENV_REMPI_ENCODE_REP) {
      rempi_clock_sync_clock(0, NULL, NULL, NULL, matching_function_type);
    }
    break;
  case REMPI_MPI_TEST:
  case REMPI_MPI_TESTANY:
  case REMPI_MPI_TESTSOME:
    is_completed = complete_mf_unmatched_recv_single(incount, array_of_requests, request_info, matching_set_id, matching_function_type, replaying_event_vec);
    break;
  case REMPI_MPI_TESTALL:
    is_completed = complete_mf_unmatched_recv_all(incount, array_of_requests, request_info, matching_set_id, matching_function_type, replaying_event_vec);
    break;
  default:
    REMPI_ERR("No such matching function type");
    break;
  }
  return is_completed;
}

int rempi_recorder_rep::get_mf_matched_index(int incount, MPI_Request array_of_requests[], int *request_info, int exclusion_flags[], int recved_rank, size_t recved_clock, int matching_set_id, int matching_function_type)
{
  int matched_index = -1;
  int requested_source, requested_tag;
  MPI_Comm requested_comm;
  int matched_condition;
  int matched_tag;
  MPI_Comm matched_comm;

  if (!rempi_msgb_get_tag_comm(recved_rank, recved_clock, &matched_tag, &matched_comm)) {
    REMPI_ERR("No such matched message");
  }


  for (int i = 0; i < incount; i++) {
    //    REMPI_DBGI(9, "index: %d, count: %d", i, incount);
    if (request_info[i] == REMPI_RECV_REQUEST && !exclusion_flags[i]) {
      rempi_reqmg_get_matching_id(&array_of_requests[i], &requested_source, &requested_tag, &requested_comm);
      // REMPI_DBGI(9, "requested_rank: %d, requested_tag: %d, recved_rank: %d, matched_tag: %d (sid: %d)", 
      //  		 requested_source, requested_tag, recved_rank, matched_tag, matching_set_id);
      matched_condition = rempi_msgb_is_matched(requested_source, requested_tag, requested_comm, 
						recved_rank, matched_tag, matched_comm);
      if (matched_condition != REMPI_MSGB_REQUEST_MATCHED_TYPE_NOT_MATCHED) {
	matched_index = i;
	break;
      }
    }
  }
  //  REMPI_DBGI(0, "matched_index: %d, rank: %d, clock: %lu", matched_index, recved_rank, recved_clock);
  return matched_index;
}

#if 1
int rempi_recorder_rep::complete_mf_matched_recv_single(int incount, MPI_Request array_of_requests[], int *request_info,
							int matching_set_id, int matching_function_type, vector<rempi_event*> &replaying_event_vec)
{
  int is_completed = 0;
  size_t recved_clock, local_clock;
  int recved_rank;
  int compare;
  list<rempi_event*> *matched_recv_event_list;
  rempi_event *next_matched_event;
  int matched_index;
  list<rempi_event*>::iterator it, it_end;

  matched_recv_event_list = matched_recv_event_list_umap.at(matching_set_id);

  for (int i = 0; i < incount; i++) exclusion_flags[i] = 0;

  //  REMPI_DBGI(0, "matched single: size: %lu", matched_recv_event_list->size());


  if (matched_recv_event_list->size() > 0) {
    rempi_clock_get_local_clock(&local_clock);  

    it     = matched_recv_event_list->begin();
    it_end = matched_recv_event_list->end();
    for (; it != it_end; it++) {
      next_matched_event = *it;
      recved_clock       = next_matched_event->get_clock();
      recved_rank        = next_matched_event->get_source();
      
      // compare = compare_clock(local_clock, local_rank, recved_clock, recved_rank);
      // if (compare >= 0) {
      // 	if (matching_function_type == REMPI_MPI_TEST ||
      // 	    matching_function_type == REMPI_MPI_TESTANY ||
      // 	    matching_function_type == REMPI_MPI_TESTSOME ||
      // 	    matching_function_type == REMPI_MPI_TESTALL) {
      // 	  REMPI_ERR("Clock is not correct <local_clock:%lu, local_rank:%d> <recved_clock:%lu, recved_rank:%d>", 
      // 		    local_clock, local_rank, recved_clock, recved_rank);
      // 	}
      // } 
      
      if ((matched_index = get_mf_matched_index(incount, array_of_requests, request_info, exclusion_flags, recved_rank, recved_clock, matching_set_id, matching_function_type)) >= 0) {
	next_matched_event->set_index(matched_index);    
	replaying_event_vec.push_back(next_matched_event);
	//	matched_recv_event_list->pop_front();
	matched_recv_event_list->erase(it);
	is_completed = 1;
	return is_completed;
      }
    }
  }     
  return is_completed;
}
#else

// int rempi_recorder_rep::complete_mf_matched_recv_single(int incount, MPI_Request array_of_requests[], int *request_info,
// 				    int matching_set_id, int matching_function_type, vector<rempi_event*> &replaying_event_vec)
// {
//   int is_completed = 0;
//   size_t recved_clock, local_clock;
//   int recved_rank;
//   int compare;
//   list<rempi_event*> *matched_recv_event_list;
//   rempi_event *next_matched_event;
//   int matched_index;

//   matched_recv_event_list = matched_recv_event_list_umap.at(matching_set_id);

//   for (int i = 0; i < incount; i++) exclusion_flags[i] = 0;

//   //  REMPI_DBGI(0, "matched single: size: %lu", matched_recv_event_list->size());


//   if (matched_recv_event_list->size() > 0) {
//     rempi_clock_get_local_clock(&local_clock);  
//     next_matched_event = matched_recv_event_list->front();
//     recved_clock       = next_matched_event->get_clock();
//     recved_rank        = next_matched_event->get_source();

//     compare = compare_clock(local_clock, local_rank, recved_clock, recved_rank);
//     if (compare >= 0) {
//       if (matching_function_type == REMPI_MPI_TEST ||
// 	  matching_function_type == REMPI_MPI_TESTANY ||
// 	  matching_function_type == REMPI_MPI_TESTSOME ||
// 	  matching_function_type == REMPI_MPI_TESTALL) {
// 	REMPI_ERR("Clock is not correct <local_clock:%lu, local_rank:%d> <recved_clock:%lu, recved_rank:%d>", 
// 		  local_clock, local_rank, recved_clock, recved_rank);
//       }
//     } 

//     if ((matched_index = get_mf_matched_index(incount, array_of_requests, request_info, exclusion_flags, recved_rank, recved_clock, matching_set_id, matching_function_type)) < 0) {
//       //      REMPI_ERR("No such matched message: rank: %d, clock: %lu", recved_rank, recved_clock);
//       return is_completed;
//     }
//     next_matched_event->set_index(matched_index);    
//     replaying_event_vec.push_back(next_matched_event);
//     matched_recv_event_list->pop_front();
//     is_completed = 1;
//   }     
//   return is_completed;
// }
#endif



int rempi_recorder_rep::complete_mf_matched_recv_all(int incount, MPI_Request array_of_requests[], int *request_info,
						     int matching_set_id, int matching_function_type, vector<rempi_event*> &replaying_event_vec)
{
  int is_completed = 0;
  size_t recved_clock, local_clock;
  int recved_rank;
  int compare;
  list<rempi_event*> *matched_recv_event_list;
  rempi_event *next_matched_event;
  int matched_index;
  list<rempi_event*>::iterator it, it_end;
  int matched_count =0;

  matched_recv_event_list = matched_recv_event_list_umap.at(matching_set_id);
  if (matched_recv_event_list->size() < incount) return is_completed;
  rempi_clock_get_local_clock(&local_clock);  


  for (int i = 0; i < incount; i++) exclusion_flags[i] = 0;

  for (it = matched_recv_event_list->begin(), it_end = matched_recv_event_list->end(); 
       it != it_end; it++) {
    next_matched_event = *it;
    recved_clock       = next_matched_event->get_clock();
    recved_rank        = next_matched_event->get_source();
    if ((matched_index = get_mf_matched_index(incount, array_of_requests, request_info, exclusion_flags, recved_rank, recved_clock, matching_set_id, matching_function_type)) < 0) {
      REMPI_ERR("No such matched message");
    }
    matched_count++;
    exclusion_flags[matched_index] = 1;
    next_matched_event->set_index(matched_index);    
    replaying_event_vec.push_back(next_matched_event);
    if (matched_count == incount) break;
  }

  if (matched_count == incount) {
    for (int i = 0; i < matched_count; i++) {
      matched_recv_event_list->remove(replaying_event_vec[i]);
    }
    is_completed = 1;    
  } else {
    replaying_event_vec.clear();
    is_completed = 0;
  }
  return is_completed;
}

int rempi_recorder_rep::complete_mf_matched_recv(int incount, MPI_Request array_of_requests[], int *request_info,
						 int matching_set_id, int matching_function_type, vector<rempi_event*> &replaying_event_vec)
{
  int is_completed = 0;
  switch(matching_function_type) {
  case REMPI_MPI_WAIT:
  case REMPI_MPI_WAITANY:
  case REMPI_MPI_WAITSOME:
  case REMPI_MPI_TEST:
  case REMPI_MPI_TESTANY:
  case REMPI_MPI_TESTSOME:
    is_completed = complete_mf_matched_recv_single(incount, array_of_requests, request_info, matching_set_id, matching_function_type, replaying_event_vec);
    break;
  case REMPI_MPI_WAITALL:
  case REMPI_MPI_TESTALL:
    is_completed = complete_mf_matched_recv_all(incount, array_of_requests, request_info, matching_set_id, matching_function_type, replaying_event_vec);
    break;
  default:
    REMPI_ERR("No such matching function type");
    break;
  }
  
  return is_completed;
}


int rempi_recorder_rep::dequeue_replay_event_set(int incount, MPI_Request array_of_requests[], int *request_info, int matching_set_id, int matching_function_type, vector<rempi_event*> &replaying_event_vec)
{
  int howto_complete;
  rempi_event *handle_event = NULL;
  int is_completed;

  this->dequeue_matched_events(matching_set_id);
  howto_complete = COMPLETE_MF_SEND;
  switch(howto_complete) {
  case COMPLETE_MF_SEND:
    is_completed = complete_mf_send(incount, array_of_requests, request_info, matching_set_id, matching_function_type, replaying_event_vec);
    if (is_completed) break;
  case COMPLETE_MF_UNMATCHED_RECV:
    is_completed = complete_mf_unmatched_recv(incount, array_of_requests, request_info, matching_set_id, matching_function_type, replaying_event_vec);
    if (is_completed) break;
  case COMPLETE_MF_MATCHED_RECV:
    is_completed = complete_mf_matched_recv(incount, array_of_requests, request_info, matching_set_id, matching_function_type, replaying_event_vec);
    break;
  default:
    REMPI_ERR("Unknow event handling");
  }  


  return is_completed;
}


int rempi_recorder_rep::record_pf(int source,
				  int tag,
				  MPI_Comm comm,
				  int *flag,
				  MPI_Status *status,
				  int probe_function_type)
{
  int ret;
  ret = rempi_mpi_pf(source, tag, comm, flag, status, probe_function_type);
  return ret;
}

int rempi_recorder_rep::replay_pf(int source, int tag, MPI_Comm comm, int *flag, MPI_Status *status, int probe_function_type)
{

  rempi_event *event = NULL;
  size_t matching_set_id;
  list<rempi_event*> *matched_recv_event_list;
  list<rempi_event*>::iterator it;
  list<rempi_event*>::iterator it_end;
  int howto_matched;
  int interval = 0;
  int is_completed = 0;


  matching_set_id = rempi_reqmg_get_matching_set_id(tag, comm);

  while (!is_completed) {
    this->progress_recv_and_safe_update_local_look_ahead_recv_clock(
                                                                    (interval++ % 10 == 0)?
                                                                    REMPI_RECORDER_DO_FETCH_REMOTE_LOOK_AHEAD_SEND_CLOCKS:
                                                                    REMPI_RECORDER_DO_NOT_FETCH_REMOTE_LOOK_AHEAD_SEND_CLOCKS,
                                                                    0, NULL, matching_set_id);
#ifdef REMPI_MAIN_THREAD_PROGRESS
    mc_encoder->progress_decoding(recording_event_list, replaying_event_list, matching_set_id);
#endif

    this->dequeue_matched_events(matching_set_id);
  
    switch(probe_function_type) {
    case REMPI_MPI_IPROBE:
      if(is_behind_time(matching_set_id)) { 
	is_completed = 1;
	*flag = 0;
      } 
      rempi_clock_sync_clock(0, NULL, NULL, NULL, probe_function_type);      
      if (is_completed) break;
    case REMPI_MPI_PROBE:
      matched_recv_event_list = matched_recv_event_list_umap.at(matching_set_id);
      it     = matched_recv_event_list->begin();
      it_end = matched_recv_event_list->end();
      for (;it != it_end; it++) {
	event = *it;
	/*TODO: add real comm for curner case */
	howto_matched = rempi_msgb_is_matched(source, tag, comm, event->get_source(), tag, comm);
	if (howto_matched != REMPI_MSGB_REQUEST_MATCHED_TYPE_NOT_MATCHED) {
	  is_completed = 1;
	  *status = rempi_msgb_get_inactive_status(event->get_source(), tag, comm);
	  if (flag != NULL) *flag = 1;
	  break;
	}
      }
      break;
    default:
      REMPI_ERR("Unknow event handling");
    }


    if (!is_completed) {
      mc_encoder->update_local_look_ahead_send_clock(REMPI_ENCODER_REPLAYING_TYPE_RECV, matching_set_id);
    }
  } 

  return MPI_SUCCESS;


#if 0
  int tmp_flag;
  do {
    rempi_msgb_probe_msg(source, tag, comm, &tmp_flag, status);
    REMPI_DBGI(0, "probe_type: %d, flag: %d", probe_function_type, tmp_flag);
  } while (probe_function_type == REMPI_MPI_PROBE && tmp_flag == 0);
  if (probe_function_type == REMPI_MPI_IPROBE) *flag = tmp_flag;
  return MPI_SUCCESS;
#endif
}



#endif /* MPI_LITE */
