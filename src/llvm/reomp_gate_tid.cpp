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
#include "reomp_config.h"
#include "reomp_gate.h"
#include "reomp_mem.h"
#include "reomp_mon.h"
#include "mutil.h"

#define REOMP_WITH_LOCK (1)
#define REOMP_WITHOUT_LOCK (2)

static void reomp_tgate_init(int control, size_t num_locks);
static void reomp_tgate_finalize();
static void reomp_tgate_in(int control, void* ptr, size_t lock_id, int lock);
static void reomp_tgate_out(int control, void* ptr, size_t lock_id, int lock);
static void reomp_tgate_in_bef_reduce_begin(int control, void* ptr, size_t null);
static void reomp_tgate_in_aft_reduce_begin(int control, void* ptr, size_t reduction_method);
static void reomp_tgate_out_reduce_end(int control, void* ptr, size_t reduction_method);
static void reomp_tgate_out_bef_reduce_end(int control, void* ptr, size_t reduction_method);
static void reomp_tgate_out_aft_reduce_end(int control, void* ptr, size_t reduction_method);


static FILE *fp = NULL;
static FILE* fds[REOMP_MAX_THREAD];


static int reomp_mode = -1;
static int replay_thread_num = -1;


static omp_lock_t file_read_mutex_lock;
static omp_nest_lock_t file_write_mutex_lock; // old
static omp_lock_t *record_locks = NULL; // new
static omp_lock_t reomp_omp_tid_umap_mutex_lock; // not used
static int critical_flag = 0;
static int reomp_omp_call_depth;
static unordered_map<int64_t, int> reomp_omp_tid_umap;
static char* record_file_dir = "/tmp";
static char record_file_path[256];

static int time_tid = 0;

static vector<list<char*>*> callstack;
static vector<size_t> callstack_hash;
static unordered_map<size_t, size_t> callstack_hash_umap;
static int is_callstack_init = 0;

static volatile int current_tid = -1;
static volatile int nest_num = 0;


reomp_gate_t reomp_gate_tid = {
  reomp_tgate_init,
  reomp_tgate_finalize,
  reomp_tgate_in,
  reomp_tgate_out,
  reomp_tgate_in_bef_reduce_begin,
  reomp_tgate_in_aft_reduce_begin,
  reomp_tgate_out_bef_reduce_end,
  reomp_tgate_out_aft_reduce_end,
};

static int reomp_tgate_get_thread_num()
{
  return omp_get_thread_num();
}

static void reomp_tgate_init_file(int control)
{
  int my_rank;
  struct stat st;

  if (control == REOMP_BEF_MAIN) {
    sprintf(record_file_path, "%s/rank_x.reomp", reomp_config.record_dir);
  } else if (control == REOMP_AFT_MPI_INIT) {
    if (fp == NULL) return;
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    reomp_util_init(my_rank);
    fclose(fp);
    sprintf(record_file_path, "%s/rank_%d.reomp", reomp_config.record_dir, my_rank);
  } else {
    MUTIL_DBG("No such control: %d", control);
  }
  int flags = -1;
  int mode = -1;
  char *fmode;

  if (reomp_config.mode == REOMP_RECORD) {
    flags = O_CREAT | O_WRONLY;
    mode  = S_IRUSR | S_IWUSR;
    fmode = (char*)"w+";
  } else {
    flags = O_RDONLY;
    mode  = 0;
    fmode = (char*)"r";
    omp_init_lock(&file_read_mutex_lock);
  }

  fp = fopen(record_file_path, fmode);
  if (fp == NULL) {
    MUTIL_ERR("file fopen failed: errno=%d path=%s flags=%d moe=%d (%d %d)", 
	      errno, record_file_path, flags, mode, O_CREAT | O_WRONLY, O_WRONLY);
  }

  return;
}

static void reomp_tgate_finalize_file()
{
  double entire_time = 0, gate_time = 0, io_time = 0;

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

  REOMP_MON_SAMPLE_CALLSTACK_PRINT();

  return;
}


static void reomp_tgate_init_locks(size_t num_locks)
{
  if (record_locks != NULL) return;
  record_locks = (omp_lock_t*)malloc(sizeof(omp_lock_t) * num_locks);
  for (int i = 0; i < num_locks; i++) {
    omp_init_lock(&record_locks[i]);
  }
  return;
}




static void reomp_tgate_init(int control, size_t num_locks)
{
  reomp_tgate_init_file(control);
  reomp_tgate_init_locks(num_locks);
  omp_init_lock(&reomp_omp_tid_umap_mutex_lock);
  return;
}

static void reomp_tgate_finalize()
{
  reomp_tgate_finalize_file();
  return;
}



static void reomp_tgate_in(int control, void* ptr, size_t lock_id, int lock)
{
  int tid;
  size_t ret;

  tid = reomp_tgate_get_thread_num();
  if (tid == current_tid) {
    nest_num++;
    return;
  }
  
  if (reomp_config.mode == REOMP_RECORD) {
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
	if (tid == time_tid) MUTIL_TIMER(MUTIL_TIMER_STOP, REOMP_TIMER_GATE_TIME, NULL);
	if (tid == time_tid) MUTIL_TIMER(MUTIL_TIMER_START, REOMP_TIMER_IO_TIME, NULL);
	ret = fread(&replay_thread_num, sizeof(int), 1, fp);
	if (feof(fp)) MUTIL_ERR("fread reached the end of record file");
	if (ferror(fp))  MUTIL_ERR("fread failed");
	if (tid == time_tid) MUTIL_TIMER(MUTIL_TIMER_STOP, REOMP_TIMER_IO_TIME, NULL);
	if (tid == time_tid) MUTIL_TIMER(MUTIL_TIMER_START, REOMP_TIMER_GATE_TIME, NULL);
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


static void reomp_tgate_out(int control, void* ptr, size_t lock_id, int lock)
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

  tid = reomp_tgate_get_thread_num();
  //  MUTIL_DBG("Gate-Out: tid: %d: lock: %d, replay_thread_num: %d", tid, lock, replay_thread_num);
  if (reomp_config.mode == REOMP_RECORD) {
    //    MUTIL_DBG("unlock: %d", tid);
    //    write(fp, &tid, sizeof(int));
#ifndef REOMP_SKIP_RECORD
    if (tid == time_tid) MUTIL_TIMER(MUTIL_TIMER_START, REOMP_TIMER_IO_TIME, NULL);
    ret = fwrite(&tid, sizeof(int), 1, fp);
    if (tid == time_tid) MUTIL_TIMER(MUTIL_TIMER_STOP, REOMP_TIMER_IO_TIME, NULL);
    if (ret != 1) MUTIL_ERR("fwrite failed");
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


static  void reomp_tgate_in_bef_reduce_begin(int control, void* ptr, size_t null)
{
  return;
}

static  void reomp_tgate_in_aft_reduce_begin(int control, void* ptr, size_t reduction_method)
{
  return;
}

static  void reomp_tgate_out_bef_reduce_end(int control, void* ptr, size_t reduction_method)
{
  return;
}

static  void reomp_tgate_out_aft_reduce_end(int control, void* ptr, size_t reduction_method)
{
  return;
}


