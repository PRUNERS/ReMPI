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

reomp_profile_t reomp_profile;
omp_lock_t reomp_profile_lock;

void reomp_profile_init()
{
  reomp_profile.rr_type_umap = new unordered_map<size_t, size_t>;
  reomp_profile.lock_id_to_pre_rr_type_umap = new unordered_map<size_t, size_t>;
  reomp_profile.load_seq_count = 0;
  reomp_profile.store_seq_count = 0;
  omp_init_lock(&reomp_profile_lock);
  
  return;
}

void reomp_profile_rr_type(size_t rr_type)
{
  omp_set_lock(&reomp_profile_lock);
  if (reomp_profile.rr_type_umap->find(rr_type) == reomp_profile.rr_type_umap->end()) {
    reomp_profile.rr_type_umap->insert(make_pair(rr_type, 0));
  }
  reomp_profile.rr_type_umap->at(rr_type)++;
  omp_unset_lock(&reomp_profile_lock);
  return;
}

void reomp_profile_rr_rw_seq(size_t rr_type, size_t lock_id)
{
  int tid;
  size_t previous_rr_type;
  omp_set_lock(&reomp_profile_lock);
  if (reomp_profile.lock_id_to_pre_rr_type_umap->find(lock_id) == reomp_profile.lock_id_to_pre_rr_type_umap->end()) {
    reomp_profile.lock_id_to_pre_rr_type_umap->insert(make_pair(lock_id, 0));
  }
  previous_rr_type = reomp_profile.lock_id_to_pre_rr_type_umap->at(lock_id);
  if (previous_rr_type == rr_type) {
    if (rr_type == REOMP_RR_TYPE_LOAD) {
      reomp_profile.load_seq_count++;
    } else if (rr_type == REOMP_RR_TYPE_STORE) {
      reomp_profile.store_seq_count++;
    }
  }
  reomp_profile.lock_id_to_pre_rr_type_umap->insert(make_pair(lock_id, rr_type));
  omp_unset_lock(&reomp_profile_lock);
  return;
}



void reomp_profile_print()
{
  //  decltype(*(profile.rr_type_umap))::iterator it, it_end;
  MUTIL_DBG("----------------------------");
  MUTIL_DBG("rr_type\tcount");
  for (auto &kv: *reomp_profile.rr_type_umap) {
    size_t type  = kv.first;
    size_t count = kv.second;
    MUTIL_DBG("%lu $lu", type, count);
  }
  return;
}

void reomp_profile_finalize()
{
  omp_destroy_lock(&reomp_profile_lock);
  delete reomp_profile.rr_type_umap;
  delete reomp_profile.lock_id_to_pre_rr_type_umap;
  return;
}




