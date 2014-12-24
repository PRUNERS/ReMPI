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

//extern int rempi_mode;

rempi_message_manager msg_manager;
rempi_event_list<rempi_event*> *recording_event_list, *replaying_event_list;
rempi_io_thread *record_thread, *read_record_thread;
//using namespace std;

int rempi_record_init(int *argc, char ***argv, int rank) 
{
  string id;

  //fprintf(stderr, "ReMPI: Function call (%s:%s:%d)\n", __FILE__, __func__, __LINE__);

  id = std::to_string((long long int)rank);
  recording_event_list = new rempi_event_list<rempi_event*>(100000, 100);
  record_thread = new rempi_io_thread(recording_event_list, id, rempi_mode); //0: recording mode
  record_thread->start();
  
  return 0;
}

int rempi_replay_init(int *argc, char ***argv, int rank) 
{
  string id;

  id = std::to_string((long long int)rank);
  replaying_event_list = new rempi_event_list<rempi_event*>(100000, 100);
  read_record_thread = new rempi_io_thread(replaying_event_list, id, rempi_mode); //1: replay mode
  read_record_thread->start();

  return 0;
}

int rempi_record_irecv(
   void *buf,
   int count,
   int datatype,
   int source,
   int tag,
   int comm, // The value is set by MPI_Comm_set_name in ReMPI_convertor
   void *request)
{
  //  event_list->add_event((int64_t)buf, count, datatype, source, tag, comm, (int64_t)request);
  //TODO: we need to record *request?
  //  event_list->push(new rempi_irecv_event(1, count, source, tag, comm, -1));
  // fprintf(stderr, "ReMPI: =>RECORD(IREC): buf:%p count:%d, datatype:%d, source:%d, tag:%d, comm:%d, request:%p\n", 
  //  	  buf, count, datatype, source, tag, comm, request);
  //  event_list.get_event((int64_t)request, source, tag, comm);
  //  fprintf(stderr, "ReMPI: Record: buf:%p count:%d, datatype:%d, source:%d, tag:%d, comm:%d, request:%p Function call (%s:%s:%d)\n", __FILE__, __func__, __LINE__);
  return 0;
}
int rempi_replay_irecv(
   void *buf,
   int count,
   int datatype,
   int source,
   int tag,
   int comm_id, // The value is set by MPI_Comm_set_name in ReMPI_convertor
   MPI_Request *request)
{
  msg_manager.add_pending_recv(request, source, tag, comm_id);
  //  msg_manager.print_pending_recv();
  return 0;
}


int rempi_record_test(
    void *request,
    int *flag,
    int source,
    int tag
)
{
  int event_count = 1;
  int is_testsome = 0;
  int request_val = -1;

  //TODO: we need to record *request ?
  recording_event_list->push(new rempi_test_event(event_count, is_testsome, request_val, *flag, source, tag));

  return 0;
}


/*This function is called after MPI_Test*/
int rempi_replay_test(
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
  replaying_test_event = replaying_event_list->pop();
  /*
  if (non matched test) {
      *flag_out =1 ;
      return;
      } 
  */

  /*
    2. Wait until this recorded message really arrives
       if (0 if next recorded maching is not mached in this run) {
          TODO: Wait until maching, and get clock
	  TODO: if matched, memorize that the matching for the next replay test
       }
    3. Set valiabiles (source, flag, tag)
  }
   */


  *flag_out = flag_in;
  *source_out = source_in;
  *tag_out = tag_in;
  // 
  // while(1) {
  // 
  //   if (replaying_test_event != NULL) {
  //     replaying_test_event->print();
  //     fprintf(stderr, " || %p\n", replaying_test_event);
  //   } else {
  //     rempi_sleep_usec(100);
  //   }
  // }

  //  Replaying_test_event->print();
  // printf("\n");



  return 0;
}

int rempi_record_testsome(
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

int rempi_replay_testsome(
    int incount,
    void *array_of_requests[],
    int *outcount,
    int array_of_indices[],
    void *array_of_statuses[])
{
  return 0;
 }

int rempi_record_finalize(void)
{

  if (rempi_mode == REMPI_ENV_REMPI_MODE_RECORD) {
    recording_event_list->push_all();
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

int rempi_replay_finalize(void)
{
  //TODO:
  //  fprintf(stderr, "ReMPI: Function call (%s:%s:%d)\n", __FILE__, __func__, __LINE__);
  return 0;
}

