#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <omp.h>

#include "reomp_rr.h"
#include "reomp_mem.h"
#include "mutil.h"

#define MODE "REOMP_MODE"


FILE *fp = NULL;



static void reomp_record(void* ptr, size_t size) 
{
  size_t s;
  if (fp == NULL) {
    fp = fopen("rank_x.reomp", "w");
  }

  fwrite(ptr, sizeof(ptr), 1, fp);
  if (reomp_mem_is_alloced(*(void**)ptr)) {
    reomp_mem_get_alloc_size(*(void**)ptr, &s);
    fwrite(*(void**)(ptr), s, 1, fp);
  }
  MUTIL_DBG("writing size: %lu", s);
  

  return;
}

static void reomp_replay(void* ptr, size_t size)
{
  size_t s;
  if (fp == NULL) {
    fp = fopen("rank_x.reomp", "r");
  }

  fread(ptr, sizeof(ptr), 1, fp);
  if (reomp_mem_is_alloced(*(void**)ptr)) {
    reomp_mem_get_alloc_size(*(void**)ptr, &s);
    fread(*(void**)(ptr), s, 1, fp);
  }
  return;
}

void reomp_rr(void* ptr, size_t size)                                                                                                               
{
  int tid;
  
  tid = omp_get_thread_num();
  if (tid) return;

  char *env;
  if (!(env = getenv(MODE))) {
    // fprintf(stderr, "REOMP_MODE=<record | replay>\n");
    // exit(0);
  } else if (!strcmp(env, "record") || atoi(env) == 0) {
    reomp_record(ptr, size);
    //    fprintf(stderr, "Record: %f (size: %lu tid: %d)\n", *(double*)ptr, sizeof(ptr), tid);
  } else if (!strcmp(env, "replay") || atoi(env) == 1) {
    reomp_replay(ptr, size);
    //    fprintf(stderr, "Replay: %f (size: %lu tid: %d)\n", *(double*)ptr, sizeof(ptr), tid);
  } else {
    fprintf(stderr, "REOMP_MODE=<record:0|replay:1>\n");
    exit(0);
  }

  return;
}                                                                                                                                                      

void logop(int i)
{
    fprintf(stderr, "Computed: %d\n", i);
}


