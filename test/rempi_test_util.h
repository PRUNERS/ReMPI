#ifndef REMPI_TEST_UTIL_H
#define REMPI_TEST_UTIL_H

double get_time(void);
int init_rand(int seed);
int init_ndrand();
int get_rand(int max);
int get_hash(int original_val, int max);

#endif
