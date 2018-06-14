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

#ifdef REOMP_USE_APIO
static int fp = -1;
#else
static FILE *fp = NULL;
static FILE* fds[REOMP_MAX_THREAD];
#endif

typedef struct {
  FILE* fd;
  size_t total_write_bytes;
  size_t write_bytes_at_reduction;
  char padding[128 - sizeof(size_t) * 2 - sizeof(FILE*)];
} reomp_fd_t;

static int reomp_mode = -1;
static int replay_thread_num = -1;
reomp_fd_t reomp_fds[REOMP_MAX_THREAD];



//static pthread_mutex_t file_read_mutex;
//static pthread_mutex_t file_write_mutex;
//static pthread_mutex_t reomp_omp_tid_umap_mutex;
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
static REOMP_INLINE int reomp_get_thread_num()
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

static void reomp_init_file_2(int control)
{    
  for (int i = 0; i < REOMP_MAX_THREAD; i++) {
    reomp_fds[i].fd = NULL;
    reomp_fds[i].total_write_bytes = 0;
    reomp_fds[i].write_bytes_at_reduction = 0;
  }
  omp_init_lock(&reomp_omp_lock);
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


static void reomp_finalize_file_2()
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


double main_s, main_e;
static void reomp_rr_init_2(int control, size_t num_locks)
{
  main_s = reomp_util_get_time();
  reomp_init_callstack();
  reomp_init_env();
  reomp_init_file_2(control);
  reomp_init_locks(num_locks);
  reomp_mem_init();
  //  pthread_mutex_init(&reomp_omp_tid_umap_mutex, NULL);
  omp_init_lock(&reomp_omp_tid_umap_mutex_lock);
  return;
}


static void reomp_rr_init(int control, size_t num_locks)
{
  main_s = reomp_util_get_time();
  reomp_init_callstack();
  reomp_init_env();
  reomp_init_file(control);
  reomp_init_locks(num_locks);
  reomp_mem_init();
  //  pthread_mutex_init(&reomp_omp_tid_umap_mutex, NULL);
  omp_init_lock(&reomp_omp_tid_umap_mutex_lock);
  return;
}

static void reomp_rr_finalize_2()
{
  reomp_finalize_file_2();
  main_e = reomp_util_get_time();
  MUTIL_DBG("REOMP: main_time = %f", main_e - main_s);
  return;
}

static void reomp_rr_finalize()
{
  reomp_finalize_file();
  main_e = reomp_util_get_time();
  MUTIL_DBG("REOMP: main_time = %f", main_e - main_s);
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



static REOMP_INLINE int reomp_get_clock_record(int tid)
{
  int tmp;
#if 0
  pthread_mutex_lock(&lock);
  tmp = reserved_clock++;
  pthread_mutex_unlock(&lock);
#else
  tmp = reserved_clock++;                    
  //  tmp = __sync_fetch_and_add(&reserved_clock, 1);
  // if (previous == tmp) fprintf(stderr, "%d\n", tmp);
  // previous = tmp;
#endif
  return tmp;
}

static REOMP_INLINE void reomp_fwrite(const void * ptr, size_t size, size_t count, int tid)
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

static REOMP_INLINE void reomp_fread(void * ptr, size_t size, size_t count, int tid)
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


static REOMP_INLINE int reomp_get_clock_replay(int tid)
{
  int clock;
  // FILE *fd;
  // size_t s;
  // fd = reomp_get_fd(tid);
  // s = fread(&clock, sizeof(int), 1, fd);
  // if (s != 1) {
  //   if (feof(fd)) MUTIL_ERR("fread reached the end of record file");
  //   if (ferror(fd))  MUTIL_ERR("fread failed");
  // }
  reomp_fread(&clock, sizeof(int), 1 , tid);
  return clock;
}

static REOMP_INLINE size_t reomp_read_reduction_method(int tid)
{
  size_t reduction_method;
  // FILE *fd;
  // size_t s;
  // fd = reomp_get_fd(tid);
  // s = fread(&reduction_method, sizeof(size_t), 1, fd);
  // if (s != 1) {
  //   if (feof(fd)) MUTIL_ERR("fread reached the end of record file");
  //   if (ferror(fd))  MUTIL_ERR("fread failed");
  // }
  //MUTIL_DBG("tid: %d: Reduction: %lu (gk: %d)", tid, reduction_method, gate_clock);
  reomp_fread(&reduction_method, sizeof(size_t), 1 , tid);
  return reduction_method;
}


static REOMP_INLINE void reomp_gate_record_reduction_method_init(int tid) 
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

static REOMP_INLINE void reomp_gate_record_reduction_method(int tid, size_t reduction_method)
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

static REOMP_INLINE void reomp_gate_ticket_wait(int tid)
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

static REOMP_INLINE void reomp_gate_record_ticket_number(int tid, int clock)
{
  reomp_fwrite((void*)&clock, sizeof(int), 1, tid);
  //  if (!tid) MUTIL_DBG("tid: %d: Tnum: %d", tid, clock);
  return;
}

static REOMP_INLINE void reomp_gate_leave(int tid)
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


static REOMP_INLINE void reomp_gate_in_2(int control, void* ptr, size_t lock_id, int lock)
{
  int clock;
  int tid;

  //  MUTIL_DBG("-- IN");
  if(omp_get_num_threads() == 1) return;
  tid = reomp_get_thread_num();

  reomp_gate_ticket_wait(tid);


  return;
}



static REOMP_INLINE void reomp_gate_out_2(int control, void* ptr, size_t lock_id, int lock)
{

  int tid;

  //  MUTIL_DBG("-- OUT");
  if(omp_get_num_threads() == 1) return;
  tid = reomp_get_thread_num();
  //  MUTIL_DBG("-- OUT: tid %d: gate_clock: %d", tid, gate_clock);
  reomp_gate_leave(tid);



  return;
}


static REOMP_INLINE void reomp_gate_in_bef_reduce_begin(int control, void* ptr, size_t null)
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

static REOMP_INLINE void reomp_gate_in_aft_reduce_begin(int control, void* ptr, size_t reduction_method)
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


  //MUTIL_DBG("IN aft: tid %d: gate_clock: %d (method: %d)", tid, gate_clock, reduction_method);
  return;
}


static REOMP_INLINE void reomp_gate_out_reduce_end(int control, void* ptr, size_t reduction_method)
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

static REOMP_INLINE void reomp_gate_out_bef_reduce_end(int control, void* ptr, size_t reduction_method)
{
  // MUTIL_DBG("OUT bef: method: %d", reduction_method);
  // if (reomp_mode == REOMP_RECORD && reduction_method != REOMP_REDUCE_ATOMIC) {
  //   reomp_gate_out_reduce_end(control, ptr, reduction_method);
  // }
  return;
}

static REOMP_INLINE void reomp_gate_out_aft_reduce_end(int control, void* ptr, size_t reduction_method)
{
  // MUTIL_DBG("OUT aft: method: %d", reduction_method);
  // if (!(reomp_mode == REOMP_RECORD && reduction_method != REOMP_REDUCE_ATOMIC)) {
    reomp_gate_out_reduce_end(control, ptr, reduction_method);
    //  }
  return;
}

size_t count = 0;

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

static REOMP_INLINE void reomp_begin_omp()
{
  //  MUTIL_DBG("Bgn: parallel reagion: (tid: %d)", omp_get_thread_num());
#if 0  
  int64_t p_tid;
  int omp_tid;
  int level;
  level = omp_get_level();
  if (level == 1) {
    p_tid = (int64_t)syscall(SYS_gettid);
    omp_tid = omp_get_thread_num();
    omp_set_lock(&reomp_omp_tid_umap_mutex_lock);
    reomp_omp_tid_umap[p_tid] = omp_tid;
    omp_unset_lock(&reomp_omp_tid_umap_mutex_lock);
  }
#endif

  return;
}

static REOMP_INLINE void reomp_end_omp()
{
  //  MUTIL_DBG("End: parallel reagion (tid: %d)", omp_get_thread_num());
#if 0
  int64_t p_tid;
  int level;
  level = omp_get_level();
  if (level == 1) {
    p_tid = (int64_t)syscall(SYS_gettid);
    omp_set_lock(&reomp_omp_tid_umap_mutex_lock);
    reomp_omp_tid_umap.erase(p_tid);
    omp_unset_lock(&reomp_omp_tid_umap_mutex_lock);
  }
#endif

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
    reomp_rr_finalize_2();
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

    reomp_gate_in_2(control, ptr, size, REOMP_WITH_LOCK);
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
    reomp_gate_out_2(control, ptr, size, REOMP_WITH_LOCK);
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
    reomp_gate_in_2(control, ptr, size, REOMP_WITHOUT_LOCK);
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
    reomp_gate_out_2(control, ptr, size, REOMP_WITHOUT_LOCK);
#endif
    //#if REOMP_RR_VERSION == 2
    //    reomp_gate_out_2(control, ptr, size, REOMP_WITHOUT_LOCK);
    //#endif
    //    MUTIL_DBG("tid: %d: out", reomp_get_thread_num());
    break;

  case REOMP_BEF_REDUCE_BEGIN: // 16
    reomp_gate_in_bef_reduce_begin(control, ptr, size);
    break;
  case REOMP_AFT_REDUCE_BEGIN: // 17
    reomp_gate_in_aft_reduce_begin(control, ptr, size);
    break;
  case REOMP_BEF_REDUCE_END:   // 18
    reomp_gate_out_bef_reduce_end(control, ptr, size);
    break;
  case REOMP_AFT_REDUCE_END:   // 19
    reomp_gate_out_aft_reduce_end(control, ptr, size);
    break;

  case REOMP_BEF_FORK: // 20
    reomp_before_fork();
    break;
  case REOMP_AFT_FORK: // 21
    reomp_after_fork();
    break;
  case REOMP_BEG_OMP: // 22
    reomp_begin_omp();
    break;
  case REOMP_END_OMP: // 23
    reomp_end_omp();
    break;
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
//  if (tid == time_tid) control_time += reomp_util_get_time() - tmp_time2;
  if (tid == time_tid) MUTIL_TIMER(MUTIL_TIMER_STOP, REOMP_TIMER_ENTIRE, NULL);

  return;
}


void logop(int i)
{
    fprintf(stderr, "Computed: %d\n", i);
}


