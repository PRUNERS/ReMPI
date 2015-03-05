#ifndef __REMPI_RECORD_H__
#define __REMPI_RECORD_H__

#include "rempi_message_manager.h"
#include "rempi_event_list.h"
#include "rempi_io_thread.h"

class irecv_inputs
{
 public:
  void *buf;
  int count;
  MPI_Datatype datatype;
  int source;
  int tag;
  MPI_Comm comm;
  MPI_Request request;
  irecv_inputs(
               void* buf,
               int count,
               MPI_Datatype datatype,
               int source,
               int tag,
               MPI_Comm comm,
               MPI_Request request):
  buf(buf), count(count), datatype(datatype), source(source),
    tag(tag), comm(comm), request(request){};
};


class rempi_recorder {
 private:
  rempi_message_manager msg_manager; //TODO: this is not used
  unordered_map<MPI_Request, irecv_inputs*> request_to_irecv_inputs_umap;

  rempi_event_list<rempi_event*> *recording_event_list, *replaying_event_list;
  rempi_io_thread *record_thread, *read_record_thread;
  
  

 public:
  int record_init(int *argc, char ***argv, int rank);
  int replay_init(int *argc, char ***argv, int rank);
  int record_irecv(
			 void *buf,
			 int count,
			 int datatype, // The value is assigned in ReMPI_convertor
			 int source,
			 int tag,
			 int comm, // The value is set by MPI_Comm_set_name in ReMPI_convertor
			 MPI_Request *request
			 );
  int replay_irecv(
			 void *buf,
			 int count,
			 int datatype, // The value is assigned in ReMPI_convertor
			 int source,
			 int tag,
			 int comm_id, // The value is set by MPI_Comm_set_name in ReMPI_convertor
			 MPI_Comm *comm, // The value is set by MPI_Comm_set_name in ReMPI_convertor
			 MPI_Request *request
			 );



  int record_test(
			MPI_Request *request,
			int *flag,
			int source, // of MPI_Status
			int tag,     // of MPI_tatus
			int clock,
			int with_previous
			);

  int record_test(
			MPI_Request *request,
			int *flag,
			int source, // of MPI_Status
			int tag,     // of MPI_tatus
			int clock,
			int with_previous,
			int test_id
			);
  
  int replay_test(
			MPI_Request *request_in,
			int flag_in,
			int source_in,
			int tag_in,
			int clock_in,
			int with_previous_in,
			int test_id_in,
			int *flag_out,
			int *source_out,
			int *tag_out
			);

  /* int record_testsome( */
  /* 			    int incount, */
  /* 			    void *array_of_requests[], */
  /* 			    int *outcount, */
  /* 			    int array_of_indices[], */
  /* 			    void *array_of_statuses[] */
  /* 			    ); */
  /* int replay_testsome( */
  /* 			    int incount, */
  /* 			    void *array_of_requests[], */
  /* 			    int *outcount, */
  /* 			    int array_of_indices[], */
  /* 			    void *array_of_statuses[] */
  /* 			    ); */

  //TODO: Comm_dup Comm_split

  int record_finalize(void);
  int replay_finalize(void);


};




#endif
