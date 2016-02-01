#ifndef __REMPI_UTIL_H__
#define __REMPI_UTIL_H__

#include "rempi_err.h"


double rempi_get_time(void);
void rempi_dbg_sleep_sec(int sec);
void rempi_sleep_sec(int sec);
void rempi_dbg_sleep_usec(int sec);
void rempi_sleep_usec(int sec);
unsigned int rempi_hash(unsigned int original_val, unsigned int new_val);

#endif
