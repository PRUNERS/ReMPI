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


int rempi_re_no_comp::init_clmpi()
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

#ifdef REVERT1
int rempi_re_no_comp::get_test_id()
{
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
#else
int rempi_re_no_comp::get_test_id(MPI_Request *requests)
{
  REMPI_ERR("Not implemented yet");
  return 0 ;
}
#endif

int rempi_re_no_comp::re_init(int *argc, char ***argv)
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

int rempi_re_no_comp::re_init_thread(
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

int rempi_re_no_comp::re_isend(
		       void *buf,
		       int count,
		       MPI_Datatype datatype,
		       int dest,
		       int tag,
		       MPI_Comm comm,
		       MPI_Request *request)
{
  int ret;
  ret = PMPI_Isend(buf, count, datatype, dest, tag, comm, request);
  return ret;
}

int rempi_re_no_comp::re_irecv(
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
    recorder->record_irecv(buf, count, datatype, source, tag, (int)comm_id[0], request);
  } else {
    /*TODO: Really need datatype ??*/
    recorder->replay_irecv(buf, count, datatype, source, tag, (int)comm_id[0], &comm, request);
  }
  return ret;
}

int rempi_re_no_comp::re_cancel(MPI_Request *request)
{
  int ret;
  ret = PMPI_Cancel(request);
  return ret;
}

  
int rempi_re_no_comp::re_test(
		    MPI_Request *request, 
		    int *flag, 
		    MPI_Status *status)
{
  int ret;
  size_t clock;

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
      //      recorder->record_test(request, flag, status->MPI_SOURCE, status->MPI_TAG, clock, REMPI_MPI_EVENT_NOT_WITH_PREVIOUS, get_test_id());
      recorder->record_test(request, flag, status->MPI_SOURCE, status->MPI_TAG, clock, REMPI_MPI_EVENT_NOT_WITH_NEXT, -1);
    }
  } else {
    int recorded_source, recorded_tag, recorded_flag;
    if(clock != PNMPI_MODULE_CLMPI_SEND_REQ_CLOCK) {
      recorder->replay_test(request, flag, status, -1);
      /*Set next recorded and matched source, and tag*/
      status->MPI_SOURCE = recorded_source;
      status->MPI_TAG    = recorded_tag;
      *flag              = recorded_flag;
    }
  }

  return ret;
}
  
int rempi_re_no_comp::re_testsome(
			  int incount, 
			  MPI_Request array_of_requests[],
			  int *outcount, 
			  int array_of_indices[], 
			  MPI_Status array_of_statuses[])
{
  int ret;
  size_t *clocks;
  int is_with_next = REMPI_MPI_EVENT_NOT_WITH_NEXT;
  
  
  if (array_of_statuses == NULL) {
    /*TODO: allocate array_of_statuses in ReMPI instead of the error below*/
    REMPI_ERR("ReMPI requires array_of_statues in MPI_Testsome");
  }
  clocks = (size_t*)malloc(sizeof(size_t) * incount);
  clmpi_register_recv_clocks(clocks, incount);


  if (rempi_mode == REMPI_ENV_REMPI_MODE_RECORD) {

    ret = PMPI_Testsome(incount, array_of_requests, outcount, array_of_indices, array_of_statuses);
    if (*outcount == 0) {
      int flag = 0;
      //      recorder->record_test(NULL, &flag, -1, -1, -1, REMPI_MPI_EVENT_NOT_WITH_PREVIOUS, get_test_id());
      recorder->record_test(NULL, &flag, -1, -1, -1, REMPI_MPI_EVENT_NOT_WITH_NEXT, -1);
    }

    is_with_next = REMPI_MPI_EVENT_WITH_NEXT;
    for (int i = 0; i < *outcount; i++) {
      int flag = 1;
      int index = array_of_indices[i];
      if(clocks[index] != PNMPI_MODULE_CLMPI_SEND_REQ_CLOCK) {
	//	recorder->record_test(&array_of_requests[index], &flag, array_of_statuses[index].MPI_SOURCE, array_of_statuses[index].MPI_TAG, clocks[index], is_with_next, get_test_id());
	if (i == *outcount - 1) {
	  is_with_next = REMPI_MPI_EVENT_NOT_WITH_NEXT;
	}
	recorder->record_test(&array_of_requests[index], &flag, array_of_statuses[index].MPI_SOURCE, array_of_statuses[index].MPI_TAG, clocks[index], is_with_next, -1);
      }
    }
  } else {
    REMPI_DBG("Not implemented yet");
    // TODO: replay
    // rempi_replay_test(buf, count, (int)(&datatype), source, tag, 0, (void*)request);
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
  free(clocks);
  return ret;
}


int rempi_re_no_comp::re_waitall(
			  int incount, 
			  MPI_Request array_of_requests[],
			  MPI_Status array_of_statuses[])
{
  REMPI_ERR("not implemented yet");
  return 0;
}


int rempi_re_no_comp::re_comm_split(MPI_Comm arg_0, int arg_1, int arg_2, MPI_Comm *arg_3)
{
  int ret;
  ret = PMPI_Comm_split(arg_0, arg_1, arg_2, arg_3);
  return ret;
}

int rempi_re_no_comp::re_comm_create(MPI_Comm arg_0, MPI_Group arg_1, MPI_Comm *arg_2)
{
  int ret;
  ret = PMPI_Comm_create(arg_0, arg_1, arg_2);
  return ret;
}




int rempi_re_no_comp::re_finalize()
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



