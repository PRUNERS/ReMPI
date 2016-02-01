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

void rempi_dbg_sleep_sec(int sec)
{
  rempi_dbg("Sleep: %d sec (%s:%s:%d)", sec, __FILE__, __func__, __LINE__);
  sleep(sec);
  return;
}
void rempi_sleep_sec(int sec)
{
  sleep(sec);
  return;
}

void rempi_dbg_sleep_usec(int usec)
{
  rempi_dbg("Sleep: %d usec (%s:%s:%d)\n", usec, __FILE__, __func__, __LINE__);
  usleep(usec);
  return;
}

void rempi_sleep_usec(int usec)
{
  usleep(usec);
  return;
}


unsigned int rempi_hash(unsigned int original_val, unsigned int new_val) {
  return ((original_val << 5) + original_val) + new_val;
}

