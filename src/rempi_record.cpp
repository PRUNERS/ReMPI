#include <stdio.h>

#include "mpi.h"

//#include <iostream>
#include "rempi_record.h"
#include "rempi_message_manager.h"
#include "rempi_event.h"
#include "rempi_event_list.h"
#include "rempi_io_thread.h"
#include "rempi_util.h"
#include "rempi_err.h"
#include "rempi_config.h"


int rempi_record::record_init(int *argc, char ***argv, int rank) 
{
  string id;

  //fprintf(stderr, "ReMPI: Function call (%s:%s:%d)\n", __FILE__, __func__, __LINE__);

  id = std::to_string((long long int)rank);
  recording_event_list = new rempi_event_list<rempi_event*>(100000, 100);
  record_thread = new rempi_io_thread(recording_event_list, id, rempi_mode); //0: recording mode
  record_thread->start();
  
  return 0;
}

int rempi_record::replay_init(int *argc, char ***argv, int rank) 
{
  string id;

  id = std::to_string((long long int)rank);
  replaying_event_list = new rempi_event_list<rempi_event*>(100000, 100);
  read_record_thread = new rempi_io_thread(replaying_event_list, id, rempi_mode); //1: replay mode
  read_record_thread->start();

  return 0;
}

int rempi_record::record_irecv(
   void *buf,
   int count,
   int datatype,
   int source,
   int tag,
   int comm, // The value is set by MPI_Comm_set_name in ReMPI_convertor
   MPI_Request *request)
{
  msg_manager.add_pending_recv(request, source, tag, comm);

  return 0;
}

int rempi_record::replay_irecv(
   void *buf,
   int count,
   int datatype,
   int source,
   int tag,
   int comm_id, // The value is set by MPI_Comm_set_name in ReMPI_convertor
   MPI_Request *request)
{
  msg_manager.add_pending_recv(request, source, tag, comm_id);
  ///msg_manager.print_pending_recv();
  return 0;
}


int rempi_record::record_test(
    MPI_Request *request,
    int *flag,
    int source,
    int tag,
    int clock)
{
  int event_count = 1;
  int is_testsome = 0;
  int request_val = -1;
  int comm_id = 0;

  /*Query befoer add_matched_recv, because pendding entry is removed when flag == 1*/
  comm_id = msg_manager.query_pending_comm_id(request);

  if (*flag) {
    msg_manager.add_matched_recv(request, source, tag);
  }  
  //TODO: we need to record *request ?
  recording_event_list->push(new rempi_test_event(event_count, is_testsome, comm_id, *flag, source, tag));

  return 0;
}


/*This function is called after MPI_Test*/

int rempi_record::replay_test(
    MPI_Request *request_in,
    int flag_in,
    int source_in,
    int tag_in,
    int *flag_out,
    int *source_out,
    int *tag_out)
{
  rempi_event *replaying_test_event;

  /**/
  if (flag_in) {
    msg_manager.add_matched_recv(request_in, source_in, tag_in);
  }

  /*1. Get request (recoeded in irecv) for this "replaying_test_event"*/
  replaying_test_event = replaying_event_list->decode_pop();
  if (replaying_test_event == NULL) {
    REMPI_ERR("No more replay event");
  } 

  /*If the event is flag == 0, simply retunr flag == 0*/
  if (!replaying_test_event->get_flag()) {
    *flag_out = 0;
    return 0;
  }
  /* So "replayint_test_event" event flag == 1*/
  *flag_out = 1;
  /*
    2. Wait until this recorded message really arrives
       if (0 if next recorded maching is not mached in this run) {
          TODO: Wait until maching, and get clock
	  TODO: if matched, memorize that the matching for the next replay test
       }
  */
  while (!msg_manager.is_matched_recv(
	      replaying_test_event->get_source(), 
	      replaying_test_event->get_tag(),
	      replaying_test_event->get_comm_id())) {
    msg_manager.refresh_matched_recv();
  }
  msg_manager.remove_matched_recv(
	      replaying_test_event->get_source(), 
	      replaying_test_event->get_tag(),
	      replaying_test_event->get_comm_id());
  /*
    3. Set valiabiles (source, flag, tag)
  }
   */
  *flag_out = 1;
  *source_out = replaying_test_event->get_source();
  *tag_out = replaying_test_event->get_tag();

  return 0;
}

int rempi_record::record_testsome(
    int incount,
    void *array_of_requests[],
    int *outcount,
    int array_of_indices[],
    void *array_of_statuses[])
{
  int event_count = 1;
  int is_testsome = 1;
  int request     = 0;
  int flag        = 0;
  int source      = -1;
  int tag         = -1;


  if (*outcount == 0) {
    recording_event_list->push(new rempi_test_event(event_count, is_testsome, request, flag, source, tag));
    return 0;
  }

  flag = 1;
  for (int i = 0; i < *outcount; i++) {
    source = array_of_indices[i];
    //TODO:  tag,
    tag    = 0;
    
    recording_event_list->push(new rempi_test_event(event_count, is_testsome, request, flag, source, tag));
  }

  //  fprintf(stderr, "ReMPI: Function call (%s:%s:%d)\n", __FILE__, __func__, __LINE__);
  return 0;
}

int rempi_record::replay_testsome(
    int incount,
    void *array_of_requests[],
    int *outcount,
    int array_of_indices[],
    void *array_of_statuses[])
{
  return 0;
 }

int rempi_record::record_finalize(void)
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

  record_thread->complete_flush();
  record_thread->join();

  //  fprintf(stderr, "ReMPI: Function call (%s:%s:%d)\n", __FILE__, __func__, __LINE__);
  return 0;
}

int rempi_record::replay_finalize(void)
{
  //TODO:
  //  fprintf(stderr, "ReMPI: Function call (%s:%s:%d)\n", __FILE__, __func__, __LINE__);
  return 0;
}

