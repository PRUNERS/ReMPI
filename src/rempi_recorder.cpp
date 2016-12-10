#include <stdio.h>

#include <unordered_map>

#include "mpi.h"

//#include <iostream>
#include "rempi_recorder.h"

#include "rempi_message_manager.h"
#include "rempi_event.h"
#include "rempi_event_list.h"
#include "rempi_io_thread.h"
#include "rempi_util.h"
#include "rempi_err.h"
#include "rempi_config.h"
#include "rempi_type.h"
#include "rempi_request_mg.h"
#include "rempi_sig_handler.h"
#include "rempi_mf.h"


void rempi_recorder::update_validation_code(int incount, int *outcount, int matched_count, int *array_of_indices, MPI_Status *array_of_statuses, int *request_info)
{
  int index = 0;
  if (outcount != NULL) {
    validation_code = rempi_hash(validation_code, *outcount);
    //    REMPI_DBGI(REMPI_DBG_REPLAY, "= Record/Replay: validation_code: outcount: %d", *outcount);
  }
  validation_code = rempi_hash(validation_code, matched_count);
  //  REMPI_DBGI(REMPI_DBG_REPLAY, "= Record/Replay: validation_code: matched_count: %d", matched_count);
  //  fprintf(stderr, "C:%d:", matched_count);
  for (int i = 0; i < matched_count; i++) {
    if (array_of_indices != NULL) {
      index = array_of_indices[i];
      validation_code = rempi_hash(validation_code, index);
      //      REMPI_DBGI(REMPI_DBG_REPLAY, "= Record/Replay: validation_code: index: %d", index);
    } else {
      index = i;
    }
    if (request_info != NULL) {
      if (request_info[index] != REMPI_SEND_REQUEST) {
	validation_code = rempi_hash(validation_code, array_of_statuses[i].MPI_SOURCE);
	validation_code = rempi_hash(validation_code, array_of_statuses[i].MPI_TAG);
	// REMPI_DBGI(REMPI_DBG_REPLAY, "= Record/Replay: validation_code: source: %d, tag: %d REQ: %d %d %p", 
	// 	   array_of_statuses[i].MPI_SOURCE, array_of_statuses[i].MPI_TAG,
	// 	   request_info[index], index, array_of_indices);
      }
    } 
    //    fprintf(stderr, "I:%d(%d,%d) ", index, array_of_statuses[i].MPI_SOURCE, array_of_statuses[i].MPI_TAG);
  }
  //  fprintf(stderr, "\n");
#ifdef REMPI_DBG_REPLAY
  REMPI_DBGI(REMPI_DBG_REPLAY, "= Record/Replay: validation_code result: %lu", validation_code);
#endif
  return;  
}

void rempi_recorder::print_event(rempi_event *e)
{
  REMPI_ALT("= Event : (count: %d, flag: %d, rank: %d, with_next: %d, index: %d, msg_id: %d) REQ: %d",
	    e->get_event_counts(),
	    e->get_flag(),
	    e->get_source(),
	    e->get_is_testsome(),
	    e->get_index(),
	    e->get_clock(),
	    e->get_type());
  return;
}

void rempi_recorder::print_event_vec(vector<rempi_event*> &event_vec)
{
  REMPI_ALT("== Event list ==");
  for (int i = 0; i < event_vec.size(); i++) {
    print_event(event_vec[i]);
  }
  return;
}


int rempi_recorder::rempi_mf(int incount,
			     MPI_Request array_of_requests[],
			     int *outcount,
			     int array_of_indices[],
			     MPI_Status array_of_statuses[],
			     size_t **msg_id, // or clock
			     int *request_info,
			     int matching_function_type)
{
  int ret;
  ret = rempi_mpi_mf(incount, array_of_requests, outcount, array_of_indices, array_of_statuses, matching_function_type);
  *msg_id =  NULL;
  return ret;
}
  
int rempi_recorder::rempi_pf(int source,
		     int tag,
		     MPI_Comm comm,
		     int *flag,
		     MPI_Status *status,
		     size_t *msg_id, // or clock                                                                                                                
		     int prove_function_type)
{
  int ret;
  ret = rempi_mpi_pf(source, tag, comm, flag, status, prove_function_type);
  *msg_id =  REMPI_MPI_EVENT_INPUT_IGNORE;
  return ret;
}

void rempi_recorder::init(int rank)
{
  string id;
  string path;

  replaying_event_list = new rempi_event_list<rempi_event*>(10000000, 100);
  recording_event_list = new rempi_event_list<rempi_event*>(10000000, 100);

  id = std::to_string((long long int)rank);
  path = rempi_record_dir_path + "/rank_" + id + ".rempi";
  mc_encoder = new rempi_encoder_basic(rempi_mode, path);
  return;
}


int rempi_recorder::record_init(int *argc, char ***argv, int rank) 
{
  this->init(rank);
  record_thread = new rempi_io_thread(recording_event_list, replaying_event_list, rempi_mode, mc_encoder); //0: recording mode
  rempi_sig_handler_init(rank, record_thread, recording_event_list, &validation_code);
  record_thread->start();
  return 0;
}

int rempi_recorder::replay_init(int *argc, char ***argv, int rank) 
{
  this->init(rank);
  read_record_thread = new rempi_io_thread(recording_event_list, replaying_event_list, rempi_mode, mc_encoder); //1: replay mode
  rempi_sig_handler_init(rank, record_thread, recording_event_list, &validation_code);
  read_record_thread->start();
  return 0;
}


int rempi_recorder::record_isend(mpi_const void *buf,
		 int count,
		 MPI_Datatype datatype,
		 int dest,
		 int tag,
		 MPI_Comm comm,
		 MPI_Request *request,
		 int send_function_type)
{
  int ret;
  int resultlen;
  int matching_set_id;

  //  REMPI_DBG("Send: Rank %d (TAG: %d)", dest, tag);
  ret = rempi_mpi_isend(buf, count, datatype, dest, tag, comm, request, send_function_type);
  rempi_reqmg_register_request(buf, count, datatype, dest, tag, comm, request, REMPI_SEND_REQUEST, &matching_set_id);
  return ret;
}

int rempi_recorder::replay_isend(mpi_const void *buf,
		 int count,
		 MPI_Datatype datatype,
		 int dest,
		 int tag,
		 MPI_Comm comm,
		 MPI_Request *request,
		 int send_function_type)
{
  int ret;
  int matching_set_id;

  ret = record_isend(buf, count, datatype, dest, tag, comm , request, send_function_type);
  //  REMPI_DBG("sending to rank %d with %p", dest, *request);
  rempi_reqmg_register_request(buf, count, datatype, dest, tag, comm, request,  REMPI_SEND_REQUEST, &matching_set_id);

  return ret; 
}

int rempi_recorder::record_irecv(
   void *buf,
   int count,
   MPI_Datatype datatype,
   int source,
   int tag,
   int comm_id, // The value is set by MPI_Comm_set_name in ReMPI_convertor
   MPI_Comm comm,
   MPI_Request *request)
{
  rempi_event *e;
  int ret;
  int matching_set_id;

  ret = PMPI_Irecv(buf, count, datatype, source, tag, comm, request);
  rempi_reqmg_register_request(buf, count, datatype, source, tag, comm, request, REMPI_RECV_REQUEST, &matching_set_id);


  e = rempi_event::create_recv_event(MPI_ANY_SOURCE, tag, NULL, request);
  recording_event_list->push(e);
  if (request_to_recv_event_umap.find(*request) != request_to_recv_event_umap.end()) {
    REMPI_ERR("Recv event of request(%p) already exists", *request);
  }
  request_to_recv_event_umap[*request] = e;

  return ret;
}

// int rempi_recorder::replay_irecv(
//    void *buf,
//    int count,
//    MPI_Datatype datatype,
//    int source,
//    int tag,
//    int comm_id, // The value is set by MPI_Comm_set_name in ReMPI_convertor
//    MPI_Comm *comm, // The value is set by MPI_Comm_set_name in ReMPI_convertor
//    MPI_Request *request)
// {
//   return MPI_SUCCESS;
// }

size_t request_id = 1;

int rempi_recorder::replay_irecv(
   void *buf,
   int count,
   MPI_Datatype datatype,
   int source,
   int tag,
   int comm_id, // The value is set by MPI_Comm_set_name in ReMPI_convertor
   MPI_Comm comm, // The value is set by MPI_Comm_set_name in ReMPI_convertor
   MPI_Request *request)
{
  int ret;

  rempi_event *replaying_recv_event;
  int replaying_queue_status;
  int replaying_source;
  int matching_set_id;
  int file_closed = 0;
  

  //  REMPI_DBGI(1, "Irecv: %p", *request);

#ifdef BGQ
  memset(request, request_id, sizeof(MPI_Request));
  request_id++;
  REMPI_ERR("TODO: remove this")
    //    REMPI_DBG("request_id: %lu request: %p", request_id, *request);
    //    *request = (MPI_Request)(request_id++);
#else
    //  *request = (MPI_Request)rempi_malloc(sizeof(MPI_Request));//((source + 1) * (tag + 1) * (comm_id * 1));
    //    memset(request, request_id, sizeof(MPI_Request));
    // *request = (MPI_Request)request_id;
    // request_id++;
#endif



    while ((replaying_recv_event = replaying_event_list->dequeue_replay(0, replaying_queue_status)) == NULL) {
      // if (file_closed == 1) {
      // 	REMPI_ALT("No more record file");
      // 	exit(15);
      // }
      // file_closed = mc_encoder->is_file_closed();
    }

  // REMPI_DBG("Replay : (count: %d, type: %d, flag: %d, rank: %d, with_next: %d, index: %d, msg_id: %d, gid: %d)",
  // 	    replaying_recv_event->get_event_counts(),
  // 	    replaying_recv_event->get_type(),
  // 	    replaying_recv_event->get_flag(),
  // 	    replaying_recv_event->get_rank(),
  // 	    replaying_recv_event->get_with_next(),
  // 	    replaying_recv_event->get_index(),
  // 	    replaying_recv_event->get_msg_id(),
  // 	    replaying_recv_event->get_matching_group_id());

  if (replaying_recv_event->get_type() == REMPI_MPI_EVENT_TYPE_RECV) {
    replaying_source = (replaying_recv_event->get_rank() == REMPI_MPI_EVENT_RANK_CANCELED)? source:replaying_recv_event->get_rank();
    ret = PMPI_Irecv(buf, count, datatype, replaying_source, tag, comm, request);  
    //    REMPI_DBG("register: %p: source: %d, tag: %d", *request,replaying_source, tag);
    rempi_reqmg_register_request(buf, count, datatype, source, tag, comm, request, REMPI_RECV_REQUEST, &matching_set_id);
    //    delete replaying_recv_event;
    
  } else {
    REMPI_ALT("MPI_Recv/Irecv should not be called according to record: %d"
	      "  (MPI_Recv/Irecv: %d, Matching function: %d, Probing function: %d)", replaying_recv_event->get_type(), 
	      REMPI_MPI_EVENT_TYPE_RECV, REMPI_MPI_EVENT_TYPE_TEST, REMPI_MPI_EVENT_TYPE_PROB);
    REMPI_ASSERT(0);
  }




  return MPI_SUCCESS;
}



int rempi_recorder::record_cancel(MPI_Request *request)
{
  int ret;
  int request_type;
  rempi_reqmg_get_request_type(request, &request_type);
  if (request_type == REMPI_RECV_REQUEST ||
      request_type == REMPI_SEND_REQUEST) {
    rempi_reqmg_deregister_request(request, request_type);
  }

  request_to_recv_event_umap.at(*request)->set_rank(REMPI_MPI_EVENT_RANK_CANCELED);
  if (request_to_recv_event_umap.erase(*request) == 0) {
    REMPI_ERR("Recv event of request(%p) does not exist", *request);
  }
  ret = PMPI_Cancel(request);


  return ret;
}

int rempi_recorder::replay_cancel(MPI_Request *request)
{
  int ret;
  int request_type;
  rempi_reqmg_get_request_type(request, &request_type);
  if (request_type != REMPI_RECV_REQUEST) {
    REMPI_ERR("only recv request can be canceled: request_type: %d", request_type);
  }
  ret = PMPI_Cancel(request);
  if (request_type == REMPI_RECV_REQUEST) {
    cancel_request(request);
    ret = MPI_SUCCESS;
  } else if (request_type == REMPI_SEND_REQUEST) {
    cancel_request(request);
    ret = PMPI_Cancel(request);
  }
  return ret;
}




void rempi_recorder::cancel_request(MPI_Request *request)
{
  int ret;
  int request_type;
  
  rempi_reqmg_get_request_type(request, &request_type);
  rempi_reqmg_deregister_request(request, request_type);

  if (request_type == REMPI_RECV_REQUEST) {
    *request = MPI_REQUEST_NULL;
  }
  return;
}




int rempi_recorder::replay_request_free(MPI_Request *request)
{
  int ret;
  int request_type;
  
  rempi_reqmg_get_request_type(request, &request_type);
  if (request_type == REMPI_RECV_REQUEST ||
      request_type == REMPI_SEND_REQUEST) {
    rempi_reqmg_deregister_request(request, request_type);
  } 
  ret = PMPI_Request_free(request);
  return ret;
}



#if PMPI_Request_c2f != MPI_Fint && MPI_Request_c2f != MPI_Fint


extern "C" MPI_Fint PMPI_Request_c2f(MPI_Request request);
MPI_Fint rempi_recorder::replay_request_c2f(MPI_Request request)
{
  MPI_Fint ret;
  int request_type;
  MPI_Request deregist_rempi_request;
  rempi_reqmg_recv_args *recv_args;

  rempi_reqmg_get_request_type(&request, &request_type);
  
  if (request_type == REMPI_RECV_REQUEST) {
    recv_args = rempi_reqmg_get_recv_args(request);
    deregist_rempi_request = request;
    PMPI_Irecv(
	       recv_args->buffer,
	       recv_args->count,
	       recv_args->datatype,
	       recv_args->source,
	       recv_args->tag,
	       recv_args->comm, 
	       &request);
    cancel_request(&deregist_rempi_request);
  } else if (request_type == REMPI_SEND_REQUEST) {
    cancel_request(&request);
  }
  ret = PMPI_Request_c2f(request);

  return ret;
}



#else
MPI_Fint rempi_recorder::replay_request_c2f(MPI_Request request)
{
  REMPI_ERR("%s should not be called", __func__);
  return 0;
}
#endif


#if 1
int rempi_recorder::record_mf(
			      int incount,
			      MPI_Request array_of_requests[],
			      int *outcount,
			      int array_of_indices[],
			      MPI_Status array_of_statuses[],
			      int matching_function_type)
{
  int ret;
  int flag;
  rempi_event *test_event = NULL;
  int is_with_next;
  int matched_count;
  size_t *msg_id;
  int sendcount, recvcount, nullcount;
  int was_array_of_indices_null = 0;
  int is_record_and_replay;
  int global_test_id;
  int matched_index;


  if (incount > PRE_ALLOCATED_REQUEST_LENGTH) {
    REMPI_ERR("incount: %d is larger than buffer", incount);
  }
  
  rempi_reqmg_get_request_info(incount, array_of_requests, &sendcount, &recvcount, &nullcount, request_info, &is_record_and_replay, &global_test_id);
  if (is_record_and_replay) {
    rempi_reqmg_store_send_statuses(incount, array_of_requests, request_info, tmp_statuses);
  }
  


  for (int i = 0; i < incount; i++) tmp_requests[i] = array_of_requests[i];


  ret = this->rempi_mf(incount, array_of_requests, outcount, array_of_indices, array_of_statuses, 
		       &msg_id, request_info, matching_function_type);
  matched_count = rempi_mpi_get_matched_count(incount, outcount, nullcount, matching_function_type);

  if (is_record_and_replay == 0 || incount == 0 || incount == nullcount) {
    /* 
       incount == 0 => it's deterministic 
       incount == nullcount => it's deterministic
    */
    for (int i = 0; i < matched_count; i++) {
      matched_index = (array_of_indices==NULL)? i:array_of_indices[i];
      rempi_reqmg_deregister_request(&tmp_requests[matched_index], request_info[matched_index]);
    }
#ifdef REMPI_DBG_REPLAY
    //    REMPI_DBGI(REMPI_DBG_REPLAY, "  = Record: Skip: ");
#endif
    return ret;
  }

  //  global_test_id = rempi_reqmg_get_test_id(array_of_requests, incount);

  

  recording_event_set_list.clear();
  if (matched_count == 0) {
    flag = 0;
    test_event = rempi_event::create_test_event(1, 
						flag, 
						REMPI_MPI_EVENT_INPUT_IGNORE, 
						REMPI_MPI_EVENT_NOT_WITH_NEXT, 
						REMPI_MPI_EVENT_INPUT_IGNORE, 
						(msg_id==NULL)? REMPI_MPI_EVENT_INPUT_IGNORE:*msg_id,// => is supposed to be REMPI_MPI_EVENT_INPUT_IGNORE
						global_test_id);
    recording_event_set_list.push_back(test_event);
//     if (test_event != NULL) {
// #ifdef REMPI_DBG_REPLAY
//       REMPI_DBGI(REMPI_DBG_REPLAY, "= Record : (count: %d, flag: %d, rank: %d, with_next: %d, index: %d, msg_id: %d, gid: %d, e: %p) MF: %d",
//       		 test_event->get_event_counts(),
//       		 test_event->get_flag(),
//       		 test_event->get_rank(),
//       		 test_event->get_with_next(),
//       		 test_event->get_index(),
//       		 test_event->get_msg_id(),
//       		 test_event->get_matching_group_id(),
// 		 test_event,
//       		 matching_function_type);
// #endif
//     } else {
//       REMPI_ERR("test_event is NULL");
//     }
//     recording_event_list->push(test_event);

  } else {
    flag = 1;
    is_with_next = REMPI_MPI_EVENT_WITH_NEXT;
    for (int i = 0; i < matched_count; i++) {
      int rank = -1;
      if (array_of_indices != NULL) {
	/*MF: Some or Any */
	matched_index = array_of_indices[i];
      } else {
	/*MF: Single or All */
	matched_index = i;
      }


      if (i == matched_count - 1) {
	is_with_next = REMPI_MPI_EVENT_NOT_WITH_NEXT;
      }

      if (request_info[matched_index] == REMPI_SEND_REQUEST) {
	rank = tmp_statuses[matched_index].MPI_SOURCE;
	rempi_reqmg_deregister_request(&tmp_requests[matched_index], REMPI_SEND_REQUEST);
	if (msg_id != NULL) msg_id[i] = REMPI_MPI_EVENT_INPUT_IGNORE; /*To distingisu between send or recv event in encoder_cdc */
      } else if (request_info[matched_index] == REMPI_RECV_REQUEST) {
	rank = array_of_statuses[i].MPI_SOURCE;
	{
	  rempi_reqmg_recv_args *recv_args;
	  int datatype_size, count;
	  recv_args = rempi_reqmg_get_recv_args(&tmp_requests[matched_index]);
	  PMPI_Type_size(recv_args->datatype, &datatype_size);
	  PMPI_Get_count(&array_of_statuses[i], recv_args->datatype, &count);
	}
	rempi_reqmg_deregister_request(&tmp_requests[matched_index], REMPI_RECV_REQUEST);
	request_to_recv_event_umap.at(tmp_requests[matched_index])->set_rank(rank);
	request_to_recv_event_umap.erase(tmp_requests[matched_index]);
      } else if (request_info[matched_index] == REMPI_NULL_REQUEST) {
	/*if request is MPI_REQUEST_NULL, it matches */
	rank = array_of_statuses[i].MPI_SOURCE;
      } else {
	REMPI_ERR("unsupported MPI call");
      }

      //      if (rank >= 0) {
	test_event = rempi_event::create_test_event(1, 
						    flag,
						    rank,
						    is_with_next,
						    matched_index, 
						    // => is supposed to be REMPI_MPI_EVENT_INPUT_IGNORE
						    (msg_id==NULL)? REMPI_MPI_EVENT_INPUT_IGNORE:msg_id[i],
						    global_test_id);
	recording_event_set_list.push_back(test_event);
// 	if (test_event != NULL) {
// #ifdef REMPI_DBG_REPLAY
// 	  REMPI_DBGI(REMPI_DBG_REPLAY, "= Record : (count: %d, flag: %d, rank: %d(tag: %d), with_next: %d, index: %d, msg_id: %d, gid: %d) MF: %d",
// 	  	     test_event->get_event_counts(),
// 	  	     test_event->get_flag(),
// 	  	     test_event->get_rank(),
// 		     array_of_statuses[i].MPI_TAG,
// 	  	     test_event->get_with_next(),
// 	  	     test_event->get_index(),
// 	  	     test_event->get_msg_id(),
// 	  	     test_event->get_matching_group_id(),
// 	  	     matching_function_type);
// 	  REMPI_DBGI(REMPI_DBG_REPLAY, "matched_count: %d (i: %d)", matched_count, i);
// #endif
// 	}
// 	recording_event_list->push(test_event);
//      }
    }
 }

 recording_event_set_list.back()->set_with_next(REMPI_MPI_EVENT_NOT_WITH_NEXT);
 list<rempi_event*>::iterator it = recording_event_set_list.begin();
 list<rempi_event*>::iterator it_end = recording_event_set_list.end();
 int index = 0;
 for (;it != it_end; it++) {
	test_event = *it;
#ifdef REMPI_DBG_REPLAY
     	REMPI_DBGI(REMPI_DBG_REPLAY, "= Record : (count: %d, flag: %d, rank: %d, with_next: %d, index: %d, msg_id: %d) MF: %d, REQ: %d, SID: %d",
     test_event->get_event_counts(),
     test_event->get_flag(),
     test_event->get_rank(),
     test_event->get_with_next(),
     test_event->get_index(),
     test_event->get_msg_id(),
     matching_function_type,
     request_info[test_event->get_index()],
     test_event->get_matching_group_id()
     );
#endif
	  index++;
	recording_event_list->push(test_event);
 }
	//	for () {}

 update_validation_code(incount, outcount, matched_count, array_of_indices, array_of_statuses, request_info);


 
 return ret;
}
#else

int rempi_recorder::record_mf(
			      int incount,
			      MPI_Request array_of_requests[],
			      int *outcount,
			      int array_of_indices[],
			      MPI_Status array_of_statuses[],
			      int matching_function_type)
{
  int ret;
  int flag;
  rempi_event *test_event = NULL;
  int is_with_next;
  int matched_count;
  size_t *msg_id;
  int sendcount, recvcount, nullcount;
  int was_array_of_indices_null = 0;
  int is_record_and_replay;
  int global_test_id;
  int matched_index;


  if (incount > PRE_ALLOCATED_REQUEST_LENGTH) {
    REMPI_ERR("incount: %d is larger than buffer", incount);
  }
  
  rempi_reqmg_get_request_info(incount, array_of_requests, &sendcount, &recvcount, &nullcount, request_info, &is_record_and_replay, &global_test_id);
  if (is_record_and_replay) {
    rempi_reqmg_store_send_statuses(incount, array_of_requests, request_info, tmp_statuses);
  }
  


  for (int i = 0; i < incount; i++) tmp_requests[i] = array_of_requests[i];

  ret = this->rempi_mf(incount, array_of_requests, outcount, array_of_indices, array_of_statuses, 
		       &msg_id, request_info, matching_function_type);
  matched_count = rempi_mpi_get_matched_count(incount, outcount, matching_function_type);
  
  
  if (is_record_and_replay == 0) {
    for (int i = 0; i < matched_count; i++) {
      matched_index = (array_of_indices==NULL)? i:array_of_indices[i];
      rempi_reqmg_deregister_request(&tmp_requests[matched_index], request_info[matched_index]);
    }
#ifdef REMPI_DBG_REPLAY
    //    REMPI_DBGI(REMPI_DBG_REPLAY, "  = Record: Skip: ");
#endif
    return ret;
  }

  //  global_test_id = rempi_reqmg_get_test_id(array_of_requests, incount);

  


  if (matched_count == 0) {
    flag = 0;
    test_event = rempi_event::create_test_event(1, 
						flag, 
						REMPI_MPI_EVENT_INPUT_IGNORE, 
						REMPI_MPI_EVENT_NOT_WITH_NEXT, 
						REMPI_MPI_EVENT_INPUT_IGNORE, 
						(msg_id==NULL)? REMPI_MPI_EVENT_INPUT_IGNORE:*msg_id,// => is supposed to be REMPI_MPI_EVENT_INPUT_IGNORE
						global_test_id);
    if (test_event != NULL) {
#ifdef REMPI_DBG_REPLAY
      REMPI_DBGI(REMPI_DBG_REPLAY, "= Record : (count: %d, flag: %d, rank: %d, with_next: %d, index: %d, msg_id: %d, gid: %d, e: %p) MF: %d",
      		 test_event->get_event_counts(),
      		 test_event->get_flag(),
      		 test_event->get_rank(),
      		 test_event->get_with_next(),
      		 test_event->get_index(),
      		 test_event->get_msg_id(),
      		 test_event->get_matching_group_id(),
		 test_event,
      		 matching_function_type);
#endif
    } else {
      REMPI_ERR("test_event is NULL");
    }
    recording_event_list->push(test_event);

  } else {
    flag = 1;
    is_with_next = REMPI_MPI_EVENT_WITH_NEXT;
    for (int i = 0; i < matched_count; i++) {
      int rank = -1;
      if (array_of_indices != NULL) {
	/*MF: Some or Any */
	matched_index = array_of_indices[i];
      } else {
	/*MF: Single or All */
	matched_index = i;
      }

      // REMPI_DBGI(1, "request_info[%d] = %d", 0, request_info[0]);
      // REMPI_DBGI(1, " index: %d", matched_index);
      REMPI_DBGI(REMPI_DBG_REPLAY, "matched_count: %d (i: %d)", matched_count, i);
      if (i == matched_count - 1) {
	is_with_next = REMPI_MPI_EVENT_NOT_WITH_NEXT;
      }

      if (request_info[matched_index] == REMPI_SEND_REQUEST) {
	rank = tmp_statuses[matched_index].MPI_SOURCE;
	rempi_reqmg_deregister_request(&tmp_requests[matched_index], REMPI_SEND_REQUEST);
	if (msg_id != NULL) msg_id[i] = REMPI_MPI_EVENT_INPUT_IGNORE; /*To distingisu between send or recv event in encoder_cdc */
      } else if (request_info[matched_index] == REMPI_RECV_REQUEST) {
	rank = array_of_statuses[i].MPI_SOURCE;
	{
	  rempi_reqmg_recv_args *recv_args;
	  int datatype_size, count;
	  recv_args = rempi_reqmg_get_recv_args(&tmp_requests[matched_index]);
	  PMPI_Type_size(recv_args->datatype, &datatype_size);
	  PMPI_Get_count(&array_of_statuses[i], recv_args->datatype, &count);
// #ifdef REMPI_DBG_REPLAY
// 	  REMPI_DBGI(REMPI_DBG_REPLAY, "Record: dest_buff: %d", rempi_compute_hash(recv_args->buffer, datatype_size * count));
// #endif
	}
	rempi_reqmg_deregister_request(&tmp_requests[matched_index], REMPI_RECV_REQUEST);
	//	REMPI_DBG("update: %p", tmp_requests[matched_index]);
	//	if (request_to_recv_event_umap.find(tmp_requests[matched_index]) != 
	//	    request_to_recv_event_umap.end()) {
	request_to_recv_event_umap.at(tmp_requests[matched_index])->set_rank(rank);
	request_to_recv_event_umap.erase(tmp_requests[matched_index]);
	  //	}
      }

      if (rank >= 0) {
	test_event = rempi_event::create_test_event(1, 
						    flag,
						    rank,
						    is_with_next,
						    matched_index, 
						    // => is supposed to be REMPI_MPI_EVENT_INPUT_IGNORE
						    (msg_id==NULL)? REMPI_MPI_EVENT_INPUT_IGNORE:msg_id[i],
						    global_test_id);
	//	if (msg_id[i] < 10) {REMPI_ERR("clock is wrong: %lu", msg_id[i]);}
	if (test_event != NULL) {

#ifdef REMPI_DBG_REPLAY
	  REMPI_DBGI(REMPI_DBG_REPLAY, "= Record : (count: %d, flag: %d, rank: %d(tag: %d), with_next: %d, index: %d, msg_id: %d, gid: %d) MF: %d",
	  	     test_event->get_event_counts(),
	  	     test_event->get_flag(),
	  	     test_event->get_rank(),
		     array_of_statuses[i].MPI_TAG,
	  	     test_event->get_with_next(),
	  	     test_event->get_index(),
	  	     test_event->get_msg_id(),
	  	     test_event->get_matching_group_id(),
	  	     matching_function_type);
	  REMPI_DBGI(REMPI_DBG_REPLAY, "matched_count: %d (i: %d)", matched_count, i);
#endif
	}
	recording_event_list->push(test_event);
      }
    }
  }

  update_validation_code(incount, outcount, matched_count, array_of_indices, array_of_statuses, request_info);

  return ret;
}
#endif

int rempi_recorder::record_pf(int source,
				  int tag,
				  MPI_Comm comm,
				  int *flag,
				  MPI_Status *status,
				  int probe_function_type)
{
  rempi_event *test_event;
  char comm_id[REMPI_COMM_ID_LENGTH];
  int resultlen;
  size_t msg_id = REMPI_MPI_EVENT_INPUT_IGNORE;
  int global_test_id;
  int ret;
  MPI_Request dummy_req = NULL;
  int record_flag, record_source;

  /*TODO: Interface for NULL request. For now, use dummy_req*/
  rempi_reqmg_register_request(NULL, 0, MPI_BYTE, source, tag, comm, &dummy_req, REMPI_RECV_REQUEST, &global_test_id);
  //  global_test_id = rempi_reqmg_get_test_id(&dummy_req, 1);
  rempi_reqmg_deregister_request(&dummy_req, REMPI_RECV_REQUEST);

  /* Call MPI matching function */
  ret = this->rempi_pf(source, tag, comm, flag, status, &msg_id, probe_function_type);

  if (flag == NULL) {
    /*MPI_Probe*/
    record_flag = 1;    
    record_source = status->MPI_SOURCE;
  } else if (*flag == 0) {
    /*MPI_Iprobe unmatched*/
    record_flag = 0;
    record_source = REMPI_MPI_EVENT_INPUT_IGNORE;
  } else {
    /*MPI_Iprobe matched*/
    record_flag = 1;
    record_source = status->MPI_SOURCE;
  }
  test_event = rempi_event::create_test_event(1,
					      record_flag,
					      record_source,
					      REMPI_MPI_EVENT_NOT_WITH_NEXT, 
					      REMPI_MPI_EVENT_INPUT_IGNORE,
					      msg_id, // is supposed to be REMPI_MPI_EVENT_INPUT_IGNORE
					      global_test_id);
#ifdef REMPI_DBG_REPLAY
	  REMPI_DBGI(REMPI_DBG_REPLAY, "= Record : (count: %d, flag: %d, rank: %d, with_next: %d, index: %d, msg_id: %d, gid: %d) PF: %d",
	  	     test_event->get_event_counts(),
	  	     test_event->get_flag(),
	  	     test_event->get_rank(),
	  	     test_event->get_with_next(),
	  	     test_event->get_index(),
	  	     test_event->get_msg_id(),
	  	     test_event->get_matching_group_id(),
	  	     probe_function_type);
#endif
  recording_event_list->push(test_event);
  update_validation_code(1, flag, record_flag, NULL, status, NULL);
  return ret;
}



int rempi_recorder::replay_mf_input(
			    int incount,
			    MPI_Request array_of_requests[],
			    int *outcount,
			    int array_of_indices[],
			    MPI_Status array_of_statuses[],
			    vector<rempi_event*> &replyaing_event_vec,
			    int matching_set_id,
			    int matching_function_type)
{
  rempi_event *replaying_test_event;
  int local_outcount = 0;
  MPI_Status status;
  int index = 0;

  status.MPI_ERROR = 0;

  for (int i = 0, n = replaying_event_vec.size(); i < n; i++) {
    replaying_test_event = replaying_event_vec[i];
    if (replaying_test_event->get_type() != REMPI_MPI_EVENT_TYPE_TEST) {
      REMPI_ALT("A matching function should not be called according to record: %d\n"
		"  (MPI_Recv/Irecv: %d, Matching function: %d, Probing function: %d)", replaying_test_event->get_type(), 
		REMPI_MPI_EVENT_TYPE_RECV, REMPI_MPI_EVENT_TYPE_TEST, REMPI_MPI_EVENT_TYPE_PROB);
      REMPI_ASSERT(0);
    }

    if (replaying_test_event->get_flag() == 0) {
      /*Unmatched*/
      if (outcount != NULL) {
	*outcount = 0;
      }
      return 0;
    }


    if (incount == 0) {
      REMPI_ERR("incount == 0, but replaying request rank: %d index: %d, with_next: %d (MF: %d)", 
    		replaying_test_event->get_rank(), replaying_test_event->get_index(),
    		replaying_test_event->get_with_next(), matching_function_type);
    }

    index = replaying_test_event->get_index();

    if (request_info[index] == REMPI_SEND_REQUEST) {
      if (tmp_statuses[index].MPI_SOURCE != replaying_test_event->get_rank()) {
	REMPI_ALT("According to record, this MPI rank %d is supposed to send this MPI message to rank %d. But destination is rank %d instead (MF: %d, matched_index: %d, incount: %d, request: %p)", rempi_my_rank, replaying_test_event->get_rank(), tmp_statuses[index].MPI_SOURCE, matching_function_type, index, 
		  incount, array_of_requests[index]);
	print_event_vec(replaying_event_vec);
	REMPI_ASSERT(0);
      }
      rempi_reqmg_deregister_request(&array_of_requests[index], REMPI_SEND_REQUEST);      
      PMPI_Wait(&array_of_requests[index], &array_of_statuses[local_outcount]);
    } else if (request_info[index] == REMPI_RECV_REQUEST) {

      int requested_source, requested_tag;
      MPI_Comm requested_comm;
      void *requested_buffer;


      rempi_reqmg_get_matching_id(&array_of_requests[index], &requested_source, &requested_tag, &requested_comm);
      rempi_reqmg_deregister_request(&array_of_requests[index], REMPI_RECV_REQUEST);
      PMPI_Wait(&array_of_requests[index], &status);
      if ((requested_source != MPI_ANY_SOURCE && requested_source != status.MPI_SOURCE) ||
	  (requested_tag    != MPI_ANY_TAG    && requested_tag    != status.MPI_TAG)  ) {
	REMPI_ERR("Replaying recv event from rank %d, but src of this recv request is rank %d: request: %p, index: %d", 
		  replaying_test_event->get_rank(), requested_source,
		  array_of_requests[index], index);
      }
      array_of_statuses[local_outcount] = status;
      //      if (status.MPI_ERROR != 0) REMPI_ERR("err: ret: %d, erro: %d, MPI_SUCCESS: %d", ret, status.MPI_ERROR, MPI_SUCCESS);


    } else if (request_info[index] == REMPI_NULL_REQUEST) {
      PMPI_Wait(&array_of_requests[index], &status);
      array_of_statuses[local_outcount] = status;
      //      REMPI_ERR("MPI_REQUEST_NULL does not match any message: %d", request_info[index]);
    } else {
      REMPI_ERR("Trying replaying with Ignored MPI_Request: %d", request_info[index]);
    }

    if (array_of_indices != NULL) {
      array_of_indices[local_outcount]  = index;
    }

    array_of_requests[index] = MPI_REQUEST_NULL;
    (local_outcount)++;
  }

  if (outcount != NULL) {
    *outcount = local_outcount;
    if (matching_function_type == REMPI_MPI_TESTALL) {
      *outcount = 1;
    }
  }



  return 0;
}

int rempi_recorder::get_next_events(
				    int incount, 
				    MPI_Request *array_of_requests, 
				    int request_info[],
				    vector<rempi_event*> &replaying_event_vec, 
				    int matching_set_id,
				    int matching_function_type)
{
  int replay_queue_status;
  rempi_event *replaying_test_event;
  int file_closed = 0;
  do {
    while ((replaying_test_event = replaying_event_list->dequeue_replay(matching_set_id, replay_queue_status)) == NULL) {
      // if (file_closed == 1) {
      // 	REMPI_ALT("No more record file");
      // 	exit(15);
      // }
      // file_closed = mc_encoder->is_file_closed();
    }
    replaying_event_vec.push_back(replaying_test_event);
  } while(replaying_test_event->get_with_next() == REMPI_MPI_EVENT_WITH_NEXT);
  return 0;
}



static int recv_from2 = 0;


int rempi_recorder::replay_mf(
			      int incount,
			      MPI_Request array_of_requests[],
			      int *outcount,
			      int array_of_indices[],
			      MPI_Status array_of_statuses[],
			      int matching_function_type)
{
  int ret;
  int recvcount, sendcount, nullcount;
  size_t *msg_id;
  int is_record_and_replay;
  int matching_set_id;
  int matched_index, matched_count;
  

  
  if (incount > PRE_ALLOCATED_REQUEST_LENGTH) {
    REMPI_ERR("incount: %d is larger than buffer", incount);
  }  
  rempi_reqmg_get_request_info(incount, array_of_requests, &sendcount, &recvcount, &nullcount, request_info, &is_record_and_replay, &matching_set_id);

  if (is_record_and_replay == 0 || incount == 0 || incount == nullcount) {
    /*this mf function is not replayed*/
    for (int i = 0; i < incount; i++) tmp_requests[i] = array_of_requests[i];
    ret = this->rempi_mf(incount, array_of_requests, outcount, array_of_indices, array_of_statuses, &msg_id, request_info,  matching_function_type);
    matched_count = rempi_mpi_get_matched_count(incount, outcount, nullcount, matching_function_type);

    for (int i = 0; i < matched_count; i++) {
      matched_index = (array_of_indices==NULL)? i:array_of_indices[i];
      if (request_info[matched_index] == REMPI_SEND_REQUEST) {
	rempi_reqmg_deregister_request(&tmp_requests[matched_index], REMPI_SEND_REQUEST);
      } else if (request_info[matched_index] == REMPI_RECV_REQUEST) {
	rempi_reqmg_deregister_request(&tmp_requests[matched_index], REMPI_RECV_REQUEST);
      } 	
    }
#ifdef REMPI_DBG_REPLAY
    //    REMPI_DBGI(REMPI_DBG_REPLAY, "  = Replay: Skip: ");
#endif
    return ret;
  }

  rempi_reqmg_store_send_statuses(incount, array_of_requests, request_info, tmp_statuses);
  // for (int i = 0; i < incount; i++) {
  //   REMPI_DBGI(0, "dest[%d/%d]: %d (req: %p)", i, incount, tmp_statuses[i].MPI_SOURCE, array_of_requests[i]);
  // }


  replaying_event_vec.clear();
  this->get_next_events(incount, array_of_requests, request_info, replaying_event_vec, matching_set_id, matching_function_type);
  this->replay_mf_input(incount, array_of_requests, outcount, array_of_indices, array_of_statuses, replaying_event_vec, matching_set_id, matching_function_type);


  
  for (int j = 0; j < replaying_event_vec.size(); j++) {
    // REMPI_DBGI(1, "= Replay: (count: %d, with_next: %d, index: %d, flag: %d, source: %d, clock: %d): matching_set_id: %d ",
    //            replaying_event_vec[j]->get_event_counts(), replaying_event_vec[j]->get_is_testsome(), 
    // 	       replaying_event_vec[j]->get_index(), replaying_event_vec[j]->get_flag(),
    //            replaying_event_vec[j]->get_source(), replaying_event_vec[j]->get_clock(), matching_set_id);
#ifdef REMPI_DBG_REPLAY
    int type = request_info[replaying_event_vec[j]->get_index()];

    REMPI_DBGI(REMPI_DBG_REPLAY, "= Replay : (count: %d, flag: %d, rank: %d, with_next: %d, index: %d, msg_id: %d) MF: %d, REQ: %d, SID: %d",
               replaying_event_vec[j]->get_event_counts(), 
	       replaying_event_vec[j]->get_flag(),
               replaying_event_vec[j]->get_source(), 
	       replaying_event_vec[j]->get_is_testsome(), 
	       replaying_event_vec[j]->get_index(), 
	       replaying_event_vec[j]->get_clock(), 
	       matching_function_type,
	       type,
	       matching_set_id);
#endif
    delete replaying_event_vec[j];
  }


  matched_count = rempi_mpi_get_matched_count(incount, outcount, nullcount, matching_function_type);
  update_validation_code(incount, outcount, matched_count, array_of_indices, array_of_statuses, request_info);


  return MPI_SUCCESS;
}

int rempi_recorder::replay_pf(int source, int tag, MPI_Comm comm, int *flag, MPI_Status *status, int probe_function_type)
{
  rempi_event *replaying_test_event;
  int has_next_event = REMPI_MPI_EVENT_WITH_NEXT;
  int test_id;
  int replay_queue_status;
  MPI_Request dummy_req = NULL;
  int file_closed = 0;

  rempi_reqmg_register_request(NULL, 0, MPI_BYTE, source, tag, comm, &dummy_req, REMPI_RECV_REQUEST, &test_id);
  //  test_id = rempi_reqmg_get_test_id(&dummy_req, 1);
  rempi_reqmg_deregister_request(&dummy_req, REMPI_RECV_REQUEST);



  /*Matched*/
  if (flag != NULL) *flag = 0;
  while (has_next_event == REMPI_MPI_EVENT_WITH_NEXT) {
    while ((replaying_test_event = replaying_event_list->dequeue_replay(test_id, replay_queue_status)) == NULL) {
      // if (file_closed == 1) {
      // 	REMPI_ALT("No more record file");
      // 	exit(15);
      // }
      // file_closed = mc_encoder->is_file_closed();
    }
    // REMPI_DBGI(0, "Replay : (count: %d, flag: %d, rank: %d, with_next: %d, index: %d, msg_id: %d, gid: %d)",
    //           replaying_test_event->get_event_counts(),
    //           replaying_test_event->get_flag(),
    //           replaying_test_event->get_rank(),
    //           replaying_test_event->get_with_next(),
    //           replaying_test_event->get_index(),
    //           replaying_test_event->get_msg_id(),
    //           replaying_test_event->get_matching_group_id());
    if (replaying_test_event->get_flag() == 0) {
      /*Unmatched*/
      update_validation_code(1, flag, 0, NULL, NULL, NULL);
      delete replaying_test_event;
      return MPI_SUCCESS;
    }
    if (flag != NULL) *flag = 1;

    if ((source != MPI_ANY_SOURCE && source != replaying_test_event->get_source())) {
      REMPI_ERR("Replaying probe event from rank %d, but src of this probe request is rank %d (MPI_ANY_SOURCE: %d)", 
		replaying_test_event->get_rank(), source, MPI_ANY_SOURCE);
    }
    PMPI_Probe(replaying_test_event->get_source(), tag, comm, status);
    has_next_event = replaying_test_event->get_is_testsome();
    delete replaying_test_event;
  }
  update_validation_code(1, flag, 1, NULL, status, NULL);
  return MPI_SUCCESS;
}

int rempi_recorder::pre_process_collective(MPI_Comm comm)
{
  /* Do nothing */
  return MPI_SUCCESS;
}

int rempi_recorder::post_process_collective()
{
  /* Do nothing */
  return MPI_SUCCESS;
}


int rempi_recorder::record_finalize(void)
{
  int ret;
  size_t global_validation_code;

  ret = PMPI_Allreduce(&validation_code, &global_validation_code, 1, MPI_UNSIGNED_LONG, MPI_SUM, MPI_COMM_WORLD);
  ret = PMPI_Finalize();

  if (rempi_mode == REMPI_ENV_REMPI_MODE_RECORD) {
    recording_event_list->push_all();
    /*TODO: set flag in event_list 
      insteand of setting flag of thread (via complete_flush)
      like in replay mode*/
  } else if (rempi_mode == REMPI_ENV_REMPI_MODE_REPLAY){
    /*TODO: if replay thread is still running, throw the error*/
  } else {
    REMPI_ERR("Unkonw rempi mode: %d", rempi_mode);
  }
  record_thread->join();
  REMPI_PRTI(0, "Global validation code: %u", global_validation_code);


  //  fprintf(stderr, "ReMPI: Function call (%s:%s:%d)\n", __FILE__, __func__, __LINE__);
  return 0;
}

int rempi_recorder::replay_finalize(void)
{
  int ret;
  size_t global_validation_code;

  ret = PMPI_Allreduce(&validation_code, &global_validation_code, 1, MPI_UNSIGNED_LONG, MPI_SUM, MPI_COMM_WORLD);
  ret = PMPI_Finalize();
  read_record_thread->join();
  //  REMPI_PRTI(0, "validation code: %u", validation_code);
  REMPI_PRTI(0, "Global validation code: %u", global_validation_code);
  //TODO:
  //  fprintf(stderr, "ReMPI: Function call (%s:%s:%d)\n", __FILE__, __func__, __LINE__);
  return 0;
}

