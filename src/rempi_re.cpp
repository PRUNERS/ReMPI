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
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>

#include <mpi.h>

#include "rempi_re.h"
#include "rempi_err.h"
#include "rempi_mem.h"
#include "rempi_mpi_init.h"
#include "rempi_config.h"
#include "rempi_type.h"
#include "rempi_status.h"
#include "rempi_util.h"
#include "rempi_recorder.h"
#include "rempi_request_mg.h"
#include "rempi_sig_handler.h"

#include "llvm/reomp_rr.h"

using namespace std;

#define CHECK_MPI_STATUS(count, statuses)	\
do { \
  if (statuses == NULL) {						\
    /*TODO: allocate array_of_statuses in ReMPI instead of the error below*/ \
    statuses = (MPI_Status*)malloc(count * sizeof(MPI_Status));	\
    REMPI_DBG("ReMPI requires status in MPI_Test");	       \
  } \
} while(0) \


int send_to0 = 0;


int rempi_re::init_after_pmpi_init(int *argc, char ***argv)
{
  signal(SIGSEGV, SIG_DFL);

  //  char comm_id[REMPI_COMM_ID_LENGTH];
  //  comm_id[0] = 0;
  PMPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
  //  PMPI_Comm_set_name(MPI_COMM_WORLD, comm_id);
  rempi_mpi_comm_get_id(MPI_COMM_WORLD);

  rempi_err_init(my_rank);

  reomp_control(REOMP_AFT_MPI_INIT, NULL, 0);
  //  rempi_set_configuration(argc, argv);

  return 0;
}

int rempi_re::re_init(int *argc, char ***argv, int fortran_init)
{
  int ret;
  ret = rempi_mpi_init(argc, argv, fortran_init);


  /*Init from configuration and for valiables for errors*/
  init_after_pmpi_init(argc, argv);
  if (rempi_mode == REMPI_ENV_REMPI_MODE_RECORD) {
    recorder->record_init(argc, argv, my_rank);
  } else {
    //TODO: Check if inputs are same as values in recording run
    //TODO: Check if this is same run as the last recording run
    recorder->replay_init(argc, argv, my_rank);
  }
  return MPI_SUCCESS;
}

int rempi_re::re_init_thread(
			     int *argc, char ***argv,
			     int required, int *provided, int fortran_init_thread)
{
  int ret;
  int rempi_required = MPI_THREAD_FUNNELED;
  int rempi_provided = MPI_THREAD_FUNNELED;
  int all_required;
  if (required > rempi_required) {
    all_required = required;
  } else {
    all_required = rempi_required;
  }
  ret = rempi_mpi_init_thread(argc, argv, all_required, provided, fortran_init_thread);
  if (*provided < rempi_required) {
    REMPI_ERR("ReMPI does not work for MPI_THREAD_SERIALIZED nor MPI_THREAD_MULTIPLE");
  } else {
    *provided = rempi_provided;
  }

  /*Init from configuration and for valiables for errors*/
  init_after_pmpi_init(argc, argv);
  if (rempi_mode == REMPI_ENV_REMPI_MODE_RECORD) {
    recorder->record_init(argc, argv, my_rank);
  } else {
    //TODO: Check if inputs are same as values in recording run
    //TODO: Check if this is same run as the last recording run
    recorder->replay_init(argc, argv, my_rank);
  }
  return MPI_SUCCESS;
}


int rempi_re::re_recv_init(void *arg_0, int arg_1, MPI_Datatype arg_2, int arg_3, int arg_4, MPI_Comm arg_5, MPI_Request *arg_6)
{
  if (rempi_mode == REMPI_ENV_REMPI_MODE_RECORD) {
    recorder->record_recv_init(arg_0, arg_1, arg_2, arg_3, arg_4, arg_5, arg_6);
  } else {
    recorder->replay_recv_init(arg_0, arg_1, arg_2, arg_3, arg_4, arg_5, arg_6);
  }
  return MPI_SUCCESS;
}

int rempi_re::re_send_init(mpi_const void *arg_0, int arg_1, MPI_Datatype arg_2, int arg_3, int arg_4, MPI_Comm arg_5, MPI_Request *arg_6, int send_function_type)
{
  if (rempi_mode == REMPI_ENV_REMPI_MODE_RECORD) {
    recorder->record_send_init(arg_0, arg_1, arg_2, arg_3, arg_4, arg_5, arg_6, send_function_type);
  } else {
    recorder->replay_send_init(arg_0, arg_1, arg_2, arg_3, arg_4, arg_5, arg_6, send_function_type);
  }
  return MPI_SUCCESS;
}

int rempi_re::re_start(MPI_Request *arg_0)
{
  if (rempi_mode == REMPI_ENV_REMPI_MODE_RECORD) {
    recorder->record_start(arg_0);
  } else {
    recorder->replay_start(arg_0);
  }
  return MPI_SUCCESS;
}

int rempi_re::re_startall(int arg_0, MPI_Request *arg_1)
{
  if (rempi_mode == REMPI_ENV_REMPI_MODE_RECORD) {
    recorder->record_startall(arg_0, arg_1);
  } else {
    recorder->replay_startall(arg_0, arg_1);
  }
  return MPI_SUCCESS;
  return MPI_SUCCESS;
}



int rempi_re::re_isend(
		       mpi_const void *buf,
		       int count,
		       MPI_Datatype datatype,
		       int dest,
		       int tag,
		       MPI_Comm comm,
		       MPI_Request *request,
		       int send_function_type)
{
  int ret;

  // if (comm != MPI_COMM_WORLD) {
  //   REMPI_ERR("Current ReMPI accept only MPI_COMM_WORLD");
  // }

  

  if (rempi_mode == REMPI_ENV_REMPI_MODE_RECORD) {
    recorder->record_isend(buf, count, datatype, dest, tag, comm, request, send_function_type);
  } else {
    recorder->replay_isend(buf, count, datatype, dest, tag, comm, request, send_function_type);
  }

  return MPI_SUCCESS;
}


int rempi_re::re_irecv(
		 void *buf,
		 int count,
		 MPI_Datatype datatype,
		 int source,
		 int tag,
		 MPI_Comm comm,
		 MPI_Request *request)
{
  int ret;
  //  char comm_id[REMPI_COMM_ID_LENGTH];

  // if (comm != MPI_COMM_WORLD) {
  //   REMPI_ERR("Current ReMPI does not multiple communicators");
  // }

  if (rempi_mode == REMPI_ENV_REMPI_MODE_RECORD) {
    // ret = PMPI_Irecv(buf, count, datatype, source, tag, comm, request);
    // rempi_reqmg_register_recv_request(request, source, tag, (int)comm_id[0]);
    ret = recorder->record_irecv(buf, count, datatype, source, tag, 0, comm, request);
  } else {
    /*TODO: Really need datatype ??*/
    ret = recorder->replay_irecv(buf, count, datatype, source, tag, 0, comm, request);
  }

  return MPI_SUCCESS;
}


static int print_outputs(int outcount, int* indices, MPI_Status *array_of_statuses)
{
#if 0
  int count;
  int index;
  if (outcount == 0 ) {
    REMPI_DBG("REVAL: unmatched");
  }

  for (int i = 0; i < outcount; i++) {
    PMPI_Get_count(&array_of_statuses[i], MPI_BYTE , &count);
    index = (indices == NULL)?0:indices[i];
    REMPI_DBG("REVAL: index: %d, source: %d tag: %d count: %d", 
	      index, array_of_statuses[i].MPI_SOURCE, array_of_statuses[i].MPI_TAG, count);
  }
#endif
  return 0;
}
  
int rempi_re::re_test(
		    MPI_Request *request, 
		    int *flag, 
		    MPI_Status *status)
{
  int ret;
  int status_flag;

  status = rempi_status_allocate(1, status, &status_flag);

  *flag = 0;
  if (rempi_mode == REMPI_ENV_REMPI_MODE_RECORD) {
    recorder->record_mf(1, request, flag, NULL, status, REMPI_MPI_TEST);
  } else {
    recorder->replay_mf(1, request, flag, NULL, status, REMPI_MPI_TEST);
  }

  print_outputs(1, NULL, status);
  if (status_flag) rempi_status_free(status);

  return MPI_SUCCESS;
}

int rempi_re::re_testany(int count, MPI_Request array_of_requests[],
		       int *index, int *flag, MPI_Status *status)
{ 
  int status_flag;

  status = rempi_status_allocate(count, status, &status_flag);

  if (rempi_mode == REMPI_ENV_REMPI_MODE_RECORD) {
    recorder->record_mf(count, array_of_requests, flag, index, status, REMPI_MPI_TESTANY);
  } else {
    recorder->replay_mf(count, array_of_requests, flag, index, status, REMPI_MPI_TESTANY);
  }

  print_outputs(1, index, status);
  if (status_flag) rempi_status_free(status);

  return MPI_SUCCESS;
}



int rempi_re::re_testsome(
			  int incount, 
			  MPI_Request array_of_requests[],
			  int *outcount, 
			  int array_of_indices[], 
			  MPI_Status array_of_statuses[])
{
  int ret;
  int status_flag;  
  
  array_of_statuses = rempi_status_allocate(incount, array_of_statuses, &status_flag);

  if (rempi_mode == REMPI_ENV_REMPI_MODE_RECORD) {
    recorder->record_mf(incount, array_of_requests, outcount, array_of_indices, array_of_statuses, REMPI_MPI_TESTSOME);
  } else {
    recorder->replay_mf(incount, array_of_requests, outcount, array_of_indices, array_of_statuses, REMPI_MPI_TESTSOME);
  }

  print_outputs(*outcount, array_of_indices, array_of_statuses);
  if (status_flag) rempi_status_free(array_of_statuses);

  return MPI_SUCCESS;
}


int rempi_re::re_testall(int count, MPI_Request array_of_requests[],
		       int *flag, MPI_Status array_of_statuses[])
{
  int ret;
  int status_flag;

  array_of_statuses = rempi_status_allocate(count, array_of_statuses, &status_flag);

  if (rempi_mode == REMPI_ENV_REMPI_MODE_RECORD) {
    recorder->record_mf(count, array_of_requests, flag, NULL, array_of_statuses, REMPI_MPI_TESTALL);
  } else {
    recorder->replay_mf(count, array_of_requests, flag, NULL, array_of_statuses, REMPI_MPI_TESTALL);
    *flag = (*flag > 0)? 1:0;
  }

  if (*flag) {
    print_outputs(count, NULL, array_of_statuses);
  } else {
    print_outputs(0, NULL, array_of_statuses);
  }
  if (status_flag) rempi_status_free(array_of_statuses);

  return MPI_SUCCESS;
}

int rempi_re::re_wait(
		    MPI_Request *request,
		    MPI_Status *status)
{
  int ret;
  int status_flag;

  status = rempi_status_allocate(1, status, &status_flag);
  
  if (rempi_mode == REMPI_ENV_REMPI_MODE_RECORD) {
    recorder->record_mf(1, request, NULL, NULL, status, REMPI_MPI_WAIT);
  } else {
    recorder->replay_mf(1, request, NULL, NULL, status, REMPI_MPI_WAIT);
  }

  print_outputs(1, NULL, status);
  if (status_flag) rempi_status_free(status);
  return MPI_SUCCESS;
}
 
  
int rempi_re::re_waitany(
		       int count, MPI_Request array_of_requests[],
		       int *index, MPI_Status *status)
{
  int ret;
  int status_flag;

  status = rempi_status_allocate(count, status, &status_flag);
  
  if (rempi_mode == REMPI_ENV_REMPI_MODE_RECORD) {
    recorder->record_mf(count, array_of_requests, NULL, index, status, REMPI_MPI_WAITANY);
  } else {
    recorder->replay_mf(count, array_of_requests, NULL, index, status, REMPI_MPI_WAITANY);
  }

  print_outputs(1, index, status);
  if (status_flag) rempi_status_free(status);
  return MPI_SUCCESS;
}
  
int rempi_re::re_waitsome(int incount, MPI_Request array_of_requests[],
			  int *outcount, int array_of_indices[],
			  MPI_Status array_of_statuses[])
{
  int ret;
  int status_flag;
  
  array_of_statuses = rempi_status_allocate(incount, array_of_statuses, &status_flag);

  if (rempi_mode == REMPI_ENV_REMPI_MODE_RECORD) {
    recorder->record_mf(incount, array_of_requests, outcount, array_of_indices, array_of_statuses, REMPI_MPI_WAITSOME);
  } else {
    recorder->replay_mf(incount, array_of_requests, outcount, array_of_indices, array_of_statuses, REMPI_MPI_WAITSOME);
  }

  print_outputs(*outcount, array_of_indices, array_of_statuses);
  if (status_flag) rempi_status_free(array_of_statuses);
  return MPI_SUCCESS;
}
  
int rempi_re::re_waitall(
			  int incount, 
			  MPI_Request array_of_requests[],
			  MPI_Status array_of_statuses[])
{
  int ret;
  int status_flag;

  array_of_statuses = rempi_status_allocate(incount, array_of_statuses, &status_flag);

  if (rempi_mode == REMPI_ENV_REMPI_MODE_RECORD) {
    recorder->record_mf(incount, array_of_requests, NULL, NULL, array_of_statuses, REMPI_MPI_WAITALL);
  } else {
    recorder->replay_mf(incount, array_of_requests, NULL, NULL, array_of_statuses, REMPI_MPI_WAITALL);
  }

  print_outputs(incount, NULL, array_of_statuses);

  if (status_flag) rempi_status_free(array_of_statuses);
  return MPI_SUCCESS;
}


int rempi_re::re_probe(int source, int tag, MPI_Comm comm, MPI_Status *status)
{
  int ret;
  int status_flag;

  //  PMPI_Comm_get_name(MPI_COMM_WORLD, comm_id, &resultlen);

  status = rempi_status_allocate(1, status, &status_flag);  

  if (rempi_mode == REMPI_ENV_REMPI_MODE_RECORD) {
    ret = recorder->record_pf(source, tag, comm, NULL, status, REMPI_MPI_PROBE);
  } else {
    ret = recorder->replay_pf(source, tag, comm, NULL, status, REMPI_MPI_PROBE);
  }
  if (status_flag) rempi_status_free(status);
  return MPI_SUCCESS;
}

int rempi_re::re_iprobe(int source, int tag, MPI_Comm comm, int *flag, MPI_Status *status)
{
  int ret;
  //  char comm_id[REMPI_COMM_ID_LENGTH];
  int status_flag;

  status = rempi_status_allocate(1, status, &status_flag);

  if (rempi_mode == REMPI_ENV_REMPI_MODE_RECORD) {
    ret = recorder->record_pf(source, tag, comm, flag, status, REMPI_MPI_IPROBE);
  } else {
    ret = recorder->replay_pf(source, tag, comm, flag, status, REMPI_MPI_IPROBE);
  }
  if (status_flag) rempi_status_free(status);

  return MPI_SUCCESS;
}


int rempi_re::re_cancel(MPI_Request *request)
{
  int ret;
  int requested_source, requested_tag;
  MPI_Comm requested_comm;
  
  // rempi_reqmg_get_matching_id(request, &requested_source, &requested_tag, &requested_comm);
  // REMPI_DBG("cancel: %d", requested_tag);

  if (rempi_mode == REMPI_ENV_REMPI_MODE_RECORD) {
    ret = recorder->record_cancel(request);
  } else {
    ret = recorder->replay_cancel(request);
  }
#ifdef REMPI_DBG_REPLAY
  REMPI_DBGI(REMPI_DBG_REPLAY, "Record/Replay canceled");
#endif
  return MPI_SUCCESS;
}

int rempi_re::re_request_free(MPI_Request *request)
{
  int ret;
  if (rempi_mode == REMPI_ENV_REMPI_MODE_RECORD) {
    int request_type;
    rempi_reqmg_get_request_type(request, &request_type);
    if (request_type == REMPI_RECV_REQUEST ||
	request_type == REMPI_SEND_REQUEST) {
      rempi_reqmg_deregister_request(request, request_type);
    }
    ret = PMPI_Request_free(request);
  } else {
    ret = recorder->replay_request_free(request);
  }
  return MPI_SUCCESS;
}




#if PMPI_Request_c2f != MPI_Fint && MPI_Request_c2f != MPI_Fint
extern "C" MPI_Fint PMPI_Request_c2f(MPI_Request request);
MPI_Fint rempi_re::re_request_c2f(MPI_Request request)
{
  MPI_Fint ret;

  if (rempi_mode == REMPI_ENV_REMPI_MODE_RECORD) {
    int request_type;
    rempi_reqmg_get_request_type(&request, &request_type);
    if (request_type == REMPI_RECV_REQUEST ||
	request_type == REMPI_SEND_REQUEST) {
      rempi_reqmg_deregister_request(&request, request_type);
    }
    ret = PMPI_Request_c2f(request);
  } else {
    ret = recorder->replay_request_c2f(request);
  }
  return ret;
}
#else 
MPI_Fint rempi_re::re_request_c2f(MPI_Request request)
{
  MPI_Fint ret;
  REMPI_ERR("%s should not be called", __func__);
  return ret;
}

#endif



int rempi_re::re_comm_split(MPI_Comm arg_0, int arg_1, int arg_2, MPI_Comm *arg_3)
{
  int ret;
  recorder->pre_process_collective(arg_0);
  ret = PMPI_Comm_split(arg_0, arg_1, arg_2, arg_3);
  recorder->post_process_collective();
  return ret;
}
  
int rempi_re::re_comm_create(MPI_Comm arg_0, MPI_Group arg_1, MPI_Comm *arg_2)
{
  int ret;
  recorder->pre_process_collective(arg_0);
  ret = PMPI_Comm_create(arg_0, arg_1, arg_2);
  recorder->post_process_collective();
  return ret;
}

int rempi_re::re_comm_dup(MPI_Comm arg_0, MPI_Comm *arg_2)
{
  int ret;   
  recorder->pre_process_collective(arg_0);
  ret = PMPI_Comm_dup(arg_0, arg_2);
  recorder->post_process_collective();
  return ret;
}




int rempi_re::re_allreduce(mpi_const void *arg_0, void *arg_1, int arg_2, MPI_Datatype arg_3, MPI_Op arg_4, MPI_Comm arg_5)
{
  int ret;
  //  REMPI_DBG(" ==== allreduce ===");
  recorder->pre_process_collective(arg_5);
  ret = PMPI_Allreduce(arg_0, arg_1, arg_2, arg_3, arg_4, arg_5);
  recorder->post_process_collective();
  return ret;
}

int rempi_re::re_reduce(mpi_const void *arg_0, void *arg_1, int arg_2, MPI_Datatype arg_3, MPI_Op arg_4, int arg_5, MPI_Comm arg_6)
{
  int ret;
  recorder->pre_process_collective(arg_6);
  ret = PMPI_Reduce(arg_0, arg_1, arg_2, arg_3, arg_4, arg_5, arg_6);
  recorder->post_process_collective();
  return ret;
}

int rempi_re::re_scan(mpi_const void *arg_0, void *arg_1, int arg_2, MPI_Datatype arg_3, MPI_Op arg_4, MPI_Comm arg_5)
{
  int ret;
  recorder->pre_process_collective(arg_5);
  ret = PMPI_Scan(arg_0, arg_1, arg_2, arg_3, arg_4, arg_5);
  recorder->post_process_collective();
  return ret; 
}

int rempi_re::re_allgather(mpi_const void *arg_0, int arg_1, MPI_Datatype arg_2, void *arg_3, int arg_4, MPI_Datatype arg_5, MPI_Comm arg_6)
{
  int ret;
  recorder->pre_process_collective(arg_6);
  ret = PMPI_Allgather(arg_0, arg_1, arg_2, arg_3, arg_4, arg_5, arg_6);
  recorder->post_process_collective();
  return ret;
}

int rempi_re::re_gatherv(mpi_const void *arg_0, int arg_1, MPI_Datatype arg_2, void *arg_3, mpi_const int *arg_4, mpi_const int *arg_5, MPI_Datatype arg_6, int arg_7, MPI_Comm arg_8)
{
  int ret;
  recorder->pre_process_collective(arg_8);
  ret = PMPI_Gatherv(arg_0, arg_1, arg_2, arg_3, arg_4, arg_5, arg_6, arg_7, arg_8);
  recorder->post_process_collective();
  return ret;
}

int rempi_re::re_reduce_scatter(mpi_const void *arg_0, void *arg_1, mpi_const int *arg_2, MPI_Datatype arg_3, MPI_Op arg_4, MPI_Comm arg_5)
{
  int ret;
  recorder->pre_process_collective(arg_5);
  ret = PMPI_Reduce_scatter(arg_0, arg_1, arg_2, arg_3, arg_4, arg_5);
  recorder->post_process_collective();
  return ret; 
}

int rempi_re::re_scatterv(mpi_const void *arg_0, mpi_const int *arg_1, mpi_const int *arg_2, MPI_Datatype arg_3, void *arg_4, int arg_5, MPI_Datatype arg_6, int arg_7, MPI_Comm arg_8)
{
  int ret;
  recorder->pre_process_collective(arg_8);
  ret = PMPI_Scatterv(arg_0, arg_1, arg_2, arg_3, arg_4, arg_5, arg_6, arg_7, arg_8);
  recorder->post_process_collective();
  return ret;
}

int rempi_re::re_allgatherv(mpi_const void *arg_0, int arg_1, MPI_Datatype arg_2, void *arg_3, mpi_const int *arg_4, mpi_const int *arg_5, MPI_Datatype arg_6, MPI_Comm arg_7)
{
  int ret;
  recorder->pre_process_collective(arg_7);
  ret = PMPI_Allgatherv(arg_0, arg_1, arg_2, arg_3, arg_4, arg_5, arg_6, arg_7);
  recorder->post_process_collective();
  return ret; 
}

int rempi_re::re_scatter(mpi_const void *arg_0, int arg_1, MPI_Datatype arg_2, void *arg_3, int arg_4, MPI_Datatype arg_5, int arg_6, MPI_Comm arg_7)
{
  int ret;
  recorder->pre_process_collective(arg_7);
  ret = PMPI_Scatter(arg_0, arg_1, arg_2, arg_3, arg_4, arg_5, arg_6, arg_7);
  recorder->post_process_collective();
  return ret;
}

int rempi_re::re_bcast(void *arg_0, int arg_1, MPI_Datatype arg_2, int arg_3, MPI_Comm arg_4)
{
  int ret;
  recorder->pre_process_collective(arg_4);
  ret = PMPI_Bcast(arg_0, arg_1, arg_2, arg_3, arg_4);
  recorder->post_process_collective();
  return ret; 
}

int rempi_re::re_alltoall(mpi_const void *arg_0, int arg_1, MPI_Datatype arg_2, void *arg_3, int arg_4, MPI_Datatype arg_5, MPI_Comm arg_6)
{
  int ret;
  recorder->pre_process_collective(arg_6);
  ret = PMPI_Alltoall(arg_0, arg_1, arg_2, arg_3, arg_4, arg_5, arg_6);
  recorder->post_process_collective();
  return ret;
}

int rempi_re::re_gather(mpi_const void *arg_0, int arg_1, MPI_Datatype arg_2, void *arg_3, int arg_4, MPI_Datatype arg_5, int arg_6, MPI_Comm arg_7)
{
  int ret;
  recorder->pre_process_collective(arg_7);
  ret = PMPI_Gather(arg_0, arg_1, arg_2, arg_3, arg_4, arg_5, arg_6, arg_7);
  recorder->post_process_collective();
  return ret;
}

int rempi_re::re_barrier(MPI_Comm arg_0)
{
  int ret;
  recorder->pre_process_collective(arg_0);
  ret = PMPI_Barrier(arg_0);
  recorder->post_process_collective();
  return ret;  
}

int rempi_re::re_finalize()
{
  int ret;


  if (rempi_mode == REMPI_ENV_REMPI_MODE_RECORD) {
    ret = recorder->record_finalize();
  } else {
    ret = recorder->replay_finalize();
  }
  return ret;
}



