#ifndef REOMP_RR_H
#define REOMP_RR_H


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <omp.h>



#ifdef __cplusplus
extern "C" {
#endif

#define xstr(s) str(s)
#define str(s) #s
#define REOMP_RR_INIT reomp_rr_init
#define REOMP_RR_INIT_STR xstr(REOMP_RR_INIT)
void REOMP_RR_INIT();
#define REOMP_RR_FINALIZE reomp_rr_finalize
#define REOMP_RR_FINALIZE_STR xstr(REOMP_RR_FINALIZE)
void REOMP_RR_FINALIZE();
void reomp_rr(void* ptr, size_t size);
void logop(int i);

#ifdef __cplusplus
}
#endif


#endif
