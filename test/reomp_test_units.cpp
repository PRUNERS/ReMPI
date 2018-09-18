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

#include "../config.h"
#include "../src/llvm/mutil.h"
#include "../src/llvm/reomp.h"
//#include "caliper/cali.h"

using namespace std;


#if 0
#define NUM_THREADS 16
 
#ifdef FP_DOUBLE
typedef double fptype;
#else
typedef float fptype;
#endif

fptype mypow(fptype x) {
  return pow(x, pow(x, sin(x)));
}

float frand()
{
  float a = (float)(rand());
  float b = RAND_MAX;
  return a/b;
}




double sum_global = 0.0;
// int reomp_test_reduction_global_primitive()
// {
//   int numThreads, nth, i;
//   double a[10];
//   int size = 10;

//   numThreads = 16;

//   for (i = 0; i < size; i++) {
//     a[i] = (size - i - 1) / M_PI;
//   }

//   omp_set_num_threads(numThreads);
// #pragma omp parallel
//   {
//     nth = omp_get_num_threads();
// #pragma omp parallel for private(i) reduction(+:sum_global)
//     for (i = 0; i < size; i++) {
//       sum_global += a[i];
//     }
//   }

//   fprintf(stderr, "sum_global=%1.20f (nth: %d)\n", sum_global, nth);
//   return 0;
// }



int reomp_test_reduction_local_primitive(int numThreads)
{
  int i, id;
  const ftype begin = 3., end = 13.; //18-23 for double
  const int count = 1000000;
  const fptype width = (end - begin) / (1. * count);
 
  fptype integral0 = 0.;

  omp_set_num_threads(numThreads);
#pragma omp parallel private(id)
  {
    id = omp_get_thread_num();
    printf("Thread[%d]\n", id);
#pragma omp for reduction(+:integral0)
    for (i = 0; i < count; i++) {
      integral0 += (mypow(begin + (i * width))
		    + mypow(begin + (i + 1) * width)) * width / 2.0;
    }
  }
  fprintf(stderr, "OpenMP Reduction: %lf\n", integral0);
  return 0;
}


int reomp_test_critical()
{
  int x, tid, nth;
  x = 0;
#pragma omp parallel shared(x) private(tid)
  {
    nth = omp_get_num_threads();
    tid = omp_get_thread_num();
#pragma omp critical 
    x = (x + tid) * 10;
    //    fprintf(stderr, "tid=%d (x: %d)\n", tid, x);
  }
  fprintf(stderr, "x=%d (nth: %d)\n", x, nth);
  return 0;
}

void reomp_test_dynamic_alloc(int v)
{
  int i, nth, thn;
  int size = v;
  fprintf(stderr, "==start=1======\n");
  double *sums = (double*)malloc(sizeof(double) * size);
  double *a   = (double*)malloc(sizeof(double) * size);
  fprintf(stderr, "==end=1======\n");

  for (i = 0; i < size; i++) {
    a[i] = (size - i - 1) / M_PI;
    sums[i] = 0;
  }
  
  fprintf(stderr, "==start=1.1======\n");
#pragma omp parallel private(nth, thn)
  {
    thn = omp_get_thread_num();
    if (thn == 0) fprintf(stderr, "==start=1.2======\n");
    nth = omp_get_num_threads();
    if (thn == 0) fprintf(stderr, "==start=1.3======\n");
#pragma omp parallel for private(i, thn) 
    for (i = 0; i < size; i++) {
    //    for (i = 0; i < size; i++) {
      sums[i] += a[i];
      sums[i] += sums[(i+1) % size];
    }
    if (thn == 0) fprintf(stderr, "==start=1.4======\n");
  }
  fprintf(stderr, "sum[0]= %f, a[0]= %f\n", sums[0], a[0]);
  free(sums);
  free(a);
  fprintf(stderr, "==end=1.1======\n");
}


void reomp_test_dynamic_alloc_cpp(int v)
{
  int i, nth;
  int size = v;
  vector<double> *sums;
  vector<double>  *a;

  fprintf(stderr, "AAAAA\n");
  sums = new vector<double>(size);
  a = new vector<double>(size);
  fprintf(stderr, "BBBBB\n");


  for (i = 0; i < size; i++) {
    (*a)[i] = (size - i - 1) / M_PI;
    (*sums)[i] = 0;
  }

#pragma omp parallel private(nth)
  {
    nth = omp_get_num_threads();
#pragma omp parallel for private(i) 
    for (i = 0; i < size; i++) {
      (*sums)[i] += (*a)[i];
      (*sums)[i] += (*sums)[(i+1) % size];
    }
  }
  
  fprintf(stderr, "sum[0]= %f\n", (*sums)[0]);
}


#include <sys/time.h>
void access2(int rn, int ro, int w, int *ro_a, int *w_a, int *ro_v, int *w_v)
{
  // struct timeval tv;
  // struct timezone tz;
  // gettimeofday(&tv, &tz);
  // settimeofday(&tv, &tz);
  //  w_a[1] = ro_a[1];
  return;
}

//define void @_Z6accessiiiPiS_S_S_PSt6vectorIiSaIiEES3_(i32, i32, i32, i32* nocapture readnone, i32* nocapture readnone, i32* nocapture readonly, i32* nocapture, %"class.std::vector.0"*, %"class.std::vector.0"*) 
void access(int rn, int ro, int w, int *ro_a, int *w_a, int *ro_v, int *w_v)
{
  //  int *tmp;
  //  w = ro;
  //  w_a   = ro_a;
  w_v[1] = ro_v[1];
  //  fprintf(stderr, "%p %p", ro_a, w_a);
  //  fprintf(stderr, "%p %p", ro_v, w_v);
  return;
}

void param(int i)
{
  fprintf(stderr, "%d\n", i);
}


#define TEST_STACK_SIZE (10)
struct a_sums {
  double *a;
  double *sums;
};

void reomp_test_stack2(int nth, struct a_sums *as)
{
  int i;
#pragma omp parallel private(nth)
  {
#pragma omp parallel for private(i)
    for (i = 0; i < TEST_STACK_SIZE; i++) {
      as->sums[i] += as->a[i];
    }
  }
   fprintf(stderr, "sum[0]=%f (nth: %d)", as->sums[0], nth);
}


void reomp_test_stack1(int nth, double *a)
{
  int i;
  struct a_sums as;
  double sums[TEST_STACK_SIZE];
  as.a = a;
  as.sums = sums;

  for (i = 0; i < TEST_STACK_SIZE; i++) {
    sums[i] = 0;
  }
  reomp_test_stack2(nth, &as);
}

void reomp_test_stack(int nth) 
{
  int i;
  double a[TEST_STACK_SIZE];
  for (i = 0; i < TEST_STACK_SIZE; i++) {
    a[i] = (TEST_STACK_SIZE - i - 1) / M_PI;
  }
  reomp_test_stack1(nth, a);
}

void reomp_test_double_buff(int argc)
{
  int i, j;
  int *a = (int*)malloc(sizeof(int) * argc);
  int *b = (int*)malloc(sizeof(int) * argc);
  int* c[2] = {a, b};

  for (i = 0; i < argc; i++) {
    a[i] = 0;
    b[i] = 0;
  }
  for (i = 0; i < 1024; i++) {
    int index = i % 2;
    for (j = 0; j < argc; j++) {
      c[index][j] = c[(index + 1) % 2][j] + 1;
    }    
  }
  fprintf(stderr, "a[0]: %d, b[0]: %d\n", a[0], b[0]);
  return;
}

void switch_test() 
{
  int a;
  switch(a) {
  case 1:
  case 2:
  case 3:
    reomp_test_stack();
    reomp_test_stack();
    reomp_test_stack();
    break;
  default:
    reomp_test_stack();
    break;
  }
  return;
}


int drace_2()
{
#define N 10000
  vector<int> test_vec;
  int sum = 0;
  for (int i = 0; i < N - 1; i++) {
    test_vec.push_back(i);
  }

#pragma omp parallel for schedule(static, 1)
  for (int i = 0; i < test_vec.size(); i++) {
    test_vec[i] = test_vec[i + 1];
  }

  for (int i = 0; i < test_vec.size(); i++) {
    sum += test_vec[i];
  }

  fprintf(stderr, "sum = %d\n", sum);
  return 1;
#undef N
}


int drace_3()
{
#define N 1000
  int tid;
  char* ptr = NULL;
#pragma omp parallel private(tid) 
  {
    tid = omp_get_thread_num();
    for (int i = 0; i < N; i++) {
      if (tid == 0) {
	ptr = (char*)malloc(1);
      } else if (tid == 1) {
	free(ptr);
      }
    }
  }
  return 1;
#undef N
}
#endif



int drace_0()
{
  int sum = 1, flag = 0;
#pragma omp parallel
  {
    int tid, count = 1;
    tid = omp_get_thread_num();
    if (tid == 0) {
      sleep(1);
      flag = 1;
    } else {
      while(!flag) count++;
      sum += sum * count;      
    }
  }
  fprintf(stderr, "sum = %d\n", sum);
  return 0;
}

int drace_1()
{
#define N (100)
  int sum = 1, flag = 0;
  list<int> list;
#pragma omp parallel
  {
    int tid, count = 1;
    tid = omp_get_thread_num();
    if (tid == 0) {
      for (count = 0; count < N; count++) {
	list.push_back(count);
      }
      flag = 1;
    } else {
      while(!flag) {
	sum += sum * list.front() * tid;      
	list.pop_front();
      }
    }
  }
  fprintf(stderr, "sum = %d\n", sum);
  return 0;
#undef N
}


int drace_2()
{
#define N 2000
  vector<int> test_vec;
  int tid = 0, sum = 0, size = 0;

#pragma omp parallel private(tid, size) shared(sum)
  {
    tid = omp_get_thread_num();
    if (tid == 0) {
      for (int i = 0; i < N; i++) {
	usleep(1);
	test_vec.push_back(i);
      }
    } else {
      for (int i = 0; i < N; i++) {
	//#pragma omp critical
	{
	  size = test_vec.size();
	  sum += size * i * tid;
	}
      }
    }
  }
  fprintf(stderr, "sum = %d\n", sum);
  return 1;
#undef N
}


int drace_4()
{
#define N 2000
  vector<int> test_vec;
  int tid = 0, tid_nested = 0, sum = 0, size = 0, size_nested = 0;
  omp_set_nested(1);

#pragma omp parallel private(tid, tid_nested, size, size_nested) shared(sum)
  {
    tid  = omp_get_thread_num();
    size = omp_get_num_threads();
    omp_set_num_threads(size);
#pragma omp parallel private(tid_nested)
    {
      tid_nested  = omp_get_thread_num();
      size_nested = omp_get_num_threads();
      fprintf(stderr, "tid: %d, size: %d, tid_nested: %d, size_nested: %d\n", 
	      tid, size, tid_nested, size_nested);
      
    }
  }
  fprintf(stderr, "sum = %d\n", sum);
  return 1;
#undef N
}

int drace_5()
{
  int cell_num_particles = 100;
  int num_particles_by_thread[cell_num_particles];

  //#include "mc_omp_parallel_for_schedule_static.hh"
#pragma omp parallel for schedule (static)
  for ( int particle_index = 0; particle_index < cell_num_particles; particle_index++ )
    {
      int task_index = omp_get_thread_num();
      num_particles_by_thread[task_index]++;
    }
  return 0;
}

void atomic()
{
  int a = 1;
  int tid;
#pragma omp parallel private(tid)
  {
    tid = omp_get_thread_num();
#pragma omp parallel for schedule (static) shared(a)
    for (int i = 1; i < 1000; i++ )  {
#pragma omp atomic
      a += a * i + 1;
    }
  }
  fprintf(stderr, "a: %d\n", a);
  return;
}

// float reomp_test_atomic_reduction_float(float nth)

// {
//   int   i;
//   float a = nth, result = 0;
//   fprintf(stderr, "-- Begin -- %f %f\n", result, a);
// #pragma omp parallel for private(i, a) reduction(+:result) 
//   for (i=0; i < nth; i++) {
//     result += 1;
//   }
//   fprintf(stderr, "--  End  -- %f %f\n", result, a);
//   return result;
// }

// int reomp_test_atomic_reduction_int(int nth)

// {
//   int   i, n;
//   int a = nth, result = 0;
// #pragma omp parallel for private(i, a) reduction(+:result)
//   for (i=0; i < n; i++) {
//     result += a;
//   }
//   return result;
// }


/* ============================== */

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
static int reomp_test_omp_critical2(reomp_input_t *input);
static int reomp_test_omp_reduction(reomp_input_t *input);
static int reomp_test_omp_atomic(reomp_input_t *input);
static int reomp_test_omp_atomic_load(reomp_input_t *input);
static int reomp_test_data_race(reomp_input_t *input);
static int reomp_test_data_race_2(reomp_input_t *input);
static int reomp_test_mpmc(reomp_input_t *input);


static double num_loops_scale = 1;

//#define REOMP_NUM_LOOPS (30 * 1000)
#define REOMP_NUM_LOOPS_SCALE (1)
reomp_test_t reomp_test_cases[] =
  {
    {(char*)"omp_critical", reomp_test_omp_critical,   {0, 0,    3000000L}},
    {(char*)"omp_atomic", reomp_test_omp_atomic,       {0, 0,   30000000L}},
    {(char*)"data_race", reomp_test_data_race,         {0, 0,   30000000L}},
    {(char*)"omp_reduction", reomp_test_omp_reduction, {0, 0,  300000000L}},
    /* ------------------- */
    //    {(char*)"omp_atomic_load", reomp_test_omp_atomic_load,       {0, 0,   30000000L}},
    //    {(char*)"data_race_test", reomp_test_data_race,         {0, 0,   20L}},
    //    {(char*)"data_race_test2", reomp_test_data_race,         {0, 0,  300000L}},
    {(char*)"data_race_test3", reomp_test_data_race_2,         {0, 0,  3000000L}},
    //    {(char*)"mpmc", reomp_test_mpmc, {0, 0,  30000000L}},
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


static int reomp_test_data_race_2(reomp_input_t *input)
{
  uint64_t i;
  volatile int sum1 = 1, sum2 = 1;
  uint64_t num_loops = input->num_loops * num_loops_scale;
#pragma omp parallel for private(i)
  for (i = 0; i < num_loops; i++) {
    sum1 += 1;
    sum2 += 1;
  }
  return sum1 + sum2;
}

static int reomp_test_mpmc(reomp_input_t *input)
{
  int counter = 1;
  int tid;
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



// static int test_a(int N)
// {
//   int *lock = (int*)malloc(sizeof(int) * N);
//   for (int i = 0; i < N; i++) {
//     lock[i] = 0;
//   }
    
// #pragma omp parallel
//   {
//     int counter = 0;
//     int tid = omp_get_thread_num();
//     int nth = omp_get_num_threads();
//     for (int j = 0; j < 10000; j++) {
//       while (counter != N) {
// 	counter = 0;
// 	for (int i = 0; i < N; i++) {
// 	  if(lock[i] == tid) counter++;
// 	}
//       }
//       for (int i = 0; i < N; i++) {
// 	lock[i] = (tid + 1) % nth;
//       }
//     }
//   }
//   return 0;
// }

int next_tid = 0;
int next_tid_global = 0;
FILE *fd;
FILE *fds[24];
omp_lock_t lock;

const char* fmode;

#if 0
static void gate_in(int mode)
{
  omp_set_lock(&lock);
}

static void gate_out(int mode)
{
  omp_unset_lock(&lock);
}
#elif 0
static void gate_in(int mode)
{
  if (next_tid < 0) next_tid = 0;
  int tid = omp_get_thread_num();
  while(tid != next_tid);
}

static void gate_out(int mode)
{
  int nth = omp_get_num_threads();
  next_tid = (next_tid + 1) % nth;
}

#elif 1

#include <setjmp.h>
static jmp_buf env[6400];
static int tmp_clock[6400];
static int num_of_revoked_clock = 0;
static int num_of_committed_clock = 0;
static int tmp_num_of_revoked_clock[6400];

static void gate_in(int mode)
{
  int tid = omp_get_thread_num();
  int val;
  fprintf(stderr, "tid: %d: start in\n", tid);
  if (mode == 0) {
    val = setjmp(env[tid * 64]);
    tmp_clock[tid * 64] = __sync_fetch_and_add(&next_tid_global, 1);
    fprintf(stderr, "tid: %d: val=%d clock=%d\n", tid, val, tmp_clock[tid * 64]);
    if (val == 1) {
      tmp_num_of_revoked_clock[tid * 64] = __sync_fetch_and_add(&num_of_revoked_clock, 1);
    } else {
      tmp_num_of_revoked_clock[tid * 64] = num_of_revoked_clock - 1;
    }
  } else {
    int clock;
    fread(&clock, sizeof(int), 1, fds[tid]);
    while(clock != next_tid);
  }
  fprintf(stderr, "tid: %d: end in\n", tid);
}

static void gate_out(int mode)
{
  int tid = omp_get_thread_num();
  fprintf(stderr, "tid: %d: start out\n", tid);
  if (mode == 0) {
  
    int index = tid * 64;
    //    fprintf(stderr, "tid: %d: %d %d", tmp_clock, );
    //    if (tmp_clock[index] == next_tid_global -1) {
    if (tmp_clock[index] > 3){
      int c = tmp_clock[index] - tmp_num_of_revoked_clock[index] ;
      fwrite(&c, sizeof(int), 1, fds[tid]);
      fprintf(stderr, "tid: %d: fwrite %d\n", tid, c);
    } else {
      fprintf(stderr, "tid: %d: jmp %d\n", tid, tmp_clock[index]);
      longjmp(env[tid * 64], 1);
    }
    // int clock = next_tid_global++;
    // omp_unset_lock(&lock);
    // fwrite(&clock, sizeof(int), 1, fds[tid]);
  } else {
    next_tid++;
  }
  fprintf(stderr, "tid: %d: end out\n", tid);
}

#elif 0

static void gate_in(int mode)
{
  if (mode == 0) {
    omp_set_lock(&lock);
  } else if (mode == 1) {
    int clock;
    int tid = omp_get_thread_num();
    fread(&clock, sizeof(int), 1, fds[tid]);
    while(clock != next_tid);
  } 
}

static void gate_out(int mode)
{
  if (mode == 0) {
    int clock = next_tid_global++;
    int tid = omp_get_thread_num();
    omp_unset_lock(&lock);
    fwrite(&clock, sizeof(int), 1, fds[tid]);
  } else if (mode == 1) {
    next_tid++;
  }
}

#elif 0
static void gate_in(int mode)
{
  int tid = omp_get_thread_num();
  int nth = omp_get_num_threads();
  while (tid != next_tid) {
    if (omp_test_lock(&lock)) {
	next_tid_global = (next_tid_global + 1) % nth;
	next_tid = next_tid_global;
    }
  }
  return;
}

static void gate_out(int mode)
{
  next_tid = -1;
  omp_unset_lock(&lock);    
}

#elif 0

static void gate_in(int mode)
{
  int tid = omp_get_thread_num();
  int nth = omp_get_num_threads();
  while (tid != next_tid) {
    if (omp_test_lock(&lock)) {
      if (mode == 0) {
	next_tid_global = (next_tid_global + 1) % nth;
	next_tid = next_tid_global;
      } else {
	fread(&next_tid, sizeof(int), 1, fd);
	__sync_synchronize();
	if (feof(fd)) fprintf(stderr,"fread reached the end of record file");
	if (ferror(fd)) fprintf(stderr, "fread failed");
      }
    }
  }
}

static void gate_out(int mode)
{
  int tid = omp_get_thread_num();
  int nth = omp_get_num_threads();
  if (mode == 0) {
    fwrite(&tid, sizeof(int), 1, fd);
  }
  next_tid = -1;
  omp_unset_lock(&lock);
}
#else
static void gate_in(int mode)
{
  int tid = omp_get_thread_num();
  if (mode == 0) {
    omp_set_lock(&lock);
  } else {
    while (tid != next_tid) {
      if (omp_test_lock(&lock)) {
	fread(&next_tid, sizeof(int), 1, fd);
	__sync_synchronize();
	if (feof(fd)) fprintf(stderr,"fread reached the end of record file");
	if (ferror(fd)) fprintf(stderr, "fread failed");
      }
    }
  }
}

static void gate_out(int mode)
{
  int tid = omp_get_thread_num();
  int nth = omp_get_num_threads();
  if (mode == 0) {
#if 0
    next_tid_global = (next_tid_global + 1) % nth;
    tid = next_tid_global;
#endif
    fwrite(&tid, sizeof(int), 1, fd);
	
    omp_unset_lock(&lock);
  } else {
    next_tid = -1;
    omp_unset_lock(&lock);    
  }
}
#endif


static int test_b()
{
  uint64_t i;
  int sum;
  reomp_control(0, NULL, 0);
#pragma omp parallel for private(i)
  for (i = 0; i <  3000000L; i++) {
    reomp_control(13, NULL, 0);
    //    #pragma omp critical
    {
      sum = 1;
    }
    reomp_control(15, NULL, 0);
  }
  reomp_control(1, NULL, 0);
  return sum;
}


int jmp = 0;
static void test_a(int mode)
{
  uint64_t i;
  int sum = 0;
  double whole_s, whole_e, s, e;
  char path[256];
  int tid;
  int index;
  int val;
  int sleep = 0;

  int clock;
  jmp_buf en;

  fmode = (mode == 0)? "w+":"r";
  fd = fopen("/tmp/test.reomp", fmode);
  for (int i = 0; i < 24; i++) {
    sprintf(path, "/tmp/test-%d.reomp", i);
    fds[i] = fopen(path, fmode);    
  }
  whole_s = reomp_util_get_time();

  omp_init_lock(&lock);

#pragma omp parallel for private(i, tid, index,val, clock, en, sleep) shared(sum)
  //  for (i = 0; i < 3000000L; i++) {
  for (i = 0; i < 300000L; i++) {
  //  for (i = 0; i < 30000L; i++) {
    //  for (i = 0; i < 24L; i++) {
    //    gate_in(mode);
    //#pragma omp critical
    if (mode == 0) {
      do {
	clock = next_tid_global; // tmp_clock[index] = next_tid_global;
	sum = 1;
      } while(!__sync_bool_compare_and_swap(&next_tid_global, clock, clock + 1));
      tid = omp_get_thread_num();
      fwrite(&clock, sizeof(int), 1, fds[tid]);
      //      fprintf(stderr, "tid: %d clock: %d\n", tid, clock);
    } else if (mode == 1) {
      tid = omp_get_thread_num();
      fread(&clock, sizeof(int), 1, fds[tid]);
      while(clock != next_tid_global);
      sum = 1;
      next_tid_global++;
    } else {
      sum = 1;
    }
    sleep = 0;
    while(sleep++ < 100000);
    //    gate_out(mode);

  }
    
  // #pragma omp parallel shared(next_tid, next_tid_global)
//   {
//     int nth = omp_get_num_threads();
//     // fprintf(stderr, "next_tid: %d global: %d\n",
//     // 	    next_tid, next_tid_global);
//     for (int j = 0; j < 3000000 / nth; j++) {
//       gate_in(mode);
// #pragma omp critical
//       {
// 	sum += 1;
//       }
//       gate_out(mode);
//     }
//   }

  
  omp_destroy_lock(&lock);
  s = reomp_util_get_time();
  fflush(fd);
  fsync(fileno(fd));
  fclose(fd);
  e = reomp_util_get_time();
  whole_e = reomp_util_get_time();
  fprintf(stderr, "time: %f (%f) sum= %d\n", whole_e - whole_s,e -s, sum);
  //  fprintf(stderr, "time: %f (%f) sum= %d (jmp: %d)\n", whole_e - whole_s,e -s, sum, jmp/3000000L);
  exit(0);
  return;
}




int main(int argc, char **argv)
{
  int i;
  int nth;
  char *test_name;
  double whole_s, whole_e, s, e;

#if 0
  test_jmp();
  exit(0);
#endif


#if 0
  MPI_Init(&argc, &argv);
  s = reomp_util_get_time();
  test_a(atoi(argv[1]));
  //  test_b();
  // //  test_b(&reomp_test_cases[0].input);
  e = reomp_util_get_time();
  fprintf(stderr, "time: %f\n", e -s);
  exit(0);
#endif
  // return 0;

  
  //CALI_CXX_MARK_FUNCTION;

  whole_s = reomp_util_get_time();
  MPI_Init(&argc, &argv);

  // for (int i = 1; i < 24; i++) {
  //   double s, e;
  //   s = MPI_Wtime();
  //   test_a(i);
  //   e = MPI_Wtime();
  //   fprintf(stderr, "N=%d: time = %f\n", i, e - s);
  // }

  // return 0;


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
