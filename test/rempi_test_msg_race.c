/* ==========================ReMPI:LICENSE==========================================   
   Copyright (c) 2016, Lawrence Livermore National Security, LLC.                     
   Produced at the Lawrence Livermore National Laboratory.                            
                                                                                       
   Written by Kento Sato, kento@llnl.gov. LLNL-CODE-711357.                           
   All rights reserved.                                                               
                                                                                       
   This file is part of ReMPI. For details, see https://github.com/PRUNER/ReMPI       
   Please also see the LICENSE file for our notice and the LGPL.                      
                                                                                       
   This program is free software; you can redistribute it and/or modify it under the   
   terms of the GNU General Public License (as published by the Free Software         
   Foundation) version 2.1 dated February 1999.                                       
                                                                                       
   This program is distributed in the hope that it will be useful, but WITHOUT ANY    
   WARRANTY; without even the IMPLIED WARRANTY OF MERCHANTABILITY or                  
   FITNESS FOR A PARTICULAR PURPOSE. See the terms and conditions of the GNU          
   General Public License for more details.                                           
                                                                                       
   You should have received a copy of the GNU Lesser General Public License along     
   with this program; if not, write to the Free Software Foundation, Inc., 59 Temple   
   Place, Suite 330, Boston, MA 02111-1307 USA                                  
   ============================ReMPI:LICENSE========================================= */
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "mpi.h"
#include "rempi_test_util.h"

#define NIN_MU_TYPE_SR  (0)
#define NIN_MU_TYPE_SSR (1)


int my_rank;
int nin_mu_type;
int nin_is_safe;
int nin_num_loops;
int nin_num_patterns;
int nin_interval_usec;


static void print_usage()
{
  fprintf(stderr, "Usage: ./matching_race <type: 0=SR, 1=SSR> <matching race: 0=safe 1=unsafe(hang) 2=unsafe(MPI_Abort) 3=unsafe(segfault) > <# of loops> <# of patterns per loop> <interval(usec)>\n");
  return;
}

/* static int nin_dest_exist(int len, int* dests, int dest)  */
/* { */
/*   int i; */
/*   for (i = 0; i < len; i++) { */
/*     if (dests[i] == dest) return 1; */
/*   } */
/*   return 0; */
/* } */

static void on_message_race(int safe, int loop_id)
{
  char *a = 0;
  rempi_test_dbg_print("Wrong matching detected at loop %d", loop_id);
  switch(safe) {
  case 0:
    rempi_test_dbg_print("Wrong matching detected shoulde not occur");
    exit(1);
    break;
  case 1:
    /* Do nothing to hang this */
    break;
  case 2:
    /* Abort */
    MPI_Abort(MPI_COMM_WORLD, 0);
    break;
  case 3:
    /* Segmentation fault */
    *a = 1;    
    break;
  default:
    rempi_test_dbg_print("Unknown matching race: %d", safe);
    exit(0);
    break;
  }
}

static void nin_dest_flags_gen(int *dest_flags, int *send_count)
{
  int dest;
  int i;
  int size;
  int scount, real_scount = 0;
  
  MPI_Comm_size(MPI_COMM_WORLD, &size);
  init_rand(my_rank);
  scount = get_rand(size + 1); /* rand = 0 ... size */
  for (i = 0; i < size; i++) {
    dest_flags[i] = 0;
  }
  for (i = 0; i < scount; i++) {
    dest = get_rand(size); /* rand = 0 ... size-1 */
    dest_flags[dest] = 1;
  }
  for (i = 0; i < size; i++) {
    if (dest_flags[i]) real_scount++;
  }
  *send_count = real_scount;
  return;
}

static int nin_allreduce_recv_counts(int *dest_flags) 
{
  int *recv_counts;
  int size;
  int i;
  int recv_count;

  MPI_Comm_size(MPI_COMM_WORLD, &size);
  recv_counts  = (int*)malloc(sizeof(int) * size);
  for (i = 0; i < size; i++) {
    recv_counts[i] = 0;
  }
  MPI_Allreduce(dest_flags, recv_counts, size, MPI_INT, MPI_SUM, MPI_COMM_WORLD);  
  recv_count = recv_counts[my_rank];
  free(recv_counts);
  return recv_count;
}

static int* nin_convert_dest_flags_to_dests(int scount, int *dest_flags)
{
  int dest;
  int *dests;
  int index = 0;
  int size;

  MPI_Comm_size(MPI_COMM_WORLD, &size);
  dests        = (int*)malloc(sizeof(int) * scount);
  for (dest = 0; dest < size; dest++) {
    if (dest_flags[dest]) { 
      //      rempi_test_dbg_print("dest: %d", dest);
      dests[index++] = dest;
    }
  }
  return dests;
}

static int* nin_dests_recv_count_gen(int *send_count, int *recv_count)
{
  int *dest_flags, *dests;
  int scount;
  int size;

  MPI_Comm_size(MPI_COMM_WORLD, &size);
  dest_flags = (int*)malloc(sizeof(int) * size);
  nin_dest_flags_gen(dest_flags, &scount);

  *send_count = scount;    
  *recv_count = nin_allreduce_recv_counts(dest_flags);
  dests = nin_convert_dest_flags_to_dests(scount, dest_flags);


  free(dest_flags);
  return dests;
}



static void sr_communication(int send_count, int *dests, int recv_count, int send_val, int tag, int loop_id)
{
  int i;
  MPI_Request *send_requests, *recv_requests;
  int *recv_vals;

  send_requests = (MPI_Request*)malloc(sizeof(MPI_Request) * send_count);
  recv_requests = (MPI_Request*)malloc(sizeof(MPI_Request) * recv_count);
  recv_vals     = (int*)malloc(sizeof(int) * recv_count);
  for (i = 0; i < send_count; i++) {
    MPI_Isend(&send_val, 1, MPI_INT, dests[i], tag, MPI_COMM_WORLD, &send_requests[i]);
  }
  for (i = 0; i < recv_count; i++) {
    MPI_Irecv(&recv_vals[i], 1, MPI_INT, MPI_ANY_SOURCE, tag, MPI_COMM_WORLD, &recv_requests[i]);
  }
  MPI_Waitall(send_count, send_requests, MPI_STATUS_IGNORE);
  MPI_Waitall(recv_count, recv_requests, MPI_STATUS_IGNORE);
  for (i = 0; i < recv_count; i++) {
    if (recv_vals[i] != send_val) {
      on_message_race(nin_is_safe, loop_id);
    }
  }
  free(send_requests);
  free(recv_requests);
  free(recv_vals);
  return;
}

static void sr(int num_patterns, int interval_usec, int loop_count, int is_safe)
{
  int i, j;
  int **dests_list, *send_counts, *recv_counts;
  int increment = 0, tag = 0, send_val = 0, loop_id = 0;;
  dests_list  = (int**)malloc(sizeof(int*) * nin_num_patterns);
  send_counts = (int*)malloc(sizeof(int) * nin_num_patterns);
  recv_counts = (int*)malloc(sizeof(int) * nin_num_patterns);
  for (i = 0; i < nin_num_patterns; i++) {
    dests_list[i] = nin_dests_recv_count_gen(&send_counts[i], &recv_counts[i]);
  }


  for (i = 0; i != loop_count; i++) {
    rempi_test_dbgi_print(my_rank, "loop: %d", i + 1);
    loop_id = i;
    for (j = 0; j < num_patterns; j++){
      send_val = increment;
      tag = (is_safe == 0)? increment:0;
      sr_communication(send_counts[j], dests_list[j], recv_counts[j], send_val, tag, loop_id);
      increment++;
      //      do_work(interval_usec);
      if(interval_usec>0) usleep(interval_usec);
    }
    MPI_Barrier(MPI_COMM_WORLD);
  }

  for (i = 0; i < nin_num_patterns; i++) {
    free(dests_list[i]);
  }

  free(recv_counts);
  free(send_counts);
  free(dests_list);  
  return;
}

static void ssr(int num_patterns, int interval_usec, int loop_count, int is_safe)
{
  int i, j, k;
  int size;
  int *dest_flags, *dests, send_count, recv_count;
  int *recv_vals;
  int increment = 0, tag = 0, send_val = 0, loop_id = 0;;
  MPI_Request *send_requests, *recv_requests;

  MPI_Comm_size(MPI_COMM_WORLD, &size);
  dest_flags = (int*)malloc(sizeof(int) * size);
  send_requests = (MPI_Request*)malloc(sizeof(MPI_Request) * size);
  recv_requests = (MPI_Request*)malloc(sizeof(MPI_Request) * size);
  recv_vals     = (int*)malloc(sizeof(int) * size);
  
  for (i = 0; i != loop_count; i++) {
    rempi_test_dbgi_print(my_rank, "loop: %d", i + 1);
    loop_id = i;
    for (j = 0; j < num_patterns; j++){
      send_val = increment;
      tag = (is_safe == 0)? increment:0;
      increment++;
      {
	nin_dest_flags_gen(dest_flags, &send_count);
	dests = nin_convert_dest_flags_to_dests(send_count, dest_flags);
	for (k = 0; k < send_count; k++) {
	  MPI_Isend(&send_val, 1, MPI_INT, dests[k], tag, MPI_COMM_WORLD, &send_requests[k]);
	}
	recv_count = nin_allreduce_recv_counts(dest_flags);      
	//	rempi_test_dbg_print("count: %d", recv_count);
	for (k = 0; k < recv_count; k++) {
	  MPI_Irecv(&recv_vals[k], 1, MPI_INT, MPI_ANY_SOURCE, tag, MPI_COMM_WORLD, &recv_requests[k]);
	}
	MPI_Waitall(send_count, send_requests, MPI_STATUS_IGNORE);
	MPI_Waitall(recv_count, recv_requests, MPI_STATUS_IGNORE);
	for (k = 0; k < recv_count; k++) {
	  if (recv_vals[k] != send_val) {
	    on_message_race(nin_is_safe, loop_id);
	  }
	}
      }
      //      do_work(interval_usec);
      if(interval_usec>0) usleep(interval_usec);
    }
    MPI_Barrier(MPI_COMM_WORLD);
  }
  
  free(dest_flags);
  free(send_requests);
  free(recv_requests);
  return;
}



int main(int argc, char **argv)
{

  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
  
  
  if (argc != 6) {
    if(my_rank==0) print_usage();
    exit(0);
  }

  nin_mu_type         = atoi(argv[1]);
  nin_is_safe        = atoi(argv[2]);
  nin_num_loops      = atoi(argv[3]);
  nin_num_patterns   = atoi(argv[4]);
  nin_interval_usec  = atoi(argv[5]);
  
  if (my_rank == 0) {
    printf("***************************\n");
    printf("Matching Type     : %d\n", nin_mu_type);
    printf("Is Matching safe ?: %d\n", nin_is_safe);
    printf("# of Loops        : %d\n", nin_num_loops);
    printf("# of Patterns     : %d\n", nin_num_patterns);
    printf("Interval(usec)    : %d\n", nin_interval_usec);
    printf("***************************\n");
    fflush(stdout);
  }


  double start, end;
  MPI_Barrier(MPI_COMM_WORLD);
  start = get_time();
  switch(nin_mu_type) {
  case NIN_MU_TYPE_SR:
    sr(nin_num_patterns, nin_interval_usec, nin_num_loops, nin_is_safe);
    break;
  case NIN_MU_TYPE_SSR:
    ssr(nin_num_patterns, nin_interval_usec, nin_num_loops, nin_is_safe);
    break;
  default:
    if(my_rank==0) print_usage();
    exit(0);
  }
  MPI_Barrier(MPI_COMM_WORLD);
  end = get_time();
  if (my_rank == 0) {
    rempi_test_dbg_print("Time: %f", end - start);
  }
  MPI_Finalize();
  return 0;
}
