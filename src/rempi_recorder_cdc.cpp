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
  return malloc(datatype_size * count);
}

/*get_test_id must be called under MPI_Irecv*/
// int rempi_recorder_cdc::get_test_id()
// {
//   string test_id_string;
//   test_id_string = rempi_btrace_string();
//   if (stacktrace_to_test_id_umap.find(test_id_string) ==  stacktrace_to_test_id_umap.end()) {
//     stacktrace_to_test_id_umap[test_id_string] = next_test_id_to_assign;
//     next_test_id_to_assign++;
//   }
//   return stacktrace_to_test_id_umap[test_id_string];
// }

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
  record_thread = new rempi_io_thread(recording_event_list, replaying_event_list, id, rempi_mode); //0: recording mode
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
  read_record_thread = new rempi_io_thread(recording_event_list, replaying_event_list, id, rempi_mode); //1: replay mode
  //  REMPI_DBG("io thread is not runnig");
  read_record_thread->start();

  return 0;
}

int rempi_recorder_cdc::record_irecv(
   void *buf,
   int count,
   int datatype,
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
  rempi_irecv_inputs *irecv_inputs;

  if (request_to_irecv_inputs_umap.find(*request) != 
      request_to_irecv_inputs_umap.end()) {
    /* If this request is posted in irecv before*/
    irecv_inputs = request_to_irecv_inputs_umap[*request];
    bool same_source = (irecv_inputs->source == source);
    bool same_tag    = (irecv_inputs->tag    == tag);
    bool same_comm   = (irecv_inputs->comm   == *comm);
    if (!same_source || !same_tag || !same_comm) {
      REMPI_ERR("Different request in (source, tag, comm)");
      // REMPI_ERR("This MPI_Request is not diactivated. This mainly caused by irecv call "
      // 		"with MPI_Request which is not diactivated by MPI_{Wait|Test}{|some|any|all}");
    }
  } else {
    request_to_irecv_inputs_umap[*request] = new rempi_irecv_inputs(buf, count, datatype, source, tag, *comm, *request, -1);
  }
  
  irecv_inputs = request_to_irecv_inputs_umap[*request];

  if (irecv_inputs->request_proxy_list.size() == 0) {
    rempi_proxy_request *proxy_request;
    //    proxy_buf = allocate_proxy_buf(count, datatype);
    proxy_request = new rempi_proxy_request(count, datatype);
    //    proxy_request = (MPI_Request*)malloc(sizeof(MPI_Request));
    irecv_inputs->request_proxy_list.push_back(proxy_request);
    ret = PMPI_Irecv(proxy_request->buf, count, datatype, source, tag, *comm, &proxy_request->request);
    //    REMPI_DBG("  irecv: request:%p", &proxy_request->request);
  } else {
    ret = MPI_SUCCESS;
  }
  
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
  recording_event_list->push(new rempi_test_event(event_count, with_previous, record_comm_id, *flag, record_source, record_tag, record_clock, test_id));

  return 0;
}


/*This function is called after MPI_Test*/
int rempi_recorder_cdc::replay_test(
    MPI_Request *request_in,
    /*Not used*/    int flag_in, 
    /*Not used*/    int source_in,
    /*Not used*/    int tag_in,
    /*Not used*/    int clock_in,
    /*Not used*/    int with_previous_in,
    int test_id_in,
    int *flag_out,
    int *source_out,
    int *tag_out)
{
  MPI_Status status;
  rempi_event *replaying_test_event = NULL;
  int event_list_status;
  int flag;
  int ret;
  unordered_map<MPI_Request, rempi_irecv_inputs*>::iterator request_to_irecv_inputs_umap_it;
  
  return PMPI_Test(request_in, flag_out, &status);

#if 0  
  if (request_to_irecv_inputs_umap.find(*request_in) == request_to_irecv_inputs_umap.end()) {
    /*This request is for Isend, so simply call PMPI_Test and return*/
    ret = PMPI_Test(reqeust_in, flag_out, status);
    sourct_out = status.MPI_SOURCE;
    tag_out    = status.MPI_TAG;
    return ret;
  }
  
  /*1. Get replaying event until REMPI_EVENT_LIST_FINISH */
  while ((replaying_test_event = replaying_event_list->dequeue_replay(test_id_in, event_list_status)) == NULL) {
    if (event_list_status == REMPI_EVENT_LIST_FINISH) {
      REMPI_ERR("No more replay event. Will switch to recording mode, but not implemented yet: event: %p", replaying_test_event);
      return 0;
    } else if (event_list_status == REMPI_EVENT_LIST_EMPTY) {
      
    } else {
      REMPI_ERR("No known event_list_status");
    } 
    for (int i = 0; i < proxy_request_pool_list.size(); i++) {
      MPI_Request *proxy_reqeust;
      MPI_Status  proxy_status;
      int flag;
      proxy_request_pool_list[i].request;
      PMPI_Test(&proxy_request, &flag, &status);
    }
    request_to_irecv_inputs_umap[*request_in];
    // for (request_to_irecv_inputs_umap_it  = request_to_irecv_inputs_umap.begin();
    // 	 request_to_irecv_inputs_umap_it != request_to_irecv_inputs_umap.end();
    // 	 request_to_irecv_inputs_umap_it++) {
    //   MPI_Request *request;
    //   rempi_irecv_inputs *inputs = request_to_irecv_inputs_umap_it->second;
    // }

  }

  REMPI_DBG("Replaying: (flag: %d, source: %d)", replaying_test_event->get_flag(), replaying_test_event->get_source());

  /*If the event is flag == 0, simply retunr flag == 0*/
  if (!replaying_test_event->get_flag()) {
    *flag_out = 0;
    *source_out = -1;
    *tag_out = -1;
    return 0;
  }
  *flag_out = 1;
  *source_out = replaying_test_event->get_source();
  *tag_out = replaying_test_event->get_tag();
  /*
    2. Wait until this replaying message really arrives
  */
  {
    rempi_irecv_inputs *inputs = request_to_irecv_inputs_umap[*request_in];
    //    REMPI_DBG("Probing");
    PMPI_Probe(*source_out, *tag_out, inputs->comm, &status);
    if (*source_out != status.MPI_SOURCE ||
	*tag_out    != status.MPI_TAG) {
      REMPI_ERR("An unexpected message is proved: tag_out checking may not be needed, "
		"because some compression approach do not record tag, and do not replay tag");
    }
    PMPI_Irecv(
	       inputs->buf, 
	       inputs->count,
	       inputs->datatype,
	       *source_out,
	       *tag_out,
	       inputs->comm,
	       &(inputs->request));
    PMPI_Wait(&(inputs->request), &status);
    if (*source_out != status.MPI_SOURCE ||
	*tag_out    != status.MPI_TAG) {
      REMPI_ERR("An unexpected message is waited: tag_out checking may not be needed, "
		"because some compression approach do not record tag, and do not replay tag");
    }
    delete request_to_irecv_inputs_umap[*request_in];
    request_to_irecv_inputs_umap.erase(*request_in);
  }

  return 0;
#endif
  /*====================*/

  /* So "replayint_test_event" event flag == 1*/
  /*
    2. Wait until this recorded message really arrives
       if (0 if next recorded maching is not mached in this run) {
          TODO: Wait until maching, and get clock
	  TODO: if matched, memorize that the matching for the next replay test
       }
  */

  // while (!msg_manager.is_matched_recv(
  // 	      replaying_test_event->get_source(), 
  // 	      replaying_test_event->get_tag(),
  // 	      replaying_test_event->get_comm_id(),
  // 	      replaying_test_event->get_test_id())) { //replaying_test_event->get_test_id() == test_id_in
  //   msg_manager.refresh_matched_recv(test_id_in);
  // }

  // msg_manager.remove_matched_recv(
  // 	      replaying_test_event->get_source(), 
  // 	      replaying_test_event->get_tag(),
  // 	      replaying_test_event->get_comm_id(), 
  // 	      replaying_test_event->get_test_id());
  // /*
  //   3. Set valiabiles (source, flag, tag)
  // }
  //  */
  // *flag_out = 1;
  // *source_out = replaying_test_event->get_source();
  // *tag_out = replaying_test_event->get_tag();

  //  return 0;
  
  /*==========================*/
  /**/
  // if (flag_in) {
  //   int event_count = 1;
  //   int with_previous = 0; //
  //   int comm_id = 0;
  //   msg_manager.add_matched_recv(request_in, source_in, tag_in);
  //   recording_event_list->push(new rempi_test_event(event_count, with_previous, comm_id, flag_in, source_in, tag_in, clock_in, test_id_in));
  // }



  // REMPI_DBG("flag: %d , source: %d", replaying_test_event->get_flag(), replaying_test_event->get_source());




}


int rempi_recorder_cdc::replay_testsome(
					int incount,
					MPI_Request array_of_requests[],
					int *outcount,
					int array_of_indices[],
					MPI_Status array_of_statuses[],
					int test_id)
{
  int ret;
  int with_next = 1;
  rempi_irecv_inputs *irecv_inputs;
  size_t clock;

  *outcount = 0;
  while (with_next) {
    for (int i = 0; i < incount; i++) {
      if (request_to_irecv_inputs_umap.find(array_of_requests[i]) != request_to_irecv_inputs_umap.end()) {
	int flag = 0;
	rempi_proxy_request *proxy_request;
	MPI_Status status;
	/*If this request is for irecv (not isend)*/
	irecv_inputs = request_to_irecv_inputs_umap[array_of_requests[i]];
	proxy_request = irecv_inputs->request_proxy_list.front();
	//	REMPI_DBG("   Test: request:%p(%p) source:%d tag:%d size:%d req:%p", &proxy_request->request, proxy_request, array_of_statuses[i].MPI_SOURCE, array_of_statuses[i].MPI_TAG, irecv_inputs->request_proxy_list.size(), array_of_requests[i]);
	clmpi_register_recv_clocks(&clock, 1);
	PMPI_Test(&proxy_request->request, &flag, &status);
	if (flag) {
	  rempi_proxy_request *next_proxy_request;
	  /* enqueue */
	  recording_event_list->push(new rempi_test_event(1, -1, -1, 1, status.MPI_SOURCE, status.MPI_TAG, clock, test_id));

	  irecv_inputs->request_proxy_list.pop_front();
	  array_of_indices[*outcount] = i;
	  *outcount = *outcount + 1;

	  /* push_back  new proxy */
	  next_proxy_request = new rempi_proxy_request(irecv_inputs->count, irecv_inputs->datatype);
	  irecv_inputs->request_proxy_list.push_back(next_proxy_request);
	  /* irecv(proxy) */
	  PMPI_Irecv(next_proxy_request->buf, 
		     irecv_inputs->count, irecv_inputs->datatype, irecv_inputs->source, irecv_inputs->tag, irecv_inputs->comm, 
		     &next_proxy_request->request);
	  //	  REMPI_DBG("Matched: request:%p source:%d tag:%d", &proxy_request->request, array_of_statuses[i].MPI_SOURCE, array_of_statuses[i].MPI_TAG);
	} else {
	  //	  REMPI_DBG("Uatched: request:%p source:%d tag:%d", &proxy_request->request, array_of_statuses[i].MPI_SOURCE, array_of_statuses[i].MPI_TAG);
	}
      }
      //event = dequeu
      //vec.push_back(event)
      //with_next = event.with_next
    }
    with_next = 0;
  }

  // copy eventvec =-> actual_buff
  return ret;
}


// int rempi_recorder_cdc::replay_testsome(
//     int incount,
//     void *array_of_requests[],
//     int *outcount,
//     int array_of_indices[],
//     void *array_of_statuses[])
// {
//   return 0;
//  }



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
  //TODO:
  //  fprintf(stderr, "ReMPI: Function call (%s:%s:%d)\n", __FILE__, __func__, __LINE__);
  return 0;
}

