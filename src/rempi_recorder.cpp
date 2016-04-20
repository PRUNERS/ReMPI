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
#include "rempi_request_mg.h"
#include "rempi_sig_handler.h"
#include "rempi_mf.h"



void rempi_recorder::update_validation_code(int outcount, int *array_of_indices, MPI_Status *array_of_statuses, int *request_info)
{
  int index = 0;
  validation_code = rempi_hash(validation_code, outcount);
  //  REMPI_DBG("vali matched: %d", outcount);
  for (int i = 0; i < outcount; i++) {
    if (array_of_indices != NULL) {
      index = array_of_indices[i];
      validation_code = rempi_hash(validation_code, index);
    }
    if (request_info != NULL) {
      if (request_info[index] != REMPI_SEND_REQUEST) {
	validation_code = rempi_hash(validation_code, array_of_statuses[i].MPI_SOURCE);
	validation_code = rempi_hash(validation_code, array_of_statuses[i].MPI_TAG);
      }
    }
    //    REMPI_DBG("vali source: %d, tag: %d", array_of_statuses[i].MPI_SOURCE, array_of_statuses[i].MPI_TAG);
  }
  return;  
}

int rempi_recorder::record_init(int *argc, char ***argv, int rank) 
{
  string id;

  id = std::to_string((long long int)rank);
  replaying_event_list = new rempi_event_list<rempi_event*>(10000000, 100);
  recording_event_list = new rempi_event_list<rempi_event*>(10000000, 100);
  record_thread = new rempi_io_thread(recording_event_list, replaying_event_list, id, rempi_mode, NULL); //0: recording mode
  rempi_sig_handler_init(rank, record_thread, recording_event_list, &validation_code);
  record_thread->start();

  
  return 0;
}

int rempi_recorder::replay_init(int *argc, char ***argv, int rank) 
{
  string id;
  id = std::to_string((long long int)rank);
  /*recording_event_list is needed for CDC, and when switching from repla mode to recording mode */
  recording_event_list = new rempi_event_list<rempi_event*>(10000000, 100); 
  replaying_event_list = new rempi_event_list<rempi_event*>(10000000, 100);
  read_record_thread = new rempi_io_thread(recording_event_list, replaying_event_list, id, rempi_mode, NULL); //1: replay mode
  read_record_thread->start();
  rempi_sleep_sec(1);
  return 0;
}

int rempi_recorder::replay_isend(MPI_Request *request)
{

  REMPI_ERR("This function is not supposed to be called");
  return 0;
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
  int ret;
  double s, e;
  ret = PMPI_Irecv(buf, count, datatype, source, tag, comm, request);
  //  s =rempi_get_time();
  rempi_reqmg_register_request(request, source, tag, comm_id, REMPI_RECV_REQUEST); 
  //  e = rempi_get_time();
  //  REMPI_DBGI(0, "recv time: %f", e - s);
  //kento  msg_manager.add_pending_recv(request, source, tag, comm);
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
  rempi_proxy_request *proxy_request_info;
  rempi_irecv_inputs *irecv_inputs;
  

  //  REMPI_DBGI(1, "Irecv: %p", *request);

#ifdef BGQ
    memset(request, request_id, sizeof(MPI_Request));
    request_id++;
    //    REMPI_DBG("request_id: %lu request: %p", request_id, *request);
    //    *request = (MPI_Request)(request_id++);
#else
    *request = (MPI_Request)rempi_malloc(sizeof(MPI_Request));//((source + 1) * (tag + 1) * (comm_id * 1));
    //    memset(request, request_id, sizeof(MPI_Request));
    // *request = (MPI_Request)request_id;
    // request_id++;
#endif

  if (request_to_irecv_inputs_umap.find(*request) != 
      request_to_irecv_inputs_umap.end()) {
    irecv_inputs = request_to_irecv_inputs_umap[*request];
    REMPI_ERR("Already exist request(req:%p) in (source, tag, comm): new (%d, %d, %p), old (%d, %d, %p)",
	      (*request), irecv_inputs->source, irecv_inputs->tag, irecv_inputs->comm,
	      source, tag, comm);
  } else {
    request_to_irecv_inputs_umap[*request] = new rempi_irecv_inputs(buf, count, datatype, source, tag, comm, *request);
    //    REMPI_DBGI(1, "Add: %p", *request);
  }

  //  REMPI_DBG("register use: %p", *request);
  rempi_reqmg_register_request(request, source, tag, comm_id, REMPI_RECV_REQUEST);

  return MPI_SUCCESS;
}


// int rempi_recorder::replay_cancel(MPI_Request *request)
// {
//   int ret;
//   ret = PMPI_Cancel(request);
//   return ret;
// }

void rempi_recorder::cancel_request(MPI_Request *request)
{
  int ret;
  rempi_irecv_inputs *irecv_inputs;
  rempi_proxy_request *proxy_request;
  int request_type;
  
  rempi_reqmg_get_request_type(request, &request_type);
  rempi_reqmg_deregister_request(request, request_type);

  if (request_type == REMPI_RECV_REQUEST) {

    if (request_to_irecv_inputs_umap.find(*request) == request_to_irecv_inputs_umap.end()) {
      REMPI_ERR("No such request: %p", request);
    }
    irecv_inputs = request_to_irecv_inputs_umap[*request];
    list<rempi_proxy_request*>::iterator proxy_request_it;
    for (proxy_request_it  = irecv_inputs->request_proxy_list.begin();
	 proxy_request_it != irecv_inputs->request_proxy_list.end();
	 proxy_request_it++) {
      proxy_request = *proxy_request_it;
      ret = PMPI_Cancel(&proxy_request->request);
      delete proxy_request;
    }
    irecv_inputs->request_proxy_list.clear();
#ifdef REMPI_DBG_REPLAY
    REMPI_DBGI(REMPI_DBG_REPLAY, "MPI_Cancel: irecv_input->recv_test_id: %d", irecv_inputs->recv_test_id);
#endif
    irecv_inputs->recv_test_id = -1;
    //    REMPI_DBGI(1, "remove 2: %p (%d %d %d)", *request, irecv_inputs->source, irecv_inputs->tag, irecv_inputs->comm);
    delete irecv_inputs;
    request_to_irecv_inputs_umap.erase(*request);
    free(*request);
    *request = MPI_REQUEST_NULL;
  }
  return;
}

int rempi_recorder::replay_cancel(MPI_Request *request)
{
  int ret;
  int request_type;
  rempi_reqmg_get_request_type(request, &request_type);
  if (request_type != REMPI_RECV_REQUEST) {
    REMPI_ERR("only recv request can be canceled: request_type: %d", request_type);
  }
  if (request_type == REMPI_RECV_REQUEST) {
    cancel_request(request);
    ret = MPI_SUCCESS;
  } else if (request_type == REMPI_SEND_REQUEST) {
    cancel_request(request);
    ret = PMPI_Cancel(request);
  }
  return ret;
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

extern "C" MPI_Fint PMPI_Request_c2f(MPI_Request request);
MPI_Fint rempi_recorder::replay_request_c2f(MPI_Request request)
{
  MPI_Fint ret;
  int request_type;
  MPI_Request deregist_rempi_request;
  rempi_irecv_inputs *irecv_inputs;

  rempi_reqmg_get_request_type(&request, &request_type);
  
  if (request_type == REMPI_RECV_REQUEST) {
    irecv_inputs = request_to_irecv_inputs_umap[request];
    deregist_rempi_request = request;
    PMPI_Irecv(
	       irecv_inputs->buf, 
	       irecv_inputs->count,
	       irecv_inputs->datatype,
	       irecv_inputs->source,
	       irecv_inputs->tag,
	       irecv_inputs->comm, &request);
    cancel_request(&deregist_rempi_request);
  } else if (request_type == REMPI_SEND_REQUEST) {
    cancel_request(&request);
  }
  ret = PMPI_Request_c2f(request);

  return ret;
}


int rempi_recorder::record_test(
    MPI_Request *request,
    int *flag,
    int source,
    int tag,
    int clock,
    int with_previous)
{
  return record_test(request, flag, source, tag, clock, with_previous, -1);
}

int rempi_recorder::record_test(
    MPI_Request *request,
    int *flag,
    int source,
    int tag,
    int clock,
    int with_previous,
    int test_id)
{
  
  REMPI_ERR("This function was retired");
  // int event_count = 1;
  // int request_val = -1;
  // int record_comm_id = 0;
  // int record_source = 0;
  // int record_tag = 0;
  // int record_clock = 0;


  // if (*flag) {
  //   /*Query befoer add_matched_recv, because pendding entry is removed when flag == 1*/
  //   //kento    record_comm_id = msg_manager.query_pending_comm_id(request);
  //   record_source  = source;
  //   record_tag     = tag;
  //   record_clock   = clock;
  //   //kento msg_manager.add_matched_recv(request, source, tag);
  //   //REMPI_DBG("%d %d %d %d %d %d %d", event_count, is_testsome, comm_id, *flag, source, tag, clock);
  // } 
  // //TODO: we need to record *request ?
  // recording_event_list->push(new rempi_test_event(event_count, with_previous, record_comm_id, *flag, record_source, record_tag, record_clock, test_id));

  return 0;
}






int rempi_recorder::replay_test(
				MPI_Request *request,
				int *flag,
				MPI_Status *status,
				int test_id)
{
  REMPI_ERR("this function is not implementd yet, or I do not implement it");
  return -1;
}

// /*This function is called after MPI_Test*/
// int rempi_recorder::replay_test(
//     MPI_Request *request_in,
//     int flag_in,
//     int source_in,
//     int tag_in,
//     int clock_in,
//     int with_previous_in,
//     int test_id_in,
//     int *flag_out,
//     int *source_out,
//     int *tag_out)
// {
//   MPI_Status status;
//   rempi_event *replaying_test_event;


//   /*1. Get replaying event */
//   //  replaying_test_event = replaying_event_list->dequeue_replay(test_id_in, -1);
//   if (replaying_test_event == NULL) {
//     REMPI_ERR("No more replay event. Will switch to recording mode, but not implemented yet");
//   } 
//   REMPI_DBG("Replaying: (flag: %d, source: %d)", replaying_test_event->get_flag(), replaying_test_event->get_source());

//   /*If the event is flag == 0, simply retunr flag == 0*/
//   if (!replaying_test_event->get_flag()) {
//     *flag_out = 0;
//     *source_out = -1;
//     *tag_out = -1;
//     return 0;
//   }
//   *flag_out = 1;
//   *source_out = replaying_test_event->get_source();
//   *tag_out = replaying_test_event->get_tag();
//   /*
//     2. Wait until this replaying message really arrives
//   */
//   {
//     rempi_irecv_inputs *inputs = request_to_irecv_inputs_umap[*request_in];
//     //    REMPI_DBG("Probing");
//     PMPI_Probe(*source_out, *tag_out, inputs->comm, &status);
//     if (*source_out != status.MPI_SOURCE ||
// 	*tag_out    != status.MPI_TAG) {
//       REMPI_ERR("An unexpected message is proved");
//     }
//     PMPI_Irecv(
// 	       inputs->buf, 
// 	       inputs->count,
// 	       inputs->datatype,
// 	       *source_out,
// 	       *tag_out,
// 	       inputs->comm,
// 	       &(inputs->request));
//     PMPI_Wait(&(inputs->request), &status);
//     if (*source_out != status.MPI_SOURCE ||
// 	*tag_out    != status.MPI_TAG) {
//       REMPI_ERR("An unexpected message is waited");
//     }
//     delete request_to_irecv_inputs_umap[*request_in];
//     request_to_irecv_inputs_umap.erase(*request_in);
//   }
//   return 0;
// }


// /*This function is called after MPI_Test*/
// int rempi_recorder::replay_test(
//     MPI_Request *request_in,
//     int flag_in,
//     int source_in,
//     int tag_in,
//     int clock_in,
//     int with_previous_in,
//     int test_id_in,
//     int *flag_out,
//     int *source_out,
//     int *tag_out)
// {
//   rempi_event *replaying_test_event;

//   /**/
//   if (flag_in) {
//     int event_count = 1;
//     int with_previous = 0; //
//     int comm_id = 0;
//     msg_manager.add_matched_recv(request_in, source_in, tag_in);
//     recording_event_list->push(new rempi_test_event(event_count, with_previous, comm_id, flag_in, source_in, tag_in, clock_in, test_id_in));
//   }

//   /*1. Get request (recoeded in irecv) for this "replaying_test_event"*/
//   replaying_test_event = replaying_event_list->dequeue_replay(test_id_in);
//   if (replaying_test_event == NULL) {
//     REMPI_DBG("No more replay event. Will switch to recording mode, but not implemented yet");
//   } 

//   REMPI_DBG("flag: %d , source: %d", replaying_test_event->get_flag(), replaying_test_event->get_source());

//   /*If the event is flag == 0, simply retunr flag == 0*/
//   if (!replaying_test_event->get_flag()) {
//     *flag_out = 0;
//     *source_out = -1;
//     *tag_out = -1;
//     return 0;
//   }
//   /* So "replayint_test_event" event flag == 1*/
//   *flag_out = 1;
//   /*
//     2. Wait until this recorded message really arrives
//        if (0 if next recorded maching is not mached in this run) {
//           TODO: Wait until maching, and get clock
// 	  TODO: if matched, memorize that the matching for the next replay test
//        }
//   */

//   while (!msg_manager.is_matched_recv(
// 	      replaying_test_event->get_source(), 
// 	      replaying_test_event->get_tag(),
// 	      replaying_test_event->get_comm_id())) {
//     msg_manager.refresh_matched_recv();
//   }

//   msg_manager.remove_matched_recv(
// 	      replaying_test_event->get_source(), 
// 	      replaying_test_event->get_tag(),
// 	      replaying_test_event->get_comm_id());
//   /*
//     3. Set valiabiles (source, flag, tag)
//   }
//    */
//   *flag_out = 1;
//   *source_out = replaying_test_event->get_source();
//   *tag_out = replaying_test_event->get_tag();

//   return 0;
// }




// int rempi_recorder::record_testsome(
//     int incount,
//     void *array_of_requests[],
//     int *outcount,
//     int array_of_indices[],
//     void *array_of_statuses[])
// {
//   int event_count = 1;
//   int is_testsome = 1;
//   int request     = 0;
//   int flag        = 0;
//   int source      = -1;
//   int tag         = -1;


//   if (*outcount == 0) {
//     recording_event_list->push(new rempi_test_event(event_count, is_testsome, request, flag, source, tag));
//     return 0;
//   }

//   flag = 1;
//   for (int i = 0; i < *outcount; i++) {
//     source = array_of_indices[i];
//     //TODO:  tag,
//     tag    = 0;
    
//     recording_event_list->push(new rempi_test_event(event_count, is_testsome, request, flag, source, tag));
//   }

//    return 0;
// }





int rempi_recorder::record_mf(
			      int incount,
			      MPI_Request array_of_requests[],
			      int *outcount,
			      int array_of_indices[],
			      MPI_Status array_of_statuses[],
			      int global_test_id,
			      int matching_function_type)
{
  int ret;
  int flag;
  rempi_event *test_event = NULL;
  int is_with_next;
  int matched_count;
  int sendcount, recvcount, nullcount;
  int was_array_of_indices_null = 0;
  int is_record_and_replay;


  if (incount > REQUEST_INFO_SIZE) {
    REMPI_ERR("incount: %d is larger than buffer", incount);
  }
  
  rempi_reqmg_get_request_info(incount, array_of_requests, &sendcount, &recvcount, &nullcount, request_info, &is_record_and_replay);
  if (is_record_and_replay) {
    rempi_reqmg_store_send_statuses(incount, array_of_requests, request_info, tmp_statuses);
  }

  for (int i = 0; i < incount; i++) tmp_requests[i] = array_of_requests[i];
  ret = rempi_mf(incount, array_of_requests, outcount, array_of_indices, array_of_statuses, matching_function_type);
  
  if (is_record_and_replay == 0) return ret;

  global_test_id = rempi_reqmg_get_test_id(array_of_requests, incount);
  if (matching_function_type == REMPI_MPI_TESTALL) {
    /*MF: Testall*/
    matched_count = (*outcount == 1)? incount:0;
  } else if (matching_function_type == REMPI_MPI_WAIT ||
	     matching_function_type == REMPI_MPI_WAITALL) {
    /*MF: Wait or Waitall*/
    matched_count = incount;
  } else if (matching_function_type == REMPI_MPI_WAITANY) {
    matched_count = 1;
  } else {
    /*MF: Waitsome, Testsome, Testany, Test*/
    matched_count = *outcount;
  }
  

  update_validation_code(matched_count, array_of_indices, array_of_statuses, request_info);

  if (matched_count == 0) {
    flag = 0;
    test_event = rempi_event::create_test_event(1, 
				      flag, 
				      REMPI_MPI_EVENT_INPUT_IGNORE, 
				      REMPI_MPI_EVENT_NOT_WITH_NEXT, 
				      REMPI_MPI_EVENT_INPUT_IGNORE, 
				      REMPI_MPI_EVENT_INPUT_IGNORE, 
				      global_test_id);
	// if (test_event != NULL) {
	//   REMPI_DBGI(1, "Record : (count: %d, flag: %d, rank: %d, with_next: %d, index: %d, msg_id: %d, gid: %d) MF: %d",
	// 	     test_event->get_event_counts(),
	// 	     test_event->get_flag(),
	// 	     test_event->get_rank(),
	// 	     test_event->get_with_next(),
	// 	     test_event->get_index(),
	// 	     test_event->get_msg_id(),
	// 	     test_event->get_matching_group_id(),
	// 	     matching_function_type);
	// }
    recording_event_list->push(test_event);
  } else {
    flag = 1;
    is_with_next = REMPI_MPI_EVENT_WITH_NEXT;
    for (int i = 0; i < matched_count; i++) {
      int rank = -1, matched_index;
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
      } else if (request_info[matched_index] == REMPI_RECV_REQUEST) {
	rank = array_of_statuses[i].MPI_SOURCE;
	rempi_reqmg_deregister_request(&tmp_requests[matched_index], REMPI_RECV_REQUEST);
      }

      if (rank >= 0) {
	test_event = rempi_event::create_test_event(1, 
					  flag,
					  rank,
					  is_with_next,
					  matched_index, 
					  REMPI_MPI_EVENT_INPUT_IGNORE,
					  global_test_id);
	// if (test_event != NULL) {
	//   REMPI_DBGI(1, "Record : (count: %d, flag: %d, rank: %d, with_next: %d, index: %d, msg_id: %d, gid: %d) MF: %d",
	// 	     test_event->get_event_counts(),
	// 	     test_event->get_flag(),
	// 	     test_event->get_rank(),
	// 	     test_event->get_with_next(),
	// 	     test_event->get_index(),
	// 	     test_event->get_msg_id(),
	// 	     test_event->get_matching_group_id(),
	// 	     matching_function_type);
	// }
	recording_event_list->push(test_event);
      }
    }
  }



  return ret;
}

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
  int global_test_id;
  int ret;
  MPI_Request dummy_req;
  int record_flag, record_source;
  PMPI_Comm_get_name(comm, comm_id, &resultlen);
  /*TODO: Interface for NULL request. For now, use dummy_req*/
  rempi_reqmg_register_request(&dummy_req, source, tag, (int)comm_id[0], REMPI_RECV_REQUEST);
  global_test_id = rempi_reqmg_get_test_id(&dummy_req, 1);

  /* Call MPI matching function */
  ret = rempi_pf(source, tag, comm, flag, status, probe_function_type);

  update_validation_code(1, NULL, status, NULL);


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
				    REMPI_MPI_EVENT_INPUT_IGNORE,
				    global_test_id);
  recording_event_list->push(test_event);
  return ret;
}




static int recv_from2 = 0;


int rempi_recorder::replay_mf(
			      int incount,
			      MPI_Request array_of_requests[],
			      int *outcount,
			      int array_of_indices[],
			      MPI_Status array_of_statuses[],
			      int global_test_id,
			      int matching_function_type)
{
  int ret;
  MPI_Status status;
  rempi_event *replaying_test_event;
  int has_next_event = REMPI_MPI_EVENT_WITH_NEXT;
  int recvcount, sendcount, nullcount;
  int test_id;
  int replay_queue_status;
  MPI_Comm comm;
  rempi_irecv_inputs *irecv_inputs;
  int is_record_and_replay;
  int local_outcount = 0;

  
  if (incount > REQUEST_INFO_SIZE) {
    REMPI_ERR("incount: %d is larger than buffer", incount);
  }  
  rempi_reqmg_get_request_info(incount, array_of_requests, &sendcount, &recvcount, &nullcount, request_info, &is_record_and_replay);

  if (is_record_and_replay == 0) {
    /*this mf function is not replayed*/
    ret = rempi_mf(incount, array_of_requests, outcount, array_of_indices, array_of_statuses, matching_function_type);
    return ret;
  }

  rempi_reqmg_store_send_statuses(incount, array_of_requests, request_info, tmp_statuses);
  test_id = rempi_reqmg_get_test_id(array_of_requests, incount);
  
  /*Matched*/
  int index = 0;
  while (has_next_event == REMPI_MPI_EVENT_WITH_NEXT) {
    while ((replaying_test_event = replaying_event_list->dequeue_replay(test_id, replay_queue_status)) == NULL);

    // REMPI_DBGI(1, "Record : (count: %d, flag: %d, rank: %d, with_next: %d, index: %d, msg_id: %d, gid: %d) MF: %d",
    // 	       replaying_test_event->get_event_counts(),
    // 	       replaying_test_event->get_flag(),
    // 	       replaying_test_event->get_rank(),
    // 	       replaying_test_event->get_with_next(),
    // 	       replaying_test_event->get_index(),
    // 	       replaying_test_event->get_msg_id(),
    // 	       replaying_test_event->get_matching_group_id(),
    // 	       matching_function_type);
    if (replaying_test_event->get_flag() == 0) {
      /*Unmatched*/
      delete replaying_test_event;
      if (outcount != NULL) {
	*outcount = 0;
      }
      goto end;
    }
    
    if (incount == 0) {
      REMPI_ERR("incount == 0, but replaying request rank: %d index: %d, with_next: %d (MF: %d)", 
    		replaying_test_event->get_rank(), replaying_test_event->get_index(),
    		replaying_test_event->get_with_next(), matching_function_type);
    }

    index = replaying_test_event->get_index();

    if (request_info[index] == REMPI_SEND_REQUEST) {
      if (tmp_statuses[index].MPI_SOURCE != replaying_test_event->get_rank()) {
	REMPI_ERR("Replaying send event to rank %d, but dest of this send request is rank %d (MF: %d, matched_index: %d) %p", 
		  replaying_test_event->get_rank(), tmp_statuses[index].MPI_SOURCE, matching_function_type, index, array_of_requests[index]);
	REMPI_ASSERT(0);
      }
      rempi_reqmg_deregister_request(&array_of_requests[index], REMPI_SEND_REQUEST);      
      PMPI_Wait(&array_of_requests[index], &array_of_statuses[index]);      
    } else if (request_info[index] == REMPI_RECV_REQUEST) {
      irecv_inputs = request_to_irecv_inputs_umap[array_of_requests[index]];
   
      if (irecv_inputs->source != MPI_ANY_SOURCE && 
	  irecv_inputs->source != replaying_test_event->get_source()) {
	REMPI_ERR("Replaying recv event from rank %d, but src of this recv request is rank %d: request: %p, index: %d", 
		  replaying_test_event->get_rank(), irecv_inputs->source,
		  array_of_requests[index], index);
      }
      // REMPI_DBG( "test: source: %d, tag: %d, flag: %d", 
      // 		   replaying_test_event->get_source(), irecv_inputs->tag, replaying_test_event->get_flag());

      PMPI_Probe(replaying_test_event->get_source(), irecv_inputs->tag,
		 irecv_inputs->comm, &status);
      

      PMPI_Recv(
		irecv_inputs->buf, 
		irecv_inputs->count,
		irecv_inputs->datatype,
		status.MPI_SOURCE,
		status.MPI_TAG,
		irecv_inputs->comm, &status);

      // REMPI_DBG( "source: %d(%d), tag: %d (%d), count: %d (id: %d)", 
      // 		 irecv_inputs->source, MPI_ANY_SOURCE, irecv_inputs->tag, MPI_ANY_TAG, irecv_inputs->count, recv_from2++);


      array_of_statuses[local_outcount].MPI_SOURCE = status.MPI_SOURCE;
      array_of_statuses[local_outcount].MPI_TAG    = status.MPI_TAG;

      rempi_reqmg_deregister_request(&array_of_requests[index], REMPI_RECV_REQUEST);
      delete irecv_inputs;
      //      REMPI_DBGI(1, "remove 1: %p", array_of_requests[index]);
      request_to_irecv_inputs_umap.erase(array_of_requests[index]);
#ifdef BGQ
#else
      free(array_of_requests[index]);
#endif

    } else if (request_info[index] == REMPI_NULL_REQUEST) {
      REMPI_ERR("MPI_REQUEST_NULL does not match any message: %d", request_info[index]);
    } else {
      REMPI_ERR("Trying replaying with Ignored MPI_Request: %d", request_info[index]);
    }

    if (array_of_indices != NULL) {
      array_of_indices[local_outcount]  = index;
    }

    array_of_requests[index] = MPI_REQUEST_NULL;
    (local_outcount)++;
    has_next_event = replaying_test_event->get_is_testsome();
    delete replaying_test_event;    
  }
  if (outcount != NULL) {
    *outcount = local_outcount;
  }

 end:
  update_validation_code(local_outcount, array_of_indices, array_of_statuses, request_info);
  return MPI_SUCCESS;
}

int rempi_recorder::replay_testsome(
				    int incount,
				    MPI_Request array_of_requests[],
				    int *outcount,
				    int array_of_indices[],
				    MPI_Status array_of_statuses[],
				    int global_test_id,
				    int mf_flag_1, 
				    int mf_flag_2)
{
 //  MPI_Status status;
 //  rempi_event *replaying_test_event;
 //  int has_next_event = REMPI_MPI_EVENT_WITH_NEXT;
 //  int recv_request_count, null_request_count, valid_request_count;
 //  int test_id;
 //  int replay_queue_status;
 //  MPI_Comm comm;
 //  rempi_irecv_inputs *irecv_inputs;

 //  recv_request_count = rempi_reqmg_get_recv_request_count(incount, array_of_requests);
 //  null_request_count = rempi_reqmg_get_null_request_count(incount, array_of_requests);
 //  valid_request_count = recv_request_count + null_request_count;

 //  // REMPI_DBG("recv_request_count: %d, null_request_count:%d, incount: %d", 
 //  // 	      recv_request_count, null_request_count, incount);

 //  if (valid_request_count == 0) {
 //    /*All send requests*/
 //    if(mf_flag_1 == REMPI_MF_FLAG_1_TEST) {
 //      switch (mf_flag_2) {
 //      case REMPI_MF_FLAG_2_SINGLE:
 // 	return PMPI_Test(array_of_requests, outcount, array_of_statuses);
 //      case REMPI_MF_FLAG_2_ANY:
 // 	return PMPI_Testany(incount, array_of_requests, outcount, array_of_indices, array_of_statuses);
 //      case REMPI_MF_FLAG_2_SOME:
 // 	return PMPI_Testsome(incount, array_of_requests, outcount, array_of_indices, array_of_statuses);
 //      case REMPI_MF_FLAG_2_ALL:
 // 	return PMPI_Testall(incount, array_of_requests, outcount, array_of_statuses);
 //      }
 //    } else if (mf_flag_1 == REMPI_MF_FLAG_1_WAIT) {
 //      switch (mf_flag_2) {
 //      case REMPI_MF_FLAG_2_SINGLE:
 // 	return PMPI_Wait(array_of_requests, array_of_statuses);
 //      case REMPI_MF_FLAG_2_ANY:
 // 	return PMPI_Waitany(incount, array_of_requests, array_of_indices, array_of_statuses);
 //      case REMPI_MF_FLAG_2_SOME:
 // 	return PMPI_Waitsome(incount, array_of_requests, outcount, array_of_indices, array_of_statuses);
 //      case REMPI_MF_FLAG_2_ALL:
 // 	*outcount = incount;
 // 	return PMPI_Waitall(incount, array_of_requests, array_of_statuses);
 //      }
 //    } else {
 //      REMPI_ERR("No such MPI matching function");
 //    }
 //  } else if (valid_request_count != incount || null_request_count > 0) {
 //    REMPI_ERR("Both send requests and recv requestes are tested once: "
 // 	      "recv_request_count: %d, null_request_count:%d, incount: %d", 
 // 	      recv_request_count, null_request_count, incount);
 //  }
 //  test_id = rempi_reqmg_get_test_id(array_of_requests, incount);
  
 //  /*Matched*/
 //  int index = 0;
 //  *outcount = 0;
 //  while (has_next_event == REMPI_MPI_EVENT_WITH_NEXT) {
 //    while ((replaying_test_event = replaying_event_list->dequeue_replay(test_id, replay_queue_status)) == NULL);
 //    if (replaying_test_event->get_flag() == 0) {
 //      /*Unmatched*/
 //      delete replaying_test_event;
 //      goto end;
 //    }

 //    irecv_inputs = request_to_irecv_inputs_umap[array_of_requests[index]];
 //    if ((irecv_inputs->source == MPI_ANY_SOURCE ||irecv_inputs->source == replaying_test_event->get_source())
 // 	// tag is not recorded.
 // 	//	&& (irecv_inputs->tag == MPI_ANY_TAG ||irecv_inputs->tag == replaying_test_event->get_tag())
 // 	) 
 //      {
 //      // REMPI_DBG("test: source: %d, tag: %d, flag: %d", 
 //      // 		replaying_test_event->get_source(), irecv_inputs->tag, replaying_test_event->get_flag());
 //      PMPI_Probe(replaying_test_event->get_source(), irecv_inputs->tag,
 // 		 irecv_inputs->comm, &status);
 //      PMPI_Recv(
 // 	 irecv_inputs->buf, 
 // 	 irecv_inputs->count,
 // 	 irecv_inputs->datatype,
 // 	 status.MPI_SOURCE,
 // 	 status.MPI_TAG,
 // 	 irecv_inputs->comm, &status);
      
 //      if (array_of_indices != NULL) {
 //      	array_of_indices[*outcount]  = index;
 //      }
 //      array_of_statuses[*outcount].MPI_SOURCE = status.MPI_SOURCE;
 //      array_of_statuses[*outcount].MPI_TAG    = status.MPI_TAG;

 //      rempi_reqmg_deregister_request(&array_of_requests[index], REMPI_RECV_REQUEST);

 //      delete irecv_inputs;
 //      request_to_irecv_inputs_umap.erase(array_of_requests[index]);
 //      free(array_of_requests[index]);
 //      array_of_requests[index] = MPI_REQUEST_NULL;
 //      (*outcount)++;
 //      has_next_event = replaying_test_event->get_is_testsome();
 //      delete replaying_test_event;
 //    } 
 //    index++;
 //  }

 // end:
 //  update_validation_code(*outcount, array_of_indices, array_of_statuses, request_info);
  return MPI_SUCCESS;
}


int rempi_recorder::replay_iprobe(int source, int tag, MPI_Comm comm, int *flag, MPI_Status *status, int comm_id)
{
  rempi_event *replaying_test_event;
  int has_next_event = REMPI_MPI_EVENT_WITH_NEXT;
  int test_id;
  int replay_queue_status;

  MPI_Request req = NULL;


  rempi_reqmg_register_request(&req, source, tag, comm_id, REMPI_RECV_REQUEST);
  test_id = rempi_reqmg_get_test_id(&req, 1);


  /*Matched*/
  *flag = 0;
  while (has_next_event == REMPI_MPI_EVENT_WITH_NEXT) {
    while ((replaying_test_event = replaying_event_list->dequeue_replay(test_id, replay_queue_status)) == NULL);
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
      delete replaying_test_event;
      return MPI_SUCCESS;
    }
    *flag = 1;

    if ((source != MPI_ANY_SOURCE && source != replaying_test_event->get_source())) {
      REMPI_ERR("Replaying probe event from rank %d, but src of this probe request is rank %d (MPI_ANY_SOURCE: %d)", 
		replaying_test_event->get_rank(), source, MPI_ANY_SOURCE);
    }
    PMPI_Probe(replaying_test_event->get_source(), tag, comm, status);
    has_next_event = replaying_test_event->get_is_testsome();
    delete replaying_test_event;
  }
  update_validation_code(1, NULL, status, NULL);
  return MPI_SUCCESS;
}


int rempi_recorder::record_finalize(void)
{
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
  REMPI_DBGI(0, "validation code: %u", validation_code);

  //  fprintf(stderr, "ReMPI: Function call (%s:%s:%d)\n", __FILE__, __func__, __LINE__);
  return 0;
}

int rempi_recorder::replay_finalize(void)
{
  read_record_thread->join();
  REMPI_DBGI(0, "validation code: %u", validation_code);
  //TODO:
  //  fprintf(stderr, "ReMPI: Function call (%s:%s:%d)\n", __FILE__, __func__, __LINE__);
  return 0;
}

void rempi_recorder::set_fd_clock_state(int flag)
{
  return;
}

void rempi_recorder::fetch_and_update_local_min_id()
{
  return;
}
