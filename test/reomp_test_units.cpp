#include <stdlib.h>
#include <stdio.h>
#include <stdio.h>
#include <math.h>
#include <omp.h>
#include <pthread.h>
#include <unistd.h>

#include <iostream>
#include <thread>
#include <vector>
#include <list>

#include "../config.h"

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
}



int main(int argc, char **argv)
{
  int nth;
  if (argc != 2) {
    fprintf(stderr, "%s <# of threads>\n", argv[0]);
    exit(0);
  }
  nth = atoi(argv[1]);
  omp_set_num_threads(nth);

  //  config_test(argc);
  // int *data = (int*)malloc(4 * sizeof(int));
  // data = (int*)realloc(data, 5 * sizeof(int));
  // free(data);

  //  fprintf(stderr, "==start=======\n");
  for (int i = 0; i < 10; i++) {
    //        drace_6();
    //    drace_5();
    //    drace_4();
    //    drace_3();
    //    drace_2();
    //    drace_1();
    //    reomp_test_stack(nth);    
    //reomp_test_dynamic_alloc(nth);
    //    reomp_test_reduction_local_primitive(nth);
    //reomp_test_reduction_global_primitive();
    //reomp_test_critical();
    //  reomp_test_dynamic_alloc_cpp(nth);
    //  reomp_test_double_buff(argc);
  }
  return 0;
}
