#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <omp.h>

#include <unordered_map>

#include "mutil.h"
#include "reomp_mem.h"

unordered_map<void*, size_t> reomp_mem_memory_alloc;

void reomp_mem_on_alloc(void* ptr, size_t size) 
{
  if (ptr == NULL) {
    reomp_util_err("memory allocation returned NULL");
    exit(1);
  }
  reomp_mem_memory_alloc[ptr] = size;
  //  MUTIL_DBG("alloc: %p (size: %lu)", ptr, size);
  return;
}  


void reomp_mem_on_free(void* ptr)
{
  if (ptr == NULL) {
    reomp_util_err("free memory whose address is NULL");
  }
  
  if (reomp_mem_memory_alloc.find(ptr) != reomp_mem_memory_alloc.end()) {
    reomp_mem_memory_alloc.erase(ptr);
  } else {
    reomp_util_dbg("freeing non-allocated memory address");
  }
  return;
}

bool reomp_mem_is_alloced(void* ptr) 
{
  return reomp_mem_memory_alloc.find(ptr) != reomp_mem_memory_alloc.end();
}

void reomp_mem_get_alloc_size(void* ptr, size_t* size)
{

  if (reomp_mem_memory_alloc.find(ptr) == reomp_mem_memory_alloc.end()) {
    *size = 0;
    //    reomp_util_err("Address: %p does not exists", ptr);
    MUTIL_DBG("get: %p (size: %lu)", ptr, sizeof(ptr));
  } else {
    *size = reomp_mem_memory_alloc.at(ptr);
    MUTIL_DBG("get: %p (size: %lu)", ptr, *size);
  }
  //  MUTIL_DBG("get: %p (size: %lu)", ptr, *size);
  return;
}
