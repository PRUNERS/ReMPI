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

#include "reomp_omp_wrapper.h"
#include "mutil.h"

static kmp_int32 (*__real_reduce_nowait)(
 ident_t *loc, kmp_int32 global_tid,
 kmp_int32 num_vars, size_t reduce_size, void *reduce_data, void (*reduce_func)(void *lhs_data, void *rhs_data),
 kmp_critical_name *lck ) = NULL;
static void (*__real_end_reduce_nowait)(ident_t *loc, kmp_int32 global_tid,
					kmp_critical_name *lck);

static kmp_int32 omp_num_vars = 0;
static size_t    omp_reduce_size = 0;
static void*     omp_reduce_data = NULL;

#if 0
kmp_int32 __kmpc_reduce_nowait(
			       ident_t *loc, kmp_int32 global_tid,
			       kmp_int32 num_vars, size_t reduce_size, void *reduce_data, void (*reduce_func)(void *lhs_data, void *rhs_data),
			       kmp_critical_name *lck ) {
  int ret;
  if (!__real_reduce_nowait) {
    __real_reduce_nowait   = (kmp_int32(*)(
  					   ident_t *, 
  					   kmp_int32, 
  					   kmp_int32, 
  					   size_t, 
  					   void *, 
  					   void (*reduce_func)(void *lhs_data, void *rhs_data),
  					   kmp_critical_name *)
  			      )dlsym(RTLD_NEXT, "__kmpc_reduce_nowait");
  }
  
  ret =  __real_reduce_nowait(loc, global_tid, num_vars, reduce_size, reduce_data, reduce_func, lck);

  omp_num_vars = num_vars;
  omp_reduce_size = reduce_size;
  omp_reduce_data = reduce_data;
  
  MUTIL_DBG("in  --> %p %d %f",  pthread_self(), omp_get_thread_num(), ((double*)reduce_data)[0]);
  
  return ret;
}

void __kmpc_end_reduce(ident_t * loc,
		       kmp_int32 global_tid,
		       kmp_critical_name * lck 
		       )
{
  MUTIL_DBG("kmpc_end_reduce");
  return;
}

void __kmpc_end_reduce_nowait(ident_t *loc, kmp_int32 global_tid,
			      kmp_critical_name *lck)
{
  if (!__real_end_reduce_nowait) {
    __real_end_reduce_nowait = (void (*)(ident_t *loc, kmp_int32 global_tid, kmp_critical_name *lck))dlsym(RTLD_NEXT, "__kmpc_end_reduce_nowait");
  }
  MUTIL_DBG("out --> %p %d %f",  pthread_self(), omp_get_thread_num(), ((double*)omp_reduce_data)[0]);
  __real_end_reduce_nowait(loc, global_tid, lck);

  // for (int i = 0; i < omp_num_vars; i++) {
  //   MUTIL_DBG("-result: %lu %f (%d)", omp_reduce_size, ((float*)omp_reduce_data)[i], i);    
  // }
  return;
}


#endif
