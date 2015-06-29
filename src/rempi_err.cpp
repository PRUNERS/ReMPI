#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <execinfo.h>
#include <unistd.h>

#include <pthread.h>

#include <string>

#include "rempi_err.h"

#define DEBUG_STDOUT stderr

pthread_mutex_t print_mutex;

using namespace std;

int my_rank;
char hostname[256];


void rempi_err_init(int r) 
{
  my_rank = r;
  if (gethostname(hostname, sizeof(hostname)) != 0) {
    rempi_err("gethostname fialed (%s:%s:%d)", __FILE__, __func__, __LINE__);
  }
  return;
}

char* rempi_gethostname() {
  return hostname;
}

void rempi_err(const char* fmt, ...)
{
  va_list argp;
  fprintf(stderr, "REMPI:ERROR:%s:%3d: ", hostname, my_rank);
  va_start(argp, fmt);
  vfprintf(stderr, fmt, argp);
  va_end(argp);
  fprintf(stderr, "\n");
  fflush(stderr);
  exit(1);
  return;
}

void rempi_erri(int r, const char* fmt, ...)
{
  if (my_rank != r) return;
  va_list argp;
  fprintf(stderr, "REMPI:ERROR:%s:%3d: ", hostname, my_rank);
  va_start(argp, fmt);
  vfprintf(stderr, fmt, argp);
  va_end(argp);
  fprintf(stderr, "\n");
  exit(1);
  return;
}

void rempi_alert(const char* fmt, ...)
{
  va_list argp;
  fprintf(stderr, "GIO:ALERT:%s:%d: ", hostname, my_rank);
  va_start(argp, fmt);
  vfprintf(stderr, fmt, argp);
  va_end(argp);
  fprintf(stderr, " (%s:%s:%d)\n", __FILE__, __func__, __LINE__);
  exit(1);
  return;
}

void rempi_dbg(const char* fmt, ...) {
  va_list argp;
  fprintf(DEBUG_STDOUT, "REMPI:DEBUG:%s:%3d: ", hostname, my_rank);
  va_start(argp, fmt);
  vfprintf(DEBUG_STDOUT, fmt, argp);
  va_end(argp);
  fprintf(DEBUG_STDOUT, "\n");
  return;
}

void rempi_print(const char* fmt, ...) {
  va_list argp;
  fprintf(stdout, "REMPI:%s:%d: ", hostname, my_rank);
  va_start(argp, fmt);
  vfprintf(stdout, fmt, argp);
  va_end(argp);
  fprintf(stdout, "\n");
  return;
}

void rempi_dbgi(int r, const char* fmt, ...) {
  if (my_rank != r && -1 != r) return;
  pthread_mutex_lock (&print_mutex);
  va_list argp;
  fprintf(DEBUG_STDOUT, "REMPI:DEBUG:%s:%3d: ", hostname, my_rank);
  va_start(argp, fmt);
  vfprintf(DEBUG_STDOUT, fmt, argp);
  va_end(argp);
  fprintf(DEBUG_STDOUT, "\n");
  pthread_mutex_unlock (&print_mutex);
}

void rempi_debug(const char* fmt, ...)
{
  va_list argp;
  fprintf(DEBUG_STDOUT, "REMPI:DEBUG:%s:%d: ", hostname, my_rank);
  va_start(argp, fmt);
  vfprintf(DEBUG_STDOUT, fmt, argp);
  va_end(argp);
  fprintf(DEBUG_STDOUT, "\n");
}


void rempi_exit(int no) {
  fprintf(stderr, "REMPI:DEBUG:%s:%d: == EXIT == sleep 1sec ...\n", hostname, my_rank);
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
  void *buffer[512];
  char **strings;
  string trace_string;

  nptrs = backtrace(buffer, 512);

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
  if (my_rank != r) return;
  rempi_btrace();
  return;
}
