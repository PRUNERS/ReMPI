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
#include <unordered_set>
#include <vector>
#include <atomic>
#include <list>

#include "reomp_rr.h"
#include "reomp_gate.h"
#include "reomp_mem.h"
#include "reomp_mon.h"
#include "mutil.h"

//#define REOMP_INLINE inline
#define REOMP_INLINE

#define MODE "REOMP_MODE"
#define REOMP_DIR "REOMP_DIR"
#define REOMP_RECORD (0)
#define REOMP_REPLAY (1)
#define REOMP_DISABLE (2)

#define REOMP_MAX_THREAD (256)


#define REOMP_WITH_LOCK (1)
#define REOMP_WITHOUT_LOCK (2)

#define USE_OMP_GET_THREAD_NUM

#define REOMP_REDUCE_LOCK_MASTER (1)
#define REOMP_REDUCE_LOCK_WORKER (0)
#define REOMP_REDUCE_ATOMIC      (2)
#define REOMP_REDUCE_NULL        (3)

#define REOMP_TIMER_ENTIRE       (0)
#define REOMP_TIMER_GATE_TIME    (1)
#define REOMP_TIMER_IO_TIME      (2)

//#define REOMP_USE_APIO
//#define REOMP_SKIP_RECORD
/* Multi lock is not implimented yet (only work with REOMP_SKIP_RECORD) */
//#define REOMP_USE_MULTI_LOCKS

/* ============= */
#include "reomp_gate_clock.h"
static reomp_gate_t *reomp_gate = NULL;
/* ============= */


#ifdef REOMP_USE_APIO
static int fp = -1;
#else
static FILE *fp = NULL;
static FILE* fds[REOMP_MAX_THREAD];
#endif


static int reomp_mode = -1;
static int replay_thread_num = -1;




//static pthread_mutex_t file_read_mutex;
//static pthread_mutex_t file_write_mutex;
//static pthread_mutex_t reomp_omp_tid_umap_mutex;
static omp_lock_t file_read_mutex_lock;
static omp_nest_lock_t file_write_mutex_lock; // old
static omp_lock_t *record_locks = NULL; // new
static omp_lock_t reomp_omp_tid_umap_mutex_lock; // not used
static int critical_flag = 0;
static int reomp_omp_call_depth;
static unordered_map<int64_t, int> reomp_omp_tid_umap;
//static char* record_file_dir = "/l/ssd";
static char* record_file_dir = "/tmp";
//static char* record_file_dir = "./";
static char record_file_path[256];


//static double tmp_time = 0, tmp_time2 = 0;
//static double io_time = 0;
//static double lock_time = 0, tmp_lock_time = 0;
//static double control_time = 0;
static int time_tid = 0;

static vector<list<char*>*> callstack;
static vector<size_t> callstack_hash;
static unordered_map<size_t, size_t> callstack_hash_umap;
static int is_callstack_init = 0;

static volatile int current_tid = -1;
static volatile int nest_num = 0;

static REOMP_INLINE int reomp_get_thread_num()
{
  return omp_get_thread_num();
}

static void reomp_init_callstack()
{
  int tid;
  tid = reomp_get_thread_num();
  if (tid == 0) {
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
  struct stat st;
  
  if (!(env = getenv(REOMP_DIR))) {
    record_file_dir = (char*)".";
  } else {
    record_file_dir = env;    
  }

  if (stat(record_file_dir, &st) != 0) {
    if (mkdir(record_file_dir, S_IRWXU) < 0) {
      MUTIL_ERR("Failded to make directory: %s", record_file_dir);
    }
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
  struct stat st;

  if (stat(record_file_dir, &st) != 0) {
    if (mkdir(record_file_dir, S_IRWXU) < 0) {
      MUTIL_ERR("Failded to make directory: %s", record_file_dir);
    }
  }

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
  double entire_time = 0, gate_time = 0, io_time = 0;
#ifdef REOMP_USE_APIO
  ap_close(fp);
#else
  //  tmp_time = reomp_util_get_time();
  MUTIL_TIMER(MUTIL_TIMER_START, REOMP_TIMER_IO_TIME, NULL);
  fsync(fileno(fp));
  fclose(fp);
  //io_time += reomp_util_get_time() - tmp_time;
  MUTIL_TIMER(MUTIL_TIMER_STOP, REOMP_TIMER_IO_TIME, NULL);

  MUTIL_TIMER(MUTIL_TIMER_STOP, REOMP_TIMER_ENTIRE, NULL);
  MUTIL_TIMER(MUTIL_TIMER_GET_TIME, REOMP_TIMER_ENTIRE, &entire_time);
  MUTIL_TIMER(MUTIL_TIMER_GET_TIME, REOMP_TIMER_GATE_TIME, &gate_time);
  MUTIL_TIMER(MUTIL_TIMER_GET_TIME, REOMP_TIMER_IO_TIME, &io_time);
  MUTIL_TIMER(MUTIL_TIMER_START, REOMP_TIMER_ENTIRE, NULL);
  MUTIL_DBG("Overhead: %f (Gate: %f, IO: %f) VERSION: %d", entire_time, gate_time, io_time, REOMP_RR_VERSION);
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
  record_locks = (omp_lock_t*)malloc(sizeof(omp_lock_t) * num_locks);
  for (int i = 0; i < num_locks; i++) {
    omp_init_lock(&record_locks[i]);
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



static void reomp_gate_in(int control, void* ptr, size_t lock_id, int lock)
{
  int tid;
  size_t ret;

  //if(omp_get_level() == 0) return;
  //  if(omp_get_num_threads() == 1) return;

  //  MUTIL_DBG("sset lock_id: %d (tid: %d)", &record_locks[lock_id], t);

  tid = reomp_get_thread_num();
  if (tid == current_tid) {
    nest_num++;
    return;
  }
  
  if (reomp_mode == REOMP_RECORD) {
    //    if (tid == time_tid) tmp_time = reomp_util_get_time();
    if (tid == time_tid) MUTIL_TIMER(MUTIL_TIMER_START, REOMP_TIMER_GATE_TIME, NULL);
    //if(lock == REOMP_WITH_LOCK) omp_set_nest_lock(&record_locks[lock_id]);
    //    MUTIL_DBG("set lock_id: %d", &record_locks[lock_id]);
    omp_set_lock(&record_locks[lock_id]);
//    if (tid == time_tid) lock_time += reomp_util_get_time() - tmp_time;
    if (tid == time_tid) MUTIL_TIMER(MUTIL_TIMER_STOP, REOMP_TIMER_GATE_TIME, NULL);
  } else {
    //    MUTIL_DBG("  (tid: %d)", tid);    
    if (tid == time_tid) MUTIL_TIMER(MUTIL_TIMER_START, REOMP_TIMER_GATE_TIME, NULL);
    while (tid != replay_thread_num) {
      if(omp_test_lock(&file_read_mutex_lock) != 0) {
#ifdef REOMP_USE_APIO	
	ap_read(fp, &replay_thread_num, sizeof(int));
#else 
	//	if (tid == time_tid) tmp_time = reomp_util_get_time();
	if (tid == time_tid) MUTIL_TIMER(MUTIL_TIMER_STOP, REOMP_TIMER_GATE_TIME, NULL);
	if (tid == time_tid) MUTIL_TIMER(MUTIL_TIMER_START, REOMP_TIMER_IO_TIME, NULL);
	ret = fread(&replay_thread_num, sizeof(int), 1, fp);
	//	replay_thread_num = (replay_thread_num + 1) % omp_get_num_threads();
	if (feof(fp)) MUTIL_ERR("fread reached the end of record file");
	if (ferror(fp))  MUTIL_ERR("fread failed");
	//	if (tid == time_tid) io_time += reomp_util_get_time() - tmp_time;
	if (tid == time_tid) MUTIL_TIMER(MUTIL_TIMER_STOP, REOMP_TIMER_IO_TIME, NULL);
	if (tid == time_tid) MUTIL_TIMER(MUTIL_TIMER_START, REOMP_TIMER_GATE_TIME, NULL);
#endif
      }
    }
    if (tid == time_tid) MUTIL_TIMER(MUTIL_TIMER_STOP, REOMP_TIMER_GATE_TIME, NULL);
  }
#ifdef ENABLE_MEM_SYNC
  __sync_synchronize();
#endif
  current_tid = tid;
  //  MUTIL_DBG("Gate-In: tid: %d: lock: %d, replay_thread_num: %d", tid, lock, replay_thread_num);
  //reomp_util_btrace();
  return;
}





static void reomp_gate_out(int control, void* ptr, size_t lock_id, int lock)
{
  int tid;
  size_t ret;

  //  if(omp_get_num_threads() == 1) return;

  if (nest_num) {
    nest_num--;
    return;
  }
  
  current_tid = -1;

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
    //    if (tid == time_tid) tmp_time = reomp_util_get_time();
    if (tid == time_tid) MUTIL_TIMER(MUTIL_TIMER_START, REOMP_TIMER_IO_TIME, NULL);
    //    MUTIL_DBG("CheckLevel: %d %d %d", tid, (long)(ptr), lock_id);
    ret = fwrite(&tid, sizeof(int), 1, fp);
    if (tid == time_tid) MUTIL_TIMER(MUTIL_TIMER_STOP, REOMP_TIMER_IO_TIME, NULL);
    //    if (tid == time_tid) io_time += reomp_util_get_time() - tmp_time;
    if (ret != 1) MUTIL_ERR("fwrite failed");
#endif
#endif
    //    if (tid == time_tid) tmp_time = reomp_util_get_time();
    if (tid == time_tid) MUTIL_TIMER(MUTIL_TIMER_START, REOMP_TIMER_GATE_TIME, NULL);
    //if(lock == REOMP_WITH_LOCK) omp_unset_nest_lock(&record_locks[lock_id]);
    //    MUTIL_DBG("lock_id: %d", &record_locks[lock_id]);
    omp_unset_lock(&record_locks[lock_id]);
    //    if (tid == time_tid) lock_time += reomp_util_get_time() - tmp_time;
    if (tid == time_tid) MUTIL_TIMER(MUTIL_TIMER_STOP, REOMP_TIMER_GATE_TIME, NULL);
  } else {
    //    MUTIL_DBG("tid: %d: end: %d (unlock: %d)", tid, replay_thread_num, lock);
    //    fprintf(stderr, "%d\n", tid);
    //    MUTIL_DBGI(1, "tid: %d", tid);
    //    if (tid != replay_thread_num) {
      //      MUTIL_ERR("tid: %d: end: %d", tid, replay_thread_num);
    //    }
    replay_thread_num = -1;
    //    pthread_mutex_unlock(&file_read_mutex);
    //    if (tid == time_tid) tmp_time = reomp_util_get_time();
    if (tid == time_tid) MUTIL_TIMER(MUTIL_TIMER_START, REOMP_TIMER_GATE_TIME, NULL);
    omp_unset_lock(&file_read_mutex_lock);
    //    if (tid == time_tid) lock_time += reomp_util_get_time() - tmp_time;
    if (tid == time_tid) MUTIL_TIMER(MUTIL_TIMER_STOP, REOMP_TIMER_GATE_TIME, NULL);
  }
  return;
}

// static void reomp_gate(int control, int lock)
// {
//   reomp_gate_in(control, lock);
//   reomp_gate_out(control, lock);
//   return;
// }

static void reomp_debug_print(void* ptr, size_t size)
{
  //  MUTIL_DBG("PTR: %p, SIZE: %lu", ptr, size);
  return;
}

void reomp_rr_init_2(int control, size_t size)
{
  reomp_config_init();
  //  reomp_gate = &reomp_gate_clock;
  reomp_gate = &reomp_gate_tid;
  reomp_gate->init(control, size);
  return;
}

int counter=0;
void REOMP_CONTROL(int control, void* ptr, size_t size)
{

  if (reomp_mode == REOMP_DISABLE) return;
  int tid = reomp_get_thread_num();

  if (control == 22 && tid == 0) {
    counter++;
    //    sleep(1);
    //    if (counter == 10) exit(0);
  }


  //  MUTIL_DBG("tid: %d, control: %d (%d)", tid, control, counter);
  //  MUTIL_DBG("tid: %d, control: %d", tid, control);
  //  if (tid == time_tid) tmp_time2 = reomp_util_get_time();
  if (tid == time_tid) MUTIL_TIMER(MUTIL_TIMER_START, REOMP_TIMER_ENTIRE, NULL);

  switch(control) {
  case REOMP_BEF_MAIN: // 0
#if REOMP_RR_VERSION == 1
    reomp_rr_init(control, size);
#else
    reomp_rr_init_2(control, size);
#endif 
    break;
  case REOMP_AFT_MAIN: // 1
#if REOMP_RR_VERSION == 1
    reomp_rr_finalize();
#else
    //reomp_rr_finalize_2();
    reomp_gate->finalize();
#endif
    break;
  case REOMP_AFT_MPI_INIT: // 2
    reomp_rr_init(control, size);
#if REOMP_RR_VERSION == 2
    int my_rank;
    int flag;
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    reomp_util_init(my_rank);    
#endif
    break;
  case REOMP_GATE_IN: // 10
#ifndef REOMP_USE_MULTI_LOCKS
    size = 0;
#endif
    
#if REOMP_RR_VERSION == 1
    reomp_gate_in(control, ptr, size, REOMP_WITH_LOCK);
#else
    //    reomp_gate_in_2(control, ptr, size, REOMP_WITH_LOCK);
    reomp_gate->in(control, ptr, size, REOMP_WITH_LOCK);
#endif
    //reomp_gate_in(control, ptr, size, REOMP_WITHOUT_LOCK);
    break;
  case REOMP_GATE_OUT: // 11
#ifndef REOMP_USE_MULTI_LOCKS
    size = 0;
#endif

#if REOMP_RR_VERSION == 1
    reomp_gate_out(control, ptr, size, REOMP_WITH_LOCK);
#else
    //    reomp_gate_out_2(control, ptr, size, REOMP_WITH_LOCK);
    reomp_gate->out(control, ptr, size, REOMP_WITH_LOCK);
#endif
    //reomp_gate_out(control, ptr, size, REOMP_WITHOUT_LOCK);
    break;
  case REOMP_GATE: // 12
    MUTIL_ERR("No such control");
    //    reomp_gate(control, REOMP_WITH_LOCK);
    break;
  case REOMP_BEF_CRITICAL_BEGIN: // 13
#if REOMP_RR_VERSION == 1
    reomp_gate_in(control, ptr, size, REOMP_WITHOUT_LOCK);
#else
    //    reomp_gate_in_2(control, ptr, size, REOMP_WITHOUT_LOCK);
    reomp_gate->in(control, ptr, size, REOMP_WITHOUT_LOCK);
#endif
    break;
  case REOMP_AFT_CRITICAL_BEGIN: // 14
#if REOMP_RR_VERSION == 1
    //    reomp_gate_out(control, ptr, size, REOMP_WITHOUT_LOCK);
#endif
    break;
  case REOMP_AFT_CRITICAL_END: // 15
#if REOMP_RR_VERSION == 1
    reomp_gate_out(control, ptr, size, REOMP_WITH_LOCK);
#else
    //    reomp_gate_out_2(control, ptr, size, REOMP_WITHOUT_LOCK);
    reomp_gate->out(control, ptr, size, REOMP_WITHOUT_LOCK);
#endif
    //#if REOMP_RR_VERSION == 2
    //    reomp_gate_out_2(control, ptr, size, REOMP_WITHOUT_LOCK);
    //#endif
    //    MUTIL_DBG("tid: %d: out", reomp_get_thread_num());
    break;

  case REOMP_BEF_REDUCE_BEGIN: // 16
    //    reomp_gate_in_bef_reduce_begin(control, ptr, size);
    reomp_gate->in_brb(control, ptr, size);
    break;
  case REOMP_AFT_REDUCE_BEGIN: // 17
    //    reomp_gate_in_aft_reduce_begin(control, ptr, size);
    reomp_gate->in_arb(control, ptr, size);
    break;
  case REOMP_BEF_REDUCE_END:   // 18
    //    reomp_gate_out_bef_reduce_end(control, ptr, size);
    reomp_gate->out_bre(control, ptr, size);
    break;
  case REOMP_AFT_REDUCE_END:   // 19
    //    reomp_gate_out_aft_reduce_end(control, ptr, size);
    reomp_gate->out_are(control, ptr, size);
    break;





  case REOMP_DEBUG_PRINT: 
    reomp_debug_print(ptr, size);
    break;

  }
//  if (tid == time_tid) control_time += reomp_util_get_time() - tmp_time2;
  if (tid == time_tid) MUTIL_TIMER(MUTIL_TIMER_STOP, REOMP_TIMER_ENTIRE, NULL);

  return;
}


void logop(int i)
{
    fprintf(stderr, "Computed: %d\n", i);
}


