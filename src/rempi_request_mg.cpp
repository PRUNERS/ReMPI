#include <stdlib.h>
#include <string.h>

#include <mpi.h>
#include <vector>
#include <unordered_map>
#include <list>

#include "rempi_err.h"
#include "rempi_mem.h"
#include "rempi_config.h"
#include "rempi_util.h"
#include "rempi_request_mg.h"

#define REMPI_REQMG_MPI_CALL_TYPE_SEND (0)
#define REMPI_REQMG_MPI_CALL_TYPE_RECV (1)
#define REMPI_REQMG_MPI_CALL_TYPE_MF   (2)

#define REMPI_REQMG_MATCHING_SET_ID_UNKNOWN   (-1)

using namespace std;

/*Key: backtrace, Value: id*/
unordered_map<string, int>recv_id_umap;
int next_recv_id_to_assign = 0;
unordered_map<string, int>mpi_call_id_umap;
int next_mpi_call_id = 0;

unordered_map<int, int>mpi_call_id_to_matching_set_id;
int next_matching_set_id = 0;



/*Index: recv_id, Value: matchingset_id */
vector<int> recv_id_to_set_id;
/*Index: matching_set_id, Value: elements of the matching set*/
vector<vector<rempi_matching_id*>*> rempi_matching_id_vec_vec;
/*Index: test_id, Value: set_id */
unordered_map<int, int> test_id_to_set_id;
unordered_map<MPI_Request, rempi_reqmg_recv_args*> request_to_recv_args_umap;
unordered_map<MPI_Request, int>request_to_send_id_umap;
unordered_map<string, int> test_ids_map;
int next_test_id_to_assign = 0;

/*Key: id, Value: Recv MPI_Request */
// unordered_map<int, MPI_Request*>recv_request_id_to_recv_request_umap;
// int next_recv_request_id_to_assign = 0;

char comm_id[REMPI_COMM_ID_LENGTH];

int mpi_request_id = 1015;

list<rempi_reqmg_recv_args*> posted_recv_requests;
list<rempi_reqmg_recv_args*> matched_non_identified_recv_requests;
list<rempi_reqmg_recv_args*> matched_identified_recv_requests;

static int is_contained(int source, int tag, int comm_id, vector<rempi_matching_id*> *vec) {
  for (vector<rempi_matching_id*>::const_iterator cit = vec->cbegin(),
         cit_end = vec->cend();
       cit != cit_end;
       cit++) {
    rempi_matching_id *mid = *cit;
    if (source  == mid->source  &&
	tag     == mid->tag     &&
	comm_id == mid->comm_id) {
      return 1;
    }      
  }
  return 0;
}

static int get_mpi_call_id()
{
  string mpi_call_id_string;
  int mpi_call_id;
  mpi_call_id_string = rempi_btrace_string();
  if (mpi_call_id_umap.find(mpi_call_id_string) == mpi_call_id_umap.end()) {
    mpi_call_id = next_mpi_call_id;
    mpi_call_id_umap[mpi_call_id_string] = mpi_call_id;
    next_mpi_call_id++;
  } else {
    mpi_call_id = mpi_call_id_umap.at(mpi_call_id_string);
  }
  return mpi_call_id;
}


static int get_recv_id()
{
  string recv_id_string;
  recv_id_string = rempi_btrace_string();
  if (recv_id_umap.find(recv_id_string) == recv_id_umap.end()) {
    recv_id_umap[recv_id_string] = next_recv_id_to_assign;
    next_recv_id_to_assign++;
  }
  return recv_id_umap[recv_id_string];
}

static int add_matching_id(int matching_set_id, int source, int tag, int comm_id)
{
  rempi_matching_id *matching_id;
  vector<rempi_matching_id*> *vec;

  vec = rempi_matching_id_vec_vec[matching_set_id];
  if (is_contained(source, tag, comm_id, vec)) return 1;
  matching_id = new rempi_matching_id(source, tag, comm_id);  
  vec->push_back(matching_id);
  return 1;  
}

static int rempi_reqmg_assign_matching_set_id_to_recv(int matching_set_id, int incount, MPI_Request array_of_requests[])
{
  rempi_reqmg_recv_args *recv_args;
  for (int i = 0; i < incount; i++) {
    if (request_to_recv_args_umap.find(array_of_requests[i]) == 
	request_to_recv_args_umap.end()) {
      if (request_to_send_id_umap.find(array_of_requests[i]) == 
	  request_to_send_id_umap.end()) {
	REMPI_ERR("No such recv MPI_Request: %p", array_of_requests[i]);
      }
    } else {
      recv_args = request_to_recv_args_umap.at(array_of_requests[i]);
      if (recv_args->matching_set_id != REMPI_REQMG_MATCHING_SET_ID_UNKNOWN) {
	/*
	  e.g.)

	  MPI_Irecv1 (matching_set_id = X) ---+
	  MPI_Irecv2 (matching_set_id = Y) ---+
	                                      |
                                              +---> MPI_MF (matching_set_id = X)
						
	*/
	REMPI_ERR("matching set id is already assigned for this recv");
      } else {
	recv_args->matching_set_id = matching_set_id;
      }
    }
  }
  return 0;
}

static int rempi_reqmg_get_matching_set_id_from_recv(int incount, MPI_Request array_of_requests[])
{
  int matching_set_id = REMPI_REQMG_MATCHING_SET_ID_UNKNOWN;
  rempi_reqmg_recv_args *recv_args;
  for (int i = 0; i < incount; i++) {
    if (request_to_recv_args_umap.find(array_of_requests[i]) == 
	request_to_recv_args_umap.end()) {
      if (request_to_send_id_umap.find(array_of_requests[i]) == 
	  request_to_send_id_umap.end()) {
	REMPI_ERR("No such recv MPI_Request: %p", array_of_requests[i]);
      }
    } else {
      recv_args = request_to_recv_args_umap.at(array_of_requests[i]);
      if (recv_args->matching_set_id != REMPI_REQMG_MATCHING_SET_ID_UNKNOWN) {
	if (matching_set_id != REMPI_REQMG_MATCHING_SET_ID_UNKNOWN &&
	    matching_set_id != recv_args->matching_set_id) {
	  /*
	    e.g.)

	    MPI_Irecv1 (matching_set_id = X) ---+
	    MPI_Irecv2 (matching_set_id = Y) ---+
	                                        |
	                                        +---> MPI_MF (matching_set_id = Unknown)
						
	  */
	  REMPI_ERR("Inconsistent matching") ;
	} 
	  /*
	    e.g.)

	    MPI_Irecv1 (matching_set_id = X) ---+
	    MPI_Irecv2 (matching_set_id = X) ---+
	                                        |
	                                        +---> MPI_MF (matching_set_id = Unknown)
            Return X						
	  */
	matching_set_id = recv_args->matching_set_id;
      } 
    }
  }  
  return matching_set_id;
}



static int rempi_reqmg_assign_matching_set_id(int mpi_mf_call_id, int incount, MPI_Request array_of_requests[])
{

  int matching_set_id;


  if (mpi_call_id_to_matching_set_id.find(mpi_mf_call_id) != 
      mpi_call_id_to_matching_set_id.end()) {
    /*If this MF call has matching_set_id, simply allocate the number. 

      e.g.)

      MPI_Irecv1 (matching_set_id = X      ) ---+
      MPI_Irecv2 (matching_set_id = unknown) ---+
                                                |
		                                +---> MPI_MF (matching_set_id = X)
						
      Assige X to MPI_Irecv2 (matching_set_id = X)						
     */
    matching_set_id = mpi_call_id_to_matching_set_id.at(mpi_mf_call_id);
    rempi_reqmg_assign_matching_set_id_to_recv(matching_set_id, incount, array_of_requests);
  } else {
    matching_set_id = rempi_reqmg_get_matching_set_id_from_recv(incount, array_of_requests);
    if(matching_set_id == REMPI_REQMG_MATCHING_SET_ID_UNKNOWN) {
    /*

      e.g.)

      MPI_Irecv1 (matching_set_id = Unknown) ---+
      MPI_Irecv2 (matching_set_id = Unknown) ---+
                                                |
		                                +---> MPI_MF1 (matching_set_id = Unknown)
						
      Assige X to MPI_Irecv1, MPI_Irecv2 and MPI_MF1 (matching_set_id = X)						
     */
      matching_set_id = mpi_mf_call_id;
      mpi_call_id_to_matching_set_id[mpi_mf_call_id] = mpi_mf_call_id;
      rempi_reqmg_assign_matching_set_id_to_recv(matching_set_id, incount, array_of_requests);
    } else {
    /*
      e.g.)

      MPI_Irecv1 (matching_set_id = X      ) ---+
                                                |
		                                +---> MPI_MF1 (matching_set_id = X)
		                                +---> MPI_MF2 (matching_set_id = Unknown)
						
      Assige X to MPI_MF2 (matching_set_id = X)						
     */
      mpi_call_id_to_matching_set_id[mpi_mf_call_id] = matching_set_id;
    }
  }
  
  return matching_set_id;
}

static int rempi_reqmg_get_matching_set_id(int *matching_set_id, int *mpi_call_id, int mpi_call_type, int incount, MPI_Request array_of_requests[])
{
  *mpi_call_id = get_mpi_call_id();
  if (mpi_call_id_to_matching_set_id.find(*mpi_call_id) != 
      mpi_call_id_to_matching_set_id.end()) {
    *matching_set_id = mpi_call_id_to_matching_set_id.at(*mpi_call_id);
    return 0;
  }

  switch(mpi_call_type) {
  case REMPI_REQMG_MPI_CALL_TYPE_SEND:
    *matching_set_id = REMPI_REQMG_MATCHING_SET_ID_UNKNOWN; // Send is not assigned
    break;
  case REMPI_REQMG_MPI_CALL_TYPE_RECV:
    *matching_set_id = REMPI_REQMG_MATCHING_SET_ID_UNKNOWN; // not assigned yet //next_matching_set_id++; /* Assign new id */
    break;
  case REMPI_REQMG_MPI_CALL_TYPE_MF:
    *matching_set_id = rempi_reqmg_assign_matching_set_id(*mpi_call_id, incount, array_of_requests);
    break;
  }

  return 0;
}

static int get_matching_set_id(MPI_Request *request, int count)
{
  int recv_id = -1;
  int matching_set_id;
  int tmp_matching_set_id;

  /*Check if matching_set_id is consistent across different requests*/
  for (int i = 0; i < count; i++) {
    if (request_to_recv_args_umap.find(request[0]) == request_to_recv_args_umap.end()) {
      tmp_matching_set_id = -1; /*This request is send request*/
    } else {
      recv_id = request_to_recv_args_umap[request[i]]->matching_set_id;
      tmp_matching_set_id = recv_id_to_set_id[recv_id];
    }
    //   REMPI_DBG("recv_id: %d, set_id: %d", recv_id, matching_set_id);

    if (i == 0) {
      matching_set_id = tmp_matching_set_id;
    } else {
      if (tmp_matching_set_id != matching_set_id) {
      REMPI_ERR("matching set ids are not consistent across different requests: %d != base:%d",
		tmp_matching_set_id, matching_set_id);
      }
    }
  }
  return  matching_set_id;
}

static int assign_matching_set_id(int recv_id, int source, int tag, int comm_id)
{
  int matching_set_id = 0;
  rempi_matching_id *new_matching_id;
  vector<rempi_matching_id*> *new_matching_id_vec;
  for (vector<vector<rempi_matching_id*>*>::const_iterator cit = rempi_matching_id_vec_vec.cbegin(),
       cit_end = rempi_matching_id_vec_vec.cend();
       cit != cit_end;
       cit++) {
    vector<rempi_matching_id*> *v = *cit;
    if(is_contained(source, tag, comm_id, v)) {
      return matching_set_id;
    }
    matching_set_id++;
  } 
  /*if does not exist, create one*/
  new_matching_id  = new rempi_matching_id(source, tag, comm_id);
  new_matching_id_vec = new vector<rempi_matching_id*>();
  new_matching_id_vec->push_back(new_matching_id);
  rempi_matching_id_vec_vec.push_back(new_matching_id_vec);
  return matching_set_id;
}



static int rempi_reqmg_register_recv_request(mpi_const void *buf, int count, MPI_Datatype datatype, int source,
					     int tag, MPI_Comm comm, MPI_Request *request)
{
  int size;
  void* pooled_buffer;
  MPI_Request *pooled_request;
  int ret;
  int matching_set_id, mpi_call_id;

  //   *request = (MPI_Request)(mpi_request_id++);
  rempi_reqmg_get_matching_set_id(&matching_set_id, &mpi_call_id, REMPI_REQMG_MPI_CALL_TYPE_RECV, 0, NULL);
  request_to_recv_args_umap[*request] = new rempi_reqmg_recv_args(buf, count, datatype, source, tag, comm, *request, mpi_call_id, matching_set_id);
  //   PMPI_Type_size(datatype, &size);
  //pooled_buffer = rempi_malloc(size * count);
  //  pooled_request = (MPI_Request*)rempi_malloc(sizeof(MPI_Request));
  //  ret = PMPI_Irecv(pooled_buffer, count, datatype, source, tag, comm, pooled_request);
  
  //  posted_recv_requests.push_back(new rempi_reqmg_recv_args(pooled_buffer, count, datatype, source, tag, comm, *pooled_request, matching_set_id));
  
  return ret;
}



// static int rempi_reqmg_assigne_matching_set_id(int matching_set_id, int incount, MPI_Request array_of_requests[])
// {
//   list<rempi_reqmg_recv_args*>::iterator it = matched_non_identified_recv_requests.begin();
//   list<rempi_reqmg_recv_args*>::iterator it_end = matched_non_identified_recv_requests.end();
//   for (; it != matched_non_identified_recv_requests.end(); it++) {
    
//   }

// }


// int rempi_reqmg_progress_recv(int matching_set_id, int incount, MPI_Request array_of_requests[])
// {
//   // int flag;
//   // rempi_proxy_request *proxy_request;
//   // MPI_Status status;
//   // size_t clock;
//   // rempi_proxy_request *next_proxy_request;
//   // rempi_event *event_pooled;
//   // int has_recv_msg = 0;
//   // rempi_irecv_inputs *irecv_inputs;
//   // int matched_request_push_back_count = 0;

//   //  this->pending_message_source_set.clear();    
//   //  this->recv_message_source_umap.clear();

//   for (unordered_map<MPI_Request, rempi_irecv_inputs*>::const_iterator cit = request_to_irecv_inputs_umap.cbegin(),
// 	 cit_end = request_to_irecv_inputs_umap.cend();
//        cit != cit_end;
//        cit++) {
//     irecv_inputs = cit->second;
//     /* Find out this request belongs to which MF (recv_test_id) */
//     if (irecv_inputs->recv_test_id == -1) {
//       for (int i = 0; i < incount; i++) {
// 	if (irecv_inputs->request == array_of_requests[i] || !rempi_is_test_id) {
// 	  irecv_inputs->recv_test_id = recv_test_id;
// 	} 
//       }
//     } else if (irecv_inputs->recv_test_id != recv_test_id) {
//       for (int i = 0; i < incount; i++) {
// 	if (irecv_inputs->request == array_of_requests[i]) {
// 	  // NOTE: comment out because MPI requests can be tested in different MF
// 	  // REMPI_ERR("A MPI request is used in different MFs: "
// 	  //     "this request used for recv_test_id: %d first, but this is recv_test_id: %d (tag: %d)",
// 	  //     irecv_inputs->recv_test_id, recv_test_id,
// 	  //     irecv_inputs->tag);
// 	}
//       }
//     } 
    
//     /* =================================================
//        1. Check if there are any matched requests (pending matched), 
//        which have already matched in different recv_test_id
//        ================================================= */
//     if (irecv_inputs->recv_test_id != -1) {
//       matched_request_push_back_count = 0;

//       for (list<rempi_proxy_request*>::iterator it = irecv_inputs->matched_pending_request_proxy_list.begin(),
// 	     it_end = irecv_inputs->matched_pending_request_proxy_list.end();
// 	   it != it_end;
// 	   it++) {
// 	rempi_proxy_request *proxy_request = *it;

// 	REMPI_ASSERT(proxy_request->matched_source != -1);
// 	//REMPI_ASSERT(proxy_request->matched_tag    != -1);
// 	REMPI_ASSERT(proxy_request->matched_clock  !=  0);
// 	REMPI_ASSERT(proxy_request->matched_count  != -1);
	
// 	// event_pooled =  new rempi_test_event(1, -1, -1, 1,
// 	//      proxy_request->matched_source, 
// 	//      proxy_request->matched_tag, 
// 	//      proxy_request->matched_clock, 
// 	//      irecv_inputs->recv_test_id);
// 	event_pooled =  rempi_event::create_test_event(1, 
// 						       1, // flag=1
// 						       proxy_request->matched_source, 
// 						       REMPI_MPI_EVENT_INPUT_IGNORE, // with_next
// 						       REMPI_MPI_EVENT_INPUT_IGNORE, // index
// 						       proxy_request->matched_clock, // msg_id
// 						       irecv_inputs->recv_test_id);  // gid

// 	event_pooled->msg_count = proxy_request->matched_count;

// 	recording_event_list->enqueue_replay(event_pooled, irecv_inputs->recv_test_id);
// 	/* next recv message is "receved clock + 1" => event_pooled->get_clock() + 1 */
// 	recv_clock_umap.insert(make_pair(event_pooled->get_source(), event_pooled->get_clock() + 1));
// #ifdef REMPI_DBG_REPLAY  
// 	REMPI_DBGI(REMPI_DBG_REPLAY, "A->RCQ  : (count: %d, with_next: %d, flag: %d, source: %d,  clock: %d, msg_count: %d %p): recv_test_id: %d",
// 		   event_pooled->get_event_counts(), event_pooled->get_is_testsome(), event_pooled->get_flag(),
// 		   event_pooled->get_source(), event_pooled->get_clock(), event_pooled->msg_count,
// 		   event_pooled, irecv_inputs->recv_test_id);
// #endif
// 	/*Move from pending request list to matched request list 
// 	  This is used later for checking if a replayed event belongs to this list.  */
// 	irecv_inputs->matched_request_proxy_list.push_back(proxy_request);
// #ifdef REMPI_DBG_REPLAY
// 	//REMPI_DBGI(REMPI_DBG_REPLAY, "Added to matched_proxy_request in %p: size:%d request: %p", irecv_inputs, irecv_inputs->matched_request_proxy_list.size(), pair_req_and_irecv_inputs.first);
// #endif
// 	matched_request_push_back_count++;
//       }
//       for (int i = 0; i < matched_request_push_back_count; i++) {
// 	irecv_inputs->matched_pending_request_proxy_list.pop_front();
//       }
//     }

//     for (list<rempi_proxy_request*>::iterator it = irecv_inputs->matched_pending_request_proxy_list.begin(),
// 	   it_end = irecv_inputs->matched_pending_request_proxy_list.end();
// 	 it != it_end;
// 	 it++) {
//       rempi_proxy_request *proxy_request = *it;
//       /* When decoding thread computes my exporsing next_clock,
// 	  1. Retrive sender next clocks (= A) MPI_Get 
// 	   2. Then, estimate the minimum clock to send
// 	    3. Update exposing next_clock (= C)
// 	     sender next clocks are minimul clocks (= A) that will be sent
// 	      If messages exist in matched_pending_request_proxy_list, 
// 	       a clock of the message (= B) is smaller than A, i.e, B < A.
// 	        If we computes my exposing next_clock based on A when matched_pending_request_proxy_list.size() > 0,
// 		 the computed exposing next_clock, C will be bigger than correct value of C.
// 		  So we set has_pending_msg = true here, to avoid sender next clocks (= A) are updated, 
// 		  and to keep A less than B.
//       */
//       //      REMPI_DBG("Pending source: %d", proxy_request->matched_source);
//       pending_message_source_set.insert(proxy_request->matched_source);
//     }
    

//     /* =================================================
//        2. Check if this request have matched.
//        ================================================= */
//     /* irecv_inputs->request_proxy_list.size() == 0 can happens 
//        when this request was canceled (MPI_Cancel) */
//     if (irecv_inputs->request_proxy_list.size() == 0) continue; 
//     proxy_request = irecv_inputs->request_proxy_list.front();
//     clmpi_register_recv_clocks(&clock, 1);
//     clmpi_clock_control(0);
//     flag = 0;
//     REMPI_ASSERT(proxy_request != NULL);
//     REMPI_ASSERT(proxy_request->request != NULL);

//     // #ifdef REMPI_DBG_REPLAY  
//     //       REMPI_DBGI(REMPI_DBG_REPLAY, "Test  : (source: %d, tag: %d, request: %p): recv_test_id: %d",
//     //  irecv_inputs->source, irecv_inputs->tag, &proxy_request->request, irecv_inputs->recv_test_id);
//     //#endif
//     PMPI_Test(&proxy_request->request, &flag, &status);
//     clmpi_clock_control(1);

    
//     if(!flag) continue;
//     /*If flag == 1*/
//     has_recv_msg = 1;
//     rempi_cp_record_recv(status.MPI_SOURCE, 0);
//     recv_message_source_umap.insert(make_pair(status.MPI_SOURCE, 0));



// #ifdef REMPI_DBG_REPLAY  
//     REMPI_DBGI(REMPI_DBG_REPLAY, "A->     : (source: %d, tag: %d,  clock: %d): current recv_test_id: %d, size: %d (time: %f)",
// 	       status.MPI_SOURCE, status.MPI_TAG, clock, irecv_inputs->recv_test_id, 
// 	       request_to_irecv_inputs_umap.size(), PMPI_Wtime());
// #endif


//     if (irecv_inputs->recv_test_id != -1) {
//       //      event_pooled =  new rempi_test_event(1, -1, -1, 1, status.MPI_SOURCE, status.MPI_TAG, clock, irecv_inputs->recv_test_id);
//       event_pooled =  rempi_event::create_test_event(1, 
// 						     1, 
// 						     status.MPI_SOURCE, 
// 						     REMPI_MPI_EVENT_INPUT_IGNORE, 
// 						     REMPI_MPI_EVENT_INPUT_IGNORE, 
// 						     clock,
// 						     irecv_inputs->recv_test_id);

//       PMPI_Get_count(&status, irecv_inputs->datatype, &(event_pooled->msg_count));

//       recording_event_list->enqueue_replay(event_pooled, irecv_inputs->recv_test_id);
//       recv_clock_umap.insert(make_pair(event_pooled->get_source(), event_pooled->get_clock()));



// #ifdef REMPI_DBG_REPLAY  
//       REMPI_DBGI(REMPI_DBG_REPLAY, "A->RCQ  : (count: %d, with_next: %d, flag: %d, source: %d, clock: %d, msg_count: %d %p): recv_test_id: %d",
// 		 event_pooled->get_event_counts(), event_pooled->get_is_testsome(), event_pooled->get_flag(),
// 		 event_pooled->get_source(), event_pooled->get_clock(), event_pooled->msg_count,
// 		 event_pooled, irecv_inputs->recv_test_id);
// #endif
//       /*Move from pending request list to matched request list 
// 	This is used later for checking if a replayed event belongs to this list.  */
//       irecv_inputs->matched_request_proxy_list.push_back(proxy_request);
// #ifdef REMPI_DBG_REPLAY
//       //      REMPI_DBGI(REMPI_DBG_REPLAY, "Added to matched_proxy_request in %p: size:%d request: %p", irecv_inputs, irecv_inputs->matched_request_proxy_list.size(), pair_req_and_irecv_inputs.first);
// #endif
//       irecv_inputs->request_proxy_list.pop_front();
//     } else {
//       REMPI_ASSERT(irecv_inputs->recv_test_id != recv_test_id);
//       proxy_request->matched_source = status.MPI_SOURCE;
//       proxy_request->matched_tag    = status.MPI_TAG;
//       proxy_request->matched_clock    = clock;
//       PMPI_Get_count(&status, irecv_inputs->datatype, &(proxy_request->matched_count));
//       if (proxy_request->matched_count == MPI_UNDEFINED) {
// 	REMPI_ERR("PMPI_Get_count returned MPI_UNDEFINED");
//       }
//       irecv_inputs->matched_pending_request_proxy_list.push_back(proxy_request);
//       //      REMPI_DBG("Pending source: %d", status.MPI_SOURCE);
//       //      pending_message_source_set->insert(irecv_inputs->source);
//       pending_message_source_set.insert(status.MPI_SOURCE);
//       irecv_inputs->request_proxy_list.pop_front();
//     }
//     /* push_back  new proxy */
//     next_proxy_request = new rempi_proxy_request(irecv_inputs->count, irecv_inputs->datatype);
//     irecv_inputs->request_proxy_list.push_back(next_proxy_request);
//     PMPI_Irecv(next_proxy_request->buf, 
// 	       irecv_inputs->count, irecv_inputs->datatype, irecv_inputs->source, irecv_inputs->tag, irecv_inputs->comm, 
// 	       &next_proxy_request->request);
//   }
  
// #ifdef REMPI_DBG_REPLAY
//   // if (!has_recv_msg) {
//   //   REMPI_DBG("======= No msg arrive =========");
//   //   for (auto &pair_req_and_irecv_inputs: request_to_irecv_inputs_umap) {
//   //     irecv_inputs = pair_req_and_irecv_inputs.second;
//   //     REMPI_DBG("== source: %d, tag: %d: length: %d", irecv_inputs->source, irecv_inputs->tag, irecv_inputs->request_proxy_list.size());
//   //   }
//   // }
// #endif
//   return has_recv_msg;


// }


// static int rempi_reqmg_register_recv_request(mpi_const void *buf, int count, MPI_Datatype datatype, int source,
// 					     int tag, MPI_Comm comm, MPI_Request *request)
// {
//   int resultlen;
//   int matching_set_id = -1;

//   if (rempi_is_test_id == 0 || rempi_is_test_id == 1) {
//     request_to_recv_args_umap[*request] = new rempi_reqmg_recv_args(buf, count, datatype, source, tag, comm, *request, -1);
//     //      REMPI_DBGI(3, "recv request: %p", *request);
//     return 1;
//   }

//   int recv_id = get_recv_id();
//   int size = recv_id_to_set_id.size();

//   if (size == recv_id) {
//     /*If this Irecv called first time 
//         1. get matching set id
//         2. add recv_id with  this matchign set id 
//     */
//     PMPI_Comm_get_name(comm, comm_id, &resultlen);
//     matching_set_id = assign_matching_set_id(recv_id, source, tag, (int)comm_id[0]);
//     recv_id_to_set_id.push_back(matching_set_id);    
//     //    REMPI_DBG("recv_id: %d, set_id: %d, tag: %d", recv_id, matching_set_id, tag);    
//   } else if (size > recv_id) {
//     /*If this Irecv called before*/
//     matching_set_id = recv_id_to_set_id[recv_id];
//     add_matching_id(matching_set_id, source, tag, (int)comm_id[0]);

// #if 0
//     /*TODO: Intersection detection among matching sets*/
//     rempi_matching_id *matching_id;
//     vector<rempi_matching_id*> *vec;
//     matching_set_id = recv_id_to_set_id[recv_id];
//     vec = rempi_matching_id_vec_umap[matching_set_id];
//     if (is_contained(source, tag, comm_id, vec)) return matching_set_id;

//     matching_id = new rempi_matching_id(source, tag, comm_id);
//     /*Check if there is any intersection with other matching_set before pushing back*/
//     int is_intersection = 0;
//     for (unordered_map<int, vector<rempi_matching_id*>*>::const_iterator cit = rempi_matching_id_vec_umap.cbegin(),
//          cit_end = rempi_matching_id_vec_umap.cend();
// 	 cit != cit_end;
// 	 cit++) {
//       int mid = cit->first;
//       vector<rempi_matching_id*> *v = cit->second;
//       if(matching_id->has_intersection_in(v)) {
// 	REMPI_ERR("matching_id has an unexpected intersection");
//       }
//     }    
//     /*Then, push back*/
//     vec->push_back(matching_id);    
// #endif
//   } else {
//     /* This does not occur because recv_id increase one by one */
//     REMPI_ERR("recv_id_to_set_id (%d) < recv_id (%d)", size, recv_id);
//   }
//   //  REMPI_DBG("register: %p", *request);
//   request_to_recv_args_umap[*request] = new rempi_reqmg_recv_args(buf, count, datatype, source, tag, comm, *request, recv_id);
//   return 1;
// }

static int rempi_reqmg_register_send_request(mpi_const void *buf, int count, MPI_Datatype datatype, int rank,
					     int tag, MPI_Comm comm, MPI_Request *request)
{
  request_to_send_id_umap[*request] = rank;
  return 0;
}


static int rempi_reqmg_deregister_recv_request(MPI_Request *request)
{
  int is_erased;
  //  REMPI_DBGI(0, "dereg request: %p", *request);
  is_erased = request_to_recv_args_umap.erase(*request);
  if (is_erased != 1) {
    REMPI_ERR("Request %p cannnot be deregistered", *request);
  }
  return 1;
}

static int rempi_reqmg_deregister_send_request(MPI_Request *request)
{
  int is_erased;
  is_erased = request_to_send_id_umap.erase(*request);
  if (is_erased != 1) {
    REMPI_ERR("Request %p cannnot be deregistered", *request);
  }
  return 1;
}

static int rempi_reqmg_is_record_and_replay(int length, int *request_info, int send_count, int recv_count, int null_count, int ignore_count)
{
  //  return send_count + recv_count + null_count > 0;
#ifdef REMPI_LITE
  if (ignore_count > 0) {
    if (send_count + recv_count + null_count == 0) {
      return 0;
    } else {
      REMPI_ERR("Ignore requests and send/recv requests are tested by a singe MF: (%d %d %d %d)",
                send_count, recv_count, null_count, ignore_count);
    }
  }
#else
  if (send_count > 0) return 0;
#endif
  return 1;
}

int rempi_reqmg_register_request(mpi_const void *buf, int count, MPI_Datatype datatype, int rank,
				 int tag, MPI_Comm comm, MPI_Request *request, int request_type)
{
  //  REMPI_DBG("Register  : %p (%d)", *request, request_type);
  int ret;
  switch(request_type) {
  case REMPI_SEND_REQUEST:
    ret = rempi_reqmg_register_send_request(buf, count, datatype, rank, tag, comm, request);
    break;
  case REMPI_RECV_REQUEST:
    ret = rempi_reqmg_register_recv_request(buf, count, datatype, rank, tag, comm, request);
    break;
  default:
    REMPI_ERR("Cannot register MPI_Request: request_type: %d", request_type);
  }
  return ret;
}

int rempi_reqmg_deregister_request(MPI_Request *request, int request_type)
{
  //  REMPI_DBG("Deregister: %p (%d)", *request, request_type);
  switch(request_type) {
  case REMPI_SEND_REQUEST:
    rempi_reqmg_deregister_send_request(request);
    break;
  case REMPI_RECV_REQUEST:
    rempi_reqmg_deregister_recv_request(request);
    break;
  default:
    REMPI_DBG("Cannot deregister MPI_Request: request_type: %d", request_type);
    break;
  }
  return 0;
}

int rempi_reqmg_get_test_id(MPI_Request *request, int incount)
{
  double s, e;
  int matching_set_id;
  if (rempi_is_test_id == 2) {
    matching_set_id =  get_matching_set_id(request, incount);
  } else if (rempi_is_test_id == 0) {
    matching_set_id = 0;
  } else if  (rempi_is_test_id == 1) {
    string test_id_string;
    test_id_string = rempi_btrace_string();
    if (test_ids_map.find(test_id_string) == test_ids_map.end()) {
      test_ids_map[test_id_string] = next_test_id_to_assign;
      next_test_id_to_assign++;
    }
    matching_set_id = test_ids_map[test_id_string];
    // REMPI_DBGI(0, "id: %d", matching_set_id);
    // REMPI_DBGI(0, "string: %s", test_id_string.c_str());    
  }

  return matching_set_id;
}

int rempi_reqmg_get_recv_request_count(int incount, MPI_Request *requests)
{
  int count = 0;
  unordered_map<MPI_Request, rempi_reqmg_recv_args*>::iterator endit = request_to_recv_args_umap.end();
  for (int i = 0; i < incount; i++) {
    if(request_to_recv_args_umap.find(requests[i]) != endit) {
      count++;
    }
  }
  return count;
}

void rempi_reqmg_get_request_info(int incount, MPI_Request *requests, int *sendcount, int *recvcount, int *nullcount, int *request_info, int *is_record_and_replay, int *matching_set_id)
{
  unordered_map<MPI_Request, int>::iterator send_endit = request_to_send_id_umap.end();
  unordered_map<MPI_Request, rempi_reqmg_recv_args*>::iterator recv_endit = request_to_recv_args_umap.end();
  int mpi_call_id;
  int ignore;
  *sendcount = *recvcount = *nullcount = ignore = 0;
  for (int i = 0; i < incount; i++) {
    if(request_to_send_id_umap.find(requests[i]) != send_endit) {
      request_info[i] = REMPI_SEND_REQUEST;
      (*sendcount)++;

      if(request_to_recv_args_umap.find(requests[i]) != recv_endit) {
	REMPI_ERR("error: %p", requests[i]);
      }

    } else if(request_to_recv_args_umap.find(requests[i]) != recv_endit) {
      request_info[i] = REMPI_RECV_REQUEST;
      (*recvcount)++;

      if(request_to_send_id_umap.find(requests[i]) != send_endit) {
	REMPI_ERR("error: %p", requests[i]);
      }

    } else if (requests[i] == MPI_REQUEST_NULL) {
      request_info[i] = REMPI_NULL_REQUEST;
      (*nullcount)++;
    } else if (requests[i] == NULL) {
      REMPI_ERR("Request is null");
    } else {
      /*For unsupported MF, such as MPI_Recv_init, MPI_start and then */
      request_info[i] = REMPI_IGNR_REQUEST; 
      ignore++;
      REMPI_DBG("Unknown request: %p index: %d", requests[i], i);
      //      REMPI_ERR("Unknown request: %p index: %d", requests[i], i);
    }
    //    REMPI_DBG("request: %p index: %d, info: %d", requests[i], i, request_info[i]);
  }
  REMPI_ASSERT(incount == *sendcount + *recvcount + *nullcount + ignore);
  *is_record_and_replay = rempi_reqmg_is_record_and_replay(incount, request_info, *sendcount, *recvcount, *nullcount, ignore);

  *matching_set_id = REMPI_REQMG_MATCHING_SET_ID_UNKNOWN;
  if (*is_record_and_replay) {
    rempi_reqmg_get_matching_set_id(matching_set_id, &mpi_call_id, REMPI_REQMG_MPI_CALL_TYPE_MF, incount, requests);
  }
  //  *matching_set_id = rempi_reqmg_get_test_id(requests, incount);

  //  REMPI_DBGI(1, "incount:%d, sendcount:%d recvcount:%d nullcount:%d igncount:%d record:%d", incount, *sendcount, *recvcount, *nullcount, ignore, *is_record_and_replay)
  return;
}

void rempi_reqmg_get_request_type(MPI_Request *request, int *request_type)
{
  unordered_map<MPI_Request, int>::iterator send_endit = request_to_send_id_umap.end();
  unordered_map<MPI_Request, rempi_reqmg_recv_args*>::iterator recv_endit = request_to_recv_args_umap.end();
  if(request_to_send_id_umap.find(*request) != send_endit) {
    *request_type = REMPI_SEND_REQUEST;
  } else if(request_to_recv_args_umap.find(*request) != recv_endit) {
    *request_type = REMPI_RECV_REQUEST;
  } else if (*request == MPI_REQUEST_NULL) {
    *request_type = REMPI_NULL_REQUEST;
  } else if (*request == NULL) {
    REMPI_ERR("Request is null");
  } else {
    *request_type = REMPI_IGNR_REQUEST;
  }
  return;
}



void rempi_reqmg_store_send_statuses(int incount, MPI_Request *requests, int *request_info, MPI_Status *statuses)
{
  for (int i = 0; i < incount; i++) {
    if(request_info[i] == REMPI_SEND_REQUEST) {
      //statuses[i].MPI_SOURCE = request_to_send_id_umap[requests[i]];
      //      statuses[i].MPI_SOURCE = request_to_send_id_umap.at(requests[i]);
      statuses[i].MPI_SOURCE = request_to_send_id_umap.at(requests[i]);
      //      request_to_send_id_umap.at(requests[i]);
      // for (int j = 0; j < incount; j++) {
      // 	REMPI_DBGI(3, "store reqeust: %p (loop: %d)", requests[j], i);
      // }
    }
  }

  return;
}

int rempi_reqmg_get_null_request_count(int incount, MPI_Request *requests)
{
  int count = 0;
  for (int i = 0; i < incount; i++) {
    if(requests[i] == MPI_REQUEST_NULL) {
      count++;
    }
  }
  return count;
}
