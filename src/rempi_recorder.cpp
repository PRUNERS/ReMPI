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

void rempi_recorder::update_validation_code(int outcount, int *array_of_indices, MPI_Status *array_of_statuses)
{
  int index = 0;
  validation_code = rempi_hash(validation_code, outcount);
  //  REMPI_DBG("vali matched: %d", outcount);
  for (int i = 0; i < outcount; i++) {
    if (array_of_indices != NULL) {
      index = array_of_indices[i];
    }
    //    validation_code = rempi_hash(validation_code, index);
    validation_code = rempi_hash(validation_code, array_of_statuses[i].MPI_SOURCE);
    validation_code = rempi_hash(validation_code, array_of_statuses[i].MPI_TAG);
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
  rempi_reqmg_register_recv_request(request, source, tag, comm_id); 
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

//   if (request_to_irecv_inputs_umap.find(*request) != 
//       request_to_irecv_inputs_umap.end()) {
//     /* If this request is posted in irecv before*/
//     irecv_inputs = request_to_irecv_inputs_umap[*request];
//     bool same_source = (irecv_inputs->source == source);
//     bool same_tag    = (irecv_inputs->tag    == tag);
//     bool same_comm   = (irecv_inputs->comm   == *comm);
//     if (!same_source || !same_tag || !same_comm) {
//       REMPI_ERR("Different request(req:%p) in (source, tag, comm): :(%d, %d, %p) != (%d, %d, %p)",
//        		(*request), irecv_inputs->source, irecv_inputs->tag, irecv_inputs->comm,
// 		source, tag, *comm);

//       // REMPI_ERR("This MPI_Request is not diactivated. This mainly caused by irecv call "
//       // 		"with MPI_Request which is not diactivated by MPI_{Wait|Test}{|some|any|all}");
//     }
//   } else {
// #ifdef BGQ
//     memset(request, request_id, sizeof(MPI_Request));
//     request_id++;
//     //    REMPI_DBG("request_id: %lu request: %p", request_id, *request);
//     //    *request = (MPI_Request)(request_id++);
// #else
//     *request = (MPI_Request)rempi_malloc(sizeof(MPI_Request));//((source + 1) * (tag + 1) * (comm_id * 1));
// #endif
//     request_to_irecv_inputs_umap[*request] = new rempi_irecv_inputs(buf, count, datatype, source, tag, *comm, *request);
//   }


  if (request_to_irecv_inputs_umap.find(*request) != 
      request_to_irecv_inputs_umap.end()) {
    irecv_inputs = request_to_irecv_inputs_umap[*request];
    REMPI_ERR("Already exist request(req:%p) in (source, tag, comm): new (%d, %d, %p), old (%d, %d, %p)",
	      (*request), irecv_inputs->source, irecv_inputs->tag, irecv_inputs->comm,
	      source, tag, comm);
  } else {
#ifdef BGQ
    memset(request, request_id, sizeof(MPI_Request));
    request_id++;
    //    REMPI_DBG("request_id: %lu request: %p", request_id, *request);
    //    *request = (MPI_Request)(request_id++);
#else
    *request = (MPI_Request)rempi_malloc(sizeof(MPI_Request));//((source + 1) * (tag + 1) * (comm_id * 1));
#endif
    request_to_irecv_inputs_umap[*request] = new rempi_irecv_inputs(buf, count, datatype, source, tag, comm, *request);
  }

  //  REMPI_DBG("register use: %p", *request);
  rempi_reqmg_register_recv_request(request, source, tag, comm_id);

  return MPI_SUCCESS;
}


// int rempi_recorder::replay_cancel(MPI_Request *request)
// {
//   int ret;
//   ret = PMPI_Cancel(request);
//   return ret;
// }

int rempi_recorder::replay_cancel(MPI_Request *request)
{
  int ret;
  rempi_irecv_inputs *irecv_inputs;
  rempi_proxy_request *proxy_request;

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
  delete irecv_inputs;
  request_to_irecv_inputs_umap.erase(*request);

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
  rempi_test_event *test_event;
  int is_with_next;
  int matched_count;
  int sendcount, recvcount, nullcount;
  int was_array_of_indices_null = 0;

  if (incount > REQUEST_INFO_SIZE) {
    REMPI_ERR("incount: %d is larger than buffer", incount);
  }
  
  rempi_reqmg_get_request_info(incount, array_of_requests, &sendcount, &recvcount, &nullcount, request_info);

  rempi_reqmg_store_send_statuses(incount, array_of_requests, request_info, tmp_statuses);

  ret = rempi_mf(incount, array_of_requests, outcount, array_of_indices, array_of_statuses, matching_function_type);

  
  if (recvcount == 0) goto end;
  global_test_id = rempi_reqmg_get_test_id(array_of_requests, incount);

  if  (matching_function_type == REMPI_MPI_WAITANY) {
    matched_count = 1;
  } else if ((matching_function_type == REMPI_MPI_TESTALL && *outcount == 1) || 
	     matching_function_type == REMPI_MPI_WAITALL) {
    matched_count = incount;
  } else if (outcount == NULL) {
    matched_count = incount;
  } else {
    matched_count = *outcount;
  }

  update_validation_code(matched_count, array_of_indices, array_of_statuses);

  if (matched_count == 0) {
    flag = 0;
    test_event = new rempi_test_event(1, REMPI_MPI_EVENT_NOT_WITH_NEXT, 0, 0, 0, 0, 0, global_test_id);
    recording_event_list->push(test_event);
  } else {
    flag = 1;
    is_with_next = REMPI_MPI_EVENT_WITH_NEXT;
    for (int i = 0; i < matched_count; i++) {
      int source, matched_index;
      if (array_of_indices == NULL) {
	matched_index = 0;
      } else {
	matched_index = array_of_indices[i];
      }
      if (i == matched_count - 1) {
	is_with_next = REMPI_MPI_EVENT_NOT_WITH_NEXT;
      }
      if (request_info[matched_index] == REMPI_SEND_REQUEST) {
	test_event = new rempi_test_event(1, is_with_next, 0, 1, tmp_statuses[matched_index].MPI_SOURCE, 
					  0, 0, global_test_id);
      } else if (request_info[matched_index] == REMPI_RECV_REQUEST) {
	test_event = new rempi_test_event(1, is_with_next, 0, 1, array_of_statuses[i].MPI_SOURCE, 
					  0, 0, global_test_id);
      }
      recording_event_list->push(test_event);
    }
  }

 end:
  //  if(request_info != NULL) rempi_free(request_info);
  //  if(was_array_of_indices_null) rempi_free(array_of_indices);
  return ret;
}

int rempi_recorder::record_pf(int source,
				  int tag,
				  MPI_Comm comm,
				  int *flag,
				  MPI_Status *status,
				  int probe_function_type)
{
  rempi_test_event *test_event;
  char comm_id[REMPI_COMM_ID_LENGTH];
  int resultlen;
  int global_test_id;
  int ret;
  MPI_Request dummy_req;
  PMPI_Comm_get_name(comm, comm_id, &resultlen);
  /*TODO: Interface for NULL request. For now, use dummy_req*/
  rempi_reqmg_register_recv_request(&dummy_req, source, tag, (int)comm_id[0]);
  global_test_id = rempi_reqmg_get_test_id(&dummy_req, 1);

  /* Call MPI matching function */
  ret = rempi_pf(source, tag, comm, flag, status, probe_function_type);

  update_validation_code(1, NULL, status);

  if (flag == NULL) {
    /*MPI_Probe*/
    test_event = new rempi_test_event(1, REMPI_MPI_EVENT_NOT_WITH_NEXT, 0, 1, status->MPI_SOURCE, status->MPI_TAG, 0, global_test_id);
  } else if (*flag == 0) {
    /*MPI_Iprobe unmatched*/
    test_event = new rempi_test_event(1, REMPI_MPI_EVENT_NOT_WITH_NEXT, 0, 0, 0, 0, 0, global_test_id);
  } else {
    /*MPI_Iprobe matched*/
    test_event = new rempi_test_event(1, REMPI_MPI_EVENT_NOT_WITH_NEXT, 0, 1, status->MPI_SOURCE, status->MPI_TAG, 0, global_test_id);
  }
  recording_event_list->push(test_event);
  return ret;
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
  MPI_Status status;
  rempi_event *replaying_test_event;
  int has_next_event = REMPI_MPI_EVENT_WITH_NEXT;
  int recv_request_count, null_request_count, valid_request_count;
  int test_id;
  int replay_queue_status;
  MPI_Comm comm;
  rempi_irecv_inputs *irecv_inputs;

  recv_request_count = rempi_reqmg_get_recv_request_count(incount, array_of_requests);
  null_request_count = rempi_reqmg_get_null_request_count(incount, array_of_requests);
  valid_request_count = recv_request_count + null_request_count;

  // REMPI_DBG("recv_request_count: %d, null_request_count:%d, incount: %d", 
  // 	      recv_request_count, null_request_count, incount);

  if (valid_request_count == 0) {
    /*All send requests*/
    if(mf_flag_1 == REMPI_MF_FLAG_1_TEST) {
      switch (mf_flag_2) {
      case REMPI_MF_FLAG_2_SINGLE:
	return PMPI_Test(array_of_requests, outcount, array_of_statuses);
      case REMPI_MF_FLAG_2_ANY:
	return PMPI_Testany(incount, array_of_requests, outcount, array_of_indices, array_of_statuses);
      case REMPI_MF_FLAG_2_SOME:
	return PMPI_Testsome(incount, array_of_requests, outcount, array_of_indices, array_of_statuses);
      case REMPI_MF_FLAG_2_ALL:
	return PMPI_Testall(incount, array_of_requests, outcount, array_of_statuses);
      }
    } else if (mf_flag_1 == REMPI_MF_FLAG_1_WAIT) {
      switch (mf_flag_2) {
      case REMPI_MF_FLAG_2_SINGLE:
	return PMPI_Wait(array_of_requests, array_of_statuses);
      case REMPI_MF_FLAG_2_ANY:
	return PMPI_Waitany(incount, array_of_requests, array_of_indices, array_of_statuses);
      case REMPI_MF_FLAG_2_SOME:
	return PMPI_Waitsome(incount, array_of_requests, outcount, array_of_indices, array_of_statuses);
      case REMPI_MF_FLAG_2_ALL:
	*outcount = incount;
	return PMPI_Waitall(incount, array_of_requests, array_of_statuses);
      }
    } else {
      REMPI_ERR("No such MPI matching function");
    }
  } else if (valid_request_count != incount || null_request_count > 0) {
    REMPI_ERR("Both send requests and recv requestes are tested once: "
	      "recv_request_count: %d, null_request_count:%d, incount: %d", 
	      recv_request_count, null_request_count, incount);
  }
  test_id = rempi_reqmg_get_test_id(array_of_requests, incount);
  
  /*Matched*/
  int index = 0;
  *outcount = 0;
  while (has_next_event == REMPI_MPI_EVENT_WITH_NEXT) {
    while ((replaying_test_event = replaying_event_list->dequeue_replay(test_id, replay_queue_status)) == NULL);
    if (replaying_test_event->get_flag() == 0) {
      /*Unmatched*/
      delete replaying_test_event;
      goto end;
    }

    irecv_inputs = request_to_irecv_inputs_umap[array_of_requests[index]];
    if ((irecv_inputs->source == MPI_ANY_SOURCE ||irecv_inputs->source == replaying_test_event->get_source())
	// tag is not recorded.
	//	&& (irecv_inputs->tag == MPI_ANY_TAG ||irecv_inputs->tag == replaying_test_event->get_tag())
	) 
      {
      // REMPI_DBG("test: source: %d, tag: %d, flag: %d", 
      // 		replaying_test_event->get_source(), irecv_inputs->tag, replaying_test_event->get_flag());
      PMPI_Probe(replaying_test_event->get_source(), irecv_inputs->tag,
		 irecv_inputs->comm, &status);
      PMPI_Recv(
	 irecv_inputs->buf, 
	 irecv_inputs->count,
	 irecv_inputs->datatype,
	 status.MPI_SOURCE,
	 status.MPI_TAG,
	 irecv_inputs->comm, &status);
      
      if (array_of_indices != NULL) {
      	array_of_indices[*outcount]  = index;
      }
      array_of_statuses[*outcount].MPI_SOURCE = status.MPI_SOURCE;
      array_of_statuses[*outcount].MPI_TAG    = status.MPI_TAG;

      rempi_reqmg_deregister_recv_request(&array_of_requests[index]);
      request_to_irecv_inputs_umap.erase(array_of_requests[index]);
      free(array_of_requests[index]);
      array_of_requests[index] = MPI_REQUEST_NULL;
      (*outcount)++;
      has_next_event = replaying_test_event->get_is_testsome();
      delete replaying_test_event;
    } 
    index++;
  }

 end:
  update_validation_code(*outcount, array_of_indices, array_of_statuses);
  return MPI_SUCCESS;
}


int rempi_recorder::replay_iprobe(int source, int tag, MPI_Comm comm, int *flag, MPI_Status *status, int comm_id)
{
  rempi_event *replaying_test_event;
  int has_next_event = REMPI_MPI_EVENT_WITH_NEXT;
  int test_id;
  int replay_queue_status;

  MPI_Request req = NULL;
  rempi_reqmg_register_recv_request(&req, source, tag, comm_id);
  test_id = rempi_reqmg_get_test_id(&req, 1);
  
  /*Matched*/
  *flag = 0;
  while (has_next_event == REMPI_MPI_EVENT_WITH_NEXT) {
    while ((replaying_test_event = replaying_event_list->dequeue_replay(test_id, replay_queue_status)) == NULL);
    if (replaying_test_event->get_flag() == 0) {
      /*Unmatched*/
      delete replaying_test_event;
      return MPI_SUCCESS;
    }
    *flag = 1;

    if ((source == MPI_ANY_SOURCE ||source == replaying_test_event->get_source())
	// tag is not recorded.
	//	&& (irecv_inputs->tag == MPI_ANY_TAG ||irecv_inputs->tag == replaying_test_event->get_tag())
	) {
      PMPI_Probe(replaying_test_event->get_source(), tag, comm, status);
      has_next_event = replaying_test_event->get_is_testsome();
      delete replaying_test_event;
    } 
  }

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
