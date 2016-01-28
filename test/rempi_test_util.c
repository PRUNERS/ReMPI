#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>

#include "rempi_test_util.h"

double get_time(void)
{
  double t;
  struct timeval tv;
  gettimeofday(&tv, NULL);
  t = ((double)(tv.tv_sec) + (double)(tv.tv_usec) * 0.001 * 0.001);
  return t;
}

int init_rand(int seed) 
{
  srand(seed);
  return 0;
}

int init_ndrand() 
{
  srand((int)get_time());
  return 0;
}

int get_rand(int max)
{
  return rand() % max;
}

int get_hash(int original_val, int max) {
  return (original_val * original_val + original_val % 23) % max;
}
