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

static reomp_profile_t reomp_prof;
static omp_lock_t reomp_prof_lock;

void reomp_profile_init()
{
  reomp_prof.rr_type_umap = new unordered_map<size_t, size_t>;
  reomp_prof.lock_id_to_pre_rr_type_umap = new unordered_map<size_t, size_t>;
  reomp_prof.load_seq_count = 0;
  reomp_prof.store_seq_count = 0;
  omp_init_lock(&reomp_prof_lock);
  
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
  int tid;
  size_t previous_rr_type;
  if (reomp_prof.lock_id_to_pre_rr_type_umap->find(lock_id) == reomp_prof.lock_id_to_pre_rr_type_umap->end()) {
    reomp_prof.lock_id_to_pre_rr_type_umap->insert(make_pair(lock_id, REOMP_RR_TYPE_NONE));
  }
  previous_rr_type = reomp_prof.lock_id_to_pre_rr_type_umap->at(lock_id);
  if (previous_rr_type == rr_type) {
    if (rr_type == REOMP_RR_TYPE_LOAD) {
      reomp_prof.load_seq_count++;
    } else if (rr_type == REOMP_RR_TYPE_STORE) {
      reomp_prof.store_seq_count++;
    }
  }
  reomp_prof.lock_id_to_pre_rr_type_umap->at(lock_id) = rr_type;
  // MUTIL_DBG("type: %lu %lu (lock_id: %lu)", previous_rr_type, rr_type, lock_id);
  // MUTIL_DBG(" -->%lu", reomp_prof.lock_id_to_pre_rr_type_umap->at(lock_id));
  return;
}

void reomp_profile(size_t rr_type, size_t lock_id)
{
  omp_set_lock(&reomp_prof_lock);
  reomp_profile_rr_type(rr_type);
  reomp_profile_rr_rw_seq(rr_type, lock_id);
  omp_unset_lock(&reomp_prof_lock);
}

void reomp_profile_print()
{
  MUTIL_DBG("----------------------------");
  MUTIL_DBG("rr_type\tcount");
  for (auto &kv: *(reomp_prof.rr_type_umap)) {
    size_t type  = kv.first;
    size_t count = kv.second;
    MUTIL_DBG("%lu %lu", type, count);
  }
  MUTIL_DBG("----------------------------");
  MUTIL_DBG("Concutive load : %lu", reomp_prof.load_seq_count );
  MUTIL_DBG("Concutive store: %lu", reomp_prof.store_seq_count);
  MUTIL_DBG("----------------------------");  

  
  return;
}

void reomp_profile_finalize()
{
  omp_destroy_lock(&reomp_prof_lock);
  delete reomp_prof.rr_type_umap;
  delete reomp_prof.lock_id_to_pre_rr_type_umap;
  return;
}




