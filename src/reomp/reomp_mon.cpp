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

#include "reomp_mon.h"
#include "reomp_mem.h"
#include "mutil.h"

#include <unordered_map>
#include <unordered_set>

typedef struct 
{
  int len;
  char** callstack;
  size_t count;
} cs_info_t;

unordered_map<size_t, cs_info_t*> callstack_umap;

void reomp_mon_sample_callstack()
{
  cs_info_t *cs_info;
  size_t hash;
  char**cs;
  hash = reomp_util_btrace_hash();
  if (callstack_umap.find(hash) == callstack_umap.end()) {
    //    void **cs = (void**)malloc(sizeof(void*) * 128);
    callstack_umap[hash] = (cs_info_t*)malloc(sizeof(cs_info_t));
    callstack_umap[hash]->len = reomp_util_btrace_get(&cs, 128);
    callstack_umap[hash]->callstack = cs;
    callstack_umap[hash]->count = 0;
  }
  cs_info = callstack_umap.at(hash);
  cs_info->count++;
  return;
}

void reomp_mon_sample_callstack_print()
{
  unordered_map<size_t, cs_info_t*>::iterator it, it_end;
  for (it = callstack_umap.begin(),
	 it_end = callstack_umap.end();
       it != it_end;
       it++) {
    size_t hash;
    cs_info_t *cs_info;
    hash    = it->first;
    cs_info = it->second;

    MUTIL_DBG("Hash = %lu, count = %lu (len: %d)", hash, cs_info->count, cs_info->len);
    for (int i = 0; i < cs_info->len; i++) {
      MUTIL_DBG("  #%d %s", i, cs_info->callstack[i]);
    }
  }
  return;
}

