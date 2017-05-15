#ifndef REOMP_RR_H
#define REOMP_RR_H


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <omp.h>



#ifdef __cplusplus
extern "C" {
#endif

void reomp_rr(void* ptr, size_t size);
void logop(int i);

#ifdef __cplusplus
}
#endif


#endif
