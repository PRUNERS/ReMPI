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
#include <dlfcn.h>


#include <unordered_map>
#include <list>

#include "mutil.h"
#include "reomp_mem.h"

#define REOMP_MEM_DUMP    (0)
#define REOMP_MEM_RESTORE (1)
//#define REOMP_USE_MMAP

#define LIBC_SO "libc.so"

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

static void *(*reomp_mem_real_malloc)(size_t) = NULL;
static void* reomp_mem_wrap_malloc(size_t);
static void (*reomp_mem_real_free)(void*) = NULL;
static void reomp_mem_wrap_free(void*);

static void *reomp_mem_malloc_hook(size_t, const void *);
static void reomp_mem_free_hook(void*, const void *);
/* Variables to save original hooks. */
static void *(*old_malloc_hook)(size_t, const void *);
static void (*old_free_hook)(void *ptr, const void *caller);

/* Override initializing hook from the C library. */
//void (*volatile __malloc_initialize_hook) (void) = my_init_hook;                                                                                     

static int entry_counter = 0;
static void* reomp_mem_dl_handle = NULL;

pthread_mutex_t reomp_mem_mutex_malloc;
pthread_mutex_t reomp_mem_mutex_free;


void reomp_mem_init()
{
  reomp_mem_pagesize = sysconf(_SC_PAGE_SIZE);
  // reomp_mem_real_malloc = NULL;
  // reomp_mem_real_malloc = (void*(*)(size_t))dlsym(RTLD_NEXT, "malloc");

  // if (!reomp_mem_real_malloc) {
  //   MUTIL_ERR("dlsym(malloc) failed");
  // }
  // reomp_mem_real_free   = (void(*)(void*))dlsym(RTLD_NEXT, "free");
  // if (!reomp_mem_real_free) {
  //   MUTIL_ERR("dlsym(free) failed");
  // }
  pthread_mutex_init(&reomp_mem_mutex_malloc, NULL);
  pthread_mutex_init(&reomp_mem_mutex_free, NULL);
  return;
}

static void reomp_mem_check_symbol_address(void * addr)
{
  Dl_info dl_info;
  if (!dladdr(addr, &dl_info)) {
    MUTIL_ERR("dladdr(failed)");
  }
  if (strstr(dl_info.dli_fname, "ld_linux")) {
    MUTIL_ERR("Could not find real free");
  }
  return;  
}

static void* reomp_mem_get_dl_handle()
{
#if 1
  reomp_mem_dl_handle = RTLD_NEXT;
#else 
  if (reomp_mem_dl_handle == NULL) {
    reomp_mem_dl_handle = dlopen(LIBC_SO, RTLD_LAZY);
    if (!reomp_mem_dl_handle) {
      reomp_mem_dl_handle = RTLD_NEXT;
    }
  }
#endif
  return reomp_mem_dl_handle;
}

static void reomp_mem_init_real_free(void)
{ 
  void *handle;
  handle = reomp_mem_get_dl_handle();
  reomp_mem_real_free   = (void(*)(void*)) dlsym(handle,"free");
  //  reomp_mem_check_symbol_address((void*)reomp_mem_real_free);
  return;
}

static void reomp_mem_init_real_malloc(void)
{ 
  void *handle;
  handle = reomp_mem_get_dl_handle();
  reomp_mem_real_malloc = (void*(*)(size_t))dlsym(handle, "malloc");
  //  reomp_mem_check_symbol_address((void*)reomp_mem_real_malloc);     
  return;
}

void free(void *ptr)
{ 
  if (ptr == NULL) return;
  if(reomp_mem_real_free==NULL) {
    reomp_mem_init_real_free();
  }
  reomp_mem_real_free(ptr);
  fprintf(stderr, "free(%p)\n", ptr);
}

void *malloc(size_t size)
{ 
  if(reomp_mem_real_malloc == NULL) {
    reomp_mem_init_real_malloc();
   }
  void *p = NULL;
  fprintf(stderr, "malloc(%lu)\n", size);
  p = reomp_mem_real_malloc(size);
  fprintf(stderr, "%p\n", p);
  return p;
}




// void* malloc(size_t size)
// {
//   if (reomp_mem_real_malloc == NULL) {
//     reomp_mem_init_memory_hook();
//   }
//   return reomp_mem_real_malloc(size);
// }

static void *reomp_mem_wrap_malloc(size_t size)
{
  void *ptr;
#ifdef REOMP_USE_MMAP
  ptr = mmap(NULL, size, PROT_EXEC | PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
#else
  fprintf(stderr, "Stop me 5!!\n");
  //  ptr = aligned_alloc(reomp_mem_pagesize, size);
  ptr = reomp_mem_real_malloc(size);
  fprintf(stderr, "Stop me 6!!\n");
#endif
  //  reomp_mem_memory_alloc[ptr] = size;
  return ptr;
}

static void *
reomp_mem_malloc_hook(size_t size, const void *caller)
{
  void *ptr;
#ifdef REOMP_USE_MMAP
  ptr = mmap(NULL, size, PROT_EXEC | PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
#else
  ptr = aligned_alloc(reomp_mem_pagesize, size);
#endif
  reomp_mem_memory_alloc[ptr] = size;
  
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


static void reomp_mem_wrap_free(void* ptr)
{
#ifdef REOMP_USE_MMAP
  if (reomp_mem_memory_alloc.find(ptr) != reomp_mem_memory_alloc.end()) {
    MUTIL_DBG("free: %p size:%lu", ptr, reomp_mem_memory_alloc.at(ptr));
    munmap(ptr, reomp_mem_memory_alloc.at(ptr));
    reomp_mem_memory_alloc.erase(ptr);
  } else {
    free(ptr);
  }
#else
  // reomp_mem_real_free(ptr);  
  // if (reomp_mem_memory_alloc.find(ptr) != reomp_mem_memory_alloc.end()) {
  //   reomp_mem_memory_alloc.erase(ptr);
  // }
#endif
  return;
}

static void
reomp_mem_free_hook(void* ptr, const void *caller)
{
  void *result;
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

