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
#include "reomp_mem.h"
#include "reomp_mon.h"
#include "apio.h"
#include "mutil.h"

#define REOMP_TIMER_ENTIRE       (0)
#define REOMP_TIMER_GATE_TIME    (1)
#define REOMP_TIMER_IO_TIME      (2)

static FILE *fp = NULL;
static FILE* fds[REOMP_MAX_THREAD];

typedef struct {
  FILE* fd;
  size_t total_write_bytes;
  size_t write_bytes_at_reduction;
  char padding[128 - sizeof(size_t) * 2 - sizeof(FILE*)];
} reomp_fd_t;

static int reomp_mode = -1;
static int replay_thread_num = -1;
reomp_fd_t reomp_fds[REOMP_MAX_THREAD];


static omp_lock_t file_read_mutex_lock;
static omp_nest_lock_t file_write_mutex_lock; // old
static omp_lock_t *record_locks = NULL; // new
static omp_lock_t reomp_omp_tid_umap_mutex_lock; // not used
static omp_lock_t reomp_omp_lock;
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

static volatile int reserved_clock = 0, gate_clock = 0;
static volatile int current_tid = -1;
static volatile int nest_num = 0;

static int reomp_cgate_get_thread_num()
{
  return omp_get_thread_num();
}

static FILE* reomp_cgate_get_fd(int my_tid)
{
  int my_rank;
  char* path;
  int flags = -1;
  int mode = -1;
  char *fmode;
  FILE *tmp_fd;

  if (reomp_fds[my_tid].fd != NULL) return reomp_fds[my_tid].fd;

  path = (char*)malloc(sizeof(char) * PATH_MAX);
  MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
  sprintf(path, "%s/rank_%d-tid_%d.reomp", record_file_dir, my_rank, my_tid);
  //  MUTIL_DBG("Open: %s", path);

  if (reomp_mode == REOMP_RECORD) {
    flags = O_CREAT | O_WRONLY;
    mode  = S_IRUSR | S_IWUSR;
    fmode = (char*)"w+";

  } else {
    flags = O_RDONLY;
    mode  = 0;
    fmode = (char*)"r";
    omp_init_lock(&file_read_mutex_lock);
  }
  tmp_fd = fopen(path, fmode);
  if (tmp_fd == NULL) {
    MUTIL_ERR("file fopen failed: errno=%d path=%s flags=%d moe=%d (%d %d)", 
	      errno, path, flags, mode, O_CREAT | O_WRONLY, O_WRONLY);
  }
  free(path);
  reomp_fds[my_tid].fd = tmp_fd;
  return tmp_fd;
}

static void reomp_cgate_init(int control)
{    
  for (int i = 0; i < REOMP_MAX_THREAD; i++) {
    reomp_fds[i].fd = NULL;
    reomp_fds[i].total_write_bytes = 0;
    reomp_fds[i].write_bytes_at_reduction = 0;
  }
  omp_init_lock(&reomp_omp_lock);
  return;
}

static void reomp_cgate_finalize_file()
{
  double entire_time = 0, gate_time = 0, io_time = 0;
  MUTIL_TIMER(MUTIL_TIMER_START, REOMP_TIMER_IO_TIME, NULL);
  for (int i = 0; i < REOMP_MAX_THREAD; i++) {
    if (reomp_fds[i].fd != NULL) {
      fsync(fileno(reomp_fds[i].fd));
      fclose(reomp_fds[i].fd);
    }
  }
  MUTIL_TIMER(MUTIL_TIMER_STOP , REOMP_TIMER_IO_TIME, NULL);

  REOMP_MON_SAMPLE_CALLSTACK_PRINT();
  unordered_map<size_t, size_t>::iterator it, it_end;
  for (it = callstack_hash_umap.begin(),
	 it_end = callstack_hash_umap.end();
       it != it_end;
       it++) {
    MUTIL_DBG("Hash: %lu, Count: %lu", it->first, it->second);
  }

  MUTIL_TIMER(MUTIL_TIMER_STOP, REOMP_TIMER_ENTIRE, NULL);
  MUTIL_TIMER(MUTIL_TIMER_GET_TIME, REOMP_TIMER_ENTIRE, &entire_time);
  MUTIL_TIMER(MUTIL_TIMER_GET_TIME, REOMP_TIMER_GATE_TIME, &gate_time);
  MUTIL_TIMER(MUTIL_TIMER_GET_TIME, REOMP_TIMER_IO_TIME, &io_time);
  MUTIL_TIMER(MUTIL_TIMER_START, REOMP_TIMER_ENTIRE, NULL);
  MUTIL_DBG("Overhead: %f (Gate: %f, IO: %f) VERSION: %d", entire_time, gate_time, io_time, REOMP_RR_VERSION);
  return;
}

static void reomp_cgate_init_locks(size_t num_locks)
{
  if (record_locks != NULL) return;
  record_locks = (omp_lock_t*)malloc(sizeof(omp_lock_t) * num_locks);
  for (int i = 0; i < num_locks; i++) {
    omp_init_lock(&record_locks[i]);
  }
  return;
}


double main_s, main_e;
static void reomp_cgate_init(int control, size_t num_locks)
{
  main_s = reomp_util_get_time();
  reomp_cgate_init_callstack();
  reomp_init_env();
  reomp_cgate_init_file(control);
  reomp_cgate_init_locks(num_locks);
  reomp_mem_init();
  //  pthread_mutex_init(&reomp_omp_tid_umap_mutex, NULL);
  omp_init_lock(&reomp_omp_tid_umap_mutex_lock);
  return;
}

static void reomp_cgate_finalize()
{
  reomp_cgate_finalize_file();
  main_e = reomp_util_get_time();
  MUTIL_DBG("REOMP: main_time = %f", main_e - main_s);
  return;
}

static int reomp_get_clock_record(int tid)
{
  int tmp;
  tmp = reserved_clock++;                    
  return tmp;
}

static void reomp_fwrite(const void * ptr, size_t size, size_t count, int tid)
{
  size_t write_count;
  FILE *stream;
  if (tid == time_tid) MUTIL_TIMER(MUTIL_TIMER_START, REOMP_TIMER_IO_TIME, NULL);
  stream = reomp_get_fd(tid);
  write_count = fwrite(ptr, size, count, stream);
  if (write_count != count) MUTIL_ERR("fwrite failed");
  reomp_fds[tid].total_write_bytes += write_count * size;
  if (tid == time_tid) MUTIL_TIMER(MUTIL_TIMER_STOP, REOMP_TIMER_IO_TIME, NULL);
  //  if (!tid) MUTIL_DBG("tid: %d: size: %lu", tid, reomp_fds[tid].total_write_bytes);
  return;
}

static void reomp_fread(void * ptr, size_t size, size_t count, int tid)
{
  FILE *fd;
  size_t s;
  if (tid == time_tid) MUTIL_TIMER(MUTIL_TIMER_START, REOMP_TIMER_IO_TIME, NULL);
  fd = reomp_get_fd(tid);
  s = fread(ptr, size, 1, fd);
  if (s != 1) {
    if (feof(fd)) MUTIL_ERR("fread reached the end of record file");
    if (ferror(fd))  MUTIL_ERR("fread failed");
  }
  if (tid == time_tid) MUTIL_TIMER(MUTIL_TIMER_STOP, REOMP_TIMER_IO_TIME, NULL);
  return;
}


static int reomp_cgate_get_clock_replay(int tid)
{
  int clock;
  reomp_fread(&clock, sizeof(int), 1 , tid);
  return clock;
}

static REOMP_INLINE size_t reomp_cgate_read_reduction_method(int tid)
{
  size_t reduction_method;
  reomp_fread(&reduction_method, sizeof(size_t), 1 , tid);
  return reduction_method;
}

static REOMP_INLINE void reomp_cgate_gate_record_reduction_method_init(int tid) 
{
  size_t val;
  if (reomp_fds[tid].write_bytes_at_reduction != 0) {
    MUTIL_ERR("write bytes at reduction is not 0");
  }
  reomp_fds[tid].write_bytes_at_reduction = reomp_fds[tid].total_write_bytes;
  val = REOMP_REDUCE_NULL;
  reomp_fwrite(&val, sizeof(size_t), 1, tid);
  return;
}

static void reomp_cgate_record_reduction_method(int tid, size_t reduction_method)
{
  FILE *fd;
  long offset;
  fd = reomp_get_fd(tid);
  offset = reomp_fds[tid].total_write_bytes - reomp_fds[tid].write_bytes_at_reduction;
  offset = -offset;
  if (!tid) MUTIL_DBG("tid: %d: %lu - %lu", tid, reomp_fds[tid].write_bytes_at_reduction, reomp_fds[tid].total_write_bytes);
  fseek(fd, offset, SEEK_END);
  reomp_fwrite(&reduction_method, sizeof(size_t), 1, tid);
  fseek(fd, 0, SEEK_END);
  reomp_fds[tid].write_bytes_at_reduction = 0;
  //  if (!tid)  MUTIL_DBG("tid: %d: Reduction: %lu (gk: %d)", tid, reduction_method, gate_clock);
  MUTIL_DBG("tid: %d: Reduction: %lu offset: %ld, (gk: %d)", tid, reduction_method, offset, gate_clock);
  return;
}

static void reomp_cgate_ticket_wait(int tid)
{
  int clock;
  if (tid == current_tid) {
    nest_num++;
    return;
  }

  if (tid == time_tid)  {
    MUTIL_TIMER(MUTIL_TIMER_START, REOMP_TIMER_GATE_TIME, NULL);
  }
  if (reomp_mode == REOMP_RECORD) {
    omp_set_lock(&reomp_omp_lock);
  } else {
    clock = reomp_get_clock_replay(tid);
    while (clock != gate_clock);
    // omp_set_lock(&reomp_omp_lock);
    // gate_clock++;
  }
  current_tid = tid;
  if (tid == time_tid) MUTIL_TIMER(MUTIL_TIMER_STOP, REOMP_TIMER_GATE_TIME, NULL);
  //MUTIL_DBG("==> WT: tid %d: clock: %d, gate_clock: %d", tid, clock, gate_clock);
  //  MUTIL_DBG("  --> IN: tid %d: gate_clock: %d", tid, gate_clock);
  //  if (gate_clock % 1000000 == 0) MUTIL_DBG("IN: tid %d: gate_clock: %d", tid, gate_clock);
  //  MUTIL_DBG("IN: tid %d: gate_clock: %d", tid, gate_clock);
  return;
}

static REOMP_INLINE void reomp_cgate_record_ticket_number(int tid, int clock)
{
  reomp_fwrite((void*)&clock, sizeof(int), 1, tid);
  //  if (!tid) MUTIL_DBG("tid: %d: Tnum: %d", tid, clock);
  return;
}

static REOMP_INLINE void reomp_cgate_leave(int tid)
{
  if (nest_num) {
    nest_num--;
    return;
  }
  
  //  if (gate_clock % 100000 == 0) MUTIL_DBG("OUT: tid %d: gate_clock: %d", tid, gate_clock);
  current_tid = -1;
  if (reomp_mode == REOMP_RECORD) {
    int clock;
    clock = gate_clock++;
    omp_unset_lock(&reomp_omp_lock);
    reomp_gate_record_ticket_number(tid, clock);
  } else {
       __sync_synchronize();
       gate_clock++;
       __sync_synchronize();
    //    omp_unset_lock(&reomp_omp_lock);
  }

  return ;
}


static REOMP_INLINE void reomp_cgate_in(int control, void* ptr, size_t lock_id, int lock)
{
  int clock;
  int tid;

  //  MUTIL_DBG("-- IN");
  if(omp_get_num_threads() == 1) return;
  tid = reomp_get_thread_num();

  reomp_gate_ticket_wait(tid);


  return;
}



static REOMP_INLINE void reomp_cgate_out(int control, void* ptr, size_t lock_id, int lock)
{

  int tid;

  //  MUTIL_DBG("-- OUT");
  if(omp_get_num_threads() == 1) return;
  tid = reomp_get_thread_num();
  //  MUTIL_DBG("-- OUT: tid %d: gate_clock: %d", tid, gate_clock);
  reomp_gate_leave(tid);



  return;
}


static REOMP_INLINE void reomp_cgate_in_bef_reduce_begin(int control, void* ptr, size_t null)
{
  size_t reduction_method = -1;
  int tid;
  
  tid = reomp_get_thread_num();
  if (reomp_mode == REOMP_RECORD) {
    /* Nothing to do */
    reomp_gate_record_reduction_method_init(tid);
  } else if (reomp_mode == REOMP_REPLAY) {
    reduction_method = reomp_read_reduction_method(tid);
    if (reduction_method == REOMP_REDUCE_LOCK_MASTER) {
      reomp_gate_ticket_wait(tid);
    } else if (reduction_method == REOMP_REDUCE_LOCK_WORKER) {
      /* Pass this gate to synchronize with master and the other workers */
    } else if (reduction_method == REOMP_REDUCE_ATOMIC) {
      /* Pass this gate to synchronize with master and the other workers */
    } else {
      MUTIL_ERR("No such reduction method: %lu (tid: %d)", reduction_method, tid);
    }
  } else {
    MUTIL_ERR("No such reomp_mode: %d", reomp_mode);
  }

  return;
}

static REOMP_INLINE void reomp_cgate_in_aft_reduce_begin(int control, void* ptr, size_t reduction_method)
{
  int tid;
  int clock;
  tid = reomp_get_thread_num();
  if (reomp_mode == REOMP_RECORD) {
    if (reduction_method == REOMP_REDUCE_LOCK_MASTER) {
      /* If all threads have REOMP_REDUCE_LOCK_MASTER: 
	     This section is already serialized by __kmpc_reduce or __kmpc_reduce_nowait
	 If the other threads has REOMP_REDUCE_LOCK_WORKER
	     This section is not even executed by workers
      */
      reomp_gate_ticket_wait(tid); /* To get ticket, and will not wait  */
    } else if (reduction_method == REOMP_REDUCE_LOCK_WORKER) {
      /* Worker does not get involved in reduction */
      /* Sicne workers do not execution neither __kmpc_end_reduce nor atomic, 
	 Workers need to record reduction method here now.
       */
      reomp_gate_record_reduction_method(tid, reduction_method);
    } else if (reduction_method == REOMP_REDUCE_ATOMIC) {
      /* This section is not serialized by __kmpc_reduce and __kmpc_reduct_nowait.
	 ReMPI needs to serialize this section.
       */
      reomp_gate_ticket_wait(tid);
    } else {
      MUTIL_ERR("No such reduction method: %lu", reduction_method);
    }
  } else if (reomp_mode == REOMP_REPLAY) {
    if (reduction_method == REOMP_REDUCE_LOCK_MASTER) {
      /* If all threads have REOMP_REDUCE_LOCK_MASTER: 
             This section is already serialized by reomp_gate_in_bef_reduce_begin and (__kmpc_reduce or __kmpc_reduce_nowait)
	 If the other threads has REOMP_REDUCE_LOCK_WORKER
	     This section is not even executed by workers
      */
      /* Nothing to do */
    } else if (reduction_method == REOMP_REDUCE_LOCK_WORKER) {
      /* Worker does not get involved in reduction. */
      /* Nothing to do */
    } else if (reduction_method == REOMP_REDUCE_ATOMIC) {
      /* This section is not serialized by __kmpc_reduce and __kmpc_reduct_nowait.
	 ReMPI needs to serialize this section.
       */
      reomp_gate_ticket_wait(tid);
    } else {
      MUTIL_ERR("No such reduction method: %lu", reduction_method);
    }
  } else {
    MUTIL_ERR("No such reomp_mode: %d", reomp_mode);
  }
  return;
}


static REOMP_INLINE void reomp_cgate_out_reduce_end(int control, void* ptr, size_t reduction_method)
{
  int tid;
  int clock;
  tid = reomp_get_thread_num();
  if (reomp_mode == REOMP_RECORD) {
    if (reduction_method == REOMP_REDUCE_LOCK_MASTER) {
      reomp_gate_leave(tid);
      reomp_gate_record_reduction_method(tid, reduction_method);
    } else if (reduction_method == REOMP_REDUCE_LOCK_WORKER) {
      /* Worker does not get involved in reduction. */
      /* =================================================== */
      /* ============= This code is never executed ========= */
      /* =================================================== */
    } else if (reduction_method == REOMP_REDUCE_ATOMIC) {
      /* This section is not serialized by __kmpc_reduce and __kmpc_reduct_nowait.
	 ReMPI needs to serialize this section.
       */
      reomp_gate_leave(tid);
      reomp_gate_record_reduction_method(tid, reduction_method);
    } else {
      MUTIL_ERR("No such reduction method: %lu", reduction_method);
    }
  } else if (reomp_mode == REOMP_REPLAY) {
    if (reduction_method == REOMP_REDUCE_LOCK_MASTER) {
      reomp_gate_leave(tid);
    } else if (reduction_method == REOMP_REDUCE_LOCK_WORKER) {
      /* This section is not even executed by workers */
      /* Worker does not call __kmpc_end_reduce and any atomic operations */
      /* Nothing to do */
    } else if (reduction_method == REOMP_REDUCE_ATOMIC) {
      reomp_gate_leave(tid);
    } else {
      MUTIL_ERR("No such reduction method: %lu", reduction_method);
    }
  } else {
    MUTIL_ERR("No such reomp_mode: %d", reomp_mode);
  }
  return;
}

static REOMP_INLINE void reomp_cgate_out_bef_reduce_end(int control, void* ptr, size_t reduction_method)
{
  return;
}

static REOMP_INLINE void reomp_cgate_out_aft_reduce_end(int control, void* ptr, size_t reduction_method)
{
  reomp_gate_out_reduce_end(control, ptr, reduction_method);
  return;
}


