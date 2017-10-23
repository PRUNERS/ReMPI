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



typedef int kmp_int32;
typedef kmp_int32 kmp_critical_name[8];
typedef struct ident {
     kmp_int32 reserved_1;   
    kmp_int32 flags;        
    kmp_int32 reserved_2;   
    kmp_int32 reserved_3;   
    char     *psource;      
} ident_t;

#ifdef __cplusplus
extern "C" {
#endif

kmp_int32 __kmpc_reduce_nowait(
 ident_t *loc, kmp_int32 global_tid,
 kmp_int32 num_vars, size_t reduce_size, void *reduce_data, void (*reduce_func)(void *lhs_data, void *rhs_data),
 kmp_critical_name *lck );

void __kmpc_end_reduce(ident_t * loc,
		       kmp_int32 global_tid,
		       kmp_critical_name * lck 
		       );

void __kmpc_end_reduce_nowait(ident_t *loc, kmp_int32 global_tid,
			      kmp_critical_name *lck);

#ifdef __cplusplus
}
#endif



