#include <stdlib.h>
#include <string.h>

#include <mpi.h>

#include "pnmpimod.h"
#include "rempi_re.h"
#include "rempi_err.h"
#include "rempi_mem.h"
#include "rempi_config.h"
#include "clmpi.h"
#include "rempi_recorder.h"

#define  PNMPI_MODULE_REMPI "rempi"

using namespace std;

rempi_re_cdc::rempi_re_cdc() {
  recorder = new rempi_recorder_cdc();
}

int rempi_re_cdc::init_clmpi()
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
  err=PNMPI_Service_GetServiceByName(handle_clmpi,"clmpi_get_local_clock","p",&serv);
  if (err!=PNMPI_SUCCESS)
    return err;
  clmpi_get_local_clock=(PNMPIMOD_get_local_clock_t) ((void*)serv.fct);

  /*Load own moduel*/
  err=PNMPI_Service_GetModuleByName(PNMPI_MODULE_REMPI, &handle_rempi);
  if (err!=PNMPI_SUCCESS)
    return err;

  return err;
}

int rempi_re_cdc::get_test_id()
{
  return 0;
  string test_id_string;
  test_id_string = rempi_btrace_string();
  //TODO: get the binary name
  //  size_t pos = test_id_string.find("MCBenchmark");
  //  test_id_string = test_id_string.substr(pos);
  if (test_ids_map.find(test_id_string) == test_ids_map.end()) {
    test_ids_map[test_id_string] = next_test_id_to_assign;
    next_test_id_to_assign++;
  }
  return test_ids_map[test_id_string];
}

int rempi_re_cdc::re_init(int *argc, char ***argv)
{
  int ret;
  /*Init CLMPI*/
  init_clmpi();

  ret = PMPI_Init(argc, argv);
  /*Init from configuration and for valiables for errors*/
  init_after_pmpi_init(argc, argv);

  if (rempi_mode == REMPI_ENV_REMPI_MODE_RECORD) {
    recorder->record_init(argc, argv, my_rank);
  } else {
    //TODO: Check if inputs are same as values in recording run
    //TODO: Check if this is same run as the last recording run
    recorder->replay_init(argc, argv, my_rank);
  }
  return ret;
}

int rempi_re_cdc::re_init_thread(
			     int *argc, char ***argv,
			     int required, int *provided)
{
  int ret;
  /*Init CLMPI*/
  init_clmpi();

  ret = PMPI_Init_thread(argc, argv, required, provided);

  /*Init from configuration and for valiables for errors*/
  init_after_pmpi_init(argc, argv);
  if (rempi_mode == REMPI_ENV_REMPI_MODE_RECORD) {
    recorder->record_init(argc, argv, my_rank);
  } else {
    //TODO: Check if inputs are same as values in recording run
    //TODO: Check if this is same run as the last recording run
    recorder->replay_init(argc, argv, my_rank);
  }
  return ret;
}

int rempi_re_cdc::re_isend(
		       void *buf,
		       int count,
		       MPI_Datatype datatype,
		       int dest,
		       int tag,
		       MPI_Comm comm,
		       MPI_Request *request)
{
  int ret;
  size_t clock;
  clmpi_get_local_clock(&clock);
  ret = PMPI_Isend(buf, count, datatype, dest, tag, comm, request);
#ifdef REMPI_DBG_REPLAY
  REMPI_DBGI(REMPI_DBG_REPLAY, "  Send: request: %p dest: %d, tag: %d, clock: %d, count: %d", *request, dest, tag, clock, count);
#endif

  return ret;
}


int rempi_re_cdc::re_irecv(
		 void *buf,
		 int count,
		 MPI_Datatype datatype,
		 int source,
		 int tag,
		 MPI_Comm comm,
		 MPI_Request *request)
{
  int ret;
  char comm_id[REMPI_COMM_ID_LENGTH];
  int comm_id_int;
  int resultlen;
  PMPI_Comm_get_name(MPI_COMM_WORLD, comm_id, &resultlen);



  if (rempi_mode == REMPI_ENV_REMPI_MODE_RECORD) {
    ret = PMPI_Irecv(buf, count, datatype, source, tag, comm, request);
    //TODO: Get Datatype,
    recorder->record_irecv(buf, count, (int)(datatype), source, tag, (int)comm_id[0], request);
  } else {
    /*TODO: Really need datatype ??*/
    recorder->replay_irecv(buf, count, (int)(datatype), source, tag, (int)comm_id[0], &comm, request);
  }
#if 0
  REMPI_DBGI(1, "source: %d, tag: %d, count: %d, request: %p", source, tag, count, *request);
#endif
  return ret;
}
  
int rempi_re_cdc::re_test(
		    MPI_Request *request, 
		    int *flag, 
		    MPI_Status *status)
{
  int ret;
  size_t clock;

#if 0
  REMPI_DBGI(1, "request: %p", *request);
#endif


  if (status == NULL) {
    /*TODO: allocate array_of_statuses in ReMPI instead of the error below*/
    REMPI_ERR("ReMPI requires status in MPI_Test");
  }

  if (rempi_mode == REMPI_ENV_REMPI_MODE_RECORD) {
    clmpi_register_recv_clocks(&clock, 1);
    ret = PMPI_Test(request, flag, status);
    /*If recoding mode, record the test function*/
    /*TODO: change froi void* to MPI_Request*/
    if(clock != PNMPI_MODULE_CLMPI_SEND_REQ_CLOCK) {
      // REMPI_DBGI(1, "Record  : (count: %d, with_next: %d, flag: %d, source: %d, tag: %d, clock: %d)",
      // 		 1, 0, *flag,
      // 		 status->MPI_SOURCE, status->MPI_TAG, clock);
      recorder->record_test(request, flag, status->MPI_SOURCE, status->MPI_TAG, clock, REMPI_MPI_EVENT_NOT_WITH_NEXT, get_test_id());
    }
  } else {
    recorder->replay_test(request, flag, status, get_test_id());
  }

  return ret;
}

int rempi_re_cdc::re_cancel(MPI_Request *request)
{
  int ret;
  if (rempi_mode == REMPI_ENV_REMPI_MODE_RECORD) {
    ret = PMPI_Cancel(request);
  } else {
    ret = recorder->replay_cancel(request);
  }
  return ret;
}

  
int rempi_re_cdc::re_testsome(
			  int incount, 
			  MPI_Request array_of_requests[],
			  int *outcount, 
			  int array_of_indices[], 
			  MPI_Status array_of_statuses[])
{
  int ret;
  size_t *clocks;
  int is_with_next = REMPI_MPI_EVENT_WITH_NEXT;
  
  if (array_of_statuses == NULL) {
    /*TODO: allocate array_of_statuses in ReMPI instead of the error below*/
    REMPI_ERR("ReMPI requires array_of_statues in MPI_Testsome");
  }

  if (rempi_mode == REMPI_ENV_REMPI_MODE_RECORD) {
    clocks = (size_t*)malloc(sizeof(size_t) * incount);
    clmpi_register_recv_clocks(clocks, incount);
#if 0
    for (int i = 0; i < incount; i++) {
      REMPI_DBGI(1, "request: %p (%d/%d)", array_of_requests[i], i, incount);
    }
#endif
    ret = PMPI_Testsome(incount, array_of_requests, outcount, array_of_indices, array_of_statuses);
    if (*outcount == 0) {
      int flag = 0;
      if(clocks[0] != PNMPI_MODULE_CLMPI_SEND_REQ_CLOCK) {
	recorder->record_test(NULL, &flag, -1, -1, -1, REMPI_MPI_EVENT_NOT_WITH_NEXT, get_test_id());
      } else {
#if 0	
	REMPI_DBGI(1, "skipp flag = 0, clock: %d (incount: %d)", clocks[0], incount);
#endif
      }
    } else {
      for (int i = 0; i < *outcount; i++) {
	int flag = 1;
	int index = array_of_indices[i];
	if(clocks[index] != PNMPI_MODULE_CLMPI_SEND_REQ_CLOCK) {
	  if (i == *outcount - 1) {
	    is_with_next = REMPI_MPI_EVENT_NOT_WITH_NEXT;
	  }

	  // REMPI_DBGI(1, "Record  : index: %d (incount: %d, outcount: %d, with_next: %d, flag: %d, source: %d, tag: %d, clock: %d)",
	  // 	     index, incount, *outcount, is_with_next, flag,
	  // 	     array_of_statuses[i].MPI_SOURCE, array_of_statuses[i].MPI_TAG, clocks[index]);
	  /*array_of_statuses only contain statuese for completed messages (i.e. length of array_of_indices == *outcount)
	    so I use [i] insted of [index]
	   */
	  recorder->record_test(&array_of_requests[index], &flag, array_of_statuses[i].MPI_SOURCE, array_of_statuses[i].MPI_TAG, clocks[index], is_with_next, get_test_id());
	}  else {
#if 0	
	  REMPI_DBGI(1, "skipp flag = 1, clock: %d (%d/%d)", clocks[i], i, *outcount);
#endif
	}
      }
    }
    free(clocks);
  } else {
    //    ret = PMPI_Testsome(incount, array_of_requests, outcount, array_of_indices, array_of_statuses);
// #ifdef REMPI_DBG_REPLAY
//     REMPI_DBGI(REMPI_DBG_REPLAY, "testsome call");
// #endif
    recorder->replay_testsome(incount, array_of_requests, outcount, array_of_indices, array_of_statuses, get_test_id());
// #ifdef REMPI_DBG_REPLAY
//     REMPI_DBGI(REMPI_DBG_REPLAY, "testsome end");
// #endif
  }

#if 0
  for (int i = 0; i < *outcount; i++) {
    int source;
    int index = array_of_indices[i];
    source =    array_of_statuses[index].MPI_SOURCE;
    // REMPI_DBG("out: (%lu |%d|) read index: %d", clocks[index], source, index);
    //    REMPI_DBG("clock %lu, source: %d (%d/%d); incount:%d", clocks[index], source, index, *outcount, incount);
    REMPI_DBG(" => Testsome: clock %lu |%d|", clocks[index], source);
    //    REMPI_DBG("source: %d (%d/%d); incount:%d", source, index, *outcount, incount);
  }
#endif


  //  REMPI_DBG(" ----------- ");

  return ret;
}


int rempi_re_cdc::re_finalize()
{
  int ret;
  ret = PMPI_Finalize();

  if (rempi_mode == REMPI_ENV_REMPI_MODE_RECORD) {
    ret = recorder->record_finalize();
  } else {
    ret = recorder->replay_finalize();
  }
  return ret;
}



