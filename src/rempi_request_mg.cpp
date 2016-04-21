#include <stdlib.h>
#include <string.h>

#include <mpi.h>
#include <vector>
#include <unordered_map>

#include "rempi_err.h"
#include "rempi_mem.h"
#include "rempi_config.h"
#include "rempi_util.h"
#include "rempi_request_mg.h"

using namespace std;

/*Key: backtrace, Value: id*/
unordered_map<string, int>recv_id_umap;
int next_recv_id_to_assign = 0;
/*Index: recv_id, Value: matchingset_id */
vector<int> recv_id_to_set_id;
/*Index: matching_set_id, Value: elements of the matching set*/
vector<vector<rempi_matching_id*>*> rempi_matching_id_vec_vec;
/*Index: test_id, Value: set_id */
unordered_map<int, int> test_id_to_set_id;
unordered_map<MPI_Request, int>request_to_recv_id_umap;
unordered_map<MPI_Request, int>request_to_send_id_umap;
unordered_map<string, int> test_ids_map;
int next_test_id_to_assign = 0;

/*Key: id, Value: Recv MPI_Request */
// unordered_map<int, MPI_Request*>recv_request_id_to_recv_request_umap;
// int next_recv_request_id_to_assign = 0;

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

static int get_matching_set_id(MPI_Request *request, int count)
{
  int recv_id = -1;
  int matching_set_id;
  int tmp_matching_set_id;

  /*Check if matching_set_id is consistent across different requests*/
  for (int i = 0; i < count; i++) {
    if (request_to_recv_id_umap.find(request[0]) == request_to_recv_id_umap.end()) {
      tmp_matching_set_id = -1; /*This request is send request*/
    } else {
      recv_id = request_to_recv_id_umap[request[i]];
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


//int rempi_reqmg_update_matching_set(MPI_Request *request, int source, int tag, int comm_id)
static int rempi_reqmg_register_recv_request(MPI_Request *request, int source, int tag, int comm_id)
{

  int matching_set_id = -1;


  if (rempi_is_test_id == 0 || rempi_is_test_id == 1) {
      request_to_recv_id_umap[*request] = 0;
      //      REMPI_DBGI(3, "recv request: %p", *request);
      return 1;
  }

  int recv_id = get_recv_id();
  int size = recv_id_to_set_id.size();

  if (size == recv_id) {
    /*If this Irecv called first time 
        1. get matching set id
        2. add recv_id with  this matchign set id 
    */
    matching_set_id = assign_matching_set_id(recv_id, source, tag, comm_id);
    recv_id_to_set_id.push_back(matching_set_id);    
    //    REMPI_DBG("recv_id: %d, set_id: %d, tag: %d", recv_id, matching_set_id, tag);    
  } else if (size > recv_id) {
    /*If this Irecv called before*/
    matching_set_id = recv_id_to_set_id[recv_id];
    add_matching_id(matching_set_id, source, tag, comm_id);

#if 0
    /*TODO: Intersection detection among matching sets*/
    rempi_matching_id *matching_id;
    vector<rempi_matching_id*> *vec;
    matching_set_id = recv_id_to_set_id[recv_id];
    vec = rempi_matching_id_vec_umap[matching_set_id];
    if (is_contained(source, tag, comm_id, vec)) return matching_set_id;

    matching_id = new rempi_matching_id(source, tag, comm_id);
    /*Check if there is any intersection with other matching_set before pushing back*/
    int is_intersection = 0;
    for (unordered_map<int, vector<rempi_matching_id*>*>::const_iterator cit = rempi_matching_id_vec_umap.cbegin(),
         cit_end = rempi_matching_id_vec_umap.cend();
	 cit != cit_end;
	 cit++) {
      int mid = cit->first;
      vector<rempi_matching_id*> *v = cit->second;
      if(matching_id->has_intersection_in(v)) {
	REMPI_ERR("matching_id has an unexpected intersection");
      }
    }    
    /*Then, push back*/
    vec->push_back(matching_id);    
#endif
  } else {
    /* This does not occur because recv_id increase one by one */
    REMPI_ERR("recv_id_to_set_id (%d) < recv_id (%d)", size, recv_id);
  }
  //  REMPI_DBG("register: %p", *request);
  request_to_recv_id_umap[*request] = recv_id;
  return 1;
}

static int rempi_reqmg_register_send_request(MPI_Request *request, int dest, int tag, int comm_id)
{
  //  double s = rempi_get_time();
  request_to_send_id_umap[*request] = dest;
  //  REMPI_DBGI(3, "send request: %p", *request);
  //  double e = rempi_get_time();
  //  REMPI_DBGI(0, "send: %f", e - s);
  return 0;
}


static int rempi_reqmg_deregister_recv_request(MPI_Request *request)
{
  int is_erased;
  //  REMPI_DBGI(0, "dereg request: %p", *request);
  is_erased = request_to_recv_id_umap.erase(*request);
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

static int rempi_reqmg_is_record_and_replay(int length, int *request_ifno, int send_count, int recv_count, int null_count, int ignore_count)
{
  //  return send_count + recv_count + null_count > 0;
  if (ignore_count > 0) {
    if (send_count + recv_count + null_count == 0) {
      return 0;
    } else {
      REMPI_ERR("Ignore requests and send/recv requests are tested by a singe MF: (%d %d %d %d)",
                send_count, recv_count, null_count, ignore_count);
    }
  }
  return 1;
}

int rempi_reqmg_register_request(MPI_Request *request, int source, int tag, int comm_id, int request_type)
{
  switch(request_type) {
  case REMPI_SEND_REQUEST:
    rempi_reqmg_register_send_request(request, source, tag, comm_id);
    break;
  case REMPI_RECV_REQUEST:
    rempi_reqmg_register_recv_request(request, source, tag, comm_id);
    break;
  default:
    REMPI_ERR("Cannot register MPI_Request: request_type: %d", request_type);
  }
  return 0;
}

int rempi_reqmg_deregister_request(MPI_Request *request, int request_type)
{
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
  }
  return matching_set_id;
}

int rempi_reqmg_get_recv_request_count(int incount, MPI_Request *requests)
{
  int count = 0;
  unordered_map<MPI_Request, int>::iterator endit = request_to_recv_id_umap.end();
  for (int i = 0; i < incount; i++) {
    if(request_to_recv_id_umap.find(requests[i]) != endit) {
      count++;
    }
  }
  return count;
}

void rempi_reqmg_get_request_info(int incount, MPI_Request *requests, int *sendcount, int *recvcount, int *nullcount, int *request_info, int *is_record_and_replay)
{
  unordered_map<MPI_Request, int>::iterator send_endit = request_to_send_id_umap.end();
  unordered_map<MPI_Request, int>::iterator recv_endit = request_to_recv_id_umap.end();
  int ignore;
  *sendcount = *recvcount = *nullcount = ignore = 0;
  for (int i = 0; i < incount; i++) {
    if(request_to_send_id_umap.find(requests[i]) != send_endit) {
      request_info[i] = REMPI_SEND_REQUEST;
      (*sendcount)++;
    } else if(request_to_recv_id_umap.find(requests[i]) != recv_endit) {
      request_info[i] = REMPI_RECV_REQUEST;
      (*recvcount)++;
    } else if (requests[i] == MPI_REQUEST_NULL) {
      request_info[i] = REMPI_NULL_REQUEST;
      (*nullcount)++;
    } else if (requests[i] == NULL) {
      REMPI_ERR("Request is null");
    } else {
      /*For unsupported MF, such as MPI_Recv_init, MPI_start and then */
      request_info[i] = REMPI_IGNR_REQUEST; 
      ignore++;
      //      REMPI_DBG("Unknown request: %p index: %d", requests[i], i);
      //      REMPI_ERR("Unknown request: %p index: %d", requests[i], i);
    }
    //    REMPI_DBG("request: %p index: %d, info: %d", requests[i], i, request_info[i]);
  }
  REMPI_ASSERT(incount == *sendcount + *recvcount + *nullcount + ignore);
  *is_record_and_replay = rempi_reqmg_is_record_and_replay(incount, request_info, *sendcount, *recvcount, *nullcount, ignore);
  //  REMPI_DBGI(1, "incount:%d, sendcount:%d recvcount:%d nullcount:%d igncount:%d record:%d", incount, *sendcount, *recvcount, *nullcount, ignore, *is_record_and_replay)
  return;
}

void rempi_reqmg_get_request_type(MPI_Request *request, int *request_type)
{
  unordered_map<MPI_Request, int>::iterator send_endit = request_to_send_id_umap.end();
  unordered_map<MPI_Request, int>::iterator recv_endit = request_to_recv_id_umap.end();
  if(request_to_send_id_umap.find(*request) != send_endit) {
    *request_type = REMPI_SEND_REQUEST;
  } else if(request_to_recv_id_umap.find(*request) != recv_endit) {
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

// void rempi_reqmg_get_recv_request_count(int incount, MPI_Request *requests, int *sendcount, int *recvcount, int *nullcount, int *array_of_indices)
// {

//   unordered_map<MPI_Request, int>::iterator send_endit = request_to_send_id_umap.end();
//   unordered_map<MPI_Request, int>::iterator recv_endit = request_to_recv_id_umap.end();

//   for (int i = 0; i < incount; i++) {
//     if(request_to_send_id_umap.find(requests[i]) != send_endit) {
//       array_of_indices[i] = REMPI_SEND_REQUEST;
//       (*sendcount)++;
//     } else if(request_to_recv_id_umap.find(requests[i]) != recv_endit) {
//       array_of_indices[i] = REMPI_RECV_REQUEST;
//       (*recvcount)++;
//     } else if (requests[i] == MPI_REQUEST_NULL) {
//       array_of_indices[i] = REMPI_RECV_REQUEST;
//       (*nullcount)++;
//     }    
//   }
//   REMPI_ASSERT(incount == *sendcount + *recvcount + *nullcount);

//   return;
// }

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
