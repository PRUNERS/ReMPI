#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h> 
#include <fcntl.h>
#include <omp.h>
#include <mpi.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <linux/limits.h>

#include <unordered_map>

#include "mutil.h"
#include "reomp.h"
#include "reomp_profile.h"

using namespace std;

typedef struct {
  size_t previous_rr_type;
  size_t count;
  size_t load_seq_count;
  size_t store_seq_count;  
} reomp_profile_access_t;

typedef struct {
  size_t count = 0;
  size_t previous_lock_id = -1;
  size_t lock_id_seq_count = 0;
  size_t previous_rr_type = REOMP_RR_TYPE_NONE;
  size_t load_seq_count = 0;
  size_t store_seq_count = 0;
  unordered_map<size_t, size_t> *rr_type_umap;
  unordered_map<size_t, reomp_profile_access_t*> *lock_id_to_access_umap;
} reomp_profile_t;

static reomp_profile_t reomp_prof;
static omp_lock_t reomp_prof_lock;

void reomp_profile_init()
{
  reomp_prof.rr_type_umap = new unordered_map<size_t, size_t>;
  reomp_prof.lock_id_to_access_umap = new unordered_map<size_t, reomp_profile_access_t*>;
  omp_init_lock(&reomp_prof_lock);
  return;
}

static reomp_profile_access_t* reomp_profile_create_profile_access()
{
  reomp_profile_access_t *acc;
  acc = (reomp_profile_access_t*)malloc(sizeof(reomp_profile_access_t));
  acc->previous_rr_type = REOMP_RR_TYPE_NONE;
  acc->count  = 0;
  acc->load_seq_count = 0;
  acc->store_seq_count = 0;
  return acc;
}

static void reomp_profile_free_profile_access(reomp_profile_access_t *acc)
{
  free(acc);
  return;
}


static void reomp_profile_rr_type(size_t rr_type)
{
  if (reomp_prof.rr_type_umap->find(rr_type) == reomp_prof.rr_type_umap->end()) {
    reomp_prof.rr_type_umap->insert(make_pair(rr_type, 0));
  }
  reomp_prof.rr_type_umap->at(rr_type)++;
  return;
}

static void reomp_profile_rr_rw_seq(size_t rr_type, size_t lock_id)
{
  reomp_profile_access_t *acc;
  if (reomp_prof.lock_id_to_access_umap->find(lock_id) == reomp_prof.lock_id_to_access_umap->end()) {
    acc = reomp_profile_create_profile_access();
    reomp_prof.lock_id_to_access_umap->insert(make_pair(lock_id, acc));
  }
  acc = reomp_prof.lock_id_to_access_umap->at(lock_id);
  if (acc->previous_rr_type == rr_type) {
    if (rr_type == REOMP_RR_TYPE_LOAD) {
      acc->load_seq_count++;
    } else if (rr_type == REOMP_RR_TYPE_STORE) {
      acc->store_seq_count++;
    } else if (rr_type == REOMP_RR_TYPE_ATOMICLOAD) {
      acc->load_seq_count++;
    } else if (rr_type == REOMP_RR_TYPE_ATOMICSTORE) {
      acc->load_seq_count++;
    }
  }
  acc->previous_rr_type = rr_type;
  acc->count++;
  //  reomp_prof.lock_id_to_access_umap->at(lock_id) = rr_type;
  // MUTIL_DBG("type: %lu %lu (lock_id: %lu)", previous_rr_type, rr_type, lock_id);
  // MUTIL_DBG(" -->%lu", reomp_prof.lock_id_to_access_umap->at(lock_id));
  return;
}

static void reomp_profile_whole(size_t rr_type, size_t lock_id)
{
  reomp_prof.count++;

  //  if (reomp_prof.count % 100000 == 0) MUTIL_DBG("%lu", reomp_prof.count++);
  
  if (reomp_prof.previous_rr_type == rr_type) {
    if (rr_type == REOMP_RR_TYPE_LOAD) {
      reomp_prof.load_seq_count++;
    } else if (rr_type == REOMP_RR_TYPE_STORE) {
      reomp_prof.store_seq_count++;
    }
  }
  reomp_prof.previous_rr_type = rr_type;

  if (reomp_prof.previous_lock_id == lock_id) {
    reomp_prof.lock_id_seq_count++;
  }
  reomp_prof.previous_lock_id = lock_id;
  return;
}

void reomp_profile(size_t rr_type, size_t lock_id)
{
  omp_set_lock(&reomp_prof_lock);
  reomp_profile_whole(rr_type, lock_id);
  reomp_profile_rr_type(rr_type);
  reomp_profile_rr_rw_seq(rr_type, lock_id);
  omp_unset_lock(&reomp_prof_lock);
}

void reomp_profile_print()
{
  size_t sum = 0;
  MUTIL_DBG("----------------------------");
  MUTIL_DBG("rr_type\tcount");
  for (auto &kv: *(reomp_prof.rr_type_umap)) {
    size_t type  = kv.first;
    size_t count = kv.second;
    MUTIL_DBG("%15s %lu", reomp_get_rr_type_str(type), count);
  }
  MUTIL_DBG("----------------------------");
  sum = 0;
  for (auto &kv: *(reomp_prof.lock_id_to_access_umap)) {
    size_t lock_id  = kv.first;
    reomp_profile_access_t *acc = kv.second;
    MUTIL_DBG("Lock_id: %lu (sequence=%lu out of %lu)", lock_id,
     	      acc->load_seq_count + acc->store_seq_count, acc->count);
    MUTIL_DBG("  Concutive load  = %lu", acc->load_seq_count );
    MUTIL_DBG("  Concutive store = %lu", acc->store_seq_count);
    sum += acc->load_seq_count + acc->store_seq_count;
  }
  MUTIL_DBG("==> Total: Sum = %lu/%lu (%lu\%)", sum, reomp_prof.count, sum * 100 / reomp_prof.count);
  MUTIL_DBG("----------------------------");
  MUTIL_DBG("Total count: %lu", reomp_prof.count);
  MUTIL_DBG("Lock_id sequence count: %lu/%lu (%d\%)",
	    reomp_prof.lock_id_seq_count, reomp_prof.count,
	    reomp_prof.lock_id_seq_count * 100 / reomp_prof.count);
  MUTIL_DBG("Consecutive load  : %lu", reomp_prof.load_seq_count);
  MUTIL_DBG("Consecutive stored: %lu", reomp_prof.store_seq_count);
  MUTIL_DBG("==> Total: %lu/%lu (%lu\%)",
	    reomp_prof.load_seq_count + reomp_prof.store_seq_count,
	    reomp_prof.count,
	    (reomp_prof.load_seq_count + reomp_prof.store_seq_count) * 100 / reomp_prof.count);  
  MUTIL_DBG("----------------------------");  

  
  return;
}

void reomp_profile_finalize()
{
  omp_destroy_lock(&reomp_prof_lock);
  delete reomp_prof.rr_type_umap;
  delete reomp_prof.lock_id_to_access_umap;
  return;
}




