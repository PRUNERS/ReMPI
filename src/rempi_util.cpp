#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>

#include "rempi_util.h"
#include "rempi_err.h"

double rempi_get_time(void)
{
  double t;
  struct timeval tv;
  gettimeofday(&tv, NULL);
  t = ((double)(tv.tv_sec) + (double)(tv.tv_usec) * 0.001 * 0.001);
  //  rempi_dbg(" -== > %f", t);
  return t;
}

void rempi_sleep(int sec)
{
  sleep(sec);
  return;
}

