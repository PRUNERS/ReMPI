#include <stdlib.h>
#include <string.h>

#include <mpi.h>
#include <vector>
#include <unordered_map>
#include <list>
#include <utility>
#include <unordered_set>

#include "rempi_err.h"
#include "rempi_mem.h"
#include "rempi_config.h"
#include "rempi_util.h"
#include "rempi_request_mg.h"
#include "rempi_clock.h"
#include "rempi_mpi_init.h"

#define REMPI_REQMG_MPI_CALL_TYPE_SEND (0)
#define REMPI_REQMG_MPI_CALL_TYPE_RECV (1)
#define REMPI_REQMG_MPI_CALL_TYPE_MF   (2)


using namespace std;

class matching_id {
public:
  int rank;
  int tag;
  MPI_Comm comm;
  matching_id(int rank, int tag, MPI_Comm comm) 
    : rank(rank)
    , tag(tag)
    , comm(comm) {};  
};


int next_recv_id_to_assign = 0;
unordered_map<string, int>mpi_call_id_umap;
int next_mpi_call_id = 0;

unordered_map<int, int>mpi_call_id_to_matching_set_id;
int next_matching_set_id = 0;

unordered_map<size_t, int>matching_id_hash_to_matching_set_id;
unordered_map<size_t, matching_id*>matching_id_hash_to_matching_id;

unordered_map<size_t, int> comm_tag_to_matching_set_id;
unordered_map<int, unordered_set<size_t>*> mpi_call_id_to_comm_tag_umap;



/*Index: recv_id, Value: matchingset_id */
vector<int> recv_id_to_set_id;
/*Index: matching_set_id, Value: elements of the matching set*/
vector<vector<rempi_matching_id*>*> rempi_matching_id_vec_vec;
/*Index: test_id, Value: set_id */
unordered_map<int, int> test_id_to_set_id;
unordered_map<MPI_Request, rempi_reqmg_recv_args*> request_to_recv_args_umap;
unordered_map<MPI_Request, rempi_reqmg_send_args*> request_to_send_id_umap;
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

static void add_comm_and_tag_to_mpi_call_id(int mpi_call_id, MPI_Comm comm, int tag)
{
  int comm_id;
  unordered_set<size_t> *comm_tag_uset;
  size_t msg_id;
  comm_id = rempi_mpi_comm_get_id(comm);
  if (mpi_call_id_to_comm_tag_umap.find(mpi_call_id) == mpi_call_id_to_comm_tag_umap.end()) {
    comm_tag_uset = new unordered_set<size_t>();
    mpi_call_id_to_comm_tag_umap[mpi_call_id] = comm_tag_uset;
  } else {
    comm_tag_uset = mpi_call_id_to_comm_tag_umap.at(mpi_call_id);
  }
  msg_id = rempi_mpi_get_msg_id(comm, tag);
  comm_tag_uset->insert(msg_id);
  return;
}

static void add_comms_and_tags_to_mpi_call_id(int mpi_call_id, MPI_Comm comm, int tag, int incount, MPI_Request array_of_requests[], int mpi_call_type)
{
  int requested_rank, requested_tag;
  MPI_Comm requested_comm;
  switch(mpi_call_type) {
  case REMPI_REQMG_MPI_CALL_TYPE_RECV:
    add_comm_and_tag_to_mpi_call_id(mpi_call_id, comm, tag);
    break;
  case REMPI_REQMG_MPI_CALL_TYPE_MF:
    for (int i = 0; i < incount; i++) {
      if (request_to_recv_args_umap.find(array_of_requests[i]) != request_to_recv_args_umap.end()) {
	rempi_reqmg_get_matching_id(&array_of_requests[i], &requested_rank, &requested_tag, &requested_comm);
	add_comm_and_tag_to_mpi_call_id(mpi_call_id, requested_comm, requested_tag);
      }
    }
    break;
  case REMPI_REQMG_MPI_CALL_TYPE_SEND:
    break;
  default:
    REMPI_ERR("No such mpi_call_type: %d", mpi_call_type);
    break;
  }
  return;
}


static int get_matching_set_id_from(int tag, MPI_Comm comm, int incount, MPI_Request array_of_requests[], int mpi_call_type, int *matching_set_id)
{
  size_t msg_id; 
  int matching_set_id_tmp;
  int is_set = 0;
  int requested_rank, requested_tag;
  MPI_Comm requested_comm;


  switch(mpi_call_type) {
  case REMPI_REQMG_MPI_CALL_TYPE_RECV:
    msg_id = rempi_mpi_get_msg_id(comm, tag);  
    if (comm_tag_to_matching_set_id.find(msg_id) == comm_tag_to_matching_set_id.end()) {
      return -1;
    }
    *matching_set_id = comm_tag_to_matching_set_id.at(msg_id);
    break;
  case REMPI_REQMG_MPI_CALL_TYPE_MF:
    for (int i = 0; i < incount; i++) {
      if (request_to_recv_args_umap.find(array_of_requests[i]) != request_to_recv_args_umap.end()) {
        rempi_reqmg_get_matching_id(&array_of_requests[i], &requested_rank, &requested_tag, &requested_comm);
	msg_id = rempi_mpi_get_msg_id(requested_comm, requested_tag);  
	if (comm_tag_to_matching_set_id.find(msg_id) == comm_tag_to_matching_set_id.end()) {
	  return -1;
	}
	matching_set_id_tmp = comm_tag_to_matching_set_id.at(msg_id);
	if (is_set && matching_set_id_tmp != *matching_set_id) {
	  REMPI_ERR("Inconsistent matching_set_id");
	}
	*matching_set_id = matching_set_id_tmp;
	is_set = 1;
      }
    }
    break;
  case REMPI_REQMG_MPI_CALL_TYPE_SEND:
    *matching_set_id = REMPI_REQMG_MATCHING_SET_ID_UNKNOWN;
    break;
  default:
    REMPI_ERR("No such mpi_call_type: %d", mpi_call_type);
    break;
  }
  return 1;
}


static size_t get_matching_id_hash(int source, int tag, MPI_Comm comm)
{
  size_t hash;
  matching_id *mid;
  
  hash =  (source + 1000) * (tag + 1000) * (size_t)comm;
  if (matching_id_hash_to_matching_id.find(hash) == matching_id_hash_to_matching_id.end()) {
    matching_id_hash_to_matching_id[hash] = new matching_id(source, tag, comm);
  } else {
    mid = matching_id_hash_to_matching_id.at(hash);
    if (source != mid->rank ||
	tag    != mid->tag    ||
	comm   != mid->comm) {
      REMPI_ERR("Hash value conflict occured");
    }
  }
  return hash;
  
}

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
#if 1
  int hash = (int)rempi_btrace_hash();
  return hash;
#else

  string mpi_call_id_string;
  int mpi_call_id;
  mpi_call_id_string = rempi_btrace_string();
  if (mpi_call_id_umap.find(mpi_call_id_string) == mpi_call_id_umap.end()) {
    mpi_call_id = next_mpi_call_id;
    //mpi_call_id = rempi_compute_hash((void*)mpi_call_id_string.c_str(), strlen(mpi_call_id_string.c_str()));
    //    REMPI_DBG("callstack (%d): %s", mpi_call_id, mpi_call_id_string.c_str());    
    //    REMPI_DBG("callstack (%d):", mpi_call_id);
    mpi_call_id_umap[mpi_call_id_string] = mpi_call_id;
    next_mpi_call_id++;

  } else {
    mpi_call_id = mpi_call_id_umap.at(mpi_call_id_string);
  } 
  return mpi_call_id;
#endif
}


// static int get_recv_id()
// {
//   string recv_id_string;
//   recv_id_string = rempi_btrace_string();
//   if (recv_id_umap.find(recv_id_string) == recv_id_umap.end()) {
//     recv_id_umap[recv_id_string] = next_recv_id_to_assign;
//     next_recv_id_to_assign++;
//   }
//   return recv_id_umap[recv_id_string];
// }

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
    if (request_to_recv_args_umap.find(array_of_requests[i]) == request_to_recv_args_umap.end()) {

      if (array_of_requests[i] != MPI_REQUEST_NULL && 	request_to_send_id_umap.find(array_of_requests[i])  == request_to_send_id_umap.end() ) {
	REMPI_ERR("No such recv MPI_Request: %p", array_of_requests[i], MPI_REQUEST_NULL);
      }
    } else {
      //      REMPI_DBG("id: %d, incount: %d, request: %p", matching_set_id, incount, array_of_requests[i]);
      recv_args = request_to_recv_args_umap.at(array_of_requests[i]);
      if (recv_args->matching_set_id != REMPI_REQMG_MATCHING_SET_ID_UNKNOWN) {
	if (recv_args->matching_set_id != matching_set_id) {
	  /*
	    e.g.)
	    
	    MPI_Irecv1 (matching_set_id = X) ---+
	    MPI_Irecv2 (matching_set_id = Y) ---+
	                                        |
                                                +---> MPI_MF (matching_set_id = X)
						
	  */
	  // REMPI_ERR("matching set id is already assigned for this recv: old matching_set_id: %d new matching_set_id: %d", 
	  // 	    recv_args->matching_set_id, matching_set_id);
	}
      } else {
	recv_args->matching_set_id = matching_set_id;
	mpi_call_id_to_matching_set_id[recv_args->mpi_call_id] = matching_set_id;
	//	REMPI_DBG("Assign new %d to call_id(recv): %d", matching_set_id, recv_args->mpi_call_id);
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
    //    REMPI_DBG("Assign set_id(mf) %d to call_id(recv): %d", matching_set_id, mpi_mf_call_id);
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
      //      REMPI_DBG("Assign new %d to call_id(mf): %d", matching_set_id, mpi_mf_call_id);
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
      //      REMPI_DBG("Assign set_id(recv) %d to call_id(mf): %d", matching_set_id, mpi_mf_call_id);
    }
  }
  
  return matching_set_id;
}

static int rempi_reqmg_get_matching_set_id_0(int *matching_set_id)
{
  *matching_set_id = 0;
  return 0;
}

static int rempi_reqmg_get_matching_set_id_1(int *matching_set_id, int *mpi_call_id, int mpi_call_type, int source, int tag, MPI_Comm comm, int incount, MPI_Request array_of_requests[])
{
  int request_type;
  *mpi_call_id = get_mpi_call_id();
  //  if (*mpi_call_id == 9 && rempi_my_rank == 1) REMPI_ASSERT(0);
  if (mpi_call_id_to_matching_set_id.find(*mpi_call_id) != 
      mpi_call_id_to_matching_set_id.end()) {
    *matching_set_id = mpi_call_id_to_matching_set_id.at(*mpi_call_id);
    if (rempi_mode == REMPI_ENV_REMPI_MODE_RECORD) {
      rempi_reqmg_assign_matching_set_id_to_recv(*matching_set_id, incount, array_of_requests);
    }
    //
    //REMPI_DBGI(1, "call_id: %d set_id: %d", *mpi_call_id, *matching_set_id);

    return 0;
  } else {
    // if (rempi_mode == REMPI_ENV_REMPI_MODE_REPLAY && 
    // 	mpi_call_type == REMPI_REQMG_MPI_CALL_TYPE_MF) {
    //    REMPI_ERR("No matching set id is assined in a record mode for mpi_call_id=%d (type: %d)", *mpi_call_id, mpi_call_type);
    // }
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

  //  REMPI_DBG("%d -> %d (type: %d)", *mpi_call_id, *matching_set_id, mpi_call_type);
  return 0;
}



static int rempi_reqmg_assign_matching_set_id_2(int source, int tag, MPI_Comm comm) 
{
  size_t hash;
  hash = get_matching_id_hash(source, tag, comm);
  if (matching_id_hash_to_matching_set_id.find(hash) != matching_id_hash_to_matching_set_id.end()) {
    return matching_id_hash_to_matching_set_id.at(hash);
  }
  return 0;
  // if (mpi_call_id_to_matching_set_id.find() != mpi_call_id_to_matching_set_id.end()) {
    
  // }

}

static int rempi_reqmg_get_matching_set_id_2(int *matching_set_id, int *mpi_call_id, int mpi_call_type, int source, int tag, MPI_Comm comm, int incount, MPI_Request array_of_requests[])
{
  switch(mpi_call_type) {
  case REMPI_REQMG_MPI_CALL_TYPE_RECV:
    break;
  case REMPI_REQMG_MPI_CALL_TYPE_MF:
    break;
  default:
    REMPI_ERR("No such mpi call type: %d", mpi_call_type);
  }
  return 0;
}

static void map_comm_and_tag_to_matching_set_id(int incount, MPI_Request array_of_requests[], int matching_set_id)
{
  int tag;
  MPI_Comm comm;
  size_t msg_id;
  for (int i = 0; i < incount; i++) {
    rempi_reqmg_get_matching_id(&array_of_requests[i], NULL, &tag, &comm);
    msg_id = rempi_mpi_get_msg_id(comm, tag);
    if (comm_tag_to_matching_set_id.find(msg_id) != comm_tag_to_matching_set_id.end()) {
      if (comm_tag_to_matching_set_id.at(msg_id) != matching_set_id) {
	REMPI_ERR("Inconsistent matching_set_id assignment");
      }
    } else {
      comm_tag_to_matching_set_id[msg_id] = matching_set_id;
    }
  }
}

static int rempi_reqmg_get_matching_set_id_3(int *matching_set_id, int *mpi_call_id, int mpi_call_type, int source, int tag, MPI_Comm comm, int incount, MPI_Request array_of_requests[])
{
  int request_type;
  size_t msg_id;
  int matching_set_id_exist;

  if (rempi_mode == REMPI_ENV_REMPI_MODE_RECORD) {
    *mpi_call_id = get_mpi_call_id();
    if (mpi_call_type == REMPI_REQMG_MPI_CALL_TYPE_RECV) {
      add_comm_and_tag_to_mpi_call_id(*mpi_call_id, comm, tag);
      //      add_comms_and_tags_to_mpi_call_id(*mpi_call_id, comm, tag, incount, array_of_requests, mpi_call_type);
    }

    //    REMPI_DBGI(0, "mpi_call_id: %d", *mpi_call_id);

    if (mpi_call_id_to_matching_set_id.find(*mpi_call_id) != 
	mpi_call_id_to_matching_set_id.end()) {
      *matching_set_id = mpi_call_id_to_matching_set_id.at(*mpi_call_id);
      if (rempi_mode == REMPI_ENV_REMPI_MODE_RECORD) {
	rempi_reqmg_assign_matching_set_id_to_recv(*matching_set_id, incount, array_of_requests);
      }
      //
      //REMPI_DBGI(1, "call_id: %d set_id: %d", *mpi_call_id, *matching_set_id);

      return 0;
    } else {
      // if (rempi_mode == REMPI_ENV_REMPI_MODE_REPLAY && 
      // 	mpi_call_type == REMPI_REQMG_MPI_CALL_TYPE_MF) {
      //    REMPI_ERR("No matching set id is assined in a record mode for mpi_call_id=%d (type: %d)", *mpi_call_id, mpi_call_type);
      // }
    }


    switch(mpi_call_type) {
    case REMPI_REQMG_MPI_CALL_TYPE_SEND:
      *matching_set_id = REMPI_REQMG_MATCHING_SET_ID_UNKNOWN; // Send is not assigned
      break;
    case REMPI_REQMG_MPI_CALL_TYPE_RECV:
      /*
	If we assign the set_id here, an error occur when:
	MPI_Recv1(req1); => set_id=1
	MPI_Recv2(req2); => set_id=2
	MPI_Waitall(req1, req2) => set_id=1or2 ?
      */
      *matching_set_id = REMPI_REQMG_MATCHING_SET_ID_UNKNOWN; // not assigned yet //next_matching_set_id++; /* Assign new id */
      break;
    case REMPI_REQMG_MPI_CALL_TYPE_MF:
      matching_set_id_exist = get_matching_set_id_from(tag, comm, incount, array_of_requests, mpi_call_type, matching_set_id);
      if (matching_set_id_exist < 0) {
	*matching_set_id = rempi_reqmg_assign_matching_set_id(*mpi_call_id, incount, array_of_requests);
	map_comm_and_tag_to_matching_set_id(incount, array_of_requests, *matching_set_id);
      }
      break;
    }
    // size_t msg_id =rempi_mpi_get_msg_id(comm, tag)
    // comm_tag_to_matching_set_id[msg_id] 

  } else if (rempi_mode == REMPI_ENV_REMPI_MODE_REPLAY) {
    *mpi_call_id = -1;
    matching_set_id_exist = get_matching_set_id_from(tag, comm, incount, array_of_requests, mpi_call_type, matching_set_id);
    if (matching_set_id_exist < 0) REMPI_ERR("No such matching set id");
  } else {
    REMPI_ERR("No such REMPI_MODE: %d", rempi_mode);
  }

  //  REMPI_DBG("%d -> %d (type: %d)", *mpi_call_id, *matching_set_id, mpi_call_type);
  return 0;
}


static int rempi_reqmg_assign_and_get_matching_set_id(int *matching_set_id, int *mpi_call_id, int mpi_call_type, int source, int tag, MPI_Comm comm, int incount, MPI_Request array_of_requests[])
{
  switch(rempi_is_test_id) {
  case 0:
    rempi_reqmg_get_matching_set_id_0(matching_set_id);
    break;
  case 1:
    rempi_reqmg_get_matching_set_id_3(matching_set_id, mpi_call_id, mpi_call_type, source, tag, comm, incount, array_of_requests);
    break;
  case 2:
    /*Record: ReMPI cannot change matching set id assigment */
    rempi_reqmg_get_matching_set_id_1(matching_set_id, mpi_call_id, mpi_call_type, source, tag, comm, incount, array_of_requests);
    break;
  case 3:
    /*Record: ReMPI can change matching set id assigment */
    rempi_reqmg_get_matching_set_id_2(matching_set_id, mpi_call_id, mpi_call_type, source, tag, comm, incount, array_of_requests);
    break;
  default:
    *matching_set_id = 0;
    //    REMPI_ERR("No such REMPI_TEST_ID = %d", rempi_is_test_id);
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



static int rempi_reqmg_register_recv_request(void *buf, int count, MPI_Datatype datatype, int source,
					     int tag, MPI_Comm comm, MPI_Request *request, int *matching_set_id)
{
  int size;
  void* pooled_buffer;
  MPI_Request *pooled_request;
  int ret;
  int mpi_call_id;

  //   *request = (MPI_Request)(mpi_request_id++);

  rempi_reqmg_assign_and_get_matching_set_id(matching_set_id, &mpi_call_id, REMPI_REQMG_MPI_CALL_TYPE_RECV, source, tag, comm, 0, NULL);
  //  REMPI_DBG("callstack tag: %d: cid: %d sid: %d", tag, mpi_call_id, *matching_set_id);
  request_to_recv_args_umap[*request] = new rempi_reqmg_recv_args(buf, count, datatype, source, tag, comm, *request, mpi_call_id, *matching_set_id);
  //   PMPI_Type_size(datatype, &size);
  //pooled_buffer = rempi_malloc(size * count);
  //  pooled_request = (MPI_Request*)rempi_malloc(sizeof(MPI_Request));
  //  ret = PMPI_Irecv(pooled_buffer, count, datatype, source, tag, comm, pooled_request);
  //  posted_recv_requests.push_back(new rempi_reqmg_recv_args(pooled_buffer, count, datatype, source, tag, comm, *pooled_request, matching_set_id));
  
  return ret;
}


static int rempi_reqmg_register_send_request(mpi_const void *buf, int count, MPI_Datatype datatype, int rank,
					     int tag, MPI_Comm comm, MPI_Request *request)
{
  size_t clock;

  /* 
     "clock" is not actually send clock because this clock is already ticked after MPI_Send/Isend 
     But, the purpose of getting clock here is to assigne "increasing id number" to each send request
     so that rempi_recorder_rep knows which send request should be completed first at least.
  */
  rempi_clock_get_local_clock(&clock);  
  request_to_send_id_umap[*request] = new rempi_reqmg_send_args(rank, clock);
  //  REMPI_DBG("send req: %p  --> rank: %d", *request, rank);
  return 0;
}


static int rempi_reqmg_deregister_recv_request(MPI_Request *request)
{
  
  int is_erased;

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
    REMPI_ERR("Request %p cannot be deregistered (MPI_REQUEST_NULL: %p)", *request, MPI_REQUEST_NULL);
  }
  return 1;
}

static int rempi_reqmg_is_record_and_replay(int length, MPI_Request array_of_requests[], int *request_info, int send_count, int recv_count, int null_count, int ignore_count, int matching_function_type)
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
  if (rempi_encode == REMPI_ENV_REMPI_ENCODE_CDC) {
    if (send_count > 0) return 0;

  } else if (rempi_encode == REMPI_ENV_REMPI_ENCODE_REP){
    /* is_record_and_replay must be consistent between record and replay 
        for consistent mpi_call_id (thereby matching_set_id) assignment
    */

    if (send_count > 0 && recv_count > 0) REMPI_ERR("send_count > 0 and recv_count > 0");


    // if (rempi_mode == REMPI_ENV_REMPI_MODE_RECORD) {
    //   if (send_count > 0) return 0;
    // } else {

    if (ignore_count > 0) {
      if (send_count + recv_count + null_count == 0) {
	return 0;
      } else {
      	REMPI_ERR("Ignore requests and send/recv requests are tested by a singe MF: (%d %d %d %d)",
      		  send_count, recv_count, null_count, ignore_count);
      }
    }
    

    
#if 0
    int is_deterministic = 1;
    for (int i = 0; i < length; i++) {
      int requested_rank;
      if (request_info[i] == REMPI_RECV_REQUEST) {
	rempi_reqmg_get_matching_id(&array_of_requests[i], &requested_rank, NULL, NULL);
      } else {
	requested_rank = 0;
      }
      if (requested_rank != MPI_ANY_SOURCE && 
	  (matching_function_type == REMPI_MPI_WAIT || matching_function_type == REMPI_MPI_WAITALL)) {
      } else {
	is_deterministic = 0;
      }
    }
    if (is_deterministic) return 0;
#endif
    

  }
#endif
  return 1;
}

int rempi_reqmg_register_request(mpi_const void *buf, int count, MPI_Datatype datatype, int rank,
				 int tag, MPI_Comm comm, MPI_Request *request, int request_type, int *matching_set_id)
{
  int ret;
  switch(request_type) {
  case REMPI_SEND_REQUEST:
    *matching_set_id = REMPI_REQMG_MATCHING_SET_ID_UNKNOWN;
    ret = rempi_reqmg_register_send_request(buf, count, datatype, rank, tag, comm, request);
    break;
  case REMPI_RECV_REQUEST:
    ret = rempi_reqmg_register_recv_request((void*)buf, count, datatype, rank, tag, comm, request, matching_set_id);
    break;
  default:
    REMPI_ERR("Cannot register MPI_Request: request_type: %d", request_type);
  }
  return ret;
}

void rempi_reqmg_report_message_matching(MPI_Request *request, MPI_Status *status)
{
  rempi_reqmg_recv_args *recv_args;
  recv_args = request_to_recv_args_umap.at(*request);
  add_comm_and_tag_to_mpi_call_id(recv_args->mpi_call_id, recv_args->comm, status->MPI_TAG);
  return;
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
#ifndef REMPI_LITE
#ifndef REMPI_HYPRE_TEST
    REMPI_DBG("Cannot deregister MPI_Request: request_type: %d", request_type);
#endif
#endif
    break;
  }
  return 0;
}

int rempi_reqmg_get_test_id(MPI_Request *request, int incount)
{
  REMPI_ERR("Non supprted");
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

void rempi_reqmg_get_request_info(int incount, MPI_Request *requests, int *sendcount, int *recvcount, int *nullcount, int *request_info, int *is_record_and_replay, int *matching_set_id, int matching_function_type)
{
  unordered_map<MPI_Request, rempi_reqmg_send_args*>::iterator send_endit = request_to_send_id_umap.end();
  unordered_map<MPI_Request, rempi_reqmg_recv_args*>::iterator recv_endit = request_to_recv_args_umap.end();
  int mpi_call_id;
  int ignore;
  int rank, tag;
  MPI_Comm comm;

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
#ifndef REMPI_LITE
#ifndef REMPI_HYPRE_TEST
      REMPI_ERR("Unknown request: %p index: %d: type: %d", requests[i], i, matching_function_type);
#endif
#endif
      //
    }
    //    REMPI_DBG("request: %p index: %d, info: %d", requests[i], i, request_info[i]);
  }
  REMPI_ASSERT(incount == *sendcount + *recvcount + *nullcount + ignore);
  *is_record_and_replay = rempi_reqmg_is_record_and_replay(incount, requests, request_info, *sendcount, *recvcount, *nullcount, ignore, matching_function_type);

  *matching_set_id = REMPI_REQMG_MATCHING_SET_ID_UNKNOWN;

  if (*is_record_and_replay) {
    rempi_reqmg_assign_and_get_matching_set_id(matching_set_id, &mpi_call_id, REMPI_REQMG_MPI_CALL_TYPE_MF, MPI_ANY_SOURCE, MPI_ANY_TAG, NULL, incount, requests);
  }

  //  *matching_set_id = rempi_reqmg_get_test_id(requests, incount);

  //  REMPI_DBGI(1, "incount:%d, sendcount:%d recvcount:%d nullcount:%d igncount:%d record:%d", incount, *sendcount, *recvcount, *nullcount, ignore, *is_record_and_replay)
  return;
}

rempi_reqmg_recv_args* rempi_reqmg_get_recv_args(MPI_Request *request) 
{
  return request_to_recv_args_umap.at(*request);
}


void rempi_reqmg_get_request_type(MPI_Request *request, int *request_type)
{
  unordered_map<MPI_Request, rempi_reqmg_send_args*>::iterator send_endit = request_to_send_id_umap.end();
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
      statuses[i].MPI_SOURCE = request_to_send_id_umap.at(requests[i])->dest;
      //      REMPI_DBG("rank %d (request: %p, index: %d)", statuses[i].MPI_SOURCE, requests[i], i);
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


int rempi_reqmg_get_matching_id(MPI_Request *request, int *rank, int *tag, MPI_Comm *comm)
{
  rempi_reqmg_recv_args *recv_args;
  if(request_to_recv_args_umap.find(*request) != 
     request_to_recv_args_umap.end()) {
    recv_args = request_to_recv_args_umap.at(*request);
    if (rank != NULL) *rank = recv_args->source;
    if (tag  != NULL) *tag  = recv_args->tag;
    if (comm != NULL) *comm = recv_args->comm;
  } else {
    REMPI_ERR("No such requst: %p", *request);
  }
  return 0;
}

int rempi_reqmg_get_buffer(MPI_Request *request, void** buffer)
{
  rempi_reqmg_recv_args *recv_args;
  if(request_to_recv_args_umap.find(*request) != 
     request_to_recv_args_umap.end()) {
    recv_args = request_to_recv_args_umap.at(*request);
    *buffer = recv_args->buffer;
  } else {
    REMPI_ERR("No such requst: %p", *request);
  }
  return 0;
}

// int rempi_reqmg_get_matching_set_id_map(int **mpi_call_ids, int **matching_set_ids, int *length)
// {
//   unordered_map<int, int>::iterator it     = mpi_call_id_to_matching_set_id.begin();
//   unordered_map<int, int>::iterator it_end = mpi_call_id_to_matching_set_id.end();
  
//   *length = mpi_call_id_to_matching_set_id.size();
//   *mpi_call_ids     = (int*)rempi_malloc(*length * sizeof(int));
//   *matching_set_ids = (int*)rempi_malloc(*length * sizeof(int));
  

//   for (int index = 0; it != it_end; it++, index++) {
//     (*mpi_call_ids)[index]     = it->first;
//     (*matching_set_ids)[index] = it->second;
//     //    REMPI_DBG("mpi_call_id: %d, matching_set_id: %d", it->first, it->second);
//   }
  
//   return 0;
// }

//unordered_map<int, unordered_set<size_t>*> mpi_call_id_to_comm_tag_umap;

static void change_matching_set_id(int old_id, int new_id) 
{
  size_t msg_id;
  int old_set_id;
  unordered_map<size_t, int>::iterator it     = comm_tag_to_matching_set_id.begin();
  unordered_map<size_t, int>::iterator it_end = comm_tag_to_matching_set_id.end();
  
  REMPI_ERR("matching_set_id assigment and (tag, comm) are inconsistent");
  
  for (; it != it_end; it++) {
    old_set_id = it->second;
    if (old_id == old_set_id) {
      msg_id = it->first;
      comm_tag_to_matching_set_id[msg_id] = new_id;
      //      REMPI_DBGI(0, "  map: %p --> %d (update)", msg_id, new_id);
    }
  }
}

static void assign_matching_set_id_to_msg_id(int mpi_call_id, int matching_set_id)
{
  size_t msg_id;
  int old_set_id;
  unordered_set<size_t> *msg_id_uset;
  //  unordered_map<int, unordered_set<size_t>*>::iterator it     = mpi_call_id_to_comm_tag_umap.begin();
  //  unordered_map<int, unordered_set<size_t>*>::iterator it_end = mpi_call_id_to_comm_tag_umap.end();
  unordered_set<size_t>::iterator it_uset;
  unordered_set<size_t>::iterator it_uset_end;
  //  REMPI_DBGI(0, "update:  %d", mpi_call_id);
  if (mpi_call_id_to_comm_tag_umap.find(mpi_call_id) == mpi_call_id_to_comm_tag_umap.end()) {
    /*This mpi_call_id is MF for send */
    return;
  }
  msg_id_uset = mpi_call_id_to_comm_tag_umap.at(mpi_call_id);
  it_uset = msg_id_uset->begin();
  it_uset_end = msg_id_uset->end();
  for (; it_uset != it_uset_end; it_uset++) {
    msg_id = *it_uset;
    if (comm_tag_to_matching_set_id.find(msg_id) != comm_tag_to_matching_set_id.end()) {
      old_set_id = comm_tag_to_matching_set_id.at(msg_id);
      if (old_set_id != matching_set_id) change_matching_set_id(old_set_id, matching_set_id);
    }
    comm_tag_to_matching_set_id[msg_id] = matching_set_id;
    //    REMPI_DBGI(0, "  map: %p --> %d", msg_id, matching_set_id);
  }    
}

int rempi_reqmg_get_matching_set_id_map(size_t **msg_ids, int **matching_set_ids, int *length)
{
  int mpi_call_id;
  int matching_set_id;
  
  unordered_map<int, int>::iterator it     = mpi_call_id_to_matching_set_id.begin();
  unordered_map<int, int>::iterator it_end = mpi_call_id_to_matching_set_id.end();


  for (int index = 0; it != it_end; it++, index++) {
    mpi_call_id = it->first;
    matching_set_id = it->second;
    assign_matching_set_id_to_msg_id(mpi_call_id, matching_set_id);
  }

  unordered_map<size_t, int>::iterator it_2     = comm_tag_to_matching_set_id.begin();
  unordered_map<size_t, int>::iterator it_end_2 = comm_tag_to_matching_set_id.end();
  *length = comm_tag_to_matching_set_id.size();
  *msg_ids          = (size_t*)rempi_malloc(*length * sizeof(size_t));
  *matching_set_ids = (int*)rempi_malloc(*length * sizeof(int));
  for (int index = 0; it_2 != it_end_2; it_2++) {
    size_t msg_id = it_2->first;
    int set_id = it_2->second;
     (*msg_ids)[index]     = msg_id;
     (*matching_set_ids)[index] = set_id;
    index++;
  }  
  
  return 0;
}

size_t rempi_reqmg_get_matching_set_id(int tag, MPI_Comm comm) 
{
  if (rempi_is_test_id == 0) return 0;
  size_t msg_id;
  msg_id = rempi_mpi_get_msg_id(comm, tag);
  if (comm_tag_to_matching_set_id.find(msg_id) == comm_tag_to_matching_set_id.end()) {
    REMPI_ERR("No such sid: (tag: %d, comm: %p): %p", tag, comm, msg_id);
  }
  return comm_tag_to_matching_set_id.at(msg_id);  
}



size_t rempi_reqmg_get_send_request_clock(MPI_Request *request)
{
  if (request_to_send_id_umap.find(*request) == request_to_send_id_umap.end()) {
    REMPI_ERR("No such request: %p", *request);
  }
  return request_to_send_id_umap.at(*request)->clock;
}

int rempi_reqmg_get_send_request_dest(MPI_Request *request)
{
  if (request_to_send_id_umap.find(*request) == request_to_send_id_umap.end()) {
    REMPI_ERR("No such request: %p", *request);
  }
  return request_to_send_id_umap.at(*request)->dest;
}

// int rempi_reqmg_set_matching_set_id_map(int *mpi_call_ids, int *matching_set_ids, int length)
// {
//   for (int i = 0; i < length; i++) {
//     //    REMPI_DBGI(1, "mpi_call_id: %d, matching_set_id: %d", mpi_call_ids[i], matching_set_ids[i]);
//     mpi_call_id_to_matching_set_id[mpi_call_ids[i]] = matching_set_ids[i];
//   }
//   return 0;
// }

int rempi_reqmg_set_matching_set_id_map(size_t *msg_ids, int *matching_set_ids, int length)
{
  for (int i = 0; i < length; i++) {
    //    REMPI_DBG("msg_id: %p, matching_set_id: %d", msg_ids[i], matching_set_ids[i]);
    comm_tag_to_matching_set_id[msg_ids[i]] = matching_set_ids[i];
  }
  return 0;
}

