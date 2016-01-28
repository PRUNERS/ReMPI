#ifndef __REMPI_RECORD_H__
#define __REMPI_RECORD_H__

#define REMPI_MF_FLAG_1_WAIT   (0)
#define REMPI_MF_FLAG_1_TEST   (1)
#define REMPI_MF_FLAG_2_SINGLE (2)
#define REMPI_MF_FLAG_2_ANY    (3)
#define REMPI_MF_FLAG_2_SOME   (4)
#define REMPI_MF_FLAG_2_ALL    (5)

#include <vector>
#include <unordered_set>

#include "rempi_mem.h"
#include "rempi_message_manager.h"
#include "rempi_event_list.h"
#include "rempi_io_thread.h"
#include "clmpi.h"
#include "rempi_encoder.h"

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
 protected:
  rempi_message_manager msg_manager; //TODO: this is not used
  //  unordered_map<string, int> stacktrace_to_test_id_umap;
  int next_test_id_to_assign;// = 0;
  unordered_map<MPI_Request, rempi_irecv_inputs*> request_to_irecv_inputs_umap; 
  //  list<rempi_proxy_request*> proxy_request_pool_list;
  //unordered_map<MPI_Request*, int> request_to_test_id_umap;
  rempi_event_list<rempi_event*> *recording_event_list, *replaying_event_list;
  rempi_io_thread *record_thread, *read_record_thread;
  /*TODO: Fix bug in PNMPI fo rmulti-threaded, and remove this outputing*/
  rempi_encoder *mc_encoder;// = NULL;

 public:
  int validation_code; /*integer to check if correctly replayed the reocrded events*/

  rempi_recorder()
    : next_test_id_to_assign(0)
    , mc_encoder(NULL) {}
  
  virtual int record_init(int *argc, char ***argv, int rank);
  virtual int replay_init(int *argc, char ***argv, int rank);
  virtual int record_irecv(
			   void *buf,
			   int count,
			   MPI_Datatype datatype, // The value is assigned in ReMPI_convertor
			   int source,
			   int tag,
			   int comm_id, // The value is set by MPI_Comm_set_name in ReMPI_convertor
			   MPI_Comm *comm,
			   MPI_Request *request
			   );

  virtual int replay_isend(MPI_Request *request);

  virtual int replay_irecv(
			   void *buf,
			   int count,
			   MPI_Datatype datatype, // The value is assigned in ReMPI_convertor
			   int source,
			   int tag,
			   int comm_id, // The value is set by MPI_Comm_set_name in ReMPI_convertor
			   MPI_Comm *comm, // The value is set by MPI_Comm_set_name in ReMPI_convertor
			   MPI_Request *request
			   );

  virtual int replay_cancel(MPI_Request *request);

  virtual int record_test(
			  MPI_Request *request,
			  int *flag,
			  int source, // of MPI_Status
			  int tag,     // of MPI_tatus
			  int clock,
			  int with_previous
			  );

  virtual int record_test(
			  MPI_Request *request,
			  int *flag,
			  int source, // of MPI_Status
			  int tag,     // of MPI_tatus
			  int clock,
			  int with_previous,
			  int test_id
			  );


  virtual int record_mf(int incount,
			MPI_Request array_of_requests[],
			int *outcount,
			int array_of_indices[],
			MPI_Status array_of_statuses[],
			int global_test_id,
			int matching_function_type);
  
  /*TODO: Conmbine replay_test with replay_testsome into replay_mf by mf_flag_1 & mf_flag_2 ??*/
  virtual int replay_test(
			  MPI_Request *request,
			  int *flag,
			  MPI_Status *status,
			  int test_id);

  virtual int replay_testsome(
			      int incount, 
			      MPI_Request array_of_requests[], 
			      int *outcount, 
			      int array_of_indices[], 
			      MPI_Status array_of_statuses[],
			      int global_test_id,
			      int mf_flag_1,
			      int mf_flag_2);

  virtual int replay_iprobe(int source, int tag, MPI_Comm comm, int *flag, MPI_Status *status, int comm_id);


  //TODO: Comm_dup Comm_split

  virtual int record_finalize(void);
  virtual int replay_finalize(void);

  virtual void set_fd_clock_state(int flag);
  virtual void fetch_and_update_local_min_id();


};


class rempi_recorder_cdc : public rempi_recorder
{
 private:
  
  void* allocate_proxy_buf(int count, MPI_Datatype datatype);
  void copy_proxy_buf(void* fromt, void* to, int count, MPI_Datatype datatype);
  int get_test_id();
  int get_recv_test_id(int test_id);
  unordered_map<int, int> test_id_to_recv_test_id_umap;
  int next_recv_test_id_to_assign; // = 0;
  int init_clmpi();

  PNMPIMOD_register_recv_clocks_t clmpi_register_recv_clocks;
  PNMPIMOD_clock_control_t clmpi_clock_control;
  PNMPIMOD_get_local_clock_t clmpi_get_local_clock;
  PNMPIMOD_sync_clock_t      clmpi_sync_clock;
  PNMPIMOD_get_num_of_incomplete_sending_msg_t clmpi_get_num_of_incomplete_sending_msg;

  size_t send_request_id;
  unordered_map<MPI_Request, MPI_Request> isend_request_umap;

  PNMPIMOD_get_local_sent_clock_t clmpi_get_local_sent_clock;

  int REMPI_Send_Wait(MPI_Request *request, MPI_Status *status);
  int REMPI_Send_Waitany(int arg_0, MPI_Request *arg_1, int *arg_2, MPI_Status *arg_3);
  int REMPI_Send_Waitall(int count, MPI_Request *array_of_requests, MPI_Status *array_of_statuses);
  int REMPI_Send_Waitsome(int incount, MPI_Request *array_of_requests, int *outcount, int *array_of_indices, MPI_Status *array_of_statuses);
  int REMPI_Send_Test(MPI_Request *request, int *flag, MPI_Status *status);
  int REMPI_Send_Testany(int count, MPI_Request *array_of_requests, int *index, int *flag, MPI_Status *status);
  int REMPI_Send_Testall(int count, MPI_Request *array_of_requests, int *flag, MPI_Status *array_of_statuses);
  int REMPI_Send_Testsome(int incount, MPI_Request *array_of_requests, int *outcount, int *array_of_indices, MPI_Status *array_of_statuses);

  bool progress_send_requests();
  
  bool progress_recv_requests(int global_test_id,
			      int incount,
			      MPI_Request array_of_requests[],
			      int global_local_min_id_rank,
			      size_t global_local_min_id_clock);



 public:
   rempi_recorder_cdc()
     : rempi_recorder()
     , next_recv_test_id_to_assign(0)
     , send_request_id(100) {}

  int record_init(int *argc, char ***argv, int rank);
  int replay_init(int *argc, char ***argv, int rank);
  int record_irecv(
		   void *buf,
		   int count,
		   MPI_Datatype datatype, // The value is assigned in ReMPI_convertor
		   int source,
		   int tag,
		   int comm_id, // The value is set by MPI_Comm_set_name in ReMPI_convertor
		   MPI_Comm *comm,
		   MPI_Request *request
		   );

  int replay_isend(MPI_Request *request);

  int replay_irecv(
		   void *buf,
		   int count,
		   MPI_Datatype datatype, // The value is assigned in ReMPI_convertor
		   int source,
		   int tag,
		   int comm_id, // The value is set by MPI_Comm_set_name in ReMPI_convertor
		   MPI_Comm *comm, // The value is set by MPI_Comm_set_name in ReMPI_convertor
		   MPI_Request *request
		   );

  int replay_cancel(MPI_Request *request);

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

  int record_mf(int incount,
		MPI_Request array_of_requests[],
		int *outcount,
		int array_of_indices[],
		MPI_Status array_of_statuses[],
		int global_test_id,
		int matching_function_type);

  int replay_test(
		  MPI_Request *request,
		  int *flag,
		  MPI_Status *status,
		  int test_id);


  int replay_testsome(
		      int incount, 
		      MPI_Request array_of_requests[], 
		      int *outcount, 
		      int array_of_indices[], 
		      MPI_Status array_of_statuses[],
		      int global_test_id,
		      int mf_flag_1,
		      int mf_flag_2);

  int replay_iprobe(int source, int tag, MPI_Comm comm, int *flag, MPI_Status *status, int comm_id);


  //TODO: Comm_dup Comm_split

  int record_finalize(void);
  int replay_finalize(void);

  void set_fd_clock_state(int flag);
  void fetch_and_update_local_min_id();


};




#endif
