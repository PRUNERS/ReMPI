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

#include <unordered_map>

#include "reomp_rr.h"
#include "reomp_mem.h"
#include "apio.h"
#include "mutil.h"

#define MODE "REOMP_MODE"
#define REOMP_RECORD (0)
#define REOMP_REPLAY (1)
#define REOMP_DISABLE (2)

#define REOMP_MAX_THREAD (256);

static int fp = -1;
static int reomp_mode = -1;
static int replay_thread_num = -1;
static pthread_mutex_t file_read_mutex;
static pthread_mutex_t file_write_mutex;
static pthread_mutex_t reomp_omp_tid_umap_mutex;
static int critical_flag = 0;
static int reomp_omp_call_depth;
static unordered_map<int64_t, int> reomp_omp_tid_umap;
static char* record_file_dir = ".";
static char record_file_path[256];

static void reomp_record(void* ptr, size_t size) 
{
  //  reomp_mem_record_or_replay_all_local_vars(fp, 0);
  return;
}

static void reomp_replay(void* ptr, size_t size)
{
  //  reomp_mem_record_or_replay_all_local_vars(fp, 1);
  return;
}

static void reomp_init_env()
{
  char *env;
  if (!(env = getenv(MODE))) {
    reomp_mode = REOMP_DISABLE;
  } else if (!strcmp(env, "record") || atoi(env) == REOMP_RECORD) {
    reomp_mode = REOMP_RECORD;
  } else if (!strcmp(env, "replay") || atoi(env) == REOMP_REPLAY) {
    reomp_mode = REOMP_REPLAY;
  } else if (!strcmp(env, "disable") || atoi(env) == REOMP_DISABLE) {
    reomp_mode = REOMP_DISABLE;
  } else {
    MUTIL_DBG("Unknown REOMP_MODE: %s", env);
    MUTIL_ERR("Set REOMP_MODE=<0(record)|1(replay)\n");
  }
  return;
}

static void reomp_init_file(int control)
{
  int my_rank;
  if (control == REOMP_BEF_MAIN) {
    sprintf(record_file_path, "%s/rank_x.reomp", record_file_dir);
  } else if (control == REOMP_AFT_MPI_INIT) {
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    reomp_util_init(my_rank);
    ap_close(fp);
    sprintf(record_file_path, "%s/rank_%d.reomp", record_file_dir, my_rank);
  } else {
    MUTIL_DBG("No such control: %d", control);
  }
  int flags = -1;
  int mode = -1;
  if (reomp_mode == REOMP_RECORD) {
    flags = O_CREAT | O_WRONLY;
    mode  = S_IRUSR | S_IWUSR;
    pthread_mutex_init(&file_write_mutex, NULL);
  } else {
    flags = O_RDONLY;
    mode  = 0;
    pthread_mutex_init(&file_read_mutex, NULL);
  }

  fp = ap_open(record_file_path, flags, mode);

  if (fp == -1) {
    MUTIL_ERR("file open failed: errno=%d path=%s flags=%d moe=%d (%d %d)", 
	      errno, record_file_path, flags, mode, O_CREAT | O_WRONLY, O_WRONLY);
  }

  return;
}

static void reomp_finalize_file()
{
  ap_close(fp);
  return;
}

static void reomp_rr_init(int control)
{
  reomp_init_env();
  reomp_init_file(control);
  reomp_mem_init();
  pthread_mutex_init(&reomp_omp_tid_umap_mutex, NULL);
  return;
}

static void reomp_rr_finalize()
{
  reomp_finalize_file();
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

static int reomp_get_thread_num()
{
  int tid;
  int level;
  int64_t p_tid;
  
  level = omp_get_level();
  
  if (level == 1) {
    tid = omp_get_thread_num();
  } else {
    pthread_mutex_lock(&reomp_omp_tid_umap_mutex);
    p_tid = (int64_t)pthread_self();
    if (reomp_omp_tid_umap.find(p_tid) == reomp_omp_tid_umap.end()) {
      MUTIL_ERR("tid map does not have p_tid: %d", p_tid);
    }
    tid = reomp_omp_tid_umap.at((int64_t)pthread_self());
    pthread_mutex_unlock(&reomp_omp_tid_umap_mutex);
  }
  return tid;
}


static void reomp_gate_in(int control)
{
  int tid;

  tid = reomp_get_thread_num();
  if (reomp_mode == REOMP_RECORD) {
  } else {
    //    MUTIL_DBG("  (tid: %d)", tid);    
    while (tid != replay_thread_num) {
      if(!pthread_mutex_trylock(&file_read_mutex)) {
	ap_read(fp, &replay_thread_num, sizeof(int));
	//	MUTIL_DBG("tid: %d: read: %d", tid, replay_thread_num);
      }
      //      MUTIL_DBG("tid: %d: read: %d", tid, replay_thread_num);
    }
    //    MUTIL_DBG("tid: %d", tid);
    //    MUTIL_DBG("tid: %d: passed: %d", tid, replay_thread_num);
  }
  return;
}

static void reomp_gate_out(int control)
{
  int tid;
  tid = reomp_get_thread_num();
  //  MUTIL_DBG("tid: %d", tid);
  if (reomp_mode == REOMP_RECORD) {
    pthread_mutex_lock(&file_write_mutex);
    write(fp, &tid, sizeof(int));
    pthread_mutex_unlock(&file_write_mutex);
  } else {
    //    MUTIL_DBG("tid: %d: end: %d", tid, replay_thread_num);
    //    MUTIL_DBGI(1, "tid: %d", tid);
    if (tid != replay_thread_num) {
      MUTIL_ERR("tid: %d: end: %d", tid, replay_thread_num);
    }
    replay_thread_num = -1;
    pthread_mutex_unlock(&file_read_mutex);
  }
  return;
}

static void reomp_gate(int control)
{
  reomp_gate_in(control);
  reomp_gate_out(control);
  return;
}

static void reomp_before_fork()
{
  //  MUTIL_DBG("before");
  // int nth;
  // nth = omp_get_thread_num();
  // if (nth == 1) {
  //   reomp_omp_call_depth = 1;
  // } else {
  //   reomp_omp_call_depth = 2;
  // }
  return;
}

static void reomp_begin_omp()
{
  int64_t p_tid;
  int omp_tid;
  int level;
  
  level = omp_get_level();
  if (level == 1) {
    p_tid = (int64_t)pthread_self();
    omp_tid = omp_get_thread_num();
    pthread_mutex_lock(&reomp_omp_tid_umap_mutex);
    reomp_omp_tid_umap[p_tid] = omp_tid;
    //    MUTIL_DBGI(0, "  (p_tid: %d --> tid: %d)", p_tid, omp_tid);    
    pthread_mutex_unlock(&reomp_omp_tid_umap_mutex);
  }

  return;
}

static void reomp_end_omp()
{
  int64_t p_tid;
  int level;
  
  level = omp_get_level();
  if (level == 1) {
    p_tid = (int64_t)pthread_self();
    pthread_mutex_lock(&reomp_omp_tid_umap_mutex);
    reomp_omp_tid_umap.erase(p_tid);
    //    MUTIL_DBGI(0, "  erase (p_tid: %d)", p_tid);
    pthread_mutex_unlock(&reomp_omp_tid_umap_mutex);
  }

  return;
}

static void reomp_after_fork()
{
  //  MUTIL_DBG("after");
  // int nth;
  // nth = omp_get_thread_num();
  // if (nth == 1) {
  //   reomp_omp_call_depth = 0;
  // }
  return;
}

static void reomp_debug_print(void* ptr, size_t size)
{
  //MUTIL_DBG("reomp_debug_print is called: %lu", size);
  return;
}


void REOMP_CONTROL(int control, void* ptr, size_t size)
{
  if (reomp_mode == REOMP_DISABLE) return;

  switch(control) {
  case REOMP_BEF_MAIN:
    reomp_rr_init(control);
    break;
  case REOMP_AFT_MAIN:
    reomp_rr_finalize();
    break;
  case REOMP_AFT_MPI_INIT:
    reomp_rr_init(control);
    break;

  case REOMP_BEF_FORK:
    reomp_before_fork();
    break;
  case REOMP_BEG_OMP:
    reomp_begin_omp();
    break;
  case REOMP_END_OMP:
    reomp_end_omp();
    break;
  case REOMP_AFT_FORK:
    reomp_after_fork();
    break;

  case REOMP_GATE_IN:
    //    reomp_gate_in(control);
    break;
  case REOMP_GATE_OUT:
    //    reomp_gate_out(control);
    break;
  case REOMP_GATE:
    reomp_gate(control);
    break;
  case REOMP_DEBUG_PRINT:
    reomp_debug_print(ptr, size);
    break;
  }
  return;
}


void logop(int i)
{
    fprintf(stderr, "Computed: %d\n", i);
}


