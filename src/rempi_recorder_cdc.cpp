/* ==========================ReMPI:LICENSE==========================================   
   Copyright (c) 2016, Lawrence Livermore National Security, LLC.                     
   Produced at the Lawrence Livermore National Laboratory.                            
                                                                                       
   Written by Kento Sato, kento@llnl.gov. LLNL-CODE-711357.                           
   All rights reserved.                                                               
                                                                                       
   This file is part of ReMPI. For details, see https://github.com/PRUNER/ReMPI       
   Please also see the LICENSE file for our notice and the LGPL.                      
                                                                                       
   This program is free software; you can redistribute it and/or modify it under the   
   terms of the GNU General Public License (as published by the Free Software         
   Foundation) version 2.1 dated February 1999.                                       
                                                                                       
   This program is distributed in the hope that it will be useful, but WITHOUT ANY    
   WARRANTY; without even the IMPLIED WARRANTY OF MERCHANTABILITY or                  
   FITNESS FOR A PARTICULAR PURPOSE. See the terms and conditions of the GNU          
   General Public License for more details.                                           
                                                                                       
   You should have received a copy of the GNU Lesser General Public License along     
   with this program; if not, write to the Free Software Foundation, Inc., 59 Temple   
   Place, Suite 330, Boston, MA 02111-1307 USA
   ============================ReMPI:LICENSE========================================= */
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
#include "rempi_util.h"
#include "rempi_err.h"
#include "rempi_config.h"
#include "rempi_mem.h"
#include "rempi_request_mg.h"
#include "rempi_cp.h"
#include "rempi_mf.h"
#include "rempi_clock.h"
#include "rempi_sig_handler.h"

#define  PNMPI_MODULE_REMPI "rempi"


double stra, dura;
int counta = 0;
int counta_update = 0;
int counta_pending = 0;
double strb, durb;
int countb = 0;






int rempi_recorder_cdc::rempi_mf(int incount,
				 MPI_Request array_of_requests[],
				 int *outcount,
				 int array_of_indices[],
				 MPI_Status array_of_statuses[],
				 size_t **msg_id, // or clock
				 int *request_info,
				 int matching_function_type)
{
  int ret;
  int recv_clock_length;
  int matched_count;
  int nullcount = 0;
  recv_clock_length = (matching_function_type == REMPI_MPI_WAITANY || matching_function_type == REMPI_MPI_TESTANY)? 1:incount;
  rempi_clock_register_recv_clocks(pre_allocated_clocks, recv_clock_length);
  ret = rempi_mpi_mf(incount, array_of_requests, outcount, array_of_indices, array_of_statuses, matching_function_type);
  for (int i = 0; i < incount; i++) { 
    if (request_info[i] == REMPI_NULL_REQUEST) nullcount++;
    //    array_of_statuses[i].MPI_ERROR = MPI_SUCCESS;
  }
  if (matching_function_type == REMPI_MPI_TESTANY ||
      matching_function_type == REMPI_MPI_WAITANY) {
    array_of_statuses[0].MPI_ERROR = MPI_SUCCESS;
  } else {
    for (int i = 0; i < incount; i++) { 
      array_of_statuses[i].MPI_ERROR = MPI_SUCCESS;
    }
  }
  matched_count = rempi_mpi_get_matched_count(incount, outcount, nullcount, matching_function_type);
  rempi_clock_sync_clock(matched_count, array_of_indices, pre_allocated_clocks, request_info, matching_function_type);
  if (msg_id != NULL) {
    *msg_id =  pre_allocated_clocks;
  }
  //  rempi_clock_

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
  rempi_clock_register_recv_clocks(pre_allocated_clocks, 1);
  ret = rempi_mpi_pf(source, tag, comm, flag, status, prove_function_type);
  *msg_id =  pre_allocated_clocks[0];
  return ret;
}

void rempi_recorder_cdc::init(int rank)
{
  string id;
  string path;

  //fprintf(stderr, "ReMPI: Function call (%s:%s:%d)\n", __FILE__, __func__, __LINE__);
  rempi_clock_init();
  replaying_event_list = new rempi_event_list<rempi_event*>(10000000, 100);
  recording_event_list = new rempi_event_list<rempi_event*>(10000000, 100);
  id = std::to_string((long long int)rank);
  path = rempi_record_dir_path + "/rank_" + id + ".rempi";
  
  mc_encoder = new rempi_encoder_cdc(rempi_mode, path);
  return;
}

int rempi_recorder_cdc::record_init(int *argc, char ***argv, int rank) 
{


  this->init(rank);
  record_thread = new rempi_io_thread(recording_event_list, replaying_event_list, rempi_mode, mc_encoder);
  rempi_sig_handler_init(rank, record_thread, recording_event_list, &validation_code);
  record_thread->start();  
  return 0;
}

int rempi_recorder_cdc::replay_init(int *argc, char ***argv, int rank) 
{
  string id;
  string path;



  this->init(rank);
  read_record_thread = new rempi_io_thread(recording_event_list, replaying_event_list, rempi_mode, mc_encoder); //1: replay mode

  id = std::to_string((long long int)rank);
  path = rempi_record_dir_path + "/rank_" + id + ".rempi";
  mc_encoder->set_record_path(path);
  ((rempi_encoder_cdc*)mc_encoder)->init_cp(path.c_str());
  rempi_msgb_init(recording_event_list, replaying_event_list);
  rempi_sig_handler_init(rank, record_thread, recording_event_list, &validation_code);



#ifdef REMPI_MULTI_THREAD
  read_record_thread->start();
#endif

  return 0;
}

int rempi_recorder_cdc::record_recv_init(void *arg_0, int arg_1, MPI_Datatype arg_2, int arg_3, int arg_4, MPI_Comm arg_5, MPI_Request *arg_6)
{
  return PMPI_Recv_init(arg_0, arg_1, arg_2, arg_3, arg_4, arg_5, arg_6);
}

int rempi_recorder_cdc::replay_recv_init(void *arg_0, int arg_1, MPI_Datatype arg_2, int arg_3, int arg_4, MPI_Comm arg_5, MPI_Request *arg_6)
{
  return PMPI_Recv_init(arg_0, arg_1, arg_2, arg_3, arg_4, arg_5, arg_6);
}

int rempi_recorder_cdc::record_send_init(mpi_const void *arg_0, int arg_1, MPI_Datatype arg_2, int arg_3, int arg_4, MPI_Comm arg_5, MPI_Request *arg_6, int send_function_type)
{
  int matching_set_id;
  return rempi_mpi_send_init(arg_0, arg_1, arg_2, arg_3, arg_4, arg_5, arg_6, send_function_type);
}

int rempi_recorder_cdc::replay_send_init(mpi_const void *arg_0, int arg_1, MPI_Datatype arg_2, int arg_3, int arg_4, MPI_Comm arg_5, MPI_Request *arg_6, int send_function_type)
{
  int matching_set_id;
  return rempi_mpi_send_init(arg_0, arg_1, arg_2, arg_3, arg_4, arg_5, arg_6, send_function_type);
}

int rempi_recorder_cdc::record_start(MPI_Request *arg_0)
{
  return PMPI_Start(arg_0);
}

int rempi_recorder_cdc::replay_start(MPI_Request *arg_0)
{
  return PMPI_Start(arg_0);
}

int rempi_recorder_cdc::record_startall(int arg_0, MPI_Request *arg_1)
{
  return PMPI_Startall(arg_0, arg_1);
}

int rempi_recorder_cdc::replay_startall(int arg_0, MPI_Request *arg_1)
{
  return PMPI_Startall(arg_0, arg_1);
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

  if (comm != MPI_COMM_WORLD) REMPI_ERR("ReMPI only supports MPI_COMM_WORLD");

#ifdef REMPI_DBG_REPLAY
  size_t sent_clock;
  rempi_clock_get_local_clock(&sent_clock);
  REMPI_DBGI(REMPI_DBG_REPLAY, "Record: Sent (rank:%d tag:%d clock:%lu)", dest, tag, sent_clock);
#endif
  ret = PMPI_Isend(buf, count, datatype, dest, tag, comm, request);
  rempi_clock_mpi_isend(buf, count, datatype, dest, tag, comm, request, send_function_type);

  rempi_reqmg_register_request(buf, count, datatype, dest, tag, comm, request, REMPI_SEND_REQUEST, &matching_set_id);


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

  rempi_clock_get_local_clock(&sent_clock);
#ifdef REMPI_DBG_REPLAY
  REMPI_DBGI(REMPI_DBG_REPLAY, "Replay: Sending (rank:%d tag:%d clock:%lu)", dest, tag, sent_clock);
#endif

  ret = PMPI_Isend(buf, count, datatype, dest, tag, comm, request);
  rempi_clock_mpi_isend(buf, count, datatype, dest, tag, comm, request, send_function_type);

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


int rempi_recorder_cdc::replay_cancel(MPI_Request *request)
{
  int ret;
  int request_type;
  rempi_reqmg_get_request_type(request, &request_type);
  rempi_reqmg_deregister_request(request, request_type);
  rempi_msgb_cancel_request(request);

  return MPI_SUCCESS;
}



int rempi_recorder_cdc::replay_request_free(MPI_Request *request)
{
  int ret;
  int request_type;
  rempi_reqmg_get_request_type(request, &request_type);
  rempi_reqmg_deregister_request(request, request_type);
  rempi_msgb_cancel_request(request);
  return MPI_SUCCESS;
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


int rempi_recorder_cdc::dequeue_replay_event_set(int incount, MPI_Request array_of_requests[], int *request_info, int matching_set_id, int matching_function_type, vector<rempi_event*> &replaying_event_vec) 
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

/* left < right: 1, left == right: 0 left > rigth: -1 */
int rempi_recorder_cdc::compare_clock(size_t left_clock, int left_rank, size_t right_clock, int right_rank)
{
  if (left_clock < right_clock) {
    return 1;
  } else if (left_clock == right_clock) {
    if (left_rank <  right_rank) return 1;
    if (left_rank == right_rank) return 0;
    if (left_rank >  right_rank) return -1;
  }

  return -1;
}



int rempi_recorder_cdc::progress_recv_and_safe_update_local_look_ahead_recv_clock(int do_fetch, 
										  int incount, MPI_Request *array_of_requests, int matching_set_id) 
{
  int is_updated;
  int has_recv_msg;


  if (do_fetch == REMPI_RECORDER_DO_FETCH_REMOTE_LOOK_AHEAD_SEND_CLOCKS) mc_encoder->fetch_remote_look_ahead_send_clocks();



  do {
    has_recv_msg = progress_recv_requests(matching_set_id, incount, array_of_requests);
  } while (has_recv_msg == 1);


  is_updated = mc_encoder->update_local_look_ahead_recv_clock(NULL, NULL, NULL, NULL, matching_set_id);

  return is_updated;
}

int rempi_recorder_cdc::get_next_events(int incount, MPI_Request *array_of_requests, int request_info[], 
					vector<rempi_event*> &replaying_event_vec, int matching_set_id, int matching_function_type)
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
    is_completed = this->dequeue_replay_event_set(incount, array_of_requests, request_info, matching_set_id, matching_function_type, replaying_event_vec);

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
      rempi_clock_sync_clock(0, array_of_indices, pre_allocated_clocks, request_info, matching_function_type);
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
      if (rempi_encode == REMPI_ENV_REMPI_ENCODE_CDC) REMPI_ERR("send wait");
      PMPI_Wait(&array_of_requests[index], &array_of_statuses[local_outcount]);
    } else if (request_info[index] == REMPI_RECV_REQUEST) {
      int requested_source, requested_tag;
      MPI_Comm requested_comm;
      void *requested_buffer;

      rempi_reqmg_get_matching_id(&array_of_requests[index], &requested_source, &requested_tag, &requested_comm);
      rempi_reqmg_get_buffer(&array_of_requests[index], &requested_buffer);
      rempi_msgb_recv_msg(requested_buffer, replaying_test_event->get_rank(), requested_tag, requested_comm, 
			  replaying_test_event->get_msg_id(), replaying_test_event->get_matching_group_id(),
			  &array_of_statuses[local_outcount]);
      rempi_reqmg_deregister_request(&array_of_requests[index], REMPI_RECV_REQUEST);
      //      clmpi_sync_clock(replaying_test_event->get_clock()); 
      //      rempi_clock_sync_clock(replaying_test_event->get_clock(), REMPI_RECV_REQUEST);
      mc_encoder->update_local_look_ahead_send_clock(REMPI_ENCODER_REPLAYING_TYPE_ANY, REMPI_ENCODER_NO_MATCHING_SET_ID);
      pre_allocated_clocks[local_outcount] = replaying_test_event->get_clock();
    } else if (request_info[index] == REMPI_NULL_REQUEST) {
      REMPI_ERR("MPI_REQUEST_NULL does not match any message: %d (replaying rank: %d, replaying clock: %lu)", 
		request_info[index], replaying_test_event->get_rank(), replaying_test_event->get_clock());
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

  int nullcount = 0;
  for (int i = 0; i < incount; i++) { 
    if (request_info[i] == REMPI_NULL_REQUEST) nullcount++;
  }
  int matched_count = rempi_mpi_get_matched_count(incount, outcount, nullcount, matching_function_type);
  rempi_clock_sync_clock(matched_count, array_of_indices, pre_allocated_clocks, request_info, matching_function_type);
  return MPI_SUCCESS;
}


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

int rempi_recorder_cdc::replay_pf(int source, int tag, MPI_Comm comm, int *flag, MPI_Status *status, int comm_id)
{
  REMPI_ERR("Probe/Iprobe is not supported yet");
  return MPI_SUCCESS;
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


int rempi_recorder_cdc::progress_recv_requests(int recv_test_id,
					       int incount,
					       MPI_Request array_of_requests[])
{
  rempi_msgb_progress_recv();
  return 0;
}






int rempi_recorder_cdc::pre_process_collective(MPI_Comm comm)
{
#ifdef REMPI_DBG_REPLAY
  REMPI_DBGI(REMPI_DBG_REPLAY, "==== Record/Replay: Collective ======");
#endif
  if (rempi_mode == REMPI_ENV_REMPI_MODE_REPLAY) mc_encoder->set_fd_clock_state(1);
  rempi_clock_collective_sync_clock(comm);
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
