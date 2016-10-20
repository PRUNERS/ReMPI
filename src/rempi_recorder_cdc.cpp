#include <mpi.h>

#if MPI_VERSION == 3 && !defined(REMPI_LITE)

#include <stdio.h>
#include <string.h>

#include <unordered_map>
#include <string>



//#include <iostream>
#include "rempi_recorder.h"
#include "rempi_msg_buffer.h"
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
#include "rempi_cp.h"
#include "rempi_mf.h"

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



int rempi_recorder_cdc::rempi_mf(int incount,
				 MPI_Request array_of_requests[],
				 int *outcount,
				 int array_of_indices[],
				 MPI_Status array_of_statuses[],
				 size_t **msg_id, // or clock
				 int matching_function_type)
{
  int ret;
  int recv_clock_length;


  if(matching_function_type == REMPI_MPI_WAITANY ||
     matching_function_type == REMPI_MPI_TESTANY) {
    recv_clock_length = 1;
  } else {
    recv_clock_length = incount;
  }
  clmpi_register_recv_clocks(pre_allocated_clocks, recv_clock_length);
  ret = rempi_mpi_mf(incount, array_of_requests, outcount, array_of_indices, array_of_statuses, matching_function_type);
  if (msg_id != NULL) {
    *msg_id =  pre_allocated_clocks;
  }
  if (rempi_mode == REMPI_ENV_REMPI_MODE_REPLAY) mc_encoder->update_local_look_ahead_send_clock(REMPI_ENCODER_REPLAYING_TYPE_ANY, REMPI_ENCODER_NO_MATCHING_SET_ID);
  return ret;
}

int rempi_recorder_cdc::rempi_pf(int source,
				 int tag,
				 MPI_Comm comm,
				 int *flag,
				 MPI_Status *status,
				 size_t *msg_id, // or clock
				 int prove_function_type)
{
  int ret;
  REMPI_ERR("MPI_Probe/Iprobe is not supported yet");
  clmpi_register_recv_clocks(pre_allocated_clocks, 1);
  ret = rempi_mpi_pf(source, tag, comm, flag, status, prove_function_type);
  *msg_id =  pre_allocated_clocks[0];
  return ret;
}

int rempi_recorder_cdc::record_init(int *argc, char ***argv, int rank) 
{
  string id;

  //fprintf(stderr, "ReMPI: Function call (%s:%s:%d)\n", __FILE__, __func__, __LINE__);
  init_clmpi();
  id = std::to_string((long long int)rank);
  replaying_event_list = new rempi_event_list<rempi_event*>(10000000, 100);
  recording_event_list = new rempi_event_list<rempi_event*>(10000000, 100);
  record_thread = new rempi_io_thread(recording_event_list, replaying_event_list, id, rempi_mode, (rempi_encoder**)&mc_encoder); //0: recording mode
  record_thread->start();
  
  return 0;
}

int rempi_recorder_cdc::replay_init(int *argc, char ***argv, int rank) 
{
  string id;
  string path;

  init_clmpi();
  id = std::to_string((long long int)rank);
  /*recording_event_list is needed for CDC, and when switching from repla mode to recording mode */
  recording_event_list = new rempi_event_list<rempi_event*>(10000000, 100); 
  replaying_event_list = new rempi_event_list<rempi_event*>(10000000, 100);
  read_record_thread = new rempi_io_thread(recording_event_list, replaying_event_list, id, rempi_mode, (rempi_encoder**)&mc_encoder); //1: replay mode
  //  REMPI_DBG("io thread is not runnig");
  path = rempi_record_dir_path + "/rank_" + id + ".rempi";
  mc_encoder->set_record_path(path);
  ((rempi_encoder_cdc*)mc_encoder)->init_cp(path.c_str());
  rempi_msgb_init(recording_event_list, replaying_event_list);

#ifdef REMPI_MULTI_THREAD
  read_record_thread->start();
#endif

  return 0;
}


int rempi_recorder_cdc::record_isend(mpi_const void *buf,
				     int count,
				     MPI_Datatype datatype,
				     int dest,
				     int tag,
				     MPI_Comm comm,
				     MPI_Request *request,
				     int send_function_type)
{
  int ret;
  char comm_id[REMPI_COMM_ID_LENGTH];
  int resultlen;
  int matching_set_id;

  ret = PMPI_Isend(buf, count, datatype, dest, tag, comm, request);
  rempi_reqmg_register_request(buf, count, datatype, dest, tag, comm, request, REMPI_SEND_REQUEST, &matching_set_id);
#ifdef REMPI_DBG_REPLAY
  size_t sent_clock;
  PNMPIMOD_get_local_clock(&sent_clock);
  sent_clock--;
  REMPI_DBGI(REMPI_DBG_REPLAY, "Record: Sent (rank:%d tag:%d clock:%lu)", dest, tag, sent_clock);
#endif
  return ret;
}

int rempi_recorder_cdc::replay_isend(mpi_const void *buf,
				     int count,  
				     MPI_Datatype datatype,
				     int dest,
				     int tag,
				     MPI_Comm comm,
				     MPI_Request *request,
				     int send_function_type)
{
  int ret;
  char comm_id[REMPI_COMM_ID_LENGTH];
  int resultlen;
  size_t sent_clock;
  int matching_set_id;

  rempi_cp_record_send(dest, 0);

  PNMPIMOD_get_local_clock(&sent_clock);
#ifdef REMPI_DBG_REPLAY
  REMPI_DBGI(REMPI_DBG_REPLAY, "Replay: Sending (rank:%d tag:%d clock:%lu)", dest, tag, sent_clock);
#endif
  ret = PMPI_Isend(buf, count, datatype, dest, tag, comm, request);
  rempi_reqmg_register_request(buf, count, datatype, dest, tag, comm, request, REMPI_SEND_REQUEST, &matching_set_id);


  if (sent_clock < rempi_cp_get_scatter_clock()) {
    REMPI_DBG("Exposting next clock (%lu) is bigger than accturally sent clock (%lu)", rempi_cp_get_scatter_clock(), sent_clock);
  }


  mc_encoder->update_local_look_ahead_send_clock(REMPI_ENCODER_REPLAYING_TYPE_ANY, REMPI_ENCODER_NO_MATCHING_SET_ID);

  return ret;
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
  rempi_event *e;
  int matching_set_id;

  ret = PMPI_Irecv(buf, count, datatype, source, tag, comm, request);
  rempi_reqmg_register_request(buf, count, datatype, source, tag, comm, request, REMPI_RECV_REQUEST, &matching_set_id);
  e = rempi_event::create_recv_event(MPI_ANY_SOURCE, tag, NULL, request);
  if (request_to_recv_event_umap.find(*request) != request_to_recv_event_umap.end()) {
    REMPI_ERR("Recv event of request(%p) already exists", *request);
  }
  request_to_recv_event_umap[*request] = e;


  return ret;
}



#define DEV_MSGB
#ifdef DEV_MSGB
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
  int matching_set_id;


  *request = rempi_msgb_allocate_request(REMPI_RECV_REQUEST);
  rempi_reqmg_register_request(buf, count, datatype, source, tag, comm, request, REMPI_RECV_REQUEST, &matching_set_id);
  ret = rempi_msgb_register_recv(buf, count, datatype, source, tag, comm, request, matching_set_id);
  
  return ret;
}
#else
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
  int matching_set_id;
    static size_t request_id = 0xffffff0000;
  if (request_to_irecv_inputs_umap.find(*request) != 
      request_to_irecv_inputs_umap.end()) {
    /* If this request is posted in irecv before*/
    irecv_inputs = request_to_irecv_inputs_umap.at(*request);
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

    //    memset(request, request_id, sizeof(MPI_Request));
    *request = (MPI_Request)request_id;
    request_id++;
    request_to_irecv_inputs_umap[*request] = new rempi_irecv_inputs(buf, count, datatype, source, tag, comm, *request);
    //    REMPI_DBGI(0, "Irecv: %p", *request);
  }

  irecv_inputs = request_to_irecv_inputs_umap.at(*request);
  rempi_reqmg_register_request(buf, count, datatype, source, tag, comm, request, REMPI_RECV_REQUEST, &matching_set_id);
  irecv_inputs->recv_test_id = matching_set_id;
  REMPI_DBG("Irecv: irecv_inputs: %p (request_id: %p)", irecv_inputs, request_id);

  if (irecv_inputs->request_proxy_list.size() == 0) {

    //    proxy_buf = allocate_proxy_buf(count, datatype);
    proxy_request_info = new rempi_proxy_request(count, datatype);
    //    proxy_request = (MPI_Request*)rempi_malloc(sizeof(MPI_Request));
    irecv_inputs->request_proxy_list.push_back(proxy_request_info);
    ret = PMPI_Irecv(proxy_request_info->buf, count, datatype, source, tag, comm, &proxy_request_info->request);
    //    rempi_reqmg_register_request(&proxy_request_info->request, source, tag, comm_id, REMPI_RECV_REQUEST);
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
#endif

// inherit from parent class
// int rempi_recorder_cdc::record_cancel(MPI_Request *request)


#ifdef DEV_MSGB
int rempi_recorder_cdc::replay_cancel(MPI_Request *request)
{
  int ret;
  int request_type;
  rempi_reqmg_get_request_type(request, &request_type);
  rempi_reqmg_deregister_request(request, request_type);
  rempi_msgb_cancel_request(request);

  return MPI_SUCCESS;
}
#else
int rempi_recorder_cdc::replay_cancel(MPI_Request *request)
{
  int ret;
  rempi_irecv_inputs *irecv_inputs;
  rempi_proxy_request *proxy_request;
  int request_type;
  rempi_reqmg_get_request_type(request, &request_type);
  rempi_reqmg_deregister_request(request, request_type);
    
  if (request_to_irecv_inputs_umap.find(*request) == request_to_irecv_inputs_umap.end()) {
    REMPI_ERR("No such request: %p", request);
  }
  irecv_inputs = request_to_irecv_inputs_umap.at(*request);
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
#endif


int rempi_recorder_cdc::replay_request_free(MPI_Request *request)
{
  REMPI_ERR("Not supported");
  return (int)0;
}




#if PMPI_Request_c2f != MPI_Fint && MPI_Request_c2f != MPI_Fint
MPI_Fint rempi_recorder_cdc::replay_request_c2f(MPI_Request request)
{
  REMPI_ERR("Not supported");
  return (MPI_Fint)0;
}
#else
MPI_Fint rempi_recorder_cdc::replay_request_c2f(MPI_Request request)
{
  REMPI_ERR("%s should not be called", __func__);
  return (int)0;
}
#endif



int rempi_recorder_cdc::dequeue_replay_event_set(vector<rempi_event*> &replaying_event_vec, int matching_set_id)
{
  int event_list_status;
  int is_completed = 0;
  rempi_event *replaying_event = NULL;
  while ((replaying_event = replaying_event_list->dequeue_replay(matching_set_id, event_list_status)) != NULL) {
#ifdef REMPI_DBG_REPLAY
    REMPI_DBGI(REMPI_DBG_REPLAY, "RPQ->A  : (count: %d, with_next: %d, flag: %d, source: %d, clock: %d) matching_set_id: %d",
	       replaying_event->get_event_counts(), replaying_event->get_is_testsome(), replaying_event->get_flag(),
	       replaying_event->get_source(), replaying_event->get_clock(), matching_set_id);
#endif

    replaying_event_vec.push_back(replaying_event);
    if (replaying_event->get_with_next() ==  REMPI_MPI_EVENT_NOT_WITH_NEXT) {
      is_completed = 1;
      break;
    }
  }

  return is_completed;
}

int rempi_recorder_cdc::progress_probe()
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


int rempi_recorder_cdc::progress_recv_and_safe_update_local_look_ahead_recv_clock(int do_fetch, 
										  int incount, MPI_Request *array_of_requests, int matching_set_id) 
{
  int is_updated;
  int has_recv_msg;

  if (do_fetch == REMPI_RECORDER_DO_FETCH_REMOTE_LOOK_AHEAD_SEND_CLOCKS) mc_encoder->fetch_remote_look_ahead_send_clocks();

  /* ======================================================== */
  /* Step 2: Test message receives (Update: Message Buffer)   */
  /* ======================================================== */
  do {
    has_recv_msg = progress_recv_requests(matching_set_id, incount, array_of_requests);
  } while (has_recv_msg == 1);
  
  progress_probe();
  //  PMPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &iprobe_flag_int, MPI_STATUS_IGNORE);


  //REMPI_DBGI(REMPI_DBG_REPLAY, " ==== update start");
#if 0
  is_updated = mc_encoder->update_local_look_ahead_recv_clock(iprobe_flag_int, &(this->pending_message_source_set), 
							      &recv_message_source_umap,
							      recv_clock_umap_p_2,
							      matching_set_id);
  {
    unordered_map<int, size_t> *tmp;
    tmp = recv_clock_umap_p_2;
    recv_clock_umap_p_2 = recv_clock_umap_p_1;
    recv_clock_umap_p_1 = tmp;
  }
#else
  is_updated = mc_encoder->update_local_look_ahead_recv_clock(
							      &probed_message_source_set,
							      &(this->pending_message_source_set), 
							      &recv_message_source_umap,
							      &recv_clock_umap,
							      matching_set_id);
#endif
  return is_updated;
}

int rempi_recorder_cdc::get_next_events(int incount, MPI_Request *array_of_requests, vector<rempi_event*> &replaying_event_vec, int matching_set_id)
{
  rempi_event *replaying_event = NULL;
  int interval = 0;
  int do_fetch = 1;
  int has_recv_msg = 0, iprobe_flag_int = 0, has_recv_or_probe_msg = 0;
  int is_completed = 0;

  /*This call is necessary, but call this just in case*/
  mc_encoder->update_local_look_ahead_send_clock(REMPI_ENCODER_REPLAYING_TYPE_ANY, REMPI_ENCODER_NO_MATCHING_SET_ID);

  while (!is_completed) {
    this->progress_recv_and_safe_update_local_look_ahead_recv_clock(
								    (interval++ % 10 == 0)? 
								    REMPI_RECORDER_DO_FETCH_REMOTE_LOOK_AHEAD_SEND_CLOCKS:
								    REMPI_RECORDER_DO_NOT_FETCH_REMOTE_LOOK_AHEAD_SEND_CLOCKS,
								    incount, array_of_requests, matching_set_id);

#ifdef REMPI_MAIN_THREAD_PROGRESS
    mc_encoder->progress_decoding(recording_event_list, replaying_event_list, matching_set_id);
#endif
    is_completed = this->dequeue_replay_event_set(replaying_event_vec, matching_set_id);

    if (!is_completed) {
      /*
	Not replayed any                 => is_completed=0: TYPE_RECV (if next event is unmatched, unmatched event must be returned)
	Replayed Incomplete Matched test => is_completed=0: TYPE_RECV
	Replayed complete unmatched test => is_completed=1: TYPE_ANY 
	Replayed complete matched test   => is_completed=1: TYPE_ANY
      */
      mc_encoder->update_local_look_ahead_send_clock(REMPI_ENCODER_REPLAYING_TYPE_RECV, matching_set_id);
    }


  } /* while (with_next == REMPI_MPI_EVENT_WITH_NEXT) */  

  return 0;
}


#ifdef DEV_MSGB
int rempi_recorder_cdc::replay_mf_input(
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
  rempi_irecv_inputs *irecv_inputs;
  MPI_Status status;
  int index = 0;


  for (int i = 0, n = replaying_event_vec.size(); i < n; i++) {
    replaying_test_event = replaying_event_vec[i];
    if (replaying_test_event->get_type() != REMPI_MPI_EVENT_TYPE_TEST) {
      REMPI_ERR("Invalid event type in mf: %d", replaying_test_event->get_type());
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
	REMPI_ERR("Replaying send event to rank %d, but dest of this send request is rank %d (MF: %d, matched_index: %d) %p", 
		  replaying_test_event->get_rank(), tmp_statuses[index].MPI_SOURCE, matching_function_type, index, array_of_requests[index]);
	REMPI_ASSERT(0);
      }
      rempi_reqmg_deregister_request(&array_of_requests[index], REMPI_SEND_REQUEST);      
      REMPI_ERR("send wait");
      PMPI_Wait(&array_of_requests[index], &array_of_statuses[local_outcount]);
    } else if (request_info[index] == REMPI_RECV_REQUEST) {
      int requested_source, requested_tag;
      MPI_Comm requested_comm;
      void *requested_buffer;

      rempi_reqmg_get_matching_id(&array_of_requests[index], &requested_source, &requested_tag, &requested_comm);
      rempi_reqmg_get_buffer(&array_of_requests[index], &requested_buffer);
      rempi_msgb_recv_msg(requested_buffer, replaying_test_event->get_rank(), requested_tag, requested_comm, replaying_test_event->get_msg_id(),
			  &array_of_statuses[local_outcount]);
      rempi_reqmg_deregister_request(&array_of_requests[index], REMPI_RECV_REQUEST);
      clmpi_sync_clock(replaying_test_event->get_clock());
      mc_encoder->update_local_look_ahead_send_clock(REMPI_ENCODER_REPLAYING_TYPE_ANY, REMPI_ENCODER_NO_MATCHING_SET_ID);

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
  }

  if (outcount != NULL) {
    *outcount = (matching_function_type == REMPI_MPI_TESTSOME ||
		 matching_function_type == REMPI_MPI_WAITSOME)? local_outcount:1;
  }


  return MPI_SUCCESS;
}
#else
int rempi_recorder_cdc::replay_mf_input(
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
  rempi_irecv_inputs *irecv_inputs;
  MPI_Status status;
  int index = 0;


  for (int i = 0, n = replaying_event_vec.size(); i < n; i++) {
    replaying_test_event = replaying_event_vec[i];
    if (replaying_test_event->get_type() != REMPI_MPI_EVENT_TYPE_TEST) {
      REMPI_ERR("Invalid event type in mf: %d", replaying_test_event->get_type());
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
	REMPI_ERR("Replaying send event to rank %d, but dest of this send request is rank %d (MF: %d, matched_index: %d) %p", 
		  replaying_test_event->get_rank(), tmp_statuses[index].MPI_SOURCE, matching_function_type, index, array_of_requests[index]);
	REMPI_ASSERT(0);
      }
      rempi_reqmg_deregister_request(&array_of_requests[index], REMPI_SEND_REQUEST);      
      REMPI_ERR("send wait");
      PMPI_Wait(&array_of_requests[index], &array_of_statuses[local_outcount]);
    } else if (request_info[index] == REMPI_RECV_REQUEST) {
      irecv_inputs = request_to_irecv_inputs_umap.at(array_of_requests[index]);
   
      if (irecv_inputs->source != MPI_ANY_SOURCE && 
	  irecv_inputs->source != replaying_test_event->get_source()) {
	REMPI_ERR("Replaying recv event from rank %d, but src of this recv request is rank %d: request: %p, index: %d", 
		  replaying_test_event->get_rank(), irecv_inputs->source,
		  array_of_requests[index], index);
      }
      rempi_reqmg_deregister_request(&array_of_requests[index], REMPI_RECV_REQUEST);



#if 1
      REMPI_DBGI(0, "irecv_inputs: %p (request_id: %p), index: %d: size: %lu", irecv_inputs, array_of_requests[index], index, irecv_inputs->matched_request_proxy_list.size());
      //      request_to_irecv_inputs_umap.erase(array_of_requests[index]);

      //PMPI_Wait(&array_of_requests[index], &status);
      //      array_of_requests[index] = MPI_REQUEST_NULL;
      array_of_statuses[local_outcount].MPI_SOURCE = replaying_test_event->get_source();
      array_of_statuses[local_outcount].MPI_TAG    = irecv_inputs->tag;

      rempi_proxy_request *proxy_request;
      clmpi_sync_clock(replaying_test_event->get_clock());
      if (irecv_inputs->matched_request_proxy_list.size() == 0) {
	REMPI_ERR("irecv_inputs->matched_proxy_list is empty: input_inputs:%p request:%p: Matching condition of this MPI_Request may be overlapping with matching condition of another MPI_Request", irecv_inputs, array_of_requests[index]);
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
      copy_proxy_buf(proxy_request->buf, irecv_inputs->buf, replaying_test_event->msg_count, irecv_inputs->datatype);
      irecv_inputs->matched_request_proxy_list.pop_front();
      delete proxy_request;
      //      delete irecv_inputs;
      mc_encoder->update_local_look_ahead_send_clock(REMPI_ENCODER_REPLAYING_TYPE_ANY, REMPI_ENCODER_NO_MATCHING_SET_ID);
#else 
      delete irecv_inputs;
      request_to_irecv_inputs_umap.erase(array_of_requests[index]);

      PMPI_Wait(&array_of_requests[index], &status);
      array_of_statuses[local_outcount].MPI_SOURCE = status.MPI_SOURCE;
      array_of_statuses[local_outcount].MPI_TAG    = status.MPI_TAG;

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
  }

  if (outcount != NULL) {
    *outcount = local_outcount;
  }

  // if (outcount == NULL) {
  //   if (local_outcount != 0) {
  //     /*print*/
  //     for (int j = 0; j < replaying_event_vec.size(); j++) {
  // REMPI_DBGI(0, "= dbg Replay: (count: %d, with_next: %d, flag: %d, source: %d, clock: %d): matching_set_id: %d func: %d",
  //    replaying_event_vec[j]->get_event_counts(), replaying_event_vec[j]->get_is_testsome(), replaying_event_vec[j]->get_flag(),
  //    replaying_event_vec[j]->get_source(), replaying_event_vec[j]->get_clock(), matching_set_id, matching_function_type);
  //     }
  //     exit(1);
  //   }
  // } else {
  //   if (*outcount != local_outcount) {
  //     /*print*/
  //     for (int j = 0; j < replaying_event_vec.size(); j++) {
  // REMPI_DBGI(0, "= dbg Replay: (count: %d, with_next: %d, flag: %d, source: %d, clock: %d): matching_set_id: %d ",
  //    replaying_event_vec[j]->get_event_counts(), replaying_event_vec[j]->get_is_testsome(), replaying_event_vec[j]->get_flag(),
  //    replaying_event_vec[j]->get_source(), replaying_event_vec[j]->get_clock(), matching_set_id);
  
  //     }
  //     exit(1);
  //   }
  // }
  


  return 0;
}
#endif


int rempi_recorder_cdc::record_pf(int source,
				  int tag,
				  MPI_Comm comm,
				  int *flag,
				  MPI_Status *status,
				  int prove_function_type)
{
  REMPI_ERR("not implemented yet");
  return 0;
}




int rempi_recorder_cdc::progress_decode()
{
  return 0;
}


/*Test all recv requests, and if matched, put the events to the queue*/
// int rempi_recorder_cdc::progress_recv_requests(int recv_test_id,
// 					       int incount,
// 					       MPI_Request array_of_requests[], 
// 					       unordered_set<int> *pending_message_sources_set,
// 					       unordered_map<int, size_t> *recv_message_source_umap,
// 					       unordered_map<int, size_t> *recv_clock_umap)

#ifdef DEV_MSGB

int rempi_recorder_cdc::progress_recv_requests(int recv_test_id,
					       int incount,
					       MPI_Request array_of_requests[])
{
  rempi_msgb_progress_recv();
  return 0;
}
#else

int rempi_recorder_cdc::progress_recv_requests(int recv_test_id,
					       int incount,
					       MPI_Request array_of_requests[])
{
  int flag;
  rempi_proxy_request *proxy_request;
  MPI_Status status;
  size_t clock;
  rempi_proxy_request *next_proxy_request;
  rempi_event *event_pooled;
  int has_recv_msg = 0;
  rempi_irecv_inputs *irecv_inputs;
  int matched_request_push_back_count = 0;

  this->pending_message_source_set.clear();    
  this->recv_message_source_umap.clear();

  for (unordered_map<MPI_Request, rempi_irecv_inputs*>::const_iterator cit = request_to_irecv_inputs_umap.cbegin(),
	 cit_end = request_to_irecv_inputs_umap.cend();
       cit != cit_end;
       cit++) {
    irecv_inputs = cit->second;
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
	  // NOTE: comment out because MPI requests can be tested in different MF
	  // REMPI_ERR("A MPI request is used in different MFs: "
	  // 	    "this request used for recv_test_id: %d first, but this is recv_test_id: %d (tag: %d)",
	  // 	    irecv_inputs->recv_test_id, recv_test_id,
	  // 	    irecv_inputs->tag);
	}
      }
    } 
    
    /* =================================================
       1. Check if there are any matched requests (pending matched), 
       which have already matched in different recv_test_id
       ================================================= */
    if (irecv_inputs->recv_test_id != -1) {
      matched_request_push_back_count = 0;

      for (list<rempi_proxy_request*>::iterator it = irecv_inputs->matched_pending_request_proxy_list.begin(),
	     it_end = irecv_inputs->matched_pending_request_proxy_list.end();
	   it != it_end;
	   it++) {
	rempi_proxy_request *proxy_request = *it;

	REMPI_ASSERT(proxy_request->matched_source != -1);
	//	REMPI_ASSERT(proxy_request->matched_tag    != -1);
	REMPI_ASSERT(proxy_request->matched_clock  !=  0);
	REMPI_ASSERT(proxy_request->matched_count  != -1);
	
	// event_pooled =  new rempi_test_event(1, -1, -1, 1,
	// 				     proxy_request->matched_source, 
	// 				     proxy_request->matched_tag, 
	// 				     proxy_request->matched_clock, 
	// 				     irecv_inputs->recv_test_id);
	event_pooled =  rempi_event::create_test_event(1, 
						       1, // flag=1
						       proxy_request->matched_source, 
						       REMPI_MPI_EVENT_INPUT_IGNORE, // with_next
						       REMPI_MPI_EVENT_INPUT_IGNORE, // index
						       proxy_request->matched_clock, // msg_id
						       irecv_inputs->recv_test_id);  // gid

	event_pooled->msg_count = proxy_request->matched_count;

	recording_event_list->enqueue_replay(event_pooled, irecv_inputs->recv_test_id);
	/* next recv message is "receved clock + 1" => event_pooled->get_clock() + 1 */
	recv_clock_umap.insert(make_pair(event_pooled->get_source(), event_pooled->get_clock() + 1));
#ifdef REMPI_DBG_REPLAY	  
	REMPI_DBGI(REMPI_DBG_REPLAY, "A->RCQ  : (count: %d, with_next: %d, flag: %d, source: %d,  clock: %d, msg_count: %d %p): recv_test_id: %d",
		   event_pooled->get_event_counts(), event_pooled->get_is_testsome(), event_pooled->get_flag(),
		   event_pooled->get_source(), event_pooled->get_clock(), event_pooled->msg_count,
		   event_pooled, irecv_inputs->recv_test_id);
#endif
	/*Move from pending request list to matched request list 
	  This is used later for checking if a replayed event belongs to this list.  */
	irecv_inputs->matched_request_proxy_list.push_back(proxy_request);
#ifdef REMPI_DBG_REPLAY
	//	REMPI_DBGI(REMPI_DBG_REPLAY, "Added to matched_proxy_request in %p: size:%d request: %p", irecv_inputs, irecv_inputs->matched_request_proxy_list.size(), pair_req_and_irecv_inputs.first);
#endif
	matched_request_push_back_count++;
      }
      for (int i = 0; i < matched_request_push_back_count; i++) {
	irecv_inputs->matched_pending_request_proxy_list.pop_front();
      }
    }

#if 1
    for (list<rempi_proxy_request*>::iterator it = irecv_inputs->matched_pending_request_proxy_list.begin(),
	   it_end = irecv_inputs->matched_pending_request_proxy_list.end();
	 it != it_end;
	 it++) {
      rempi_proxy_request *proxy_request = *it;
      /* When decoding thread computes my exporsing next_clock,
	 1. Retrive sender next clocks (= A) MPI_Get 
	 2. Then, estimate the minimum clock to send
	 3. Update exposing next_clock (= C)
	 sender next clocks are minimul clocks (= A) that will be sent
	 If messages exist in matched_pending_request_proxy_list, 
	 a clock of the message (= B) is smaller than A, i.e, B < A.
	 If we computes my exposing next_clock based on A when matched_pending_request_proxy_list.size() > 0,
	 the computed exposing next_clock, C will be bigger than correct value of C.
	 So we set has_pending_msg = true here, to avoid sender next clocks (= A) are updated, 
	 and to keep A less than B.	
      */
      //      REMPI_DBG("Pending source: %d", proxy_request->matched_source);
      pending_message_source_set.insert(proxy_request->matched_source);
    }
#else
    if (irecv_inputs->matched_pending_request_proxy_list.size() > 0) {
      /* When decoding thread computes my exporsing next_clock,
	 1. Retrive sender next clocks (= A) MPI_Get 
	 2. Then, estimate the minimum clock to send
	 3. Update exposing next_clock (= C)
	 sender next clocks are minimul clocks (= A) that will be sent
	 If messages exist in matched_pending_request_proxy_list, 
	 a clock of the message (= B) is smaller than A, i.e, B < A.
	 If we computes my exposing next_clock based on A when matched_pending_request_proxy_list.size() > 0,
	 the computed exposing next_clock, C will be bigger than correct value of C.
	 So we set has_pending_msg = true here, to avoid sender next clocks (= A) are updated, 
	 and to keep A less than B.	
      */
      //      REMPI_DBG("Pending source: %d", irecv_inputs->source);
      pending_message_source_set.insert(irecv_inputs->source);
    }
#endif
    
    /* ================================================= */


    
    /* =================================================
       2. Check if this request have matched.
       ================================================= */
    /* irecv_inputs->request_proxy_list.size() == 0 can happens 
       when this request was canceled (MPI_Cancel) */
    if (irecv_inputs->request_proxy_list.size() == 0) continue; 
    proxy_request = irecv_inputs->request_proxy_list.front();
    clmpi_register_recv_clocks(&clock, 1);
    clmpi_clock_control(0);
    flag = 0;
    REMPI_ASSERT(proxy_request != NULL);
    REMPI_ASSERT(proxy_request->request != NULL);

    // #ifdef REMPI_DBG_REPLAY	  
    //       REMPI_DBGI(REMPI_DBG_REPLAY, "Test  : (source: %d, tag: %d, request: %p): recv_test_id: %d",
    // 		 irecv_inputs->source, irecv_inputs->tag, &proxy_request->request, irecv_inputs->recv_test_id);
    //#endif
    PMPI_Test(&proxy_request->request, &flag, &status);
    clmpi_clock_control(1);

    
    if(!flag) continue;
    /*If flag == 1*/
    has_recv_msg = 1;
    rempi_cp_record_recv(status.MPI_SOURCE, 0);
    recv_message_source_umap.insert(make_pair(status.MPI_SOURCE, 0));


    REMPI_DBGI(0, "A->     : (source: %d, tag: %d,  clock: %d): current irecv: %p recv_test_id: %d, size: %d (time: %f)",
	       status.MPI_SOURCE, status.MPI_TAG, clock, irecv_inputs, irecv_inputs->recv_test_id, 
	       request_to_irecv_inputs_umap.size(), PMPI_Wtime());
#ifdef REMPI_DBG_REPLAY	  
    REMPI_DBGI(REMPI_DBG_REPLAY, "A->     : (source: %d, tag: %d,  clock: %d): current irecv: %p recv_test_id: %d, size: %d (time: %f)",
	       status.MPI_SOURCE, status.MPI_TAG, clock, irecv_inputs, irecv_inputs->recv_test_id, 
	       request_to_irecv_inputs_umap.size(), PMPI_Wtime());
#endif



    if (irecv_inputs->recv_test_id != -1) {
      //      event_pooled =  new rempi_test_event(1, -1, -1, 1, status.MPI_SOURCE, status.MPI_TAG, clock, irecv_inputs->recv_test_id);
      event_pooled =  rempi_event::create_test_event(1, 
						     1, 
						     status.MPI_SOURCE, 
						     REMPI_MPI_EVENT_INPUT_IGNORE, 
						     REMPI_MPI_EVENT_INPUT_IGNORE, 
						     clock,
						     irecv_inputs->recv_test_id);

      PMPI_Get_count(&status, irecv_inputs->datatype, &(event_pooled->msg_count));

      recording_event_list->enqueue_replay(event_pooled, irecv_inputs->recv_test_id);
      recv_clock_umap.insert(make_pair(event_pooled->get_source(), event_pooled->get_clock()));



#ifdef REMPI_DBG_REPLAY	  
      REMPI_DBGI(REMPI_DBG_REPLAY, "A->RCQ  : (count: %d, with_next: %d, flag: %d, source: %d, clock: %d, msg_count: %d %p): recv_test_id: %d",
		 event_pooled->get_event_counts(), event_pooled->get_is_testsome(), event_pooled->get_flag(),
		 event_pooled->get_source(), event_pooled->get_clock(), event_pooled->msg_count,
		 event_pooled, irecv_inputs->recv_test_id);
#endif
      /*Move from pending request list to matched request list 
	This is used later for checking if a replayed event belongs to this list.  */
      irecv_inputs->matched_request_proxy_list.push_back(proxy_request);
#ifdef REMPI_DBG_REPLAY
      //      REMPI_DBGI(REMPI_DBG_REPLAY, "Added to matched_proxy_request in %p: size:%d request: %p", irecv_inputs, irecv_inputs->matched_request_proxy_list.size(), pair_req_and_irecv_inputs.first);
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
      //      REMPI_DBG("Pending source: %d", status.MPI_SOURCE);
      //      pending_message_source_set->insert(irecv_inputs->source);
      pending_message_source_set.insert(status.MPI_SOURCE);
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
  // if (!has_recv_msg) {
  //   REMPI_DBG("======= No msg arrive =========");
  //   for (auto &pair_req_and_irecv_inputs: request_to_irecv_inputs_umap) {
  //     irecv_inputs = pair_req_and_irecv_inputs.second;
  //     REMPI_DBG("== source: %d, tag: %d: length: %d", irecv_inputs->source, irecv_inputs->tag, irecv_inputs->request_proxy_list.size());
  //   }
  // }
#endif
  return has_recv_msg;
}
#endif


int rempi_recorder_cdc::replay_pf(int source, int tag, MPI_Comm comm, int *flag, MPI_Status *status, int comm_id)
{
  REMPI_ERR("Probe/Iprobe is not supported yet");
  return MPI_SUCCESS;
}


int rempi_recorder_cdc::pre_process_collective(MPI_Comm comm)
{
#ifdef REMPI_DBG_REPLAY
  REMPI_DBGI(REMPI_DBG_REPLAY, "==== Record/Replay: Collective ======");
#endif
  if (rempi_mode == REMPI_ENV_REMPI_MODE_REPLAY) mc_encoder->set_fd_clock_state(1);
  PNMPIMOD_collective_sync_clock(comm);
  if (rempi_mode == REMPI_ENV_REMPI_MODE_REPLAY) mc_encoder->set_fd_clock_state(0);

  return MPI_SUCCESS;
}

int rempi_recorder_cdc::post_process_collective()
{
  if (rempi_mode == REMPI_ENV_REMPI_MODE_REPLAY) {
    progress_recv_and_safe_update_local_look_ahead_recv_clock(REMPI_RECORDER_DO_FETCH_REMOTE_LOOK_AHEAD_SEND_CLOCKS, 
							      0, NULL, REMPI_MATCHING_SET_ID_IGNORE);
    //    mc_encoder->fetch_remote_look_ahead_send_clocks(); /*Becuase local_clock is 3*/
    //    mc_encoder->update_local_look_ahead_recv_clock(-1, NULL, NULL, NULL, -1);  
    mc_encoder->update_local_look_ahead_send_clock(REMPI_ENCODER_REPLAYING_TYPE_ANY, REMPI_ENCODER_NO_MATCHING_SET_ID);
  }
  return MPI_SUCCESS;
}



int rempi_recorder_cdc::record_finalize(void)
{
  int ret;

  pre_process_collective(MPI_COMM_WORLD);
  ret = PMPI_Barrier(MPI_COMM_WORLD);
  ret = PMPI_Finalize();

  if (rempi_mode == REMPI_ENV_REMPI_MODE_RECORD) {
    recording_event_list->push_all();
    /*TODO: set flag in event_list 
      insteand of setting flag of thread (via complete_flush)
      like in replay mode*/
  } else if (rempi_mode == REMPI_ENV_REMPI_MODE_REPLAY) {
    /*TODO: if replay thread is still running, throw the error*/
  } else {
    REMPI_ERR("Unkonw rempi mode: %d", rempi_mode);
  }
  record_thread->join();

  REMPI_DBGI(0, "validation code: %u", validation_code);
  return ret;
}



int rempi_recorder_cdc::replay_finalize(void)
{
  int ret;

  pre_process_collective(MPI_COMM_WORLD);
  ret = PMPI_Barrier(MPI_COMM_WORLD);
  ret = PMPI_Finalize();

#ifdef REMPI_MULTI_THREAD
  read_record_thread->join();
#endif

#if 0
  if (counta == 0) {
    REMPI_DBGI(0, "Get: %f (count: %lu, is_pending:%lu, is_updated:%lu, single: %f), waiting: %f (count: %lu, single: %f)", 
	       dura, counta, counta_pending, counta_update, -1, durb, countb, dura/countb);
  } else {
    REMPI_DBGI(0, "Get: %f (count: %lu, is_pending:%lu, is_updated:%lu, single: %f), waiting: %f (count: %lu, single: %f)", 
	       dura, counta, counta_pending, counta_update, dura/counta, durb, countb, dura/countb);
  }
#endif

  REMPI_DBGI(0, "validation code: %u", validation_code);
  return ret;
}





#endif /* MPI_LITE */
