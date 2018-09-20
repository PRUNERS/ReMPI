#include <stdlib.h>
#include <stdio.h>
#include <stdio.h>
#include <math.h>
#include <omp.h>
#include <pthread.h>
#include <unistd.h>
#include <mpi.h>
#include <string.h>

#include <iostream>
#include <thread>
#include <vector>
#include <list>

#include "../../src/reomp/mutil.h"
#include "../../src/reomp/reomp.h"

#define REOMP_NUM_LOOPS_SCALE (1)

using namespace std;

typedef struct {
  int int_val_1;
  int int_val_2;
  uint64_t num_loops;
} reomp_input_t;


typedef struct {
  char* name;
  int(*func)(reomp_input_t*);
  reomp_input_t input;
} reomp_test_t;

static int reomp_test_omp_critical(reomp_input_t *input);
static int reomp_test_omp_reduction(reomp_input_t *input);
static int reomp_test_omp_atomic(reomp_input_t *input);
static int reomp_test_omp_atomic_load(reomp_input_t *input);
static int reomp_test_data_race(reomp_input_t *input);
static int reomp_test_mpmc(reomp_input_t *input);

static double num_loops_scale = 1;


reomp_test_t reomp_test_cases[] =
  {
    {(char*)"omp_critical", reomp_test_omp_critical,   {0, 0,    3000000L}},
    {(char*)"omp_atomic", reomp_test_omp_atomic,       {0, 0,   30000000L}},
    {(char*)"data_race", reomp_test_data_race,         {0, 0,   30000000L}},
    {(char*)"omp_reduction", reomp_test_omp_reduction, {0, 0,  300000000L}},
  };



static int reomp_test_omp_critical(reomp_input_t *input)
{
  uint64_t i;
  volatile int sum;
  uint64_t num_loops = input->num_loops * num_loops_scale;
#pragma omp parallel for private(i)
  for (i = 0; i < num_loops; i++) {
#pragma omp critical
    {
      sum += 1;
    }
  }
  return sum;
}

static int reomp_test_omp_reduction(reomp_input_t *input)
{
  uint64_t i;
  volatile int sum;
  uint64_t num_loops = input->num_loops * num_loops_scale;
  //  uint64_t num_loops = omp_get_num_threads()  * num_loops_scale;
#pragma omp parallel for private(i) reduction(+: sum)
  for (i = 0; i < num_loops; i++) {
    sum += 1;
  }
  return sum;
}

static int reomp_test_omp_atomic(reomp_input_t *input)
{
  uint64_t i;
  volatile int sum = 1;
  uint64_t num_loops = input->num_loops * num_loops_scale;
#pragma omp parallel for private(i)
  for (i = 0; i < num_loops; i++) {
#pragma omp atomic
    sum++;
  }
  return sum;
}


static int reomp_test_omp_atomic_load(reomp_input_t *input)
{
  uint64_t i;
  static volatile size_t sum = 1;
  static volatile size_t zero = 0;
  uint64_t num_loops = input->num_loops * num_loops_scale;
#pragma omp parallel for private(i)
  for (i = 0; i < num_loops; i++) {
#pragma omp atomic
    sum = sum + zero;
  }
  return sum;
}


static int reomp_test_data_race(reomp_input_t *input)
{
  uint64_t i;
  volatile int sum = 1;
  uint64_t num_loops = input->num_loops * num_loops_scale;
#pragma omp parallel for private(i)
  for (i = 0; i < num_loops; i++) {
    sum += 1;
  }
  return sum;
}


static int reomp_test_mpmc(reomp_input_t *input)
{
  int counter = 1;
  int tid = 0;
  uint64_t num_loops = input->num_loops * num_loops_scale;
#pragma omp parallel private(tid)
  tid = omp_get_thread_num();
  for (uint64_t i = 0; i < num_loops; i++) {
    if (tid) {
#pragma omp critical(sec)
      {
	counter++;
      }
    } else {
      if (counter > 0) {
#pragma omp critical(sec)
	{
	  if (counter > 0) counter--;
	}
      }
    }
  }
  return counter;
}

int main(int argc, char **argv)
{
  int i;
  int nth;
  char *test_name;
  double whole_s, whole_e, s, e;

  whole_s = reomp_util_get_time();
  MPI_Init(&argc, &argv);

  if (argc < 2 || 4 < argc) {
    fprintf(stderr, "%s <# of threads> <num_loops scale> [<test name>]\n", argv[0]);
    exit(0);
  }
  
  nth = atoi(argv[1]);
  num_loops_scale = (argc >= 3)? atof(argv[2]):REOMP_NUM_LOOPS_SCALE;
  test_name = (argc >= 4)? argv[3]:NULL;
  fprintf(stderr, "=============================================\n");  
  fprintf(stderr, "# of Thread   : %d\n", nth);
  fprintf(stderr, "Scale for # of Loops    : %f\n", num_loops_scale);
  fprintf(stderr, "Test case name: %s\n",
	  (test_name == NULL)?(char*)"Test all":test_name);
  fprintf(stderr, "=============================================\n");  
  
  omp_set_num_threads(nth);


  int did_test = 0;
  for (i = 0; i < sizeof(reomp_test_cases)/sizeof(reomp_test_t); i++) {
    int do_this_test = 0;
    reomp_test_t *test;
    test = &reomp_test_cases[i];
    if (test_name == NULL) {
      do_this_test = 1;
    } else if (!strcmp(test_name, test->name)) {
      do_this_test = 1;
    } else {
      do_this_test = 0;
    }    

    if (do_this_test) {
      int ret;
      s = MPI_Wtime();
      ret = test->func(&(test->input));
      e = MPI_Wtime();
      fprintf(stderr, "Test: %s: time = %f time per iter = %f usec (ret: %d)\n",
	      test->name, e - s, (e - s) / (test->input.num_loops / 1000000.0), ret);
      did_test = 1;
    }
  }
  
  if (!did_test) {
    fprintf(stderr, "No such test case: %s\n", test_name);
    exit(0);
  }

  whole_e = reomp_util_get_time();
  fprintf(stderr, "Test: main_time = %f\n", whole_e - whole_s);

  return 0;
}
