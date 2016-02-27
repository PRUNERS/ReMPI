#ifndef REMPI_LITE

#include <stdio.h>
#include <string.h>

#include <unordered_map>
#include <string>

#include "mpi.h"

//#include <iostream>
#include "rempi_recorder.h"

#include "rempi_message_manager.h"
#include "rempi_event.h"
#include "rempi_event_list.h"
#include "rempi_io_thread.h"
#include "rempi_send_mf.h"
#include "rempi_util.h"
#include "rempi_err.h"
#include "rempi_config.h"
#include "rempi_mem.h"
#include "clmpi.h"
#include "rempi_request_mg.h"

#define  PNMPI_MODULE_REMPI "rempi"


double stra, dura;
int counta = 0;
int counta_update = 0;
int counta_pending = 0;
double strb, durb;
int countb = 0;


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

bool compare3(int source1, size_t clock1,
	      int source2, size_t clock2)
{
  if (clock1 < clock2) {
    return true;
  } else if (clock1 == clock2) {
    return source1 < source2;
  }
  return false;
}
  

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

    /*PMPI_Type_size retur size (Payload + piggyback_clock(size_t)).
      So subtract by sizeof(size_t) */
    if (datatype_size * count - sizeof(size_t) <= 0) {
      REMPI_ERR("copy_proxy_buf failed: to: %p, from: %p, data_size: %d, count: %d, sum: %d", 
		to, from, datatype_size, count, datatype_size * count - sizeof(size_t));
    }
#ifdef REMPI_DBG_REPLAY
    REMPI_DBGI(REMPI_DBG_REPLAY, "copy_proxy_buf: to: %p, from: %p, data_size: %d, count: %d, sum: %d", 
		to, from, datatype_size, count, datatype_size * count - sizeof(size_t));
#endif
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
  // PNMPI_modHandle_t handle_rempi, handle_clmpi;
  // PNMPI_Service_descriptor_t serv;
  // /*Load clock-mpi*/
  // err=PNMPI_Service_GetModuleByName(PNMPI_MODULE_CLMPI, &handle_clmpi);
  // if (err!=PNMPI_SUCCESS) {
  //   return err;
  // }
  // /*Get clock-mpi service*/
  // err=PNMPI_Service_GetServiceByName(handle_clmpi,"clmpi_register_recv_clocks","pi",&serv);
  // if (err!=PNMPI_SUCCESS)
  //   return err;
  // clmpi_register_recv_clocks=(PNMPIMOD_register_recv_clocks_t) ((void*)serv.fct);
  clmpi_register_recv_clocks=PNMPIMOD_register_recv_clocks;

  // /*Get clock-mpi service*/
  // err=PNMPI_Service_GetServiceByName(handle_clmpi,"clmpi_clock_control","i",&serv);
  // if (err!=PNMPI_SUCCESS)
  //   return err;
  // clmpi_clock_control=(PNMPIMOD_clock_control_t) ((void*)serv.fct);
  clmpi_clock_control=PNMPIMOD_clock_control;

  // /*Get clock-mpi service*/
  // err=PNMPI_Service_GetServiceByName(handle_clmpi,"clmpi_get_local_clock","p",&serv);
  // if (err!=PNMPI_SUCCESS)
  //   return err;
  // clmpi_get_local_clock=(PNMPIMOD_get_local_clock_t) ((void*)serv.fct);
  clmpi_get_local_clock=PNMPIMOD_get_local_clock;

  // /*Get clock-mpi service*/
  // err=PNMPI_Service_GetServiceByName(handle_clmpi,"clmpi_get_local_sent_clock","p",&serv);
  // if (err!=PNMPI_SUCCESS)
  //   return err;
  // clmpi_get_local_sent_clock=(PNMPIMOD_get_local_sent_clock_t) ((void*)serv.fct);
  clmpi_get_local_sent_clock=PNMPIMOD_get_local_sent_clock;

  // /*Get clock-mpi service*/
  // err=PNMPI_Service_GetServiceByName(handle_clmpi,"clmpi_sync_clock","i",&serv);
  // if (err!=PNMPI_SUCCESS)
  //   return err;
  // clmpi_sync_clock=(PNMPIMOD_sync_clock_t) ((void*)serv.fct);
  clmpi_sync_clock=PNMPIMOD_sync_clock;

  // /*Get clock-mpi service*/                        
  // err=PNMPI_Service_GetServiceByName(handle_clmpi,"clmpi_get_incomplete_smsg_num","p",&serv);
  // if (err!=PNMPI_SUCCESS)
  //   return err;
  // clmpi_get_num_of_incomplete_sending_msg=(PNMPIMOD_get_num_of_incomplete_sending_msg_t) ((void*)serv.fct);
  clmpi_get_num_of_incomplete_sending_msg=PNMPIMOD_get_num_of_incomplete_sending_msg;

  // /*Load own moduel*/
  // err=PNMPI_Service_GetModuleByName(PNMPI_MODULE_REMPI, &handle_rempi);
  // if (err!=PNMPI_SUCCESS)
  //   return err;

  return MPI_SUCCESS;
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
   int comm_id, // The value is set by MPI_Comm_set_name in ReMPI_convertor
   MPI_Comm comm,
   MPI_Request *request)
{
  int ret;
  // ret = PMPI_Irecv(buf, count, datatype, source, tag, comm, request);                                                                             
  // rempi_reqmg_register_recv_request(request, source, tag, comm_id); 
  // int test_id = get_test_id();
  // if (request_to_test_id_umap.find(request) == request_to_test_id_umap.end()) {
  //   request_to_test_id_umap[request] = test_id;
  //   REMPI_DBGI(0, "Map: %p -> %d", request, test_id);
  // }
  return ret;
}

int rempi_recorder_cdc::replay_isend(MPI_Request *request)
{
  int ret;
  if ((MPI_Request)send_request_id != MPI_REQUEST_NULL) {
    send_request_id++;
  }  
  
  isend_request_umap[(MPI_Request)send_request_id] = *request;
#ifdef BGQ
  MPI_Request tmp = *request;
  //  REMPI_DBG("send_request:%p",*request);
  *request = (MPI_Request)send_request_id;
  if (isend_request_umap[*request] != tmp) {
    REMPI_ERR("not matched: key:%p to %p: %p and %p", (MPI_Request)send_request_id, *request, isend_request_umap[*request], tmp);
  }
#else
  *request = (MPI_Request)send_request_id;
#endif

  send_request_id++;

  return ret;
}

size_t request_id = 847589431;
int rempi_recorder_cdc::replay_irecv(
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

  if (request_to_irecv_inputs_umap.find(*request) != 
      request_to_irecv_inputs_umap.end()) {
    /* If this request is posted in irecv before*/
    irecv_inputs = request_to_irecv_inputs_umap[*request];
    bool same_source = (irecv_inputs->source == source);
    bool same_tag    = (irecv_inputs->tag    == tag);
    bool same_comm   = (irecv_inputs->comm   == comm);
    if (!same_source || !same_tag || !same_comm) {
      REMPI_ERR("Different request(req:%p) in (source, tag, comm): :(%d, %d, %p) != (%d, %d, %p)",
       		(*request), irecv_inputs->source, irecv_inputs->tag, irecv_inputs->comm,
		source, tag, comm);

      // REMPI_ERR("This MPI_Request is not diactivated. This mainly caused by irecv call "
      // 		"with MPI_Request which is not diactivated by MPI_{Wait|Test}{|some|any|all}");
    }
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

  irecv_inputs = request_to_irecv_inputs_umap[*request];

  if (irecv_inputs->request_proxy_list.size() == 0) {

    //    proxy_buf = allocate_proxy_buf(count, datatype);
    proxy_request_info = new rempi_proxy_request(count, datatype);
    //    proxy_request = (MPI_Request*)rempi_malloc(sizeof(MPI_Request));
    irecv_inputs->request_proxy_list.push_back(proxy_request_info);
    ret = PMPI_Irecv(proxy_request_info->buf, count, datatype, source, tag, comm, &proxy_request_info->request);
    //    REMPI_DBG("matched_proxy: request: %p", *request);
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

#ifdef REMPI_DBG_REPLAY
  REMPI_DBGI(REMPI_DBG_REPLAY, "MPI_Cancel: irecv_input->recv_test_id: %d", irecv_inputs->recv_test_id);
#endif
  irecv_inputs->recv_test_id = -1;

  return ret;
}

int rempi_recorder_cdc::replay_request_free(MPI_Request *request)
{
  REMPI_ERR("Not supported");
  return (int)0;
}

MPI_Fint rempi_recorder_cdc::replay_request_c2f(MPI_Request request)
{
  REMPI_ERR("Not supported");
  return (MPI_Fint)0;
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


int rempi_recorder_cdc::record_mf(int incount,
					MPI_Request array_of_requests[],
					int *outcount,
					int array_of_indices[],
					MPI_Status array_of_statuses[],
					int global_test_id,
					int matching_function_type)
{
  REMPI_ERR("not implemented yet");
  return 0;
}

int rempi_recorder_cdc::rempi_pf(int source,
				 int tag,
				 MPI_Comm comm,
				 int *flag,
				 MPI_Status *status,
				 int prove_function_type)
{
  REMPI_ERR("not implemented yet");
  return 0;
}

int rempi_recorder_cdc::replay_test(
                                MPI_Request *request,
                                int *flag,
                                MPI_Status *status,
				int test_id)
{
  int index;
  int ret = replay_testsome(1, request, flag, &index, status, test_id, REMPI_MF_FLAG_1_TEST, REMPI_MF_FLAG_2_SINGLE);
  return ret;
}


bool rempi_recorder_cdc::progress_send_requests()
{
  int flag = 0;
  MPI_Status status;
  MPI_Request send_request, send_request_id;
  size_t clock;
#ifdef BGQ
  for (unordered_map<MPI_Request, MPI_Request>::iterator it = isend_request_umap.begin(), it_end = isend_request_umap.end();
       it != it_end;
       it++) {
    send_request_id = it->first;
    send_request    = it->second;
#else
  for (unordered_map<MPI_Request, MPI_Request>::iterator it = isend_request_umap.begin(), it_end = isend_request_umap.end();
       it != it_end;
       it++) {
    send_request_id = it->first;
    send_request    = it->second;
  // for (auto &pair_send_req_and_send_req:isend_request_umap) {
  //   send_request_id = pair_send_req_and_send_req.first;
  //   send_request    = pair_send_req_and_send_req.second;
#endif  

    if (send_request != MPI_REQUEST_SEND_NULL && 
	send_request != MPI_REQUEST_NULL) {
      /*TODO: Actually, clock needs to be registered for PMPI_Test for send request*/
      clmpi_register_recv_clocks(&clock, 1); 
      //      REMPI_DBG("request: %p (size: %d)", send_request, isend_request_umap.size());
      flag = 0;
      PMPI_Test(&send_request, &flag, &status);
      //      REMPI_DBG("request: %p end (MPI_REQUEST_NULL: %p)", send_request, MPI_REQUEST_NULL);
      if (flag) {
	isend_request_umap[(MPI_Request)send_request_id] = MPI_REQUEST_SEND_NULL;
	//	REMPI_DBG("matched: request %p", isend_request_umap[(MPI_Request)send_request_id]);
      }
    }
  }  

  // for (auto &pair_send_req_and_send_req:isend_request_umap) {
  //   key          = pair_send_req_and_send_req.first;
  //   send_request = pair_send_req_and_send_req.second;
  //   REMPI_DBG("==>: request %p->%p", key, send_request);
  // }
  return true;
}


/*Test all recv requests, and if matched, put the events to the queue*/
bool rempi_recorder_cdc::progress_recv_requests(int recv_test_id,
						int incount,
						MPI_Request array_of_requests[], 
						int global_local_min_id_rank, 
						size_t global_local_min_id_clock)
{
  int flag;
  rempi_proxy_request *proxy_request;
  MPI_Status status;
  size_t clock;
  rempi_proxy_request *next_proxy_request;
  rempi_event *event_pooled;
  bool has_pending_msg = false;
  rempi_irecv_inputs *irecv_inputs;
  int matched_request_push_back_count = 0;

#ifdef BGQ
  for (unordered_map<MPI_Request, rempi_irecv_inputs*>::const_iterator cit = request_to_irecv_inputs_umap.cbegin(),
	 cit_end = request_to_irecv_inputs_umap.cend();
       cit != cit_end;
       cit++) {
    irecv_inputs = cit->second;
#else
  for (unordered_map<MPI_Request, rempi_irecv_inputs*>::const_iterator cit = request_to_irecv_inputs_umap.cbegin(),
	 cit_end = request_to_irecv_inputs_umap.cend();
       cit != cit_end;
       cit++) {
    irecv_inputs = cit->second;
  // for (auto &pair_req_and_irecv_inputs: request_to_irecv_inputs_umap) {
  //   irecv_inputs = pair_req_and_irecv_inputs.second;
#endif
    /* Find out this request belongs to which MF (recv_test_id) */
    if (irecv_inputs->recv_test_id == -1) {
      for (int i = 0; i < incount; i++) {
	if (irecv_inputs->request == array_of_requests[i] || !rempi_is_test_id) {
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

#ifdef BGQ
      for (list<rempi_proxy_request*>::iterator it = irecv_inputs->matched_pending_request_proxy_list.begin(),
	     it_end = irecv_inputs->matched_pending_request_proxy_list.end();
	   it != it_end;
	   it++) {
	rempi_proxy_request *proxy_request = *it;
#else
      for (list<rempi_proxy_request*>::iterator it = irecv_inputs->matched_pending_request_proxy_list.begin(),
      	     it_end = irecv_inputs->matched_pending_request_proxy_list.end();
      	   it != it_end;
      	   it++) {
      	rempi_proxy_request *proxy_request = *it;
	//	for (rempi_proxy_request *proxy_request: irecv_inputs->matched_pending_request_proxy_list) {
#endif      

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
	if (compare3(event_pooled->get_source(), event_pooled->get_clock(), global_local_min_id_rank, global_local_min_id_clock)) {
	  REMPI_ERR("Enqueueing an event whose <source: %d, clock: %lu> is smaller than global_local_min <source: %d, clock: %lu>",
		    event_pooled->get_source(), event_pooled->get_clock(), global_local_min_id_rank, global_local_min_id_clock);
	}
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
#ifdef REMPI_DBG_REPLAY
	REMPI_DBGI(REMPI_DBG_REPLAY, "Added to matched_proxy_request in %p: size:%d request: %p", irecv_inputs, irecv_inputs->matched_request_proxy_list.size(), pair_req_and_irecv_inputs.first);
#endif
	matched_request_push_back_count++;
      }
      for (int i = 0; i < matched_request_push_back_count; i++) {
	irecv_inputs->matched_pending_request_proxy_list.pop_front();
      }
    }
    /* ================================================= */
    
    if (irecv_inputs->matched_pending_request_proxy_list.size() > 0) {
      has_pending_msg = true;
      REMPI_DBG("has_pending: %d", has_pending_msg);
    }
    
    /* =================================================
       2. Check if this request have matched.
       ================================================= */
    /* irecv_inputs->request_proxy_list.size() == 0 can happens 
       when this request was canceled (MPI_Cancel)*/
    if (irecv_inputs->request_proxy_list.size() == 0) continue; 
    proxy_request = irecv_inputs->request_proxy_list.front();
    clmpi_register_recv_clocks(&clock, 1);
    clmpi_clock_control(0);
    flag = 0;
    REMPI_ASSERT(proxy_request != NULL);
    REMPI_ASSERT(proxy_request->request != NULL);
// #ifdef REMPI_DBG_REPLAY	  
//       REMPI_DBGI(aREMPI_DBG_REPLAY, "MPI_Test: (source: %d, tag: %d): recv_test_id: %d: size: %d",
// 		 irecv_inputs->source, irecv_inputs->tag, recv_test_id, request_to_irecv_inputs_umap.size());
// #endif
    PMPI_Test(&proxy_request->request, &flag, &status);
    clmpi_clock_control(1);
    
    if(!flag) continue;
    /*If flag == 1*/
    has_pending_msg = true;


#ifdef REMPI_DBG_REPLAY	  
    REMPI_DBGI(REMPI_DBG_REPLAY, "A->     : (source: %d, tag: %d,  clock: %d): current recv_test_id: %d, size: %d",
	       status.MPI_SOURCE, status.MPI_TAG, clock, irecv_inputs->recv_test_id, 
	       request_to_irecv_inputs_umap.size());
#endif
    if (compare3(status.MPI_SOURCE, clock, global_local_min_id_rank, global_local_min_id_clock)) {
      REMPI_ERR("Received an event whose <source: %d, tag: %d, clock: %lu> is smaller than global_local_min <source: %d, clock: %lu>",
		status.MPI_SOURCE, status.MPI_TAG, clock, global_local_min_id_rank, global_local_min_id_clock);
    }

    if (irecv_inputs->recv_test_id != -1) {
      //      REMPI_ASSERT(irecv_inputs->recv_test_id == recv_test_id);
      event_pooled =  new rempi_test_event(1, -1, -1, 1, status.MPI_SOURCE, status.MPI_TAG, clock, irecv_inputs->recv_test_id);

      PMPI_Get_count(&status, irecv_inputs->datatype, &(event_pooled->msg_count));
      if (compare3(status.MPI_SOURCE, clock, global_local_min_id_rank, global_local_min_id_clock)) {
	REMPI_ERR("Enqueueing an event whose <source: %d, clock: %lu> is smaller than global_local_min <source: %d, clock: %lu>",
		  status.MPI_SOURCE, clock, global_local_min_id_rank, global_local_min_id_clock);
      }
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
#ifdef REMPI_DBG_REPLAY
      REMPI_DBGI(REMPI_DBG_REPLAY, "Added to matched_proxy_request in %p: size:%d request: %p", irecv_inputs, irecv_inputs->matched_request_proxy_list.size(),
		 pair_req_and_irecv_inputs.first);
#endif
      irecv_inputs->request_proxy_list.pop_front();
    } else {
      REMPI_ASSERT(irecv_inputs->recv_test_id != recv_test_id);
      proxy_request->matched_source = status.MPI_SOURCE;
      proxy_request->matched_tag    = status.MPI_TAG;
      proxy_request->matched_clock    = clock;
      PMPI_Get_count(&status, irecv_inputs->datatype, &(proxy_request->matched_count));
      if (proxy_request->matched_count == MPI_UNDEFINED) {
	REMPI_ERR("PMPI_Get_count returned MPI_UNDEFINED");
      }
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
  
#ifdef REMPI_DBG_REPLAY
  // if (!has_pending_msg) {
  //   REMPI_DBG("======= No msg arrive =========");
  //   for (auto &pair_req_and_irecv_inputs: request_to_irecv_inputs_umap) {
  //     irecv_inputs = pair_req_and_irecv_inputs.second;
  //     REMPI_DBG("== source: %d, tag: %d: length: %d", irecv_inputs->source, irecv_inputs->tag, irecv_inputs->request_proxy_list.size());
  //   }
  // }
#endif
  return has_pending_msg;
}

int rempi_recorder_cdc::replay_testsome(
					int incount,
					MPI_Request array_of_requests[],
					int *outcount,
					int array_of_indices[],
					MPI_Status array_of_statuses[],
					int global_test_id,
					int mf_flag_1,
					int mf_flag_2)
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
    // 
    /* If this test call is all for Send requests */
    //    clocks = (size_t*)rempi_malloc(sizeof(size_t) * incount);
    //    clmpi_register_recv_clocks(clocks, incount);
#ifdef REMPI_DBG_REPLAY	  
    for (int j = 0; j < incount; j++) {
      REMPI_DBGI(REMPI_DBG_REPLAY, "Isend:  index %d: request: %p", j, array_of_requests[j]);
    }
#endif


#if 1
    if (mf_flag_1 == REMPI_MF_FLAG_1_TEST) {
      if (mf_flag_2 == REMPI_MF_FLAG_2_SOME) {
	ret = REMPI_Send_Testsome(incount, array_of_requests, outcount, array_of_indices, array_of_statuses);
	//ret = PMPI_Testsome(incount, array_of_requests, outcount, array_of_indices, array_of_statuses);
      } else if (mf_flag_2 == REMPI_MF_FLAG_2_SINGLE) {
	REMPI_ASSERT(incount  == 1);
	ret = REMPI_Send_Testsome(incount, array_of_requests, outcount, array_of_indices, array_of_statuses);	
	//ret = PMPI_Testsome(incount, array_of_requests, outcount, array_of_indices, array_of_statuses);
      } else {
	REMPI_ERR("current ReMPI does not support this MF: %d %d", mf_flag_1, mf_flag_2);
      }
    } else if (mf_flag_1 == REMPI_MF_FLAG_1_WAIT) {
      if (mf_flag_2 == REMPI_MF_FLAG_2_ALL) {
	ret = REMPI_Send_Waitall(incount, array_of_requests, array_of_statuses);
	//ret = PMPI_Waitall(incount, array_of_requests, array_of_statuses);
	*outcount = incount;
      } else {
	REMPI_ERR("current ReMPI does not support this MF: %d %d", mf_flag_1, mf_flag_2);
      }
    } else {
      REMPI_ERR("unexpected error")
    }
#endif

    mc_encoder->update_fd_next_clock(0, 0, 0, 0, 0, 0); /*Update next sending out clock for frontier detection*/

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
  REMPI_DBGI(REMPI_DBG_REPLAY, "Testsome recv call: global_test_id: %d => recv_test_id: %d (flag1: %d flag2: %d)", 
	     global_test_id, recv_test_id, mf_flag_1, mf_flag_2);
#endif
  int interval = 0;
  while (with_next == REMPI_MPI_EVENT_WITH_NEXT) {
    int unmatched_flag_count = 0;
    int min_recv_rank;
    size_t min_next_clock;
    bool has_pending_msg = false;

    /*The last two 0s are not used, if the first vaiable is 0 
      Update next sending out clock for frontier detection*/
    mc_encoder->update_fd_next_clock(0, 0, 0, 0, 0, 0); /*Update next sending out clock for frontier detection*/
    /* ======================================================== */
    /* Frontier detection: Step 1                               */
    /* ======================================================== */
    /* First, fetch next_clock from senders, and compute local_min_id. 
       This function needs to be called before PMPI_Test. 
       If flag=0, we can make sure there are no in-flight messages, and 
       local_min_id is really minimal. */

    if (interval++ % 2 == 0) {
      stra = MPI_Wtime();
      mc_encoder->fetch_local_min_id(&min_recv_rank, &min_next_clock);
      counta++;
      dura += MPI_Wtime() - stra;
    }

    unmatched_flag_count = 0;
    /* ======================================================== */

    strb = MPI_Wtime();
    /*progress for send: 
       if a send operation is completed, we can increment clock */
    progress_send_requests();
    /*progress for recv*/
    has_pending_msg = progress_recv_requests(recv_test_id, incount, array_of_requests, 
					     mc_encoder->global_local_min_id.rank, 
					     mc_encoder->global_local_min_id.clock);

    
// #ifdef REMPI_DBG_REPLAY
//     REMPI_DBGI(REMPI_DBG_REPLAY, "progrell: global: %d recv: %d", global_test_id, recv_test_id);
// #endif


    bool iprobe_flag = false;
    int iprobe_flag_int = 0;
    /* TODO: multiple communicators */
    PMPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &iprobe_flag_int, MPI_STATUS_IGNORE);
    if (iprobe_flag_int) iprobe_flag = true;
    //    REMPI_DBG("has_pending_msg: %d, iprobe_flag: %d", has_pending_msg, iprobe_flag_int);
    has_pending_msg = has_pending_msg || iprobe_flag;
#if 0
    if (iprobe_flag_int) {
       REMPI_ERR("Current ReMPI assume that Iresv is posted before the message arrives."
		 "You can remove this routine, but if it deadlocks, current ReMPI does not work for your code");
    }
#else
    if (iprobe_flag_int) {
      REMPI_DBG("Current ReMPI assume that Iresv is posted before the message arrives.");
    }
#endif


    /* ======================================================== */
    /* Frontier detection: Step 2                               */
    /* ======================================================== */
    if (!has_pending_msg) {
      int is_updated = 0;
      //      REMPI_DBGI(REMPI_DBG_REPLAY, " ==== update start");
      is_updated = mc_encoder->update_local_min_id(min_recv_rank, min_next_clock);
      if (is_updated) {
	counta_update++;
      }
      
      //      REMPI_DBGI(REMPI_DBG_REPLAY, " ==== update end");
    } else {
      counta_pending++;
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
      REMPI_DBGI(REMPI_DBG_REPLAY, "RPQ->A  : (count: %d, with_next: %d, flag: %d, source: %d, clock: %d) recv_test_id",
		 replaying_event->get_event_counts(), replaying_event->get_is_testsome(), replaying_event->get_flag(),
		 replaying_event->get_source(), replaying_event->get_clock(), recv_test_id);
#endif
	
      replaying_event_vec.push_back(replaying_event);
      with_next = replaying_event->get_is_testsome();
      if (with_next ==  REMPI_MPI_EVENT_NOT_WITH_NEXT) {
	break;
      }
    }
    
    /*checking Condition B*/
    if (with_next !=  REMPI_MPI_EVENT_NOT_WITH_NEXT) {
      size_t tmp_sent_clock, tmp_clock;
      size_t num_of_incomplete_msg = 0;
      //      clmpi_get_local_clock(&tmp_clock);
      //      clmpi_get_local_sent_clock(&tmp_sent_clock);
      bool is_next_event_recv = num_of_recv_msg_in_next_event > 0;
      clmpi_get_num_of_incomplete_sending_msg(&num_of_incomplete_msg);
      if (is_next_event_recv && num_of_incomplete_msg == 0) { /*checking Condition A*/
	mc_encoder->update_fd_next_clock(1, num_of_recv_msg_in_next_event, interim_min_clock_in_next_event, 
					 recording_event_list->get_enqueue_count(recv_test_id), recv_test_id, 0);
      }
    }

    countb += 1;
    durb += MPI_Wtime() - strb;
    // if (MPI_Wtime() - strb > 0.00005) {
    //   REMPI_DBG("Big: %f", MPI_Wtime() - strb);
    // }


  } /* while (with_next == REMPI_MPI_EVENT_WITH_NEXT) */
  
  /*======================================================*/
  /*Now, next replaying events are in replaying_event_vec */
  /*======================================================*/

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

	  clmpi_sync_clock(replaying_event_vec[j]->get_clock());

	  *outcount = *outcount + 1;

	  if (irecv_inputs->matched_request_proxy_list.size() == 0) {
	    //	    REMPI_DBG("irecv_inputs->matched_proxy_list is empty: input_inputs:%p request:%p: Matching condition of this MPI_Request may be overlapping with matching condition of another MPI_Request", irecv_inputs, array_of_requests[i]);
	    /*
	      Matching condition of this MPI_Request may be overlapping with matching condition of another MPI_Request.
	      To check another MPI_Request, continue; is called
	    */
	    continue;
	  }
	  proxy_request = irecv_inputs->matched_request_proxy_list.front();
	  if (proxy_request == NULL) {
	    REMPI_ERR("matched_proxy_request(%d) is NULL: irecv(%p)->%p size:%d", i, irecv_inputs, proxy_request,
		      irecv_inputs->matched_request_proxy_list.size());
	  }
	  
#if REMPI_DBG_REPLAY
	  REMPI_DBGI(REMPI_DBG_REPLAY, "%p %p %p", irecv_inputs, replaying_event_vec[j], proxy_request);
#endif
	  /* copy eventvec =-> actual_buff*/
	  copy_proxy_buf(proxy_request->buf, irecv_inputs->buf, replaying_event_vec[j]->msg_count, irecv_inputs->datatype);
	  //copy_proxy_buf(proxy_request->buf, irecv_inputs->buf, irecv_inputs->count, irecv_inputs->datatype);
	  irecv_inputs->matched_request_proxy_list.pop_front();
#if REMPI_DBG_REPLAY
	  REMPI_DBGI(REMPI_DBG_REPLAY, "matched_proxy_request(%d) is pop: irecv(%p)->%p size:%d request: %p", i, irecv_inputs, proxy_request,
		     irecv_inputs->matched_request_proxy_list.size(), array_of_requests[i]);
#endif
	  delete proxy_request;
	  break;
	}
      }
      /*The last two 0s are not used, if the first vaiable is 0 
	Update next sending out clock for frontier detection*/
      mc_encoder->update_fd_next_clock(0, 0, 0, 0, 0, 1); 
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


      delete replaying_event_vec[j];
    }
  

    return ret;
}


int rempi_recorder_cdc::replay_iprobe(int source, int tag, MPI_Comm comm, int *flag, MPI_Status *status, int comm_id)
{
  REMPI_ERR("Probe/Iprobe is not supported yet");
  return MPI_SUCCESS;
}




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

  return 0;
}



int rempi_recorder_cdc::replay_finalize(void)
{
  read_record_thread->join();

  //TODO:
  //fprintf(stderr, "ReMPI: Function call (%s:%s:%d)\n", __FILE__, __func__, __LINE__);
#if 0
  if (counta == 0) {
    REMPI_DBG("Get: %f (count: %lu, is_pending:%lu, is_updated:%lu, single: %f), waiting: %f (count: %lu, single: %f)", 
	      dura, counta, counta_pending, counta_update, -1, durb, countb, dura/countb);
  } else {
    REMPI_DBG("Get: %f (count: %lu, is_pending:%lu, is_updated:%lu, single: %f), waiting: %f (count: %lu, single: %f)", 
	      dura, counta, counta_pending, counta_update, dura/counta, durb, countb, dura/countb);
  }
#endif
  return 0;
}

void rempi_recorder_cdc::set_fd_clock_state(int flag)
{
  mc_encoder->set_fd_clock_state(flag);
  return;
}


void rempi_recorder_cdc::fetch_and_update_local_min_id()
{
  int min_recv_rank;
  size_t  min_next_clock;
  int is_updated;

  stra = MPI_Wtime();
  mc_encoder->fetch_local_min_id(&min_recv_rank, &min_next_clock);
  counta++;
  dura += MPI_Wtime() - stra;
  is_updated = mc_encoder->update_local_min_id(min_recv_rank, min_next_clock);  
  if (is_updated) {
    counta_update++;
  }
  return;
}


int rempi_recorder_cdc::REMPI_Send_Wait(MPI_Request *send_request_id, MPI_Status *status)
{
  int ret = 0;
  size_t clock;
  MPI_Request send_request;
  send_request = isend_request_umap[*send_request_id];
  /*TODO: set status value*/
  if (send_request == MPI_REQUEST_SEND_NULL) {
    *send_request_id = MPI_REQUEST_NULL;
    isend_request_umap.erase(*send_request_id);
    return MPI_SUCCESS;
  }
  clmpi_register_recv_clocks(&clock, 1);
  ret = PMPI_Wait(&send_request, status);

  /*remove from umap*/
  if (isend_request_umap.find(*send_request_id) == isend_request_umap.end()) {
    REMPI_DBG("no such request: %lu", *send_request_id);
  }
  isend_request_umap.erase(*send_request_id);

  return ret;
}

int rempi_recorder_cdc::REMPI_Send_Waitany(int arg_0, MPI_Request *arg_1, int *arg_2, MPI_Status *arg_3)
{
  REMPI_ERR("REMPI_Send_Waitanay is not implemented yet");
  int _wrap_py_return_val = 0;
  size_t clock;
  {
    clmpi_register_recv_clocks(&clock, 1);
    _wrap_py_return_val = PMPI_Waitany(arg_0, arg_1, arg_2, arg_3);
  }    return _wrap_py_return_val;
}

int rempi_recorder_cdc::REMPI_Send_Waitall(int count, MPI_Request *array_of_send_request_ids, MPI_Status *array_of_statuses)
{
  int ret = 0;
  int outcount = 0;
  int is_all_matched  = 0;
  while(!is_all_matched) {
    ret = REMPI_Send_Waitsome(count, array_of_send_request_ids, &outcount, MPI_INDICES_IGNORE, array_of_statuses);
    if (ret != MPI_SUCCESS) return ret;
    is_all_matched = 1;
    for (int i = 0; i < count; i++) {
      if (array_of_send_request_ids[i] != MPI_REQUEST_NULL) {
	is_all_matched = 0;
	break;
      }
    }
  }
  return ret;
}

int rempi_recorder_cdc::REMPI_Send_Waitsome(int incount, MPI_Request *array_of_send_request_ids, int *outcount, int *array_of_indices, MPI_Status *array_of_statuses)
{
  int ret = 0;
  int local_outcount = 0;
  int flag;
  size_t clock;
  MPI_Request send_request;
  while(!local_outcount) {
    for (int i = 0; i < incount; i++) {
      send_request = isend_request_umap[array_of_send_request_ids[i]];
      if (send_request == MPI_REQUEST_SEND_NULL) {
	if (array_of_indices != MPI_INDICES_IGNORE) array_of_indices[local_outcount] = i;
	local_outcount++;
	array_of_send_request_ids[i] = MPI_REQUEST_NULL;
	isend_request_umap.erase(array_of_send_request_ids[i]);
	//	REMPI_DBG("checked request(SEND_NULL): %p", array_of_send_request_ids[i]);
      } else if (array_of_send_request_ids[i] !=MPI_REQUEST_NULL) {
	clmpi_register_recv_clocks(&clock, 1);
	//	REMPI_DBG("check request: %p state: %p", array_of_send_request_ids[i], send_request);
	PMPI_Test(&send_request, &flag, &array_of_statuses[i]);
	if (flag) {
	  if (array_of_indices != MPI_INDICES_IGNORE) array_of_indices[local_outcount] = i;
	  local_outcount++;
	  array_of_send_request_ids[i] = MPI_REQUEST_NULL;
	  /*remove from umap*/
	  if (isend_request_umap.find(array_of_send_request_ids[i]) == isend_request_umap.end()) {
	    REMPI_DBG("no such request: %p", array_of_send_request_ids[i]);
	  }
	  isend_request_umap.erase(array_of_send_request_ids[i]);
	}
      }
    }
  }
  *outcount = local_outcount;
  return ret;
}

int rempi_recorder_cdc::REMPI_Send_Test(MPI_Request *send_request_id, int *flag, MPI_Status *status)
{
  return REMPI_Send_Testsome(1, send_request_id, flag, MPI_INDICES_IGNORE, status);
}  

int rempi_recorder_cdc::REMPI_Send_Testany(int count, MPI_Request *array_of_send_request_ids, int *index, int *flag, MPI_Status *status)
{
  int ret = 0;
  *flag = 0;
  for (int i = 0; i < count; i++) {
    ret = REMPI_Send_Test(&array_of_send_request_ids[i], flag, status);
    if (ret != MPI_SUCCESS) return ret;
    if (*flag == 1) {
      *index = i;
      break;
    }
  }
  return ret;
}

int rempi_recorder_cdc::REMPI_Send_Testall(int count, MPI_Request *array_of_send_request_ids, int *flag, MPI_Status *array_of_statuses)
{
  int ret = 0;
  int outcount = 0;
  int is_all_matched  = 0;
  ret = REMPI_Send_Testsome(count, array_of_send_request_ids, &outcount, MPI_INDICES_IGNORE, array_of_statuses);
  if (ret != MPI_SUCCESS) return ret;
  *flag = 1;
  for (int i = 0; i < count; i++) {
    if (array_of_send_request_ids[i] != MPI_REQUEST_NULL) {
      *flag = 0;
      break;
    }
  }
  return ret;
}


/*
Note:
ReMPI internally periodically can MPI_Test(send_request). 
If this MPI_Test call matches, the send_request becomes MPI_REQUEST_NULL.
But an application have no idead this MPI_Test(send_request) was internally matched or not.
So if an application calls MPI_Test(send_request), this means MPI_Test(MPI_REQUEST_NULL).
It results in an MPI error.

Thus, ReMPI call matched MPI_Test, ReMPI set the request to MPI_REQUEST_SEND_NULL.
The application call MPI_Test, ReMPI set MPI_REQUEST_SEND_NULL to MPI_REQUEST_NULL.

MPI_REQUEST_SEND_NULL: Internally matched, but an application does not know.
MPI_REQUEST_NULL: an application call the MPI_Test, so an application know it.

*/
int rempi_recorder_cdc::REMPI_Send_Testsome(int incount, MPI_Request *array_of_send_request_ids, int *outcount, int *array_of_indices, MPI_Status *array_of_statuses)
{
  int ret = 0;
  int tail_index = 0;
  int flag = 0;
  size_t clock;
  MPI_Request send_request;
  for (int i = 0; i < incount; i++){
    flag = 0;
    send_request = isend_request_umap[array_of_send_request_ids[i]];
    if (send_request == MPI_REQUEST_SEND_NULL) {
      flag = 1;
      array_of_send_request_ids[i] = MPI_REQUEST_NULL;
      isend_request_umap.erase(array_of_send_request_ids[i]);
      /*TODO: set status value*/
    } else {
      clmpi_register_recv_clocks(&clock, 1);
      ret = PMPI_Test(&send_request, &flag, &array_of_statuses[i]);
      if (ret != MPI_SUCCESS) return ret;
      if (flag) {
	/*remove from umap*/
	if (isend_request_umap.find(array_of_send_request_ids[i]) == isend_request_umap.end()) {
	  REMPI_DBG("no such request: %p", array_of_send_request_ids[i]);
	}
	isend_request_umap.erase(array_of_send_request_ids[i]);
#ifdef REMPI_DBG_REPLAY
	size_t sent_clock;
	clmpi_get_local_sent_clock(&sent_clock);
	REMPI_DBGI(REMPI_DBG_REPLAY, " Send completed: sent_clock: %lu", sent_clock);
#endif
	//	REMPI_DBG("erase registered request: %p", array_of_send_request_ids[i]);
      }
    }
    if (flag) {
      if (array_of_indices != MPI_INDICES_IGNORE) array_of_indices[tail_index] = i;
      tail_index++;
    }
  }

  *outcount = tail_index;
  return ret;
}

#endif /* MPI_LITE */
