#ifndef REOMP_PROFILE_H_
#define REOMP_PROFILE_H_

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

#include "reomp_config.h"

//#define REOMP_PROFILE_ENABLE
#ifdef REOMP_PROFILE_ENABLE
#define REOMP_PROFILE(profile_func)			\
  do {							\
    if (reomp_config.profile_level > 0) profile_func;	\
  } while(0)
#else
#define REOMP_PROFILE(profile_func)
#endif


void reomp_profile_init();
void reomp_profile_io(size_t s);
void reomp_profile_epoch(size_t epoch);
void reomp_profile(size_t rr_type, size_t lock_id);
void reomp_profile_print();
void reomp_profile_finalize();

#endif

