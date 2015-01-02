#include <stdio.h>

#include "rempi_test_util.h"


int init_rand(int seed) 
{
  srand(seed);
}

int get_rand(int max)
{
  return rand() % max;
}

int get_hash(int original_val, int max) {
  return (original_val + get_rand(max)) % max;
}
