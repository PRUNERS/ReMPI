#ifndef __REMPI_MEM_H__
#define __REMPI_MEM_H__

#include <stdlib.h>


void* rempi_malloc(size_t size) ;
void rempi_free(void* addr);

#endif
