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

#include "reomp.h"
#include "reomp_gate.h"
#include "reomp_gate_clock.h"
#include "reomp_mem.h"
#include "reomp_mon.h"
#include "mutil.h"
#include "reomp_config.h"

#define REOMP_REDUCE_LOCK_MASTER (1)
#define REOMP_REDUCE_LOCK_WORKER (0)
#define REOMP_REDUCE_ATOMIC      (2)
#define REOMP_REDUCE_NULL        (3)

static void reomp_cgate_init(int control, size_t num_locks);
static void reomp_cgate_finalize();
static void reomp_cgate_in(int control, void* ptr, size_t lock_id, int lock);
static void reomp_scgate_in(int control, void* ptr, size_t lock_id, int lock);
static void reomp_cgate_out(int control, void* ptr, size_t lock_id, int lock);
static void reomp_scgate_out(int control, void* ptr, size_t lock_id, int lock);
static void reomp_cgate_in_bef_reduce_begin(int control, void* ptr, size_t null);
static void reomp_cgate_in_aft_reduce_begin(int control, void* ptr, size_t reduction_method);
static void reomp_cgate_out_reduce_end(int control, void* ptr, size_t reduction_method);
static void reomp_cgate_out_bef_reduce_end(int control, void* ptr, size_t reduction_method);
static void reomp_cgate_out_aft_reduce_end(int control, void* ptr, size_t reduction_method);

typedef struct {
  FILE* fd;
  size_t total_write_bytes;
  size_t write_bytes_at_reduction;
  char padding[128 - sizeof(size_t) * 2 - sizeof(FILE*)];
} reomp_fd_t;
reomp_fd_t reomp_fds[REOMP_MAX_THREAD];

static omp_lock_t reomp_omp_lock;
reomp_gate_t reomp_gate_clock = {
  reomp_cgate_init,
  reomp_cgate_finalize,
  reomp_cgate_in,
  reomp_cgate_out,
  reomp_cgate_in_bef_reduce_begin,
  reomp_cgate_in_aft_reduce_begin,
  reomp_cgate_out_bef_reduce_end,
  reomp_cgate_out_aft_reduce_end,
};

reomp_gate_t reomp_gate_sclock = {
  reomp_cgate_init,
  reomp_cgate_finalize,
  reomp_scgate_in,
  reomp_scgate_out,
  reomp_cgate_in_bef_reduce_begin,
  reomp_cgate_in_aft_reduce_begin,
  reomp_cgate_out_bef_reduce_end,
  reomp_cgate_out_aft_reduce_end,
};

static volatile int current_tid = -1;
static volatile int gate_clock = 0;
static volatile int nest_num = 0;
static int time_tid = 0;


/* ---- For speculative cgate ---- */
#include <setjmp.h>
#define REOMP_SCGATE_LONGJMP (1)
typedef struct {
  jmp_buf jmpenv;
  int speculative_clock = 0;
  int num_aborted = 0;
  //  char padding[128 - sizeof(jmp_buf) - sizeof(int) * 2];
} reomp_scgate_t;
static reomp_scgate_t reomp_scgate[REOMP_MAX_THREAD];
static volatile int reomp_scgate_num_aborted = 0;
static volatile int next_clock = 0;
/* -------------------------------  */


static int reomp_get_thread_num()
{
  return omp_get_thread_num();
}

static void reomp_cgate_init_file(int control)
{
  MUTIL_TIMER(MUTIL_TIMER_START, REOMP_TIMER_ENTIRE, NULL);
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
  MUTIL_TIMER(MUTIL_TIMER_STOP, REOMP_TIMER_ENTIRE, NULL);
  
  MUTIL_TIMER(MUTIL_TIMER_GET_TIME, REOMP_TIMER_ENTIRE, &entire_time);
  MUTIL_TIMER(MUTIL_TIMER_GET_TIME, REOMP_TIMER_GATE_TIME, &gate_time);
  MUTIL_TIMER(MUTIL_TIMER_GET_TIME, REOMP_TIMER_IO_TIME, &io_time);
  MUTIL_DBG("Overhead: %f (Gate: %f, IO: %f) method= %d", entire_time, gate_time, io_time, reomp_config.method);
  return;
}

static void reomp_cgate_init(int control, size_t num_locks)
{
  reomp_cgate_init_file(control);
  return;
}


static void reomp_cgate_finalize()
{
  reomp_cgate_finalize_file();
  return;
}

static FILE* reomp_get_fd(int my_tid)
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
  sprintf(path, "%s/rank_%d-tid_%d.reomp", reomp_config.record_dir, my_rank, my_tid);
  //  MUTIL_DBG("Open: %s", path);

  if (reomp_config.mode == REOMP_ENV_MODE_RECORD) {
    flags = O_CREAT | O_WRONLY;
    mode  = S_IRUSR | S_IWUSR;
    fmode = (char*)"w+";

  } else {
    flags = O_RDONLY;
    mode  = 0;
    fmode = (char*)"r";
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

static void reomp_fwrite(const void * ptr, size_t size, size_t count, int tid)
{
  size_t write_count;
  FILE *stream;
  //  if (tid == time_tid) MUTIL_TIMER(MUTIL_TIMER_START, REOMP_TIMER_IO_TIME, NULL);
  stream = reomp_get_fd(tid);
  write_count = fwrite(ptr, size, count, stream);
  if (write_count != count) MUTIL_ERR("fwrite failed");
  reomp_fds[tid].total_write_bytes += write_count * size;
  //if (tid == time_tid) MUTIL_TIMER(MUTIL_TIMER_STOP, REOMP_TIMER_IO_TIME, NULL);
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

static  size_t reomp_cgate_read_reduction_method(int tid)
{
  size_t reduction_method;
  reomp_fread(&reduction_method, sizeof(size_t), 1 , tid);
  return reduction_method;
}

static  void reomp_cgate_record_reduction_method_init(int tid) 
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
  if (reomp_config.mode == REOMP_ENV_MODE_RECORD) {
    omp_set_lock(&reomp_omp_lock);
  } else {
    clock = reomp_cgate_get_clock_replay(tid);
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

static  void reomp_cgate_record_ticket_number(int tid, int clock)
{
  reomp_fwrite((void*)&clock, sizeof(int), 1, tid);
  //  if (!tid) MUTIL_DBG("tid: %d: Tnum: %d", tid, clock);
  return;
}

static  void reomp_cgate_leave(int tid)
{
  if (nest_num) {
    nest_num--;
    return;
  }
  
  //  if (gate_clock % 100000 == 0) MUTIL_DBG("OUT: tid %d: gate_clock: %d", tid, gate_clock);
  current_tid = -1;
  if (reomp_config.mode == REOMP_ENV_MODE_RECORD) {
    int clock;
    clock = gate_clock++;
    omp_unset_lock(&reomp_omp_lock);
    reomp_cgate_record_ticket_number(tid, clock);
  } else {
       __sync_synchronize();
       gate_clock++;
       __sync_synchronize();
    //    omp_unset_lock(&reomp_omp_lock);
  }

  return ;
}


static  void reomp_cgate_in(int control, void* ptr, size_t lock_id, int lock)
{
  int clock;
  int tid;

  //  MUTIL_DBG("-- IN");
  if(omp_get_num_threads() == 1) return;
  tid = reomp_get_thread_num();
  reomp_cgate_ticket_wait(tid);
  return;
}



static  void reomp_cgate_out(int control, void* ptr, size_t lock_id, int lock)
{

  int tid;

  //  MUTIL_DBG("-- OUT");
  if(omp_get_num_threads() == 1) return;
  tid = reomp_get_thread_num();
  //  MUTIL_DBG("-- OUT: tid %d: gate_clock: %d", tid, gate_clock);
  reomp_cgate_leave(tid);
  return;
}

static void reomp_scgate_in(int control, void* ptr, size_t lock_id, int lock)
{
  int tid;
  int val;
  reomp_scgate_t *scgate;

  if(omp_get_num_threads() == 1) return;
  
  tid = omp_get_thread_num();
  scgate =&reomp_scgate[tid];
  if (reomp_config.mode == REOMP_ENV_MODE_RECORD) {
#if 0
    val = setjmp(scgate->jmpenv);
    scgate->speculative_clock = __sync_fetch_and_add(&gate_clock, 1);
    //    fprintf(stderr, "tid: %d: val=%d clock=%d\n", tid, val, tmp_clock[tid * 64]);
    if (val == REOMP_SCGATE_LONGJMP) {
      scgate->num_aborted = __sync_fetch_and_add(&reomp_scgate_num_aborted, 1);
    } else {
      scgate->num_aborted = reomp_scgate_num_aborted - 1;
    }
#else
    scgate->speculative_clock = __sync_fetch_and_add(&next_clock, 1);
    while(scgate->speculative_clock != gate_clock);
#endif
  } else {
    reomp_cgate_ticket_wait(tid);
  }
  //  fprintf(stderr, "tid: %d: end in\n", tid);
}

static void reomp_scgate_out(int control, void* ptr, size_t lock_id, int lock)
{
  int tid;
  reomp_scgate_t *scgate;
  int clock_to_record;

  if(omp_get_num_threads() == 1) return;
  
  tid = omp_get_thread_num();
  scgate =&reomp_scgate[tid];
  //  fprintf(stderr, "tid: %d: start out\n", tid);
  if (reomp_config.mode == REOMP_ENV_MODE_RECORD) {
#if 0    
    if (scgate->speculative_clock == gate_clock -1) {
      clock_to_record = scgate->speculative_clock - scgate->num_aborted;
      reomp_cgate_record_ticket_number(tid, clock_to_record - 1);
      fprintf(stderr, "Tid: %d: fwrite %d\n", tid, clock_to_record - 1);
    } else {
      //      fprintf(stderr, "tid: %d: jmp %d\n", tid, tmp_clock[index]);
      longjmp(scgate->jmpenv, REOMP_SCGATE_LONGJMP);
    }
#else
    gate_clock++;
    reomp_cgate_record_ticket_number(tid,  scgate->speculative_clock);
    //    fprintf(stderr, "Tid: %d: fwrite %d\n", tid, scgate->speculative_clock);
#endif
  } else {
    reomp_cgate_leave(tid);
  }
  //  fprintf(stderr, "tid: %d: end out\n", tid);
}



static  void reomp_cgate_in_bef_reduce_begin(int control, void* ptr, size_t null)
{
  size_t reduction_method = -1;
  int tid;
  
  tid = reomp_get_thread_num();
  if (reomp_config.mode == REOMP_ENV_MODE_RECORD) {
    /* Nothing to do */
    reomp_cgate_record_reduction_method_init(tid);
  } else if (reomp_config.mode == REOMP_ENV_MODE_REPLAY) {
    reduction_method = reomp_cgate_read_reduction_method(tid);
    if (reduction_method == REOMP_REDUCE_LOCK_MASTER) {
      reomp_cgate_ticket_wait(tid);
    } else if (reduction_method == REOMP_REDUCE_LOCK_WORKER) {
      /* Pass this gate to synchronize with master and the other workers */
    } else if (reduction_method == REOMP_REDUCE_ATOMIC) {
      /* Pass this gate to synchronize with master and the other workers */
    } else {
      MUTIL_ERR("No such reduction method: %lu (tid: %d)", reduction_method, tid);
    }
  } else {
    MUTIL_ERR("No such reomp_mode: %d", reomp_config.mode);
  }

  return;
}

static  void reomp_cgate_in_aft_reduce_begin(int control, void* ptr, size_t reduction_method)
{
  int tid;
  int clock;
  tid = reomp_get_thread_num();
  if (reomp_config.mode == REOMP_ENV_MODE_RECORD) {
    if (reduction_method == REOMP_REDUCE_LOCK_MASTER) {
      /* If all threads have REOMP_REDUCE_LOCK_MASTER: 
	     This section is already serialized by __kmpc_reduce or __kmpc_reduce_nowait
	 If the other threads has REOMP_REDUCE_LOCK_WORKER
	     This section is not even executed by workers
      */
      reomp_cgate_ticket_wait(tid); /* To get ticket, and will not wait  */
    } else if (reduction_method == REOMP_REDUCE_LOCK_WORKER) {
      /* Worker does not get involved in reduction */
      /* Sicne workers do not execution neither __kmpc_end_reduce nor atomic, 
	 Workers need to record reduction method here now.
       */
      reomp_cgate_record_reduction_method(tid, reduction_method);
    } else if (reduction_method == REOMP_REDUCE_ATOMIC) {
      /* This section is not serialized by __kmpc_reduce and __kmpc_reduct_nowait.
	 ReMPI needs to serialize this section.
       */
      reomp_cgate_ticket_wait(tid);
    } else {
      MUTIL_ERR("No such reduction method: %lu", reduction_method);
    }
  } else if (reomp_config.mode == REOMP_ENV_MODE_REPLAY) {
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
      reomp_cgate_ticket_wait(tid);
    } else {
      MUTIL_ERR("No such reduction method: %lu", reduction_method);
    }
  } else {
    MUTIL_ERR("No such reomp_mode: %d", reomp_config.mode);
  }
  return;
}


static  void reomp_cgate_out_reduce_end(int control, void* ptr, size_t reduction_method)
{
  int tid;
  int clock;
  tid = reomp_get_thread_num();
  if (reomp_config.mode == REOMP_ENV_MODE_RECORD) {
    if (reduction_method == REOMP_REDUCE_LOCK_MASTER) {
      reomp_cgate_leave(tid);
      reomp_cgate_record_reduction_method(tid, reduction_method);
    } else if (reduction_method == REOMP_REDUCE_LOCK_WORKER) {
      /* Worker does not get involved in reduction. */
      /* =================================================== */
      /* ============= This code is never executed ========= */
      /* =================================================== */
    } else if (reduction_method == REOMP_REDUCE_ATOMIC) {
      /* This section is not serialized by __kmpc_reduce and __kmpc_reduct_nowait.
	 ReMPI needs to serialize this section.
       */
      reomp_cgate_leave(tid);
      reomp_cgate_record_reduction_method(tid, reduction_method);
    } else {
      MUTIL_ERR("No such reduction method: %lu", reduction_method);
    }
  } else if (reomp_config.mode == REOMP_ENV_MODE_REPLAY) {
    if (reduction_method == REOMP_REDUCE_LOCK_MASTER) {
      reomp_cgate_leave(tid);
    } else if (reduction_method == REOMP_REDUCE_LOCK_WORKER) {
      /* This section is not even executed by workers */
      /* Worker does not call __kmpc_end_reduce and any atomic operations */
      /* Nothing to do */
    } else if (reduction_method == REOMP_REDUCE_ATOMIC) {
      reomp_cgate_leave(tid);
    } else {
      MUTIL_ERR("No such reduction method: %lu", reduction_method);
    }
  } else {
    MUTIL_ERR("No such reomp_mode: %d", reomp_config.mode);
  }
  return;
}

static  void reomp_cgate_out_bef_reduce_end(int control, void* ptr, size_t reduction_method)
{
  return;
}

static  void reomp_cgate_out_aft_reduce_end(int control, void* ptr, size_t reduction_method)
{
  reomp_cgate_out_reduce_end(control, ptr, reduction_method);
  return;
}


