#ifndef REOMP_RR_H
#define REOMP_RR_H


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <omp.h>

//#define REOMP_RR_VERSION (1)
#define REOMP_RR_VERSION (2)

#ifdef __cplusplus
extern "C" {
#endif

#define xstr(s) str(s)
#define str(s) #s

void reomp_rr(void* ptr, size_t size);
void logop(int i);

#define REOMP_CONTROL reomp_control
#define REOMP_CONTROL_STR xstr(REOMP_CONTROL)
void REOMP_CONTROL(int control, void* ptr, size_t size);

/* #define REOMP_RR_INIT     (0) */
/* #define REOMP_RR_FINALIZE (1) */

#define REOMP_BEF_MAIN (0)
#define REOMP_AFT_MAIN (1)
#define REOMP_AFT_MPI_INIT (2)

#define REOMP_GATE_IN  (10)
#define REOMP_GATE_OUT (11)
#define REOMP_GATE     (12)

#define REOMP_BEF_CRITICAL_BEGIN  (13)
#define REOMP_AFT_CRITICAL_BEGIN  (14)
#define REOMP_AFT_CRITICAL_END    (15)

#define REOMP_BEF_REDUCE_BEGIN    (16)
#define REOMP_AFT_REDUCE_BEGIN    (17)
#define REOMP_BEF_REDUCE_END      (18)
#define REOMP_AFT_REDUCE_END      (19)


#define REOMP_BEF_FORK (20)
#define REOMP_AFT_FORK (21)
#define REOMP_BEG_OMP (22)
#define REOMP_END_OMP (23)
#define REOMP_BEG_FUNC_CALL (24)
#define REOMP_END_FUNC_CALL (25)

#define REOMP_RR_TYPE_NONE         (000)
#define REOMP_RR_TYPE_MAIN         (100)
#define REOMP_RR_TYPE_LOAD         (101)
#define REOMP_RR_TYPE_STORE        (102)
#define REOMP_RR_TYPE_REDUCTION    (103)
#define REOMP_RR_TYPE_CRITICAL     (104)
#define REOMP_RR_TYPE_SINGLE       (105)
#define REOMP_RR_TYPE_MASTER       (106)
#define REOMP_RR_TYPE_ATOMIC       (107)
#define REOMP_RR_TYPE_CPP_STL      (108)
  


#define REOMP_DEBUG_PRINT (90)

#ifdef __cplusplus
}
#endif


#endif
