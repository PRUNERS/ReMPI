#ifndef REMPI_TEST_UTIL_H
#define REMPI_TEST_UTIL_H


#define rempi_test_dbg_print(format, ...) \
  do { \
  fprintf(stderr, "REMPI(test):%3d: " format " (%s:%d)\n", my_rank, ## __VA_ARGS__, __FILE__, __LINE__); \
  } while (0)

#define rempi_test_dbgi_print(rank, format, ...) \
  do { \
  if (rank == 0) { \
  fprintf(stderr, "REMPI(test):%3d: " format " (%s:%d)\n", my_rank, ## __VA_ARGS__, __FILE__, __LINE__); \
  } \
  } while (0)



double get_time(void);
int init_rand(int seed);
int init_ndrand();
int get_rand(int max);
int get_hash(int original_val, int max);

#endif
