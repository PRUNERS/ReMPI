#include <stdio.h>

#include <unordered_map>
#include <string>

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
#include "rempi_mem.h"
#include "pnmpimod.h"
#include "clmpi.h"

#define  PNMPI_MODULE_REMPI "rempi"

// void rempi_irecv_inputs::insert_request(MPI_Request* proxy_request, void* proxy_buf)
// {
//   if (proxy_requests_umap.find(proxy_request) == proxy_requests_umap.end()) {
//     REMPI_ERR("The request is alread in the proxy_requests map");
//   }
//   proxy_requests_umap[proxy_request] = proxy_buf;
//   return;
// }

// void rempi_irecv_inputs::erase_request(MPI_Request* proxy_request)
// {
//   if(!proxy_requests_umap.erase(proxy_request)) {
//     REMPI_ERR("key %p does not exist in proxy request map", proxy_request);
//   }
//   return;
// }


void* rempi_recorder_cdc::allocate_proxy_buf(int count, MPI_Datatype datatype)
{
  int datatype_size;
  MPI_Type_size(datatype, &datatype_size);
  return rempi_malloc(datatype_size * count - sizeof(PNMPI_MODULE_CLMPI_PB_TYPE));
}

void rempi_recorder_cdc::copy_proxy_buf(void* from, void* to, int count, MPI_Datatype datatype)
{
  // TODO: if datatype is self-defined one, simple memcpy does not work
  int datatype_size;
  if (datatype == MPI_UNSIGNED_LONG_LONG ||
      datatype == MPI_INT || 
      datatype == MPI_CHAR) {
    PMPI_Type_size(datatype, &datatype_size);
    memcpy(to, from, datatype_size * count - sizeof(size_t)); 
  } else {
    REMPI_ERR("Unsurpported MPI_Datatype: %d", datatype);
  }

  return ;
}

/*get_test_id must be called under MPI_Irecv*/
// int rempi_recorder_cdc::get_test_id()
// {
//    string test_id_string;
//   test_id_string = rempi_btrace_string();
//   if (stacktrace_to_test_id_umap.find(test_id_string) ==  stacktrace_to_test_id_umap.end()) {
//     stacktrace_to_test_id_umap[test_id_string] = next_test_id_to_assign;
//     next_test_id_to_assign++;
//   }
//   return stacktrace_to_test_id_umap[test_id_string];
// }


int rempi_recorder_cdc::get_recv_test_id(int test_id)
{
  if (test_id_to_recv_test_id_umap.find(test_id) == test_id_to_recv_test_id_umap.end()) {
    test_id_to_recv_test_id_umap[test_id] = next_recv_test_id_to_assign;
    next_recv_test_id_to_assign++;
  }
  return test_id_to_recv_test_id_umap[test_id];
}

int rempi_recorder_cdc::init_clmpi()
{
  int err;
  PNMPI_modHandle_t handle_rempi, handle_clmpi;
  PNMPI_Service_descriptor_t serv;
  /*Load clock-mpi*/
  err=PNMPI_Service_GetModuleByName(PNMPI_MODULE_CLMPI, &handle_clmpi);
  if (err!=PNMPI_SUCCESS) {
    return err;
  }
  /*Get clock-mpi service*/
  err=PNMPI_Service_GetServiceByName(handle_clmpi,"clmpi_register_recv_clocks","pi",&serv);
  if (err!=PNMPI_SUCCESS)
    return err;
  clmpi_register_recv_clocks=(PNMPIMOD_register_recv_clocks_t) ((void*)serv.fct);

  /*Get clock-mpi service*/
  err=PNMPI_Service_GetServiceByName(handle_clmpi,"clmpi_clock_control","i",&serv);
  if (err!=PNMPI_SUCCESS)
    return err;
  clmpi_clock_control=(PNMPIMOD_clock_control_t) ((void*)serv.fct);

  /*Get clock-mpi service*/
  err=PNMPI_Service_GetServiceByName(handle_clmpi,"clmpi_get_local_clock","p",&serv);
  if (err!=PNMPI_SUCCESS)
    return err;
  clmpi_get_local_clock=(PNMPIMOD_get_local_clock_t) ((void*)serv.fct);

  /*Get clock-mpi service*/
  err=PNMPI_Service_GetServiceByName(handle_clmpi,"clmpi_sync_clock","i",&serv);
  if (err!=PNMPI_SUCCESS)
    return err;
  clmpi_sync_clock=(PNMPIMOD_sync_clock_t) ((void*)serv.fct);

  /*Load own moduel*/
  err=PNMPI_Service_GetModuleByName(PNMPI_MODULE_REMPI, &handle_rempi);
  if (err!=PNMPI_SUCCESS)
    return err;

  return err;
}

int rempi_recorder_cdc::record_init(int *argc, char ***argv, int rank) 
{
  string id;

  //fprintf(stderr, "ReMPI: Function call (%s:%s:%d)\n", __FILE__, __func__, __LINE__);
  id = std::to_string((long long int)rank);
  replaying_event_list = new rempi_event_list<rempi_event*>(10000000, 100);
  recording_event_list = new rempi_event_list<rempi_event*>(10000000, 100);
  record_thread = new rempi_io_thread(recording_event_list, replaying_event_list, id, rempi_mode, &mc_encoder); //0: recording mode
  record_thread->start();
  
  return 0;
}

int rempi_recorder_cdc::replay_init(int *argc, char ***argv, int rank) 
{
  string id;

  init_clmpi();
  id = std::to_string((long long int)rank);
  /*recording_event_list is needed for CDC, and when switching from repla mode to recording mode */
  recording_event_list = new rempi_event_list<rempi_event*>(10000000, 100); 
  replaying_event_list = new rempi_event_list<rempi_event*>(10000000, 100);
  read_record_thread = new rempi_io_thread(recording_event_list, replaying_event_list, id, rempi_mode, &mc_encoder); //1: replay mode
  //  REMPI_DBG("io thread is not runnig");
  read_record_thread->start();
  return 0;
}

int rempi_recorder_cdc::record_irecv(
   void *buf,
   int count,
   MPI_Datatype datatype,
   int source,
   int tag,
   int comm, // The value is set by MPI_Comm_set_name in ReMPI_convertor
   MPI_Request *request)
{
  // int test_id = get_test_id();
  // if (request_to_test_id_umap.find(request) == request_to_test_id_umap.end()) {
  //   request_to_test_id_umap[request] = test_id;
  //   REMPI_DBGI(0, "Map: %p -> %d", request, test_id);
  // }
  return 0;
}

int rempi_recorder_cdc::replay_irecv(
   void *buf,
   int count,
   MPI_Datatype datatype,
   int source,
   int tag,
   int comm_id, // The value is set by MPI_Comm_set_name in ReMPI_convertor
   MPI_Comm *comm, // The value is set by MPI_Comm_set_name in ReMPI_convertor
   MPI_Request *request)
{
  int ret;
  void *proxy_buf;
  MPI_Request *proxy_request;
  rempi_proxy_request *proxy_request_info;
  rempi_irecv_inputs *irecv_inputs;

  if (request_to_irecv_inputs_umap.find(*request) != 
      request_to_irecv_inputs_umap.end()) {
    /* If this request is posted in irecv before*/
    irecv_inputs = request_to_irecv_inputs_umap[*request];
    bool same_source = (irecv_inputs->source == source);
    bool same_tag    = (irecv_inputs->tag    == tag);
    bool same_comm   = (irecv_inputs->comm   == *comm);
    if (!same_source || !same_tag || !same_comm) {
      REMPI_ERR("Different request in (source, tag, comm): (%d, %d, %p) != (%d, %d, %p)",
       		irecv_inputs->source, irecv_inputs->tag, irecv_inputs->comm,
		source, tag, *comm);

      // REMPI_ERR("This MPI_Request is not diactivated. This mainly caused by irecv call "
      // 		"with MPI_Request which is not diactivated by MPI_{Wait|Test}{|some|any|all}");
    }
  } else {
    *request = (MPI_Request)rempi_malloc(sizeof(MPI_Request));//((source + 1) * (tag + 1) * (comm_id * 1));
    request_to_irecv_inputs_umap[*request] = new rempi_irecv_inputs(buf, count, datatype, source, tag, *comm, *request);
  }

  irecv_inputs = request_to_irecv_inputs_umap[*request];

  if (irecv_inputs->request_proxy_list.size() == 0) {

    //    proxy_buf = allocate_proxy_buf(count, datatype);
    proxy_request_info = new rempi_proxy_request(count, datatype);
    //    proxy_request = (MPI_Request*)rempi_malloc(sizeof(MPI_Request));
    irecv_inputs->request_proxy_list.push_back(proxy_request_info);
    ret = PMPI_Irecv(proxy_request_info->buf, count, datatype, source, tag, *comm, &proxy_request_info->request);
  } else {
    /*Irecv request with the same (source, tag, comm) was already posted, so simply return MPI_SUCCESS*/
    ret = MPI_SUCCESS;
  }

#ifdef REMPI_DBG_REPLAY
  REMPI_DBGI(REMPI_DBG_REPLAY, "  Irecv called (source:%d, tag:%d, proxy_length:%d)", source, tag, irecv_inputs->request_proxy_list.size());
#endif
  
  return ret;
}

int rempi_recorder_cdc::replay_cancel(MPI_Request *request)
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

  return ret;
}


int rempi_recorder_cdc::record_test(
    MPI_Request *request,
    int *flag,
    int source,
    int tag,
    int clock,
    int with_previous)
{
  return record_test(request, flag, source, tag, clock, with_previous, -1);
}

int rempi_recorder_cdc::record_test(
    MPI_Request *request,
    int *flag,
    int source,
    int tag,
    int clock,
    int with_previous,
    int test_id)
{
  int event_count = 1;
  int request_val = -1;
  int record_comm_id = 0;
  int record_source = 0;
  int record_tag = 0;
  int record_clock = 0;
  //  REMPI_DBG("flag: %d , source: %d", *flag, source);
  if (*flag) {
    /*Query befoer add_matched_recv, because pendding entry is removed when flag == 1*/
    //kento    record_comm_id = msg_manager.query_pending_comm_id(request);
    record_source  = source;
    record_tag     = tag;
    record_clock   = clock;
    //kento msg_manager.add_matched_recv(request, source, tag);
    //    REMPI_DBG("%d %d %d %d %d %d %d", event_count, is_testsome, comm_id, *flag, source, tag, clock);
  } 

  // REMPI_DBGI(0, "Map: %p -> ???", request);
  // if (request_to_test_id_umap.find(request) == request_to_test_id_umap.end()) {
  //   REMPI_DBGI(0, "Map: %p(%p) -> ?", request, *request);
  //   REMPI_ERR("No test_id for this MPI_Request");
  // }
  // test_id = request_to_test_id_umap[request];
#ifdef REMPI_DBG_REPLAY
  REMPI_DBGI(REMPI_DBG_REPLAY, "= Record: (count: %d, with_next: %d, flag: %d, source: %d, tag: %d, clock: %d): test_id: %d",
	     event_count, with_previous, *flag, record_source, record_tag, record_clock, test_id);
#endif
  // if (*flag == 1) {
  //   REMPI_DBGI(0, "= Record  : (count: %d, with_next: %d, flag: %d, source: %d, tag: %d, clock: %d): test_id: %d",
  // 	       event_count, with_previous, *flag, record_source, record_tag, record_clock, test_id);
  // }
    // REMPI_DBG("Record  : (count: %d, with_next: %d, flag: %d, source: %d, tag: %d, clock: %d)",
  //    event_count, with_previous, *flag,                                      
  // 	     record_source, record_tag, record_clock);
  recording_event_list->push(new rempi_test_event(event_count, with_previous, record_comm_id, *flag, record_source, record_tag, record_clock, test_id));

  return 0;
}

int rempi_recorder_cdc::replay_test(
                                MPI_Request *request,
                                int *flag,
                                MPI_Status *status,
				int test_id)
{
  int index;
  int ret = replay_testsome(1, request, flag, &index, status, test_id);
  return ret;
}

/*This function is called after MPI_Test*/
// int rempi_recorder_cdc::replay_test(
//     MPI_Request *request_in,
//     /*Not used*/    int flag_in, 
//     /*Not used*/    int source_in,
//     /*Not used*/    int tag_in,
//     /*Not used*/    int clock_in,
//     /*Not used*/    int with_previous_in,
//     int test_id_in,
//     int *flag_out,
//     int *source_out,
//     int *tag_out)
// {
//   MPI_Status status;
//   rempi_event *replaying_test_event = NULL;
//   int event_list_status;
//   int flag;
//   int ret;
//   unordered_map<MPI_Request, rempi_irecv_inputs*>::iterator request_to_irecv_inputs_umap_it;
  
//   return PMPI_Test(request_in, flag_out, &status);

// #if 0  
//   if (request_to_irecv_inputs_umap.find(*request_in) == request_to_irecv_inputs_umap.end()) {
//     /*This request is for Isend, so simply call PMPI_Test and return*/
//     ret = PMPI_Test(reqeust_in, flag_out, status);
//     sourct_out = status.MPI_SOURCE;
//     tag_out    = status.MPI_TAG;
//     return ret;
//   }
  
//   /*1. Get replaying event until REMPI_EVENT_LIST_FINISH */
//   while ((replaying_test_event = replaying_event_list->dequeue_replay(test_id_in, event_list_status)) == NULL) {
//     if (event_list_status == REMPI_EVENT_LIST_FINISH) {
//       replaying_test_event = replaying_event_list->dequeue_replay(test_id_in, event_list_status)
//       REMPI_ERR("No more replay event. Will switch to recording mode, but not implemented yet: event: %p", replaying_test_event);
//       return 0;
//     } else if (event_list_status == REMPI_EVENT_LIST_EMPTY) {
      
//     } else {
//       REMPI_ERR("No known event_list_status");
//     } 
//     for (int i = 0; i < proxy_request_pool_list.size(); i++) {
//       MPI_Request *proxy_reqeust;
//       MPI_Status  proxy_status;
//       int flag;
//       proxy_request_pool_list[i].request;
//       PMPI_Test(&proxy_request, &flag, &status);
//     }
//     request_to_irecv_inputs_umap[*request_in];
//     // for (request_to_irecv_inputs_umap_it  = request_to_irecv_inputs_umap.begin();
//     // 	 request_to_irecv_inputs_umap_it != request_to_irecv_inputs_umap.end();
//     // 	 request_to_irecv_inputs_umap_it++) {
//     //   MPI_Request *request;
//     //   rempi_irecv_inputs *inputs = request_to_irecv_inputs_umap_it->second;
//     // }

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
//       REMPI_ERR("An unexpected message is proved: tag_out checking may not be needed, "
// 		"because some compression approach do not record tag, and do not replay tag");
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
//       REMPI_ERR("An unexpected message is waited: tag_out checking may not be needed, "
// 		"because some compression approach do not record tag, and do not replay tag");
//     }
//     delete request_to_irecv_inputs_umap[*request_in];
//     request_to_irecv_inputs_umap.erase(*request_in);
//   }

//   return 0;
// #endif
//   /*====================*/

//   /* So "replayint_test_event" event flag == 1*/
//   /*
//     2. Wait until this recorded message really arrives
//        if (0 if next recorded maching is not mached in this run) {
//           TODO: Wait until maching, and get clock
// 	  TODO: if matched, memorize that the matching for the next replay test
//        }
//   */

//   // while (!msg_manager.is_matched_recv(
//   // 	      replaying_test_event->get_source(), 
//   // 	      replaying_test_event->get_tag(),
//   // 	      replaying_test_event->get_comm_id(),
//   // 	      replaying_test_event->get_test_id())) { //replaying_test_event->get_test_id() == test_id_in
//   //   msg_manager.refresh_matched_recv(test_id_in);
//   // }

//   // msg_manager.remove_matched_recv(
//   // 	      replaying_test_event->get_source(), 
//   // 	      replaying_test_event->get_tag(),
//   // 	      replaying_test_event->get_comm_id(), 
//   // 	      replaying_test_event->get_test_id());
//   // /*
//   //   3. Set valiabiles (source, flag, tag)
//   // }
//   //  */
//   // *flag_out = 1;
//   // *source_out = replaying_test_event->get_source();
//   // *tag_out = replaying_test_event->get_tag();

//   //  return 0;
  
//   /*==========================*/
//   /**/
//   // if (flag_in) {
//   //   int event_count = 1;
//   //   int with_previous = 0; //
//   //   int comm_id = 0;
//   //   msg_manager.add_matched_recv(request_in, source_in, tag_in);
//   //   recording_event_list->push(new rempi_test_event(event_count, with_previous, comm_id, flag_in, source_in, tag_in, clock_in, test_id_in));
//   // }



//   // REMPI_DBG("flag: %d , source: %d", replaying_test_event->get_flag(), replaying_test_event->get_source());
// }


/*Test all recv requests, and if matched, put the events to the queue*/
bool rempi_recorder_cdc::progress_recv_requests(int recv_test_id,
						int incount,
						MPI_Request array_of_requests[])
{
  int flag;
  rempi_proxy_request *proxy_request;
  MPI_Status status;
  size_t clock;
  rempi_proxy_request *next_proxy_request;
  rempi_event *event_pooled;
  bool is_progressed = false;
  rempi_irecv_inputs *irecv_inputs;
  int matched_request_push_back_count = 0;

  for (auto &pair_req_and_irecv_inputs: request_to_irecv_inputs_umap) {
    irecv_inputs = pair_req_and_irecv_inputs.second;
    /* Find out this request belongs to which MF (recv_test_id) */
    if (irecv_inputs->recv_test_id == -1) {
      for (int i = 0; i < incount; i++) {
	if (irecv_inputs->request == array_of_requests[i]) {
	  irecv_inputs->recv_test_id = recv_test_id;
	}
      }
    } else if (irecv_inputs->recv_test_id != recv_test_id) {
      for (int i = 0; i < incount; i++) {
	if (irecv_inputs->request == array_of_requests[i]) {
	  REMPI_ERR("A MPI request is used in different MFs: "
		    "this request used for recv_test_id: %d first, but this is recv_test_id: %d",
		    irecv_inputs->recv_test_id, recv_test_id);
	}
      }
    } 

    /* =================================================
       1. Check if there are any matched requests (pending matched), 
            which have already matched in different recv_test_id
       ================================================= */
    if (irecv_inputs->recv_test_id != -1) {
      matched_request_push_back_count = 0;
      for (rempi_proxy_request *proxy_request: irecv_inputs->matched_pending_request_proxy_list) {
	REMPI_ASSERT(proxy_request->matched_source != -1);
	REMPI_ASSERT(proxy_request->matched_tag    != -1);
	REMPI_ASSERT(proxy_request->matched_clock  !=  0);
	REMPI_ASSERT(proxy_request->matched_count  != -1);
	
	event_pooled =  new rempi_test_event(1, -1, -1, 1, 
					     proxy_request->matched_source, 
					     proxy_request->matched_tag, 
					     proxy_request->matched_clock, 
					     irecv_inputs->recv_test_id);
	event_pooled->msg_count = proxy_request->matched_count;
	recording_event_list->enqueue_replay(event_pooled, irecv_inputs->recv_test_id);
#ifdef REMPI_DBG_REPLAY	  
      REMPI_DBGI(REMPI_DBG_REPLAY, "A->RCQ  : (count: %d, with_next: %d, flag: %d, source: %d, tag: %d,  clock: %d, msg_count: %d %p): recv_test_id: %d",
		 event_pooled->get_event_counts(), event_pooled->get_is_testsome(), event_pooled->get_flag(),
		 event_pooled->get_source(), event_pooled->get_tag(), event_pooled->get_clock(), event_pooled->msg_count,
		 event_pooled, irecv_inputs->recv_test_id);
#endif
	/*Move from pending request list to matched request list 
	  This is used later for checking if a replayed event belongs to this list.  */
	irecv_inputs->matched_request_proxy_list.push_back(proxy_request);
	matched_request_push_back_count++;
      }
      for (int i = 0; i < matched_request_push_back_count; i++) {
	irecv_inputs->matched_pending_request_proxy_list.pop_front();
      }
    }
    /* ================================================= */

    
    /* =================================================
       2. Check if this request have matched.
       ================================================= */
    proxy_request = irecv_inputs->request_proxy_list.front();
    clmpi_register_recv_clocks(&clock, 1);
    clmpi_clock_control(0);
    flag = 0;
    PMPI_Test(&proxy_request->request, &flag, &status);
    clmpi_clock_control(1);
    
    if(!flag) continue;
    is_progressed = true;
    /*If flag == 1*/
    
    if (irecv_inputs->recv_test_id != -1) {
      //      REMPI_ASSERT(irecv_inputs->recv_test_id == recv_test_id);
      event_pooled =  new rempi_test_event(1, -1, -1, 1, status.MPI_SOURCE, status.MPI_TAG, clock, irecv_inputs->recv_test_id);

      PMPI_Get_count(&status, irecv_inputs->datatype, &(event_pooled->msg_count));
      recording_event_list->enqueue_replay(event_pooled, irecv_inputs->recv_test_id);

#ifdef REMPI_DBG_REPLAY	  
      REMPI_DBGI(REMPI_DBG_REPLAY, "A->RCQ  : (count: %d, with_next: %d, flag: %d, source: %d, tag: %d,  clock: %d, msg_count: %d %p): recv_test_id: %d",
		 event_pooled->get_event_counts(), event_pooled->get_is_testsome(), event_pooled->get_flag(),
		 event_pooled->get_source(), event_pooled->get_tag(), event_pooled->get_clock(), event_pooled->msg_count,
		 event_pooled, irecv_inputs->recv_test_id);
#endif

      /*Move from pending request list to matched request list 
	This is used later for checking if a replayed event belongs to this list.  */
      irecv_inputs->matched_request_proxy_list.push_back(proxy_request);
#if 0
      REMPI_DBGI(0, "matched_proxy_request(%d) is added: irecv(%p)->%p size:%d", i, irecv_inputs, proxy_request,
		 irecv_inputs->matched_request_proxy_list.size());
#endif
      irecv_inputs->request_proxy_list.pop_front();
    } else {
      REMPI_ASSERT(irecv_inputs->recv_test_id != recv_test_id);
      proxy_request->matched_source = status.MPI_SOURCE;
      proxy_request->matched_tag    = status.MPI_TAG;
      proxy_request->matched_clock    = clock;
      PMPI_Get_count(&status, irecv_inputs->datatype, &(proxy_request->matched_count));
      irecv_inputs->matched_pending_request_proxy_list.push_back(proxy_request);
      irecv_inputs->request_proxy_list.pop_front();
    }
    /* push_back  new proxy */
    next_proxy_request = new rempi_proxy_request(irecv_inputs->count, irecv_inputs->datatype);
    irecv_inputs->request_proxy_list.push_back(next_proxy_request);
    PMPI_Irecv(next_proxy_request->buf, 
	       irecv_inputs->count, irecv_inputs->datatype, irecv_inputs->source, irecv_inputs->tag, irecv_inputs->comm, 
	       &next_proxy_request->request);
  }
  return is_progressed;
}

int rempi_recorder_cdc::replay_testsome(
					int incount,
					MPI_Request array_of_requests[],
					int *outcount,
					int array_of_indices[],
					MPI_Status array_of_statuses[],
					int global_test_id)
{
  int ret;
  int with_next = REMPI_MPI_EVENT_WITH_NEXT;
  rempi_irecv_inputs *irecv_inputs;
  size_t clock;
  size_t *clocks;
  rempi_event *replaying_event;
  vector<rempi_event*> replaying_event_vec; /*TODO: vector => list*/
  int recv_test_id = -1;
  //  unordered_uset;


#ifdef REMPI_DBG_REPLAY
  REMPI_DBGI(REMPI_DBG_REPLAY, "testsome call: original test_id %d", global_test_id);
#endif

#ifdef REMPI_DBG_REPLAY	  
  // for (int i = 0; i < incount; i++) {
  //   if (request_to_irecv_inputs_umap.find(array_of_requests[i]) != request_to_irecv_inputs_umap.end()) {
  //     rempi_proxy_request *proxy_request;
  //     irecv_inputs = request_to_irecv_inputs_umap[array_of_requests[i]];
  //     proxy_request = irecv_inputs->request_proxy_list.front();
  //     REMPI_DBGI(REMPI_DBG_REPLAY, "   Test: request:%p(%p) source:%d tag:%d count:%d", &proxy_request->request, proxy_request, irecv_inputs->source, irecv_inputs->tag, irecv_inputs->count);
  //   }
  // }
#endif

  bool is_all_recv_reqs = false;
  { /*Check if array_of_requests has both send & recv request*/
    bool has_send_req = false;
    bool has_recv_req = false;    
    for (int i = 0; i < incount; i++) {
      if (request_to_irecv_inputs_umap.find(array_of_requests[i]) == request_to_irecv_inputs_umap.end()) {
	has_send_req = true;
      } else {
	has_recv_req = true;
      }
    }
    if (has_send_req == true && has_recv_req == true) {
      /*TODO: Can we remove this limitation ? */
      REMPI_ERR("Currenty ReMPI does not support a Test family having both send and recv request within a single Test family call");
    } else if (has_send_req == false && has_recv_req == true) {
      is_all_recv_reqs = true;
    } else {
      is_all_recv_reqs = false;
    }
  }
    
  if (!is_all_recv_reqs) { 
    mc_encoder->update_fd_next_clock(0, 0, 0, 0, 0); /*Update next sending out clock for frontier detection*/
    /* If this test call is all for Send requests */
    clocks = (size_t*)rempi_malloc(sizeof(size_t) * incount);
    clmpi_register_recv_clocks(clocks, incount);
#ifdef REMPI_DBG_REPLAY	  
    for (int j = 0; j < incount; j++) {
      REMPI_DBGI(REMPI_DBG_REPLAY, "Isend:  index %d: request: %p", j, array_of_requests[j]);
    }
#endif
    ret = PMPI_Testsome(incount, array_of_requests, outcount, array_of_indices, array_of_statuses);
#ifdef REMPI_DBG_REPLAY	  
    for (int j = 0; j < *outcount; j++) {
    	REMPI_DBGI(REMPI_DBG_REPLAY, "Isend:  matched index: %d", array_of_indices[j]);
    }
#endif
    free(clocks);
    return ret;
  }
  /* else, this test call is all for Recv requests*/

  recv_test_id = get_recv_test_id(global_test_id);
#ifdef REMPI_DBG_REPLAY
  REMPI_DBGI(REMPI_DBG_REPLAY, "Testsome recv call: global_test_id: %d => recv_test_id: %d", global_test_id, recv_test_id);
#endif
  while (with_next == REMPI_MPI_EVENT_WITH_NEXT) {
    int unmatched_flag_count = 0;
    int min_recv_rank;
    size_t min_next_clock;
    bool is_progressed = false;

    /*The last two 0s are not used, if the first vaiable is 0 
      Update next sending out clock for frontier detection*/
    mc_encoder->update_fd_next_clock(0, 0, 0, 0, 0); /*Update next sending out clock for frontier detection*/
    /* ======================================================== */
    /* Frontier detection: Step 1                               */
    /* ======================================================== */
    /* First, fetch next_clock from senders, and compute local_min_id. 
       This function needs to be called before PMPI_Test. 
       If flag=0, we can make sure there are no in-flight messages, and 
       local_min_id is really minimal. */
    mc_encoder->fetch_local_min_id(&min_recv_rank, &min_next_clock);
    unmatched_flag_count = 0;
    /* ======================================================== */

    is_progressed = progress_recv_requests(recv_test_id, incount, array_of_requests);
// #ifdef REMPI_DBG_REPLAY
//     REMPI_DBGI(REMPI_DBG_REPLAY, "progrell: global: %d recv: %d", global_test_id, recv_test_id);
// #endif

    bool iprobe_flag = false;
    int iprobe_flag_int = 0;
    PMPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &iprobe_flag_int, MPI_STATUS_IGNORE);
    if (iprobe_flag_int) iprobe_flag = true;
    is_progressed = is_progressed || iprobe_flag;

    /* ======================================================== */
    /* Frontier detection: Step 2                               */
    /* ======================================================== */
    if (!is_progressed) {
      //      REMPI_DBGI(REMPI_DBG_REPLAY, " ==== update start");
      mc_encoder->update_local_min_id(min_recv_rank, min_next_clock);
      //      REMPI_DBGI(REMPI_DBG_REPLAY, " ==== update end");
    }
    /* ======================================================== */


    int event_list_status;
    int num_of_recv_msg_in_next_event = 0;
    size_t interim_min_clock_in_next_event = 0;
    if (mc_encoder->num_of_recv_msg_in_next_event != NULL && 
	mc_encoder->interim_min_clock_in_next_event != NULL) {
      num_of_recv_msg_in_next_event = mc_encoder->num_of_recv_msg_in_next_event[recv_test_id];
      /* ===== NOTE ================================================================================================
	 If num_of_recv_msg_in_next_event > 0 (Condition A), this means CDC now trying to replay recv event next. 
	 So if replaying_events_list->dequeue_replay does not return enough replay events (Condition B), this means 
	 this main thread deuque "recv" events next
	 -- Main thread --
	 Get  : num_of_recv_msg_in_next_event & interim_min_clock_in_next_event
	 Test : Any in Replay_queue ?
	 if (Test is false) Update : fd_next_clock
	 -----------------
	 -- CDC thread --
	 Test : Is replaying matched events ?
	 if (Test is true) Update: num_of_recv_msg_in_next_event & interim_min_clock_in_next_event
	 if (Replaying events) : num_of_recv_msg_in_next_event = 0
	 Update: Replay_queue
	 -------------------
	 ===========================================================================================================*/
      if (mc_encoder->interim_min_clock_in_next_event == NULL) { /*TODO: remove this because this is already checked in the above*/
	REMPI_ERR("interim: %p", mc_encoder->interim_min_clock_in_next_event);
      }
      interim_min_clock_in_next_event = mc_encoder->interim_min_clock_in_next_event[recv_test_id];
    }

    while ((replaying_event = replaying_event_list->dequeue_replay(recv_test_id, event_list_status)) != NULL) {
#ifdef REMPI_DBG_REPLAY
      REMPI_DBGI(REMPI_DBG_REPLAY, "RPQ->A  : (count: %d, with_next: %d, flag: %d, source: %d, clock: %d)",
		 replaying_event->get_event_counts(), replaying_event->get_is_testsome(), replaying_event->get_flag(),
		 replaying_event->get_source(), replaying_event->get_clock());
#endif
	
      replaying_event_vec.push_back(replaying_event);
      with_next = replaying_event->get_is_testsome();
      if (with_next ==  REMPI_MPI_EVENT_NOT_WITH_NEXT) {
	break;
      }
    }
    
    /*checking Condition B*/
    if (with_next !=  REMPI_MPI_EVENT_NOT_WITH_NEXT) {
      if (num_of_recv_msg_in_next_event > 0) { /*checking Condition A*/
	mc_encoder->update_fd_next_clock(1, num_of_recv_msg_in_next_event, interim_min_clock_in_next_event, 
					 recording_event_list->get_enqueue_count(recv_test_id), recv_test_id);
      }
    }


  } /* while (with_next == REMPI_MPI_EVENT_WITH_NEXT) */
  


  if (replaying_event_vec.size() == 1) { 
    if (replaying_event_vec.front()->get_flag() == 0) { /*if unmatched event*/
      rempi_event *e = replaying_event_vec.front();
      *outcount = 0;
#ifdef REMPI_DBG_REPLAY      
      REMPI_DBGI(REMPI_DBG_REPLAY, "= Replay: (count: %d, with_next: %d, flag: %d, source: %d, tag: %d, clock: %d): recv_test_id: %d",
		 e ->get_event_counts(), e ->get_is_testsome(), e ->get_flag(),
		 e ->get_source(), e->get_tag(), e ->get_clock(), recv_test_id);
#endif
      delete replaying_event_vec.front();
      return ret;
    }
  }


  /*Copy from proxy to actuall buff, then set array_of_indices/statuses, outcount*/
  *outcount = 0;
  for (int i = 0; i < incount; i++) {
    rempi_proxy_request *proxy_request;

    if (request_to_irecv_inputs_umap.find(array_of_requests[i]) != request_to_irecv_inputs_umap.end()) {

      irecv_inputs = request_to_irecv_inputs_umap[array_of_requests[i]];
      for (int j = 0; j < replaying_event_vec.size(); j++) {
	bool is_any_source =  (irecv_inputs->source == MPI_ANY_SOURCE);
	bool is_source     =  (irecv_inputs->source == replaying_event_vec[j]->get_source());

#if REMPI_DBG_REPLAY
	REMPI_DBGI(REMPI_DBG_REPLAY, "irecv_inputs.source: %d === replaying_events[j].source: %d (MPI_ANY_SOURCE: %d)", 
		   irecv_inputs->source, replaying_event_vec[j]->get_source(), MPI_ANY_SOURCE);
#endif
	/*TODO: if array_of_requests has requests from the same rank, is_source does not work. And it introduces inconsistent replay
	  Use MPI_Request pointer (is_request) instead of is_source. In MCB, this is OK.*/
	if (is_source || is_any_source) {
	  array_of_indices[*outcount] = i;
	  array_of_statuses[*outcount].MPI_SOURCE = replaying_event_vec[j]->get_source();
	  array_of_statuses[*outcount].MPI_TAG    = replaying_event_vec[j]->get_tag();
	  {
	    clmpi_sync_clock(replaying_event_vec[j]->get_clock());
	  }
	  *outcount = *outcount + 1;
	  /* copy eventvec =-> actual_buff*/
	  proxy_request = irecv_inputs->matched_request_proxy_list.front();
	  if (proxy_request == NULL) {
	    REMPI_ERR("matched_proxy_request(%d) is NULL: irecv(%p)->%p size:%d", i, irecv_inputs, proxy_request,
		      irecv_inputs->matched_request_proxy_list.size());
	  }
	  copy_proxy_buf(proxy_request->buf, irecv_inputs->buf, replaying_event_vec[j]->msg_count, irecv_inputs->datatype);
	  //copy_proxy_buf(proxy_request->buf, irecv_inputs->buf, irecv_inputs->count, irecv_inputs->datatype);
	  irecv_inputs->matched_request_proxy_list.pop_front();
#if REMPI_DBG_REPLAY
	  REMPI_DBGI(REMPI_DBG_REPLAY, "matched_proxy_request(%d) is pop: irecv(%p)->%p size:%d", i, irecv_inputs, proxy_request,
		     irecv_inputs->matched_request_proxy_list.size());
#endif
	  delete proxy_request;
	  break;
	}
      }
      /*The last two 0s are not used, if the first vaiable is 0 
	Update next sending out clock for frontier detection*/
      mc_encoder->update_fd_next_clock(0, 0, 0, 0, 0); 
    } else {
      REMPI_ERR("request does no exist in request_to_irecv_inputs_umap");
    }
  }


  if (*outcount != replaying_event_vec.size()) {
    REMPI_DBG(" ==== tete 6.0 : replayng matching count is %d, but found  %d messages", replaying_event_vec.size(), *outcount);
    for (int j = 0; j < replaying_event_vec.size(); j++) {
      REMPI_DBG(" = Wrong: (count: %d, with_next: %d, flag: %d, source: %d, clock: %d): recv_test_id: %d ",
		replaying_event_vec[j]->get_event_counts(), replaying_event_vec[j]->get_is_testsome(), replaying_event_vec[j]->get_flag(),
		replaying_event_vec[j]->get_source(), replaying_event_vec[j]->get_clock(), recv_test_id);
    }
    REMPI_ERR("All events are not replayed");
  }

    for (int j = 0; j < replaying_event_vec.size(); j++) {
#ifdef REMPI_DBG_REPLAY
      REMPI_DBGI(REMPI_DBG_REPLAY, "= Replay: (count: %d, with_next: %d, flag: %d, source: %d, tag: %d, clock: %d): recv_test_id: %d ",
		 replaying_event_vec[j]->get_event_counts(), replaying_event_vec[j]->get_is_testsome(), replaying_event_vec[j]->get_flag(),
		 replaying_event_vec[j]->get_source(), replaying_event_vec[j]->get_tag(), replaying_event_vec[j]->get_clock(), recv_test_id);
#endif
      // REMPI_DBGI(0, "= Replay: (count: %d, with_next: %d, flag: %d, source: %d, tag: %d, clock: %d): recv_test_id: %d ",
      // 		 replaying_event_vec[j]->get_event_counts(), replaying_event_vec[j]->get_is_testsome(), replaying_event_vec[j]->get_flag(),
      // 		 replaying_event_vec[j]->get_source(), replaying_event_vec[j]->get_tag(), replaying_event_vec[j]->get_clock(), recv_test_id);

      delete replaying_event_vec[j];
    }
  

    return ret;
}

// int rempi_recorder_cdc::replay_testsome(
// 					int incount,
// 					MPI_Request array_of_requests[],
// 					int *outcount,
// 					int array_of_indices[],
// 					MPI_Status array_of_statuses[],
// 					int global_test_id)
// {
//   int ret;
//   int with_next = REMPI_MPI_EVENT_WITH_NEXT;
//   rempi_irecv_inputs *irecv_inputs;
//   size_t clock;
//   size_t *clocks;
//   rempi_event *replaying_event;
//   vector<rempi_event*> replaying_event_vec; /*TODO: vector => list*/
//   int recv_test_id = -1;
//   //  unordered_uset;


// #ifdef REMPI_DBG_REPLAY
//   REMPI_DBGI(REMPI_DBG_REPLAY, "testsome call: original test_id %d", global_test_id);
// #endif

// #ifdef REMPI_DBG_REPLAY	  
//   // for (int i = 0; i < incount; i++) {
//   //   if (request_to_irecv_inputs_umap.find(array_of_requests[i]) != request_to_irecv_inputs_umap.end()) {
//   //     rempi_proxy_request *proxy_request;
//   //     irecv_inputs = request_to_irecv_inputs_umap[array_of_requests[i]];
//   //     proxy_request = irecv_inputs->request_proxy_list.front();
//   //     REMPI_DBGI(REMPI_DBG_REPLAY, "   Test: request:%p(%p) source:%d tag:%d count:%d", &proxy_request->request, proxy_request, irecv_inputs->source, irecv_inputs->tag, irecv_inputs->count);
//   //   }
//   // }
// #endif

//   bool is_all_recv_reqs = false;
//   { /*Check if array_of_requests has both send & recv request*/
//     bool has_send_req = false;
//     bool has_recv_req = false;    
//     for (int i = 0; i < incount; i++) {
//       if (request_to_irecv_inputs_umap.find(array_of_requests[i]) == request_to_irecv_inputs_umap.end()) {
// 	has_send_req = true;
//       } else {
// 	has_recv_req = true;
//       }
//     }
//     if (has_send_req == true && has_recv_req == true) {
//       /*TODO: Can we remove this limitation ? */
//       REMPI_ERR("Currenty ReMPI does not support a Test family having both send and recv request within a single Test family call");
//     } else if (has_send_req == false && has_recv_req == true) {
//       is_all_recv_reqs = true;
//     } else {
//       is_all_recv_reqs = false;
//     }
//   }
    
//   if (!is_all_recv_reqs) { 
//     /* If this test call is all for Send requests */
//     clocks = (size_t*)rempi_malloc(sizeof(size_t) * incount);
//     clmpi_register_recv_clocks(clocks, incount);
// #ifdef REMPI_DBG_REPLAY	  
//     // for (int j = 0; j < incount; j++) {
//     // 	REMPI_DBGI(REMPI_DBG_REPLAY, "Isend:  index %d: request: %p", i;
//     // }
// #endif
//     ret = PMPI_Testsome(incount, array_of_requests, outcount, array_of_indices, array_of_statuses);
// #ifdef REMPI_DBG_REPLAY	  
//     // for (int j = 0; j < *outcount; j++) {
//     // 	REMPI_DBGI(REMPI_DBG_REPLAY, "Isend:  matched index: %d", array_of_indices[i]);
//     // }
// #endif
//     free(clocks);
//     return ret;
//   }
//   /* else, this test call is all for Recv requests*/


//   while (with_next == REMPI_MPI_EVENT_WITH_NEXT) {
//     int unmatched_flag_count = 0;
//     int min_recv_rank;
//     size_t min_next_clock;

//     /*The last two 0s are not used, if the first vaiable is 0 
//       Update next sending out clock for frontier detection*/
//     mc_encoder->update_fd_next_clock(0, 0, 0, 0, 0); /*Update next sending out clock for frontier detection*/
//     /* ======================================================== */
//     /* Frontier detection: Step 1                               */
//     /* ======================================================== */
//     /* First, fetch next_clock from senders, and compute local_min_id. 
//        This function needs to be called before PMPI_Test. 
//        If flag=0, we can make sure there are no in-flight messages, and 
//        local_min_id is really minimal. */
//     mc_encoder->fetch_local_min_id(&min_recv_rank, &min_next_clock);
//     unmatched_flag_count = 0;
//     /* ======================================================== */


//     for (int i = 0; i < incount; i++) {
//       /*If this is testsome for recv*/
//       int flag = 0;
//       rempi_proxy_request *proxy_request;
//       MPI_Status status;


//       /*If this request is for irecv (not isend)*/
//       irecv_inputs = request_to_irecv_inputs_umap[array_of_requests[i]];
//       proxy_request = irecv_inputs->request_proxy_list.front();
//       recv_test_id = get_recv_test_id(global_test_id);

// #ifdef REMPI_DBG_REPLAY
//       //	REMPI_DBGI(REMPI_DBG_REPLAY, "   Test: request:%p(%p) source:%d tag:%d size:%d req:%p", &proxy_request->request, proxy_request, array_of_statuses[i].MPI_SOURCE, array_of_statuses[i].MPI_TAG, irecv_inputs->request_proxy_list.size(), array_of_requests[i]);
// #endif

//       clmpi_register_recv_clocks(&clock, 1);
//       clmpi_clock_control(0);

//       PMPI_Test(&proxy_request->request, &flag, &status);

//       clmpi_clock_control(1);

//       if (flag) {
// 	rempi_proxy_request *next_proxy_request;
// 	rempi_event *event_pooled;


// 	/* enqueue */
// 	event_pooled =  new rempi_test_event(1, -1, -1, 1, status.MPI_SOURCE, status.MPI_TAG, clock, recv_test_id);

// 	PMPI_Get_count(&status, irecv_inputs->datatype, &(event_pooled->msg_count));
// 	recording_event_list->enqueue_replay(event_pooled, recv_test_id);

// #ifdef REMPI_DBG_REPLAY	  
// 	REMPI_DBGI(REMPI_DBG_REPLAY, "A->RCQ  : (count: %d, with_next: %d, flag: %d, source: %d, tag: %d,  clock: %d, msg_count: %d %p): recv_test_id: %d",
// 		   event_pooled->get_event_counts(), event_pooled->get_is_testsome(), event_pooled->get_flag(),
// 		   event_pooled->get_source(), event_pooled->get_tag(), event_pooled->get_clock(), event_pooled->msg_count,
// 		   event_pooled, recv_test_id);
// #endif

// 	/*Move from pending request list to matched request list*/
// 	irecv_inputs->matched_request_proxy_list.push_back(proxy_request);
// #if 0
// 	REMPI_DBGI(0, "matched_proxy_request(%d) is added: irecv(%p)->%p size:%d", i, irecv_inputs, proxy_request,
// 		   irecv_inputs->matched_request_proxy_list.size());
// #endif
// 	irecv_inputs->request_proxy_list.pop_front();

// 	/* push_back  new proxy */
// 	next_proxy_request = new rempi_proxy_request(irecv_inputs->count, irecv_inputs->datatype);
// 	irecv_inputs->request_proxy_list.push_back(next_proxy_request);
// 	/* irecv(proxy) */
// 	PMPI_Irecv(next_proxy_request->buf, 
// 		   irecv_inputs->count, irecv_inputs->datatype, irecv_inputs->source, irecv_inputs->tag, irecv_inputs->comm, 
// 		   &next_proxy_request->request);
// 	//	  REMPI_DBG("Matched: request:%p source:%d tag:%d", &proxy_request->request, array_of_statuses[i].MPI_SOURCE, array_of_statuses[i].MPI_TAG);
//       } else {
// 	//	  REMPI_DBG("Unmatched: request:%p source:%d tag:%d", &proxy_request->request, array_of_statuses[i].MPI_SOURCE, array_of_statuses[i].MPI_TAG);
// 	unmatched_flag_count++;

//       }


//       int event_list_status;
//       int num_of_recv_msg_in_next_event = 0;
//       size_t interim_min_clock_in_next_event = 0;
//       if (mc_encoder->num_of_recv_msg_in_next_event != NULL && 
// 	  mc_encoder->interim_min_clock_in_next_event != NULL) {
// 	num_of_recv_msg_in_next_event = mc_encoder->num_of_recv_msg_in_next_event[recv_test_id];
// 	/* ===== NOTE ================================================================================================
// 	   If num_of_recv_msg_in_next_event > 0 (Condition A), this means CDC now trying to replay recv event next. 
// 	   So if replaying_events_list->dequeue_replay does not return enough replay events (Condition B), this means 
// 	   this main thread deuque "recv" events next
// 	   -- Main thread --
// 	   Get  : num_of_recv_msg_in_next_event & interim_min_clock_in_next_event
// 	   Test : Any in Replay_queue ?
// 	   if (Test is false) Update : fd_next_clock
// 	   -----------------
// 	   -- CDC thread --
// 	   Test : Is replaying matched events ?
// 	   if (Test is true) Update: num_of_recv_msg_in_next_event & interim_min_clock_in_next_event
// 	   if (Replaying events) : num_of_recv_msg_in_next_event = 0
// 	   Update: Replay_queue
// 	   -------------------
// 	   ===========================================================================================================*/
// 	if (mc_encoder->interim_min_clock_in_next_event == NULL) { /*TODO: remove this because this is already checked in the above*/
// 	  REMPI_ERR("interim: %p", mc_encoder->interim_min_clock_in_next_event);
// 	}
// 	interim_min_clock_in_next_event = mc_encoder->interim_min_clock_in_next_event[recv_test_id];
//       }

//       while ((replaying_event = replaying_event_list->dequeue_replay(recv_test_id, event_list_status)) != NULL) {

// #ifdef REMPI_DBG_REPLAY
// 	REMPI_DBGI(REMPI_DBG_REPLAY, "RPQ->A  : (count: %d, with_next: %d, flag: %d, source: %d, clock: %d)",
// 		   replaying_event->get_event_counts(), replaying_event->get_is_testsome(), replaying_event->get_flag(),
// 		   replaying_event->get_source(), replaying_event->get_clock());
// #endif
	
// 	replaying_event_vec.push_back(replaying_event);
// 	with_next = replaying_event->get_is_testsome();
// 	if (with_next ==  REMPI_MPI_EVENT_NOT_WITH_NEXT) {
// 	  /*Label:1*/
// 	  break;
// 	}
//       }

//       /*checking Condition B*/
//       if (with_next ==  REMPI_MPI_EVENT_NOT_WITH_NEXT) {
// 	/*If breaked i Label:1, I also want to breakthis for loop*/
// 	break;
//       } else {
// 	if (num_of_recv_msg_in_next_event > 0) { /*checking Condition A*/
// 	  mc_encoder->update_fd_next_clock(1, num_of_recv_msg_in_next_event, interim_min_clock_in_next_event, 
// 					   recording_event_list->get_enqueue_count(recv_test_id), recv_test_id);
// 	}
//       }

//       if (event_list_status == REMPI_EVENT_LIST_FINISH) {
// 	if (i == incount) {
// 	  REMPI_ERR("No thing to replay");
// 	}
//       }
//     }   /*End of for for(i = 0; i < incount; i++)*/

//     /* ======================================================== */
//     /* Frontier detection: Step 2                               */
//     /* ======================================================== */
//     /* Now, flag=0, so we can make sure there are no in-flight messages, and 
//        local_min_id is really minimal.
//     */
//     if (unmatched_flag_count == incount) {
//       //      REMPI_DBGI(REMPI_DBG_REPLAY, " ==== update start");
//       mc_encoder->update_local_min_id(min_recv_rank, min_next_clock);
//       //      REMPI_DBGI(REMPI_DBG_REPLAY, " ==== update end");
//     }
//     /* ======================================================== */
//   } /* while (with_next == REMPI_MPI_EVENT_WITH_NEXT) */

//   if (replaying_event_vec.size() == 1) { 
//     if (replaying_event_vec.front()->get_flag() == 0) { /*if unmatched event*/
//       rempi_event *e = replaying_event_vec.front();
//       *outcount = 0;
// #ifdef REMPI_DBG_REPLAY      
//       REMPI_DBGI(REMPI_DBG_REPLAY, "= Replay: (count: %d, with_next: %d, flag: %d, source: %d, tag: %d, clock: %d): recv_test_id: %d",
// 		 e ->get_event_counts(), e ->get_is_testsome(), e ->get_flag(),
// 		 e ->get_source(), e->get_tag(), e ->get_clock(), recv_test_id);
// #endif
//       delete replaying_event_vec.front();
//       return ret;
//     }
//   }


//   /*Copy from proxy to actuall buff, then set array_of_indices/statuses, outcount*/
//   *outcount = 0;
//   for (int i = 0; i < incount; i++) {
//     rempi_proxy_request *proxy_request;

//     if (request_to_irecv_inputs_umap.find(array_of_requests[i]) != request_to_irecv_inputs_umap.end()) {

//       irecv_inputs = request_to_irecv_inputs_umap[array_of_requests[i]];
//       for (int j = 0; j < replaying_event_vec.size(); j++) {
// 	bool is_any_source =  (irecv_inputs->source == MPI_ANY_SOURCE);
// 	bool is_source     =  (irecv_inputs->source == replaying_event_vec[j]->get_source());

// #if REMPI_DBG_REPLAY
// 	REMPI_DBGI(REMPI_DBG_REPLAY, "irecv_inputs.source: %d === replaying_events[j].source: %d (MPI_ANY_SOURCE: %d)", 
// 		   irecv_inputs->source, replaying_event_vec[j]->get_source(), MPI_ANY_SOURCE);
// #endif
// 	/*TODO: if array_of_requests has requests from the same rank, is_source does not work. And it introduces inconsistent replay
// 	  Use MPI_Request pointer (is_request) instead of is_source. In MCB, this is OK.*/
// 	if (is_source || is_any_source) {
// 	  array_of_indices[*outcount] = i;
// 	  array_of_statuses[*outcount].MPI_SOURCE = replaying_event_vec[j]->get_source();
// 	  array_of_statuses[*outcount].MPI_TAG    = replaying_event_vec[j]->get_tag();
// 	  {
// 	    clmpi_sync_clock(replaying_event_vec[j]->get_clock());
// 	  }
// 	  *outcount = *outcount + 1;
// 	  /* copy eventvec =-> actual_buff*/
// 	  proxy_request = irecv_inputs->matched_request_proxy_list.front();
// 	  if (proxy_request == NULL) {
// 	    REMPI_ERR("matched_proxy_request(%d) is NULL: irecv(%p)->%p size:%d", i, irecv_inputs, proxy_request,
// 		      irecv_inputs->matched_request_proxy_list.size());
// 	  }
// 	  copy_proxy_buf(proxy_request->buf, irecv_inputs->buf, replaying_event_vec[j]->msg_count, irecv_inputs->datatype);
// 	  //copy_proxy_buf(proxy_request->buf, irecv_inputs->buf, irecv_inputs->count, irecv_inputs->datatype);
// 	  irecv_inputs->matched_request_proxy_list.pop_front();
// #if REMPI_DBG_REPLAY
// 	  REMPI_DBGI(REMPI_DBG_REPLAY, "matched_proxy_request(%d) is pop: irecv(%p)->%p size:%d", i, irecv_inputs, proxy_request,
// 		     irecv_inputs->matched_request_proxy_list.size());
// #endif
// 	  delete proxy_request;
// 	  break;
// 	}
//       }
//       /*The last two 0s are not used, if the first vaiable is 0 
// 	Update next sending out clock for frontier detection*/
//       mc_encoder->update_fd_next_clock(0, 0, 0, 0, 0); 
//     } else {
//       REMPI_ERR("request does no exist in request_to_irecv_inputs_umap");
//     }
//   }


//   if (*outcount != replaying_event_vec.size()) {
//     REMPI_DBG(" ==== tete 6.0 : replayng matching count is %d, but found  %d messages", replaying_event_vec.size(), *outcount);
//     for (int j = 0; j < replaying_event_vec.size(); j++) {
//       REMPI_DBG(" = Wrong: (count: %d, with_next: %d, flag: %d, source: %d, clock: %d): recv_test_id: %d ",
// 		replaying_event_vec[j]->get_event_counts(), replaying_event_vec[j]->get_is_testsome(), replaying_event_vec[j]->get_flag(),
// 		replaying_event_vec[j]->get_source(), replaying_event_vec[j]->get_clock(), recv_test_id);
//     }
//     REMPI_ERR("All events are not replayed");
//   }

//     for (int j = 0; j < replaying_event_vec.size(); j++) {
// #ifdef REMPI_DBG_REPLAY
//       REMPI_DBGI(REMPI_DBG_REPLAY, "= Replay: (count: %d, with_next: %d, flag: %d, source: %d, tag: %d, clock: %d): recv_test_id: %d ",
// 		 replaying_event_vec[j]->get_event_counts(), replaying_event_vec[j]->get_is_testsome(), replaying_event_vec[j]->get_flag(),
// 		 replaying_event_vec[j]->get_source(), replaying_event_vec[j]->get_tag(), replaying_event_vec[j]->get_clock(), recv_test_id);
// #endif
//       // REMPI_DBGI(0, "= Replay: (count: %d, with_next: %d, flag: %d, source: %d, tag: %d, clock: %d): recv_test_id: %d ",
//       // 		 replaying_event_vec[j]->get_event_counts(), replaying_event_vec[j]->get_is_testsome(), replaying_event_vec[j]->get_flag(),
//       // 		 replaying_event_vec[j]->get_source(), replaying_event_vec[j]->get_tag(), replaying_event_vec[j]->get_clock(), recv_test_id);

//       delete replaying_event_vec[j];
//     }
  

//     return ret;
// }

int rempi_recorder_cdc::record_finalize(void)
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

  //  fprintf(stderr, "ReMPI: Function call (%s:%s:%d)\n", __FILE__, __func__, __LINE__);
  return 0;
}

int rempi_recorder_cdc::replay_finalize(void)
{
  read_record_thread->join();

  //TODO:
  //fprintf(stderr, "ReMPI: Function call (%s:%s:%d)\n", __FILE__, __func__, __LINE__);
  return 0;
}

