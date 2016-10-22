#ifndef __REMPI_RECORD_H__
#define __REMPI_RECORD_H__

#define REMPI_MF_FLAG_1_WAIT   (0)
#define REMPI_MF_FLAG_1_TEST   (1)
#define REMPI_MF_FLAG_2_SINGLE (2)
#define REMPI_MF_FLAG_2_ANY    (3)
#define REMPI_MF_FLAG_2_SOME   (4)
#define REMPI_MF_FLAG_2_ALL    (5)

#define PRE_ALLOCATED_REQUEST_LENGTH (128)

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

#if !defined(REMPI_LITE)
#include "clmpi.h"
#endif

/* class rempi_matching_id */
/* { */
/*  public: */
/*   int source; */
/*   int tag; */
/*   int comm_id; */
/*  rempi_matching_id(int source, int tag, int comm_id): */
/*     source(source), tag(tag), comm_id(comm_id){}; */

/*   bool operator<(const rempi_matching_id &matching_id) const { */
/*     if (source == matching_id.source) { */
/*       if (tag == matching_id.tag) { */
/*   	return comm_id < matching_id.comm_id; */
/*       } */
/*       return tag < matching_id.tag; */
/*     } */
/*     return source < matching_id.source; */
/*   } */
/* }; */

class rempi_proxy_request
{
 public:
  MPI_Request request;
  void* buf;
  int    matched_source; /*If matched, memorize the matched source for matched_pending_request */
  int    matched_tag;    /*If matched, memorize the matched tag    for matched_pending_request*/
  size_t matched_clock;  /*If matched, memorize the matched clock  for matched_pending_request*/
  int    matched_count;  /*If matched, memorize the count          for matched_pending_request*/

 rempi_proxy_request(int count, MPI_Datatype datatype)
    : matched_source(-1)
    , matched_tag(-1)
    , matched_clock(0)
    , matched_count(-1) {
      int datatype_size;
      PMPI_Type_size(datatype, &datatype_size);
      buf = rempi_malloc(datatype_size * count);
  };
  ~rempi_proxy_request() {
    free(buf);
  };
};

class rempi_irecv_inputs
{

 public:
  void *buf;
  int count;
  MPI_Datatype datatype;
  int source;
  int tag;
  MPI_Comm comm;
  MPI_Request request;
  //  int test_id;
  int recv_test_id;

  list<rempi_proxy_request*> request_proxy_list; /*Posted by Irecv, but not matched yet*/
  list<rempi_proxy_request*> matched_pending_request_proxy_list; /*Matched, but not enqueued*/
  list<rempi_proxy_request*> matched_request_proxy_list; /*Enqueued, but not replayed*/
 rempi_irecv_inputs(
		    void* buf,
		    int count,
		    MPI_Datatype datatype,
		    int source,
		    int tag,
		    MPI_Comm comm,
		    MPI_Request request) 
    : buf(buf)
    , count(count)
    , datatype(datatype)
    , source(source)
    , tag(tag)
    , comm(comm)
    , request(request)
    , recv_test_id(-1) {};
  /* unordered_map<MPI_Request*, void*> proxy_requests_umap; */
  /* void insert_request(MPI_Request* proxy_request, void* proxy_buf); */
  /* void erase_request(MPI_Request* proxy_request); */
};


class rempi_recorder {
 private:
  rempi_encoder *mc_encoder;// = NULL;

 protected:
  vector<rempi_event*> replaying_event_vec; /*TODO: vector => list*/
  rempi_message_manager msg_manager; //TODO: this is not used
  int next_test_id_to_assign;// = 0;
  //  unordered_map<MPI_Request, rempi_irecv_inputs*> request_to_irecv_inputs_umap_a; 
  unordered_map<MPI_Request, rempi_event*> request_to_recv_event_umap;
  rempi_event_list<rempi_event*> *recording_event_list, *replaying_event_list;
  rempi_io_thread *record_thread, *read_record_thread;
  /*TODO: Fix bug in PNMPI fo rmulti-threaded, and remove this outputing*/

  unsigned int validation_code; /*integer to check if correctly replayed the reocrded events*/
  void update_validation_code(int incount, int *outcount, int *array_of_indices, MPI_Status *array_of_statuses, int* request_info);

  int request_info[PRE_ALLOCATED_REQUEST_LENGTH];
  MPI_Status  tmp_statuses[PRE_ALLOCATED_REQUEST_LENGTH];
  MPI_Request tmp_requests[PRE_ALLOCATED_REQUEST_LENGTH];


  void cancel_request(MPI_Request *request);
  int rempi_get_matched_count(int incount, int *outcount, int matching_function_type);
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
			      vector<rempi_event*> &replaying_event_vec, 
			      int matching_set_id);


 public:

  rempi_recorder()
    : next_test_id_to_assign(0)
    , record_thread(NULL)
    , read_record_thread(NULL)
    , mc_encoder(NULL)
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
  rempi_encoder_cdc *mc_encoder;// = NULL;
  size_t pre_allocated_clocks[PRE_ALLOCATED_REQUEST_LENGTH];
  void* allocate_proxy_buf(int count, MPI_Datatype datatype);
  void copy_proxy_buf(void* fromt, void* to, int count, MPI_Datatype datatype);
  int get_test_id();
  int get_recv_test_id(int test_id);
  unordered_map<int, int> test_id_to_recv_test_id_umap;
  int next_recv_test_id_to_assign; // = 0;
  int init_clmpi();

  int progress_decode();

  PNMPIMOD_register_recv_clocks_t clmpi_register_recv_clocks;
  PNMPIMOD_clock_control_t clmpi_clock_control;
  PNMPIMOD_get_local_clock_t clmpi_get_local_clock;
  PNMPIMOD_sync_clock_t      clmpi_sync_clock;
  PNMPIMOD_get_num_of_incomplete_sending_msg_t clmpi_get_num_of_incomplete_sending_msg;

  size_t send_request_id;


  /* Map for memorizing probed messages, but not received  */
  unordered_set<int> probed_message_source_set;
  /* To detect  which next_clock(or from which rank) should be updated at update_local_min_id. 
     This is a set of ranks from which messages are received.
     However, the messages have not been assigend to particulaer MF (recv_test_id=-1)
   */
  unordered_set<int> pending_message_source_set; 
  /* Map for memorizing recieved messages with clock */
  unordered_map<int, size_t> recv_message_source_umap; 
  /* Map for memorizing recieved clock */
  unordered_map<int, size_t> recv_clock_umap;
  unordered_map<int, size_t> recv_clock_umap_1;
  unordered_map<int, size_t> recv_clock_umap_2;
  unordered_map<int, size_t> *recv_clock_umap_p_1;
  unordered_map<int, size_t> *recv_clock_umap_p_2;

  PNMPIMOD_get_local_sent_clock_t clmpi_get_local_sent_clock;


  int progress_probe();

  int progress_recv_and_safe_update_local_look_ahead_recv_clock(int do_fetch,
								int incount, MPI_Request *array_of_requests, int message_set_id);

  
  int dequeue_replay_event_set(vector<rempi_event*> &replaying_event_vec, int matching_set_id);
  
  int progress_recv_requests(int global_test_id,
			     int incount,
			     MPI_Request array_of_requests[]);



 protected:

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
			      vector<rempi_event*> &replaying_event_vec, 
			      int matching_set_id);

  


 public:
  rempi_recorder_cdc()
    : rempi_recorder()
    , next_recv_test_id_to_assign(0)
    , send_request_id(100)
    , recv_clock_umap_p_1(&recv_clock_umap_1)
    , recv_clock_umap_p_2(&recv_clock_umap_2)
    {}

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

#endif /* REMPI_LITE */


#endif
