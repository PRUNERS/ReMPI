#ifndef __REMPI_RECORD_H__
#define __REMPI_RECORD_H__

#define REMPI_MF_FLAG_1_WAIT   (0)
#define REMPI_MF_FLAG_1_TEST   (1)
#define REMPI_MF_FLAG_2_SINGLE (2)
#define REMPI_MF_FLAG_2_ANY    (3)
#define REMPI_MF_FLAG_2_SOME   (4)
#define REMPI_MF_FLAG_2_ALL    (5)

#define PRE_ALLOCATED_REQUEST_LENGTH (128 * 128)

#define REMPI_RECORDER_DO_NOT_FETCH_REMOTE_LOOK_AHEAD_SEND_CLOCKS (0)
#define REMPI_RECORDER_DO_FETCH_REMOTE_LOOK_AHEAD_SEND_CLOCKS (1)


#include <string.h>

#include <vector>
#include <unordered_set>


#include "rempi_mem.h"
#include "rempi_type.h"
#include "rempi_message_manager.h"
#include "rempi_event_list.h"
#include "rempi_io_thread.h"
#include "rempi_encoder.h"




class rempi_recorder {
 private:

  rempi_encoder *mc_encoder;// = NULL;
 protected:

  vector<rempi_event*> replaying_event_vec; /*TODO: vector => list*/
  list<rempi_event*> recording_event_set_list; /*TODO: vector => list*/

  unordered_map<MPI_Request, rempi_event*> request_to_recv_event_umap;
  rempi_event_list<rempi_event*> *recording_event_list, *replaying_event_list;
  rempi_io_thread *record_thread, *read_record_thread;
  /*TODO: Fix bug in PNMPI fo rmulti-threaded, and remove this outputing*/
  unsigned int validation_code; /*integer to check if correctly replayed the reocrded events*/
  void update_validation_code(int incount, int *outcount, int matched_count, int *array_of_indices, MPI_Status *array_of_statuses, int* request_info);

  int request_info[PRE_ALLOCATED_REQUEST_LENGTH];
  MPI_Status  tmp_statuses[PRE_ALLOCATED_REQUEST_LENGTH];
  MPI_Request tmp_requests[PRE_ALLOCATED_REQUEST_LENGTH];


  void cancel_request(MPI_Request *request);
  virtual int rempi_mf(int incount,
		       MPI_Request array_of_requests[],
		       int *outcount,
		       int array_of_indices[],
		       MPI_Status array_of_statuses[],
		       size_t **msg_id, // or clock
		       int *request_info,
		       int matching_function_type);

  virtual int rempi_pf(int source,
		       int tag,
		       MPI_Comm comm,
		       int *flag,
		       MPI_Status *status,
		       size_t *msg_id, // or clock
		       int prove_function_type);

  virtual int replay_mf_input(
		      int incount,
		      MPI_Request array_of_requests[],
		      int *outcount,
		      int array_of_indices[],
		      MPI_Status array_of_statuses[],
		      vector<rempi_event*> &replyaing_event_vec,
		      int matching_set_id,
		      int matching_function_type);

  virtual int get_next_events(
			      int incount, 
			      MPI_Request *array_of_requests, 
			      int request_info[],
			      vector<rempi_event*> &replaying_event_vec, 
			      int matching_set_id,
			      int matching_function_type);


 public:

  rempi_recorder()
    : mc_encoder(NULL)
    , record_thread(NULL)
    , read_record_thread(NULL)
    , validation_code(5371) {
    memset(request_info, 0, sizeof(int) * PRE_ALLOCATED_REQUEST_LENGTH);
    memset(tmp_statuses, 0, sizeof(MPI_Status) * PRE_ALLOCATED_REQUEST_LENGTH);
    memset(tmp_requests, 0, sizeof(MPI_Request) * PRE_ALLOCATED_REQUEST_LENGTH);
    //    request_info = (int*)rempi_malloc(sizeof(int) * PRE_ALLOCATED_REQUEST_LENGTH);
  }
  
  ~rempi_recorder()
    {
      delete replaying_event_list;
      delete recording_event_list;
      if (record_thread != NULL) delete record_thread;
      if (read_record_thread != NULL) delete read_record_thread;
      /* unordered_map<MPI_Request, rempi_irecv_inputs*>::const_iterator request_to_irecv_inputs_umap_it; */
      /* unordered_map<MPI_Request, rempi_irecv_inputs*>::const_iterator request_to_irecv_inputs_umap_it_end; */
      /* for (request_to_irecv_inputs_umap_it     = request_to_irecv_inputs_umap.cbegin(), */
      /* 	     request_to_irecv_inputs_umap_it_end = request_to_irecv_inputs_umap.cend(); */
      /* 	   request_to_irecv_inputs_umap_it != request_to_irecv_inputs_umap_it_end; */
      /* 	   request_to_irecv_inputs_umap_it++) { */
      /* 	rempi_irecv_inputs *irecv_inputs =  request_to_irecv_inputs_umap_it->second; */
      /* 	delete irecv_inputs; */
      /* } */
    }
  
  virtual void init(int rank);
  virtual int record_init(int *argc, char ***argv, int rank);
  virtual int replay_init(int *argc, char ***argv, int rank);
  virtual int record_irecv(
			   void *buf,
			   int count,
			   MPI_Datatype datatype, // The value is assigned in ReMPI_convertor
			   int source,
			   int tag,
			   int comm_id, // The value is set by MPI_Comm_set_name in ReMPI_convertor
			   MPI_Comm comm,
			   MPI_Request *request
			   );

  virtual int record_isend(mpi_const void *buf,
			   int count,
			   MPI_Datatype datatype,
			   int dest,
			   int tag,
			   MPI_Comm comm,
			   MPI_Request *request,
			   int send_function_type);

  virtual int replay_isend(mpi_const void *buf,
			   int count,
			   MPI_Datatype datatype,
			   int dest,
			   int tag,
			   MPI_Comm comm,
			   MPI_Request *request,
			   int send_function_type);

  virtual int replay_irecv(
			   void *buf,
			   int count,
			   MPI_Datatype datatype, // The value is assigned in ReMPI_convertor
			   int source,
			   int tag,
			   int comm_id, // The value is set by MPI_Comm_set_name in ReMPI_convertor
			   MPI_Comm comm, // The value is set by MPI_Comm_set_name in ReMPI_convertor
			   MPI_Request *request
			   );

  virtual int record_cancel(MPI_Request *request);
  virtual int replay_cancel(MPI_Request *request);
  virtual int replay_request_free(MPI_Request *request);
  virtual MPI_Fint replay_request_c2f(MPI_Request request);


  virtual int record_mf(int incount,
			MPI_Request array_of_requests[],
			int *outcount,
			int array_of_indices[],
			MPI_Status array_of_statuses[],
			int matching_function_type);

  virtual int record_pf(int source,
			int tag,
			MPI_Comm comm,
			int *flag,
			MPI_Status *status,
			int prove_function_type);

  virtual int replay_mf(
			int incount, 
			MPI_Request array_of_requests[], 
			int *outcount, 
			int array_of_indices[], 
			MPI_Status array_of_statuses[],
			int matching_function_type);


  virtual int replay_pf(int source, int tag, MPI_Comm comm, int *flag, MPI_Status *status, int comm_id);



  virtual int pre_process_collective(MPI_Comm comm);
  virtual int post_process_collective();

  //TODO: Comm_dup Comm_split

  virtual int record_finalize(void);
  virtual int replay_finalize(void);



};


#ifndef REMPI_LITE
class rempi_recorder_cdc : public rempi_recorder
{
 private:
  size_t pre_allocated_clocks[PRE_ALLOCATED_REQUEST_LENGTH];
  int progress_decode();
  int progress_recv_requests(int global_test_id,
			     int incount,
			     MPI_Request array_of_requests[]);


 protected:
  rempi_encoder_cdc *mc_encoder;// = NULL;

  /* left < right: 1, left == right: 0 left > rigth: -1 */
  int compare_clock(size_t left_clock, int left_rank, size_t right_clock, int right_rank);

  int progress_recv_and_safe_update_local_look_ahead_recv_clock(int do_fetch,
								int incount, MPI_Request *array_of_requests, int message_set_id);
  virtual int dequeue_replay_event_set(int incount, MPI_Request array_of_requests[], int *request_info, int matching_set_id, int matching_function_type, 
			       vector<rempi_event*> &replaying_event_vec);


  virtual int rempi_mf(int incount,
  		       MPI_Request array_of_requests[],
  		       int *outcount,
  		       int array_of_indices[],
  		       MPI_Status array_of_statuses[],
  		       size_t **msg_id, // or clock
		       int *request_info,
  		       int matching_function_type);

  virtual int rempi_pf(int source,
  		       int tag,
  		       MPI_Comm comm,
  		       int *flag,
  		       MPI_Status *status,
  		       size_t *msg_id, // or clock
  		       int prove_function_type);

  virtual int replay_mf_input(
			      int incount,
			      MPI_Request array_of_requests[],
			      int *outcount,
			      int array_of_indices[],
			      MPI_Status array_of_statuses[],
			      vector<rempi_event*> &replyaing_event_vec,
			      int matching_set_id,
			      int matching_function_type);
  
  virtual int get_next_events(
			      int incount, 
			      MPI_Request *array_of_requests, 
			      int request_info[],
			      vector<rempi_event*> &replaying_event_vec, 
			      int matching_set_id,
			      int matching_function_type);



 public:

  rempi_recorder_cdc()
    : rempi_recorder()
    {}


  virtual void init(int rank);
  int record_init(int *argc, char ***argv, int rank);
  int replay_init(int *argc, char ***argv, int rank);
  int record_irecv(
		   void *buf,
		   int count,
		   MPI_Datatype datatype, // The value is assigned in ReMPI_convertor
		   int source,
		   int tag,
		   int comm_id, // The value is set by MPI_Comm_set_name in ReMPI_convertor
		   MPI_Comm comm,
		   MPI_Request *request
		   );

  int record_isend(mpi_const void *buf,
			   int count,
			   MPI_Datatype datatype,
			   int dest,
			   int tag,
			   MPI_Comm comm,
			   MPI_Request *request,
			   int send_function_type);

  int replay_isend(mpi_const void *buf,
			   int count,
			   MPI_Datatype datatype,
			   int dest,
			   int tag,
			   MPI_Comm comm,
			   MPI_Request *request,
			   int send_function_type);

  int replay_irecv(
		   void *buf,
		   int count,
		   MPI_Datatype datatype, // The value is assigned in ReMPI_convertor
		   int source,
		   int tag,
		   int comm_id, // The value is set by MPI_Comm_set_name in ReMPI_convertor
		   MPI_Comm comm, // The value is set by MPI_Comm_set_name in ReMPI_convertor
		   MPI_Request *request
		   );

  //  int record_cancel(MPI_Request *request);
  int replay_cancel(MPI_Request *request);
  int replay_request_free(MPI_Request *request);
  MPI_Fint replay_request_c2f(MPI_Request request);

  int record_pf(int source,
		int tag,
		MPI_Comm comm,
		int *flag,
		MPI_Status *status,
		int prove_function_type);

  int replay_pf(int source, int tag, MPI_Comm comm, int *flag, MPI_Status *status, int comm_id);


  int pre_process_collective(MPI_Comm comm);
  int post_process_collective();


  //TODO: Comm_dup Comm_split

  int record_finalize(void);
  int replay_finalize(void);



};




class rempi_recorder_rep : public rempi_recorder_cdc
{
 private:
  int exclusion_flags[PRE_ALLOCATED_REQUEST_LENGTH];
  unordered_map<int, list<rempi_event*>*> matched_recv_event_list_umap;

  int is_behind_time(int matching_set_id);
  int dequeue_matched_events(int matching_set_id);
  int complete_mf_send_single(int incount, MPI_Request array_of_requests[], int *request_info, 
		       int matching_set_id, int matching_function_type, vector<rempi_event*> &replaying_event_vec);
  int complete_mf_send_all(int incount, MPI_Request array_of_requests[], int *request_info, 
		       int matching_set_id, int matching_function_type, vector<rempi_event*> &replaying_event_vec);
  int complete_mf_send(int incount, MPI_Request array_of_requests[], int *request_info, 
		       int matching_set_id, int matching_function_type, vector<rempi_event*> &replaying_event_vec);

  int complete_mf_unmatched_recv_all(int incount, MPI_Request array_of_requests[], int *request_info, 
		       int matching_set_id, int matching_function_type, vector<rempi_event*> &replaying_event_vec);
  int complete_mf_unmatched_recv_single(int incount, MPI_Request array_of_requests[], int *request_info, 
		       int matching_set_id, int matching_function_type, vector<rempi_event*> &replaying_event_vec);
  int complete_mf_unmatched_recv(int incount, MPI_Request array_of_requests[], int *request_info, 
		       int matching_set_id, int matching_function_type, vector<rempi_event*> &replaying_event_vec);

  int get_mf_matched_index(int incount, MPI_Request array_of_requests[], int *request_info, int exclusion_flags[],int recved_rank, size_t recved_clock, int matching_set_id, int matching_function_type);
  int complete_mf_matched_recv_all(int incount, MPI_Request array_of_requests[], int *request_info, 
		       int matching_set_id, int matching_function_type, vector<rempi_event*> &replaying_event_vec);
  int complete_mf_matched_recv_single(int incount, MPI_Request array_of_requests[], int *request_info, 
		       int matching_set_id, int matching_function_type, vector<rempi_event*> &replaying_event_vec);
  int complete_mf_matched_recv(int incount, MPI_Request array_of_requests[], int *request_info, 
		       int matching_set_id, int matching_function_type, vector<rempi_event*> &replaying_event_vec);


 protected:
  int dequeue_replay_event_set(int incount, MPI_Request array_of_requests[], int *request_info, int matching_set_id, int matching_function_type,
                               vector<rempi_event*> &replaying_event_vec);


 public:
  virtual void init(int rank);

  rempi_recorder_rep()
    : rempi_recorder_cdc()
    {
      //      matched_recv_event_list_umap.find(0);
    }



};

#endif /* REMPI_LITE */


#endif
