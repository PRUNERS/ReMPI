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

#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <atomic>
#include <list>

#include "reomp_rr.h"
#include "reomp_mem.h"
#include "reomp_mon.h"
#include "apio.h"
#include "mutil.h"

#define MODE "REOMP_MODE"
#define REOMP_DIR "REOMP_DIR"
#define REOMP_RECORD (0)
#define REOMP_REPLAY (1)
#define REOMP_DISABLE (2)

#define REOMP_MAX_THREAD (256);

#define REOMP_WITH_LOCK (1)
#define REOMP_WITHOUT_LOCK (2)

#define USE_OMP_GET_THREAD_NUM

//#define REOMP_USE_APIO
//#define REOMP_SKIP_RECORD
//#define REOMP_USE_MULTI_LOCKS

#ifdef REOMP_USE_APIO
static int fp = -1;
#else
static FILE *fp = NULL;
#endif

static int reomp_mode = -1;
static int replay_thread_num = -1;
//static pthread_mutex_t file_read_mutex;
//static pthread_mutex_t file_write_mutex;
//static pthread_mutex_t reomp_omp_tid_umap_mutex;
static omp_lock_t file_read_mutex_lock;
static omp_nest_lock_t file_write_mutex_lock; // old
static omp_nest_lock_t *record_locks = NULL; // new
static omp_lock_t reomp_omp_tid_umap_mutex_lock; // not used
static int critical_flag = 0;
static int reomp_omp_call_depth;
static unordered_map<int64_t, int> reomp_omp_tid_umap;
//static char* record_file_dir = "/l/ssd";
static char* record_file_dir = "/tmp";
//static char* record_file_dir = "./";
static char record_file_path[256];


static double tmp_time = 0, tmp_time2 = 0;
static double io_time = 0;
static double lock_time = 0, tmp_lock_time = 0;
static double control_time = 0;
static int time_tid = 0;

static vector<list<char*>*> callstack;
static vector<size_t> callstack_hash;
static unordered_map<size_t, size_t> callstack_hash_umap;
static int is_callstack_init = 0;

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

#ifdef USE_OMP_GET_THREAD_NUM
static int reomp_get_thread_num()
{
  return omp_get_thread_num();
}
#else
static int reomp_get_thread_num()
{
  int tid;
  int level;
  int64_t p_tid;
  
  level = omp_get_level();
  switch(level) {
  case 0:
    tid = 0;
    break;
  case 1:
    tid = omp_get_thread_num();
    break;
  default:
    omp_set_lock(&reomp_omp_tid_umap_mutex_lock);
    p_tid = (int64_t)pthread_self();
    if (reomp_omp_tid_umap.find(p_tid) == reomp_omp_tid_umap.end()) {
      MUTIL_ERR("tid map does not have p_tid: %d (level: %d)", p_tid, level);
    }
    tid = reomp_omp_tid_umap.at((int64_t)pthread_self());
    omp_unset_lock(&reomp_omp_tid_umap_mutex_lock);
    break;
  }

  return tid;
}
#endif


static void reomp_init_callstack()
{
  int tid;
  tid = reomp_get_thread_num();
  if (tid == 0) {
    MUTIL_DBG("Init");
    callstack_hash.resize(128);
    for (int i = 0; i < 128; i++) {
      callstack.push_back(new list<char*>());
      //      callstack_hash_umap.push_back(new unordered_map<size_t, size_t>());
    }
    is_callstack_init = 1;
  }

  return;
}

static void reomp_init_env()
{
  char *env;
  
  if (!(env = getenv(REOMP_DIR))) {
    record_file_dir = ".";
  } else {
    record_file_dir = env;    
  }  

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
#ifdef REOMP_USE_APIO
    if (fp == -1) return;
#else
    if (fp == NULL) return;
#endif
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    reomp_util_init(my_rank);
#ifdef REOMP_USE_APIO
    ap_close(fp);
#else
    fclose(fp);
#endif
    sprintf(record_file_path, "%s/rank_%d.reomp", record_file_dir, my_rank);
  } else {
    MUTIL_DBG("No such control: %d", control);
  }
  int flags = -1;
  int mode = -1;
  char *fmode;
  //  pthread_mutexattr_t mattr;
  //  pthread_mutexattr_init(&mattr);

  if (reomp_mode == REOMP_RECORD) {
    flags = O_CREAT | O_WRONLY;
    mode  = S_IRUSR | S_IWUSR;
    fmode = (char*)"w+";
  } else {
    flags = O_RDONLY;
    mode  = 0;
    fmode = (char*)"r";
    //    pthread_mutexattr_settype(&mattr, NULL);
    //    pthread_mutex_init(&file_read_mutex, &mattr);
    omp_init_lock(&file_read_mutex_lock);

  }

#ifdef REOMP_USE_APIO
  fp = ap_open(record_file_path, flags, mode);
  if (fp == -1) {
    MUTIL_ERR("file open failed: errno=%d path=%s flags=%d moe=%d (%d %d)", 
	      errno, record_file_path, flags, mode, O_CREAT | O_WRONLY, O_WRONLY);
  }
#else
  fp = fopen(record_file_path, fmode);
  if (fp == NULL) {
    MUTIL_ERR("file fopen failed: errno=%d path=%s flags=%d moe=%d (%d %d)", 
	      errno, record_file_path, flags, mode, O_CREAT | O_WRONLY, O_WRONLY);
  }
#endif

  return;
}

static void reomp_finalize_file()
{
#ifdef REOMP_USE_APIO
  ap_close(fp);
#else
  tmp_time = reomp_util_get_time();
  fclose(fp);
  io_time += reomp_util_get_time() - tmp_time;
  MUTIL_DBG("%f + %f = %f", lock_time, io_time, control_time);
#endif

  REOMP_MON_SAMPLE_CALLSTACK_PRINT();
  unordered_map<size_t, size_t>::iterator it, it_end;
  for (it = callstack_hash_umap.begin(),
	 it_end = callstack_hash_umap.end();
       it != it_end;
       it++) {
    MUTIL_DBG("Hash: %lu, Count: %lu", it->first, it->second);
  }

  return;
}

static void reomp_init_locks(size_t num_locks)
{
  if (record_locks != NULL) return;
  record_locks = (omp_nest_lock_t*)malloc(sizeof(omp_nest_lock_t) * num_locks);
  for (int i = 0; i < num_locks; i++) {
    omp_init_nest_lock(&record_locks[i]);
  }
  return;
}

static void reomp_rr_init(int control, size_t num_locks)
{
  reomp_init_callstack();
  reomp_init_env();
  reomp_init_file(control);
  reomp_init_locks(num_locks);
  reomp_mem_init();
  //  pthread_mutex_init(&reomp_omp_tid_umap_mutex, NULL);
  omp_init_lock(&reomp_omp_tid_umap_mutex_lock);
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




size_t count = 0;
static void reomp_gate_in(int control, void* ptr, size_t lock_id, int lock)
{
  int tid;
  size_t ret;

  //if(omp_get_level() == 0) return;
  if(omp_get_num_threads() == 1) return;

  tid = reomp_get_thread_num();
  
  if (reomp_mode == REOMP_RECORD) {
    if (tid == time_tid) tmp_time = reomp_util_get_time();
    //    if(lock == REOMP_WITH_LOCK) omp_set_nest_lock(&file_write_mutex_lock);

    if(lock == REOMP_WITH_LOCK) omp_set_nest_lock(&record_locks[lock_id]);
    
    // if (callstack_hash_umap.find(callstack_hash[tid]) == callstack_hash_umap.end()) {
    //   int level = 0;
    //   callstack_hash_umap[callstack_hash[tid]] = 0;
    //   list<char*>::reverse_iterator it, it_end;
    //   MUTIL_DBG("Callstack: hash: %lu", callstack_hash[tid]);
    //   for (it = callstack[tid]->rbegin(), 
    // 	     it_end = callstack[tid]->rend();
    // 	   it != it_end;
    // 	   it++) {
    // 	char *func_name;
    // 	func_name = *it;
    // 	MUTIL_DBG("  #%d %s", level++, func_name);
    //   }
    // } else {
    //   callstack_hash_umap.at(callstack_hash[tid])++;
    // }

    //    PrintBacktrace();
    REOMP_MON_SAMPLE_CALLSTACK();
    //    MUTIL_DBG("LOCK");
    //    if(lock == REOMP_WITH_LOCK) omp_set_nest_lock(&record_locks[tid]);
    if (tid == time_tid) lock_time += reomp_util_get_time() - tmp_time;
  } else {
    //    MUTIL_DBG("  (tid: %d)", tid);    
    if (tid == time_tid) tmp_lock_time = reomp_util_get_time();
    while (tid != replay_thread_num) {
      if(omp_test_lock(&file_read_mutex_lock) != 0) {
#ifdef REOMP_USE_APIO	
	ap_read(fp, &replay_thread_num, sizeof(int));
#else 
	if (tid == time_tid) tmp_time = reomp_util_get_time();
	ret = fread(&replay_thread_num, sizeof(int), 1, fp);
	if (tid == time_tid) io_time += reomp_util_get_time() - tmp_time;
	if (feof(fp)) MUTIL_ERR("fread reached the end of record file");
	if (ferror(fp))  MUTIL_ERR("fread failed");
#endif
      }
    }
    if (tid == time_tid) lock_time += reomp_util_get_time() - tmp_lock_time;
  }
#ifdef ENABLE_MEM_SYNC
  __sync_synchronize();
#endif
  //  MUTIL_DBG("Gate-In: tid: %d: lock: %d, replay_thread_num: %d", tid, lock, replay_thread_num);
  //reomp_util_btrace();
  return;
}

static void reomp_gate_out(int control, void* ptr, size_t lock_id, int lock)
{
  int tid;
  size_t ret;

  if(omp_get_num_threads() == 1) return;

#ifdef ENABLE_MEM_SYNC
  __sync_synchronize();
#endif

  tid = reomp_get_thread_num();
  //  MUTIL_DBG("Gate-Out: tid: %d: lock: %d, replay_thread_num: %d", tid, lock, replay_thread_num);
  if (reomp_mode == REOMP_RECORD) {
    //    MUTIL_DBG("unlock: %d", tid);
    //    write(fp, &tid, sizeof(int));
#ifndef REOMP_SKIP_RECORD
#ifdef REOMP_USE_APIO
    ap_write(fp, &tid, sizeof(int));
#else
    if (tid == time_tid) tmp_time = reomp_util_get_time();
    ret = fwrite(&tid, sizeof(int), 1, fp);
    if (tid == time_tid) io_time += reomp_util_get_time() - tmp_time;
    if (ret != 1) MUTIL_ERR("fwrite failed");
#endif
#endif
    if (tid == time_tid) tmp_time = reomp_util_get_time();
    //    if(lock == REOMP_WITH_LOCK) omp_unset_nest_lock(&file_write_mutex_lock);
    if(lock == REOMP_WITH_LOCK) omp_unset_nest_lock(&record_locks[lock_id]);
    //    if(lock == REOMP_WITH_LOCK) omp_unset_nest_lock(&record_locks[tid]);
    if (tid == time_tid) lock_time += reomp_util_get_time() - tmp_time;
  } else {
    //    MUTIL_DBG("tid: %d: end: %d (unlock: %d)", tid, replay_thread_num, lock);
    //    MUTIL_DBGI(1, "tid: %d", tid);
    //    if (tid != replay_thread_num) {
      //      MUTIL_ERR("tid: %d: end: %d", tid, replay_thread_num);
    //    }
    replay_thread_num = -1;
    //    pthread_mutex_unlock(&file_read_mutex);
    if (tid == time_tid) tmp_time = reomp_util_get_time();
    omp_unset_lock(&file_read_mutex_lock);
    if (tid == time_tid) lock_time += reomp_util_get_time() - tmp_time;
  }
  return;
}

// static void reomp_gate(int control, int lock)
// {
//   reomp_gate_in(control, lock);
//   reomp_gate_out(control, lock);
//   return;
// }

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
    //    p_tid = (int64_t)pthread_self();
    p_tid = (int64_t)syscall(SYS_gettid);
    omp_tid = omp_get_thread_num();
    //pthread_mutex_lock(&reomp_omp_tid_umap_mutex);
    omp_set_lock(&reomp_omp_tid_umap_mutex_lock);
    reomp_omp_tid_umap[p_tid] = omp_tid;
    //    MUTIL_DBGI(0, "  (p_tid: %d --> tid: %d)", p_tid, omp_tid);    
    //    pthread_mutex_unlock(&reomp_omp_tid_umap_mutex);
    omp_unset_lock(&reomp_omp_tid_umap_mutex_lock);
  }

  return;
}

static void reomp_end_omp()
{
  int64_t p_tid;
  int level;
  
  level = omp_get_level();
  if (level == 1) {
    //    p_tid = (int64_t)pthread_self();
    p_tid = (int64_t)syscall(SYS_gettid);
    //    pthread_mutex_lock(&reomp_omp_tid_umap_mutex);
    omp_set_lock(&reomp_omp_tid_umap_mutex_lock);
    reomp_omp_tid_umap.erase(p_tid);
    //    MUTIL_DBGI(0, "  erase (p_tid: %d)", p_tid);
    //    pthread_mutex_unlock(&reomp_omp_tid_umap_mutex);
    omp_unset_lock(&reomp_omp_tid_umap_mutex_lock);
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



static void reomp_begin_func_call(void* ptr, size_t size)
{
  int tid;
  if (!is_callstack_init) return;
  tid = reomp_get_thread_num();
  //  MUTIL_DBG("Push: %s (%lu %d)", (char*)ptr, size, tid);
  callstack[tid]->push_back((char*)ptr);
  callstack_hash[tid] += size;
}

static void reomp_end_func_call(void* ptr, size_t size)
{
  int tid;
  if (!is_callstack_init) return;
  tid = reomp_get_thread_num();
  if (callstack[tid]->back() != ptr) {
    MUTIL_ERR("Call stack is broken: %s:%s", callstack[tid]->front(), (char*)ptr);
  }
  callstack[tid]->pop_back();
  callstack_hash[tid] -= size;
  //  MUTIL_DBG("Pop: %s (%lu)", (char*)ptr, size);
}

static void reomp_debug_print(void* ptr, size_t size)
{
  //  MUTIL_DBG("PTR: %p, SIZE: %lu", ptr, size);
  return;
}


void REOMP_CONTROL(int control, void* ptr, size_t size)
{
  if (reomp_mode == REOMP_DISABLE) return;

  int tid = reomp_get_thread_num();
  if (tid == time_tid) tmp_time2 = reomp_util_get_time();

  switch(control) {
  case REOMP_BEF_MAIN: // 0
    reomp_rr_init(control, size);
    break;
  case REOMP_AFT_MAIN: // 1
    reomp_rr_finalize();
    break;
  case REOMP_AFT_MPI_INIT: // 2
    reomp_rr_init(control, size);
    break;
  case REOMP_GATE_IN: // 10
#ifndef REOMP_USE_MULTI_LOCKS
    size = 0;
#endif
    reomp_gate_in(control, ptr, size, REOMP_WITH_LOCK);
    //reomp_gate_in(control, ptr, size, REOMP_WITHOUT_LOCK);
    break;
  case REOMP_GATE_OUT: // 11
#ifndef REOMP_USE_MULTI_LOCKS
    size = 0;
#endif
    reomp_gate_out(control, ptr, size, REOMP_WITH_LOCK);
    //reomp_gate_out(control, ptr, size, REOMP_WITHOUT_LOCK);
    break;
  case REOMP_GATE: // 12
    MUTIL_ERR("No such control");
    //    reomp_gate(control, REOMP_WITH_LOCK);
    break;
  case REOMP_BEF_CRITICAL_BEGIN: // 13
    reomp_gate_in(control, ptr, size, REOMP_WITHOUT_LOCK);
    break;
  case REOMP_AFT_CRITICAL_BEGIN: // 14
    reomp_gate_out(control, ptr, size, REOMP_WITHOUT_LOCK);
    break;

  case REOMP_BEF_FORK: // 20
    reomp_before_fork();
    break;
  case REOMP_AFT_FORK: // 21
    reomp_after_fork();
    break;
#ifndef USE_OMP_GET_THREAD_NUM
  case REOMP_BEG_OMP: // 22
    reomp_begin_omp();
    break;
  case REOMP_END_OMP: // 23
    reomp_end_omp();
    break;
#endif
  case REOMP_BEG_FUNC_CALL: // 24
    //    reomp_begin_func_call(ptr, size);
    break;
  case REOMP_END_FUNC_CALL: // 25
    //    reomp_end_func_call(ptr, size);
    break;

  case REOMP_DEBUG_PRINT: 
    reomp_debug_print(ptr, size);
    break;

  }
  if (tid == time_tid) control_time += reomp_util_get_time() - tmp_time2;

  return;
}


void logop(int i)
{
    fprintf(stderr, "Computed: %d\n", i);
}


