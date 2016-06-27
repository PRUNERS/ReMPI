#include <stdlib.h>
#include <stdio.h>

#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>

#include "rempi_err.h"

unsigned long total_alloc_size = 0;
unsigned long total_alloc_count = 0;

void* rempi_malloc(size_t size) 
{
  void* addr;
  if ((addr = malloc(size)) == NULL) {
    REMPI_DBG("Memory allocation returned NULL (%s:%s:%d)",  __FILE__, __func__, __LINE__);
    REMPI_ASSERT(0);
  }
  total_alloc_count++;

  //TODO: Manage memory consumption
  //  total_alloc_size += size;
  // pid_t tid;
  // tid = syscall(SYS_gettid);
  // REMPI_DBG("malloc: %d, size: %lu", tid, size);
  //  rempi_dbg("malloc: done %d", total_alloc_size);
  return addr;
}

void rempi_free(void* addr) 
{
  free(addr);
  total_alloc_count--;

  //TODO: Manage memory consumption
  //  total_alloc_size -= size;
  return;
}
