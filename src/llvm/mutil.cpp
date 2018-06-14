#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <stdarg.h>
#include <execinfo.h>
#include <unistd.h>
#include <assert.h>
#include <pthread.h>
#include <sys/time.h>
#include <sys/syscall.h>
#include <assert.h>
//#include <bfd.h>
#include <libgen.h>
#include <execinfo.h>

#include <string>
#include <queue>
#include <unordered_map>

#include "mutil.h"

#define DEBUG_STDOUT stderr


typedef struct
{
  double start;
  double sum;
} timer_counters_t;

using namespace std;
int mutil_my_rank = -1;
char mutil_hostname[256];
char header[128];
char message[2048];

static queue<char*> mutil_log_q;
static unsigned long mutil_total_alloc_size = 0;
static unsigned long mutil_total_alloc_count = 0;

static pthread_mutex_t print_mutex = PTHREAD_MUTEX_INITIALIZER;

#define MUTIL_OUTPUT(stream, label, fmt)        \
  do {                                          \
  va_list argp;                               \
  va_start(argp, fmt);                        \
  sprintf(header, "%s:%s:%s:%3d: ", MUTIL_PREFIX, label, mutil_hostname, mutil_my_rank); \
  vsprintf(message, fmt, argp); \
  va_end(argp);  \
  fprintf(stream, "%s%s\n", header, message);   \
  fflush(stream);				\
  } while(0)


void MUTIL_FUNC(err)(const char* fmt, ...)
{
#ifdef MUTIL_DBG_LOG
  mutil_dbg_log_print();
#endif
  MUTIL_OUTPUT(stderr, "** ERROR **", fmt);
  exit(15);
  return;
}

void MUTIL_FUNC(erri)(int r, const char* fmt, ...)
{
  if (mutil_my_rank != r) return;
  MUTIL_OUTPUT(stderr, "** ERROR **", fmt);
  exit(15);
  return;
}

void MUTIL_FUNC(alert)(const char* fmt, ...)
{
  MUTIL_OUTPUT(stderr, "ALERT", fmt);
  exit(1);
  return;
}

void MUTIL_FUNC(dbg)(const char* fmt, ...) {
  pthread_mutex_lock (&print_mutex);
  MUTIL_OUTPUT(stderr, "DEBUG", fmt);
  pthread_mutex_unlock (&print_mutex);
  return;
}

void MUTIL_FUNC(print)(const char* fmt, ...) {
  MUTIL_OUTPUT(stdout, "", fmt);
  return;
}

void MUTIL_FUNC(dbg_log_print)()
{
  pthread_mutex_lock (&print_mutex);
  fprintf(stdout, "%s:DEBUG:%s:%3d: Debug log dumping ... \n", MUTIL_PREFIX, mutil_hostname, mutil_my_rank);
  fflush(stdout);
  while (!mutil_log_q.empty()) {
    char* log = mutil_log_q.front();
    fprintf(DEBUG_STDOUT, "%s:DEBUG:%s:%3d: %s\n", MUTIL_PREFIX, mutil_hostname, mutil_my_rank, log);
    mutil_log_q.pop();
    free(log);
  }
  pthread_mutex_unlock (&print_mutex);
}

void MUTIL_FUNC(dbgi)(int r, const char* fmt, ...) {
  if (mutil_my_rank != r && -1 != r) return;
  va_list argp;
  pthread_mutex_lock (&print_mutex);

#ifdef MUTIL_DBG_LOG
  char *log;
  if (mutil_log_q.size() > MUTIL_DBG_LOG_BUF_LENGTH) {
    log = mutil_log_q.front();
    mutil_log_q.pop();
  } else {
    log = (char*)MUTIL_FUNC(malloc)(MUTIL_DBG_LOG_BUF_SIZE);
  }

  va_start(argp, fmt);
  vsnprintf(log, MUTIL_DBG_LOG_BUF_SIZE, fmt, argp);
  //  sprintf(log, "%s\0", log);
  va_end(argp);
  mutil_log_q.push(log);

#else
  fprintf(DEBUG_STDOUT, "%s:DEBUG:%s:%3d: ", MUTIL_PREFIX, mutil_hostname, mutil_my_rank);
  va_start(argp, fmt);
  vfprintf(DEBUG_STDOUT, fmt, argp);
  va_end(argp);
  fprintf(DEBUG_STDOUT, "\n");
#endif
  pthread_mutex_unlock (&print_mutex);
}


void MUTIL_FUNC(exit)(int no) {
  fprintf(stderr, "%s:DEBUG:%s:%d: == EXIT == sleep 1sec ...\n", MUTIL_PREFIX, mutil_hostname, mutil_my_rank);
  sleep(1);
  exit(no);
  return;
}





void MUTIL_FUNC(set_configuration)(int *argc, char ***argv)
{

  return;
}


void MUTIL_FUNC(init)(int r) {
  mutil_my_rank = r;
  if (gethostname(mutil_hostname, sizeof(mutil_hostname)) != 0) {
    MUTIL_FUNC(err)("gethostname fialed (%s:%s:%d)", __FILE__, __func__, __LINE__);
  }
  return;
}

char* steam_gethostname() {
  return mutil_hostname;
}


void MUTIL_FUNC(assert)(int b)
{
  assert(b);
  return;
}



// string MUTIL_FUNC(btrace_string)()
// {
//   int j, nptrs;
//   void *buffer[1024];
//   char **strings;
//   string trace_string;

//   nptrs = backtrace(buffer, 1024);

//   /* backtrace_symbols_fd(buffer, nptrs, STDOUT_FILENO)*/
//   strings = backtrace_symbols(buffer, nptrs);
//   if (strings == NULL) {
//     perror("backtrace_symbols");
//     exit(EXIT_FAILURE);
//   }
//   /*
//     You can translate the address to function name by
//     addr2line -f -e ./a.out <address>
//   */
//   for (j = 0; j < nptrs; j++)
//     trace_string += strings[j] ;
//   free(strings);
//   return trace_string;
// }

void MUTIL_FUNC(btrace)() 
{
  int j, nptrs;
  void *buffer[100];
  char **strings;

  nptrs = backtrace(buffer, 100);

  /* backtrace_symbols_fd(buffer, nptrs, STDOUT_FILENO)*/
  strings = backtrace_symbols(buffer, nptrs);
  if (strings == NULL) {
    perror("backtrace_symbols");
    exit(EXIT_FAILURE);
  }   

  /*
    You can translate the address to function name by
    addr2line -f -e ./a.out <address>
  */
  for (j = 0; j < nptrs; j++) {
    MUTIL_FUNC(dbg)(" #%d %s %p", j, strings[j], buffer[j]);
  }
  free(strings);
  return;
}

int MUTIL_FUNC(btrace_get)(char*** strings, int len) 
{
  int j, nptrs;
  void *buffer[100];

  nptrs = backtrace(buffer, 100);

  /* backtrace_symbols_fd(buffer, nptrs, STDOUT_FILENO)*/
  *strings = backtrace_symbols(buffer, nptrs);
  if (strings == NULL) {
    perror("backtrace_symbols");
    exit(EXIT_FAILURE);
  }   
  
  return nptrs;
}

size_t MUTIL_FUNC(btrace_hash)() 
{
  int nptrs;
  void *buffer[100];
  size_t hash = 1883;

  nptrs = backtrace(buffer, 100);
  for (int i = 0; i < nptrs; i++){
    hash = MUTIL_FUNC(hash)(hash, (size_t)buffer[i]);    
  }
  return hash;
}






#if 0
void bt_sighandler(int sig, struct sigcontext ctx) {

  void *trace[16];
  char **messages = (char **)NULL;
  int i, trace_size = 0;

  if (sig == SIGSEGV)
    printf("Got signal %d, faulty address is %p, "
           "from %p\n", sig, ctx.cr2, ctx.eip);
  else
    printf("Got signal %d\n", sig);

  trace_size = backtrace(trace, 16);
  /* overwrite sigaction with caller's address */
  trace[1] = (void *)ctx.eip;
  messages = backtrace_symbols(trace, trace_size);
  /* skip first stack frame (points here) */
  printf("[bt] Execution path:\n");
  for (i=1; i<trace_size; ++i)
    {
      printf("[bt] #%d %s\n", i, messages[i]);

      /* find first occurence of '(' or ' ' in message[i] and assume
       * everything before that is the file name. (Don't go beyond 0 though
       * (string terminator)*/
      size_t p = 0;
      while(messages[i][p] != '(' && messages[i][p] != ' '
            && messages[i][p] != 0)
        ++p;

      char syscom[256];
      sprintf(syscom,"addr2line %p -e %.*s", trace[i], p, messages[i]);
      //last parameter is the file name of the symbol
      system(syscom);
    }

  exit(0);
}
#endif

void MUTIL_FUNC(btracei)(int r) 
{
  if (mutil_my_rank != r) return;
  MUTIL_FUNC(btrace)();
  return;
}


void* MUTIL_FUNC(malloc)(size_t size) 
{
  void* addr;
  if ((addr = malloc(size)) == NULL) {
    MUTIL_FUNC(dbg)("Memory allocation returned NULL (%s:%s:%d)",  __FILE__, __func__, __LINE__);
    MUTIL_FUNC(assert)(0);
  }
  mutil_total_alloc_count++;

  //TODO: Manage memory consumption
  //  total_alloc_size += size;
  // pid_t tid;
  // tid = syscall(SYS_gettid);
  // MUTIL_DBG("malloc: %d, size: %lu", tid, size);
  //  mutil_dbg("malloc: done %d", total_alloc_size);
  return addr;
}


void* MUTIL_FUNC(realloc)(void *ptr, size_t size) 
{
  void* addr;
  if ((addr = realloc(ptr, size)) == NULL) {
    MUTIL_FUNC(dbg)("Memory reallocation returned NULL: ptr: %p, size: %lu (%s:%s:%d)", ptr, size,  __FILE__, __func__, __LINE__);
    MUTIL_FUNC(assert)(0);
  }
  if (ptr == NULL) {
    mutil_total_alloc_count++;
  }

  //TODO: Manage memory consumption
  //  total_alloc_size += size;
  // pid_t tid;
  // tid = syscall(SYS_gettid);
  // MUTIL_DBG("malloc: %d, size: %lu", tid, size);
  //  mutil_dbg("malloc: done %d", total_alloc_size);
  return addr;
}

void MUTIL_FUNC(free)(void* addr) 
{
  free(addr);
  mutil_total_alloc_count--;

  //TODO: Manage memory consumption
  //  total_alloc_size -= size;
  return;
}




double MUTIL_FUNC(get_time)(void)
{
  double t;
  struct timeval tv;
  gettimeofday(&tv, NULL);
  t = ((double)(tv.tv_sec) + (double)(tv.tv_usec) * 0.001 * 0.001);
  //  mutil_dbg(" -== > %f", t);
  return t;
}

void MUTIL_FUNC(timer)(int mode, int timerIndex, double *time)
{
  static unordered_map<int, timer_counters_t*> timer_umap;
  timer_counters_t *tcs;
  switch(mode) {
  case MUTIL_TIMER_START:
    if (timer_umap.find(timerIndex) == timer_umap.end()) {
      tcs = (timer_counters_t*)malloc(sizeof(timer_counters_t));
      tcs->start = 0;
      tcs->sum = 0;
      timer_umap[timerIndex] = tcs;
    } else {
      tcs = timer_umap[timerIndex];
    }
    if (tcs->start != 0) {
      MUTIL_ERR("Double timer start: Timer is still running: timer index: %d", timerIndex);
      exit(1);
    }
    tcs->start = MUTIL_FUNC(get_time)();
    break;
  case MUTIL_TIMER_STOP:
    if (timer_umap.find(timerIndex) == timer_umap.end()) {
      MUTIL_ERR("No such timer index");
      exit(1);
    }
    tcs = timer_umap[timerIndex];
    if (tcs->start == 0) {
      MUTIL_ERR("Timer has not started yet");
      exit(1);
    }
    tcs->sum += MUTIL_FUNC(get_time)() - tcs->start;
    tcs->start = 0;
    break;
  case MUTIL_TIMER_GET_TIME:
    if (timer_umap.find(timerIndex) == timer_umap.end()) {
      MUTIL_ERR("No such timer index: %d", timerIndex);
      exit(1);
    }
    tcs = timer_umap[timerIndex];
    if (tcs->start != 0) {
      MUTIL_ERR("Timer is still running: timer index: %d", timerIndex);
      exit(1);
    }
    if (time == NULL) {
      MUTIL_ERR("Time variable is NUL");
      exit(1);
    }
    *time = tcs->sum;
    break;
  }
  return;
}


void MUTIL_FUNC(dbg_sleep_sec)(int sec)
{
  MUTIL_FUNC(dbg)("Sleep: %d sec (%s:%s:%d)", sec, __FILE__, __func__, __LINE__);
  sleep(sec);
  return;
}
void MUTIL_FUNC(sleep_sec)(int sec)
{
  sleep(sec);
  return;
}

void MUTIL_FUNC(dbg_sleep_usec)(int usec)
{
  MUTIL_FUNC(dbg)("Sleep: %d usec (%s:%s:%d)\n", usec, __FILE__, __func__, __LINE__);
  usleep(usec);
  return;
}

void MUTIL_FUNC(sleep_usec)(int usec)
{
  usleep(usec);
  return;
}


unsigned int MUTIL_FUNC(hash)(unsigned int original_val, unsigned int new_val)
{
  return ((original_val << 5) + original_val) + new_val;
}

unsigned int MUTIL_FUNC(hash_str)(const char* c, unsigned int length)
{
  unsigned int i;
  unsigned int hash = 15;
  for (i = 0; i < length; i++) {
    hash = MUTIL_FUNC(hash)(hash, (int)(c[i])) ;
  }
  return hash;
}


int MUTIL_FUNC(init_rand)(int seed) 
{
  srand(seed);
  return 0;
}

int MUTIL_FUNC(init_ndrand)() 
{
  srand((int)MUTIL_FUNC(get_time)());
  return 0;
}

int MUTIL_FUNC(get_rand)(int max)
{
  return rand() % max;
}

int MUTIL_FUNC(str_starts_with)(const char* base, const char* str)
{
  size_t base_len = strlen(base);
  size_t str_len  = strlen(str);
  if (base_len < str_len) return 0;
  //  fprintf(stderr, "##### %s & %s #######\n", base, str);
  if (strncmp(str, base, str_len) != 0) return 0;
  return 1;
}


#if 0

#ifndef _GNU_SOURCE
#define _GNU_SOURCE /* for Dl_info */
#endif

#include <dlfcn.h>

#define lengthof(array) (sizeof(array)/sizeof((array)[0]))

extern void *__libc_stack_end;

static int is_init_bfd = 0;
static bfd *abfd;
static asymbol **symbols;
static int nsymbols;


/**
 *  Frame structure.
 */
#if defined(__x86_64)
struct backtrace_frame {
  void *fp;
  void *lr;
};
#elif defined(__ARM_ARCH_ISA_ARM)
struct backtrace_frame {
  void *fp;
  void *sp;
  void *lr;
  void *pc;
};
#endif


size_t Backtrace(void **frames, size_t size)
{
  void *top_frame_p;
  void *current_frame_p;
  struct backtrace_frame *frame_p;
  size_t frame_count;

  top_frame_p     = __builtin_frame_address(0);
  current_frame_p = top_frame_p;
  frame_count     = 0;

  if ((current_frame_p != NULL)
      && (current_frame_p > (void *) &frame_count)
      && (current_frame_p < __libc_stack_end)) {
    
    while ((frame_count < size)
	   && (current_frame_p != NULL)
	   && (current_frame_p > (void *) &frame_count)
	   && (current_frame_p < __libc_stack_end)) {
        
#if defined(__x86_64)
      frame_p = (struct backtrace_frame *) (void **) current_frame_p;
#elif defined(__ARM_ARCH_ISA_ARM)
      frame_p = (struct backtrace_frame *) ((void **) current_frame_p - 3);
#endif
      frames[frame_count] = frame_p->lr;
      ++frame_count;
      current_frame_p = frame_p->fp;
    }
  }

  return frame_count;
}

void PrintBacktrace(void)
{
  //  if (!is_init_bfd) init_bfd_stuff();

  void *stack[128] = { NULL }, *address;
  Dl_info info;
  asection *section = bfd_get_section_by_name(abfd, ".debug_info");
  const char *file_name;
  const char *func_name;
  unsigned int lineno;
  int found;
  int i;

  
  if (Backtrace(stack, lengthof(stack)) == 0) {
    fprintf(stderr, "not found backtrace...\n");
    return;
  }
  for (i = 0; stack[i] != NULL; ++i) {
    address = stack[i];

    if (section != NULL) {
      found = bfd_find_nearest_line(abfd, section, symbols, (long) address - 1, &file_name, &func_name, &lineno);
    }
    if (dladdr(address, &info) == 0) {
      fprintf(stderr, "  unknown(?+?)[%p]\n",
	      address);
    } else if (found == 0 || file_name == NULL) {
      fprintf(stderr, "  %s(%s+%p)[%p]\n",
	      info.dli_fname, info.dli_sname, (long)address - (long)info.dli_saddr, address);
    } else {
      fprintf(stderr, "  %s(%s+%p)[%p] at %s:%d\n",
	      info.dli_fname, info.dli_sname, (long)address - (long)info.dli_saddr, address, file_name, lineno);
    }
  }

  return;
}


__attribute__((constructor))
void init_bfd_stuff ()
{
  abfd = bfd_openr("/proc/self/exe", NULL);
  assert(abfd != NULL);
  bfd_check_format(abfd, bfd_object);

  int size = bfd_get_symtab_upper_bound(abfd);
  assert(size > 0);
  symbols = (asymbol**)malloc(size);
  assert(symbols != NULL);
  nsymbols = bfd_canonicalize_symtab(abfd, symbols);
  is_init_bfd = 1;
}


static int get_btrace_line(long address, const char** file_name, const char** func_name, unsigned int* line_no, int *column)
{
  asection *section;
  int found;
  

  section = bfd_get_section_by_name(abfd, ".debug_info");
  assert(section != NULL);
  found = bfd_find_nearest_line(abfd, section, symbols,
				address,
				file_name,
				func_name,
				line_no);
  if (!found) return 0;
    //  if (!found) MUTIL_FUNC(err)("failed to get bfd info");

  //  MUTIL_FUNC(dbg)("%s:%s:%d\n", file_name, function_name, lineno);
  //MUTIL_FUNC(dbg)("%s:%s:%d", f_name, func_name, ln);
  return 1;
}

void MUTIL_FUNC(btrace_line)() 
{
  void *trace[128];
  int n;
  const char* file_name;
  const char* func_name;
  unsigned int line_no;
  
  if (!is_init_bfd) init_bfd_stuff();

  n = backtrace(trace, sizeof(trace) / sizeof(trace[0]));
  for (int i = 0; i < n; i++) {
    long t;
    t = (long)trace[i];
    if (get_btrace_line(t--, &file_name, &func_name, &line_no, NULL) > 0) {
      MUTIL_FUNC(dbg)(" #%d %s:%d %d", i, file_name, line_no, n);
    } else {
      MUTIL_FUNC(dbg)(" #%d %s:%d %d", i, file_name, line_no, n);
      //    MUTIL_FUNC(dbg)(" #%d (null):0 %d", i, n);
    }

  }
  return;
}
#endif
