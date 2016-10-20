#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <execinfo.h>
#include <unistd.h>
#include <assert.h>
#include <pthread.h>

#include <string>
#include <queue>

#include "rempi_err.h"
#include "rempi_mem.h"
#include "rempi_config.h"

#define DEBUG_STDOUT stderr

pthread_mutex_t print_mutex;

using namespace std;

int rempi_my_rank = -1;
static char hostname[256];

queue<char*> rempi_log_q;

char header[128];
char message[1024];

#define REMPI_OUTPUT(stream, label, fmt)		\
  do { \
    va_list argp; \
    va_start(argp, fmt); \
    rempi_set_header_message(label, fmt, argp); \
    va_end(argp); \
    rempi_print_header_message(DEBUG_STDOUT); \
  } while(0)

static void rempi_set_header_message(const char* label, const char* fmt, va_list argp)
{
  sprintf(header, "REMPI:%s:%s:%3d: ", label, hostname, rempi_my_rank);
  vsprintf(message, fmt, argp);
  return;
}

static void rempi_print_header_message(FILE *stream)
{
  fprintf(stream, "%s%s\n", header, message);
  return;
}

void rempi_err_init(int r) {
  rempi_my_rank = r;
  if (gethostname(hostname, sizeof(hostname)) != 0) {
    rempi_err("gethostname fialed (%s:%s:%d)", __FILE__, __func__, __LINE__);
  }
  return;
}

char* rempi_gethostname() {
  return hostname;
}


void rempi_assert(int b)
{
  assert(b);
  return;
}

void rempi_err(const char* fmt, ...)
{
  va_list argp;
#ifdef REMPI_DBG_LOG
  rempi_dbg_log_print();
#endif
  REMPI_OUTPUT(stderr, " ** ERROR **", fmt);
  exit(15);
  return;
}

void rempi_erri(int r, const char* fmt, ...)
{
  if (rempi_my_rank != r) return;
  REMPI_OUTPUT(stderr, " ** ERROR **", fmt);
  exit(15);
  return;
}

void rempi_alert(const char* fmt, ...)
{
  REMPI_OUTPUT(stderr, "ALERT", fmt);
  exit(1);
  return;
}

void rempi_dbg(const char* fmt, ...) {
  pthread_mutex_lock (&print_mutex);
  REMPI_OUTPUT(DEBUG_STDOUT, "DEBUG", fmt);
  pthread_mutex_unlock (&print_mutex);
  return;
}

void rempi_print(const char* fmt, ...) {
  REMPI_OUTPUT(stdout, "", fmt);
  return;
}

void rempi_dbg_log_print()
{
  pthread_mutex_lock (&print_mutex);
  fprintf(stdout, "REMPI:DEBUG:%s:%3d: Debug log dumping ... \n", hostname, rempi_my_rank);
  fflush(stdout);
  while (!rempi_log_q.empty()) {
    char* log = rempi_log_q.front();
    fprintf(DEBUG_STDOUT, "REMPI:DEBUG:%s:%3d: %s\n", hostname, rempi_my_rank, log);
    rempi_log_q.pop();
    free(log);
  }
  pthread_mutex_unlock (&print_mutex);
}

void rempi_dbgi(int r, const char* fmt, ...) {
  if (rempi_my_rank != r && -1 != r) return;
  va_list argp;
  pthread_mutex_lock (&print_mutex);

#ifdef REMPI_DBG_LOG
  char *log;
  if (rempi_log_q.size() > REMPI_DBG_LOG_BUF_LENGTH) {
    log = rempi_log_q.front();
    rempi_log_q.pop();
  } else {
    log = (char*)rempi_malloc(REMPI_DBG_LOG_BUF_SIZE);
  }

  va_start(argp, fmt);
  vsnprintf(log, REMPI_DBG_LOG_BUF_SIZE, fmt, argp);
  //  sprintf(log, "%s\0", log);
  va_end(argp);
  rempi_log_q.push(log);

#else
  REMPI_OUTPUT(DEBUG_STDOUT, "DEBUG", fmt);
#endif
  pthread_mutex_unlock (&print_mutex);
}

void rempi_printi(int r, const char* fmt, ...) {
  if (rempi_my_rank != r && -1 != r) return;
  pthread_mutex_lock (&print_mutex);
  REMPI_OUTPUT(stdout, "", fmt);
  pthread_mutex_unlock (&print_mutex);
}

void rempi_debug(const char* fmt, ...)
{
  pthread_mutex_lock (&print_mutex);
  REMPI_OUTPUT(stderr, "DEBUG", fmt);
  pthread_mutex_unlock (&print_mutex);
}


void rempi_exit(int no) {
  fprintf(stderr, "REMPI:DEBUG:%s:%d: == EXIT == sleep 1sec ...\n", hostname, rempi_my_rank);
  sleep(1);
  exit(no);
  return;
}

/* void rempi_test(void* ptr, char* file, char* func, int line) */
/* { */
/*   if (ptr == NULL) { */
/*     rempi_err("NULL POINTER EXCEPTION (%s:%s:%d)", file, func, line); */
/*   } */
/* } */

string rempi_btrace_string()
{
  int j, nptrs;
  void *buffer[1024];
  char **strings;
  string trace_string;

  nptrs = backtrace(buffer, 1024);
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
  for (j = 0; j < nptrs; j++)
    trace_string += strings[j] ;
  free(strings);
  return trace_string;
}

void rempi_btrace() 
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
  for (j = 0; j < nptrs; j++)
    rempi_dbg("%s", strings[j]);
  free(strings);
  return;
}

void rempi_btracei(int r) 
{
  if (rempi_my_rank != r) return;
  rempi_btrace();
  return;
}
