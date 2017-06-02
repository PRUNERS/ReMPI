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
  if (fp == NULL) {
    fp = fopen("rank_x.reomp", "w");
  }

  reomp_mem_record_or_replay_all_local_vars(fp, 0);
  return;
}

static void reomp_replay(void* ptr, size_t size)
{
  if (fp == NULL) {
    fp = fopen("rank_x.reomp", "r");
  }
  reomp_mem_record_or_replay_all_local_vars(fp, 1);
  return;
}

void reomp_rr_init()
{
  MUTIL_DBG("init");
  reomp_mem_init();
  return;
}

void reomp_rr_finalize()
{
  return;
}


void reomp_rr(void* ptr, size_t size)                                                                                                               
{
  int tid;

  //reomp_mem_disable_hook();
  tid = omp_get_thread_num();

  if (tid) goto end;

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

end:
  //  reomp_mem_enable_hook();

  return;
}                                                                                                                                                      

void logop(int i)
{
    fprintf(stderr, "Computed: %d\n", i);
}


