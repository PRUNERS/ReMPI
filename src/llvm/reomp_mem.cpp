#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <omp.h>
#include <malloc.h>
#include <sys/mman.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <pthread.h>

#include <unordered_map>
#include <list>

#include "mutil.h"
#include "reomp_mem.h"

#define REOMP_MEM_DUMP    (0)
#define REOMP_MEM_RESTORE (1)
//#define REOMP_USE_MMAP

static int reomp_mem_pagesize = -1;

class reomp_mem_variable {
public:
  void* addr;
  size_t size;
  reomp_mem_variable(void* a, size_t s)
    : addr(a)
    , size(s){};
};

unordered_map<void*, size_t> reomp_mem_memory_alloc;
list<reomp_mem_variable*> reomp_mem_local_variable_list;

static void *reomp_mem_malloc_hook(size_t, const void *);
static void reomp_mem_free_hook(void*, const void *);

/* Variables to save original hooks. */
static void *(*old_malloc_hook)(size_t, const void *);
static void (*old_free_hook)(void *ptr, const void *caller);

/* Override initializing hook from the C library. */
//void (*volatile __malloc_initialize_hook) (void) = my_init_hook;                                                                                     

pthread_mutex_t reomp_mem_mutex_malloc;
pthread_mutex_t reomp_mem_mutex_free;


void reomp_mem_init()
{
  reomp_mem_pagesize = sysconf(_SC_PAGE_SIZE);
  pthread_mutex_init(&reomp_mem_mutex_malloc, NULL);
  pthread_mutex_init(&reomp_mem_mutex_free, NULL);
  return;
}

static void
reomp_mem_free_hook(void* ptr, const void *caller)
{
  void *result;
 
  pthread_mutex_lock(&reomp_mem_mutex_malloc);
  reomp_mem_disable_hook();
#ifdef REOMP_USE_MMAP
  if (reomp_mem_memory_alloc.find(ptr) != reomp_mem_memory_alloc.end()) {
    MUTIL_DBG("free: %p size:%lu", ptr, reomp_mem_memory_alloc.at(ptr));
    munmap(ptr, reomp_mem_memory_alloc.at(ptr));
    reomp_mem_memory_alloc.erase(ptr);
  } else {
    free(ptr);
  }
#else
  free(ptr);  
  if (reomp_mem_memory_alloc.find(ptr) != reomp_mem_memory_alloc.end()) {
    reomp_mem_memory_alloc.erase(ptr);
  }
#endif
  reomp_mem_enable_hook();
  pthread_mutex_unlock(&reomp_mem_mutex_malloc);
  
  return;
}



static void *
reomp_mem_malloc_hook(size_t size, const void *caller)
{
  void *ptr;
  
  // pthread_mutex_lock(&reomp_mem_mutex_malloc);
  //  MUTIL_DBG("malloc enter: %d", omp_get_thread_num());
  reomp_mem_disable_hook();  
  //  MUTIL_DBG("malloc enter 1: %d", omp_get_thread_num());
#ifdef REOMP_USE_MMAP
  //  ptr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
  //ptr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  ptr = mmap(NULL, size, PROT_EXEC | PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
  //MUTIL_DBG("before mmap: %p size:%lu (caller: %p)", ptr, size, caller);
#else
  //  ptr = malloc(size);
  ptr = aligned_alloc(reomp_mem_pagesize, size);
  //  MUTIL_DBG("before malloc: %p size:%lu (caller: %p)", ptr, size, caller);
#endif
  //  MUTIL_DBG("malloc enter 2: %d (%p %lu)", omp_get_thread_num(), ptr, size);

  reomp_mem_memory_alloc[ptr] = size;
  //  MUTIL_DBG("malloc enter 3: %d", omp_get_thread_num());
  reomp_mem_enable_hook();
  //  MUTIL_DBG("malloc enter 4: %d", omp_get_thread_num());
  //  pthread_mutex_unlock(&reomp_mem_mutex_malloc);
  //  MUTIL_DBG("malloc leave: %d", omp_get_thread_num());

  
  return ptr;
}

static void reomp_mem_dump_or_restore_heap(FILE *fp, int dump_or_restore)
{
  void* ptr;
  size_t size;
  unordered_map<void*, size_t>::iterator it, it_end; 
  for (it = reomp_mem_memory_alloc.begin(), it_end = reomp_mem_memory_alloc.end();
       it != it_end;
       it++)  {
    ptr  = it->first;
    size = it->second;
    if (dump_or_restore == REOMP_MEM_DUMP) {
      MUTIL_DBG("DUMP: %p size:%lu", ptr, size);
      fwrite(ptr, size, 1, fp);
    } else if (dump_or_restore == REOMP_MEM_RESTORE) {
      fread(ptr, size, 1, fp);
    }
  }
  return;
}



void reomp_mem_print_addr(void* ptr)
{
  printf("==> %p\n", ptr);
  return;
}

// void reomp_mem_remove_old_loval_var_addr()
// {
//   unordered_map<void*, size_t>::iterator it, it_end;
//   for (it = reomp_mem_local_variables.begin(), it_end = reomp_mem_local_variables.end();
//        it != it_end;
//        it++)  {
//   }
// }

void reomp_mem_register_local_var_addr(void* local_ptr, size_t size)
{
  reomp_mem_disable_hook();  
  reomp_mem_local_variable_list.push_back(new reomp_mem_variable(local_ptr, size));
  reomp_mem_enable_hook();  
  return;
}

void reomp_mem_record_or_replay_all_local_vars(FILE *fp, int is_replay)
{
  reomp_mem_disable_hook();  
  void *stack_var_addr, *last_stack_var_addr;
  reomp_mem_variable *var;
  list<reomp_mem_variable*>::reverse_iterator rit, rit_end;
  list<reomp_mem_variable*>::iterator it, it_end;
  //  MUTIL_DBG("size: %lu", reomp_mem_local_variable_list.size());
  //  MUTIL_DBG("%p %p %p", &fp, &is_replay, &var);

  stack_var_addr = &is_replay;
  last_stack_var_addr = 0;
  
  it_end = reomp_mem_local_variable_list.end();
  for (rit = reomp_mem_local_variable_list.rbegin(), rit_end = reomp_mem_local_variable_list.rend();
       rit != rit_end;
       rit++)  {
    var = *rit;
    if (var->addr > stack_var_addr && var->addr > last_stack_var_addr) {
      if (is_replay) {
	fread(var->addr, var->size, 1, fp);
      } else {
	fwrite(var->addr, var->size, 1, fp);
      }
      last_stack_var_addr = var->addr;
    } else {
      it = reomp_mem_local_variable_list.erase(--rit.base());
      if (it == it_end) {
	goto end;
      }
      delete var;
    }
  }

 end:
  reomp_mem_enable_hook();  
  return;
}

void reomp_mem_enable_hook(void)
{
  if (__malloc_hook != &reomp_mem_malloc_hook) {
    /* Save underlying hooks */
    old_malloc_hook = __malloc_hook;
    /* Restore our own hooks */
    __malloc_hook = reomp_mem_malloc_hook;
  }

  if (__free_hook != &reomp_mem_free_hook) {
    /* Save underlying hooks */
    old_free_hook = __free_hook;
    /* Restore our own hooks */
    __free_hook = reomp_mem_free_hook;
  }
}

void reomp_mem_disable_hook(void)
{
  /* Restore all old hooks */
  if (__malloc_hook != old_malloc_hook) {
    __malloc_hook = old_malloc_hook;
  }
  if (__free_hook != old_free_hook) {
    __free_hook = old_free_hook;
  }
}


void reomp_mem_dump_heap(FILE *fp)
{
  reomp_mem_dump_or_restore_heap(fp, REOMP_MEM_DUMP);
}

void reomp_mem_restore_heap(FILE *fp)
{
  reomp_mem_dump_or_restore_heap(fp, REOMP_MEM_RESTORE);
}


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

