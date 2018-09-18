#ifndef REOMP_MON_H
#define REOMP_MON_H


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <omp.h>

//#define REOMP_ENABLE_MONITORING

#ifdef REOMP_ENABLE_MONITORING
#define REOMP_MON_SAMPLE_CALLSTACK() \
do { \
  reomp_mon_sample_callstack(); \
} while(0)

#define REOMP_MON_SAMPLE_CALLSTACK_PRINT() \
do { \
  reomp_mon_sample_callstack_print(); \
} while(0)


#else
#define REOMP_MON_SAMPLE_CALLSTACK()
#define REOMP_MON_SAMPLE_CALLSTACK_PRINT()
#endif

#ifdef __cplusplus
extern "C" {
#endif

void  reomp_mon_sample_callstack();
void  reomp_mon_sample_callstack_print();

#ifdef __cplusplus
}
#endif


#endif
