#include <stdio.h>
#include <mpi.h>
#include <sys/time.h>

#include "rempi_test_util.h"
#include "clmpi.h"
#include "pnmpimod.h"

#define PNMPI_MODULE_TEST "test-clmpi"
#define MAX_NEXT_CLOCK (10000)


PNMPIMOD_register_recv_clocks_t clmpi_register_recv_clocks;
PNMPIMOD_clock_control_t        clmpi_clock_control;
PNMPIMOD_get_local_clock_t      clmpi_get_local_clock;
PNMPIMOD_sync_clock_t           clmpi_sync_clock;
/* PNMPIMOD_fetch_next_clocks_t clmpi_fetch_next_clocks; */
/* PNMPIMOD_get_next_clock_t       clmpi_get_next_clock; */
/* PNMPIMOD_set_next_clock_t       clmpi_set_next_clock; */

int init_clmpi()
{
  int err;
  PNMPI_modHandle_t handle_test, handle_clmpi;
  PNMPI_Service_descriptor_t serv;
  /*Load clock-mpi*/
  err=PNMPI_Service_GetModuleByName(PNMPI_MODULE_CLMPI, &handle_clmpi);
  if (err!=PNMPI_SUCCESS) {
    return err;
  }
  /*Get clock-mpi service*/
  err=PNMPI_Service_GetServiceByName(handle_clmpi,"clmpi_register_recv_clocks","pi",&serv);
  if (err!=PNMPI_SUCCESS)
    return err;
  clmpi_register_recv_clocks=(PNMPIMOD_register_recv_clocks_t) ((void*)serv.fct);

  /*Get clock-mpi service*/
  err=PNMPI_Service_GetServiceByName(handle_clmpi,"clmpi_clock_control","i",&serv);
  if (err!=PNMPI_SUCCESS)
    return err;
  clmpi_clock_control=(PNMPIMOD_clock_control_t) ((void*)serv.fct);

  /*Get clock-mpi service*/
  err=PNMPI_Service_GetServiceByName(handle_clmpi,"clmpi_get_local_clock","p",&serv);
  if (err!=PNMPI_SUCCESS)
    return err;
  clmpi_get_local_clock=(PNMPIMOD_get_local_clock_t) ((void*)serv.fct);

  /*Get clock-mpi service*/
  err=PNMPI_Service_GetServiceByName(handle_clmpi,"clmpi_sync_clock","i",&serv);
  if (err!=PNMPI_SUCCESS)
    return err;
  clmpi_sync_clock=(PNMPIMOD_sync_clock_t) ((void*)serv.fct);

  /* /\*Get clock-mpi service*\/ */
  /* err=PNMPI_Service_GetServiceByName(handle_clmpi,"clmpi_fetch_next_clocks","ipp",&serv); */
  /* if (err!=PNMPI_SUCCESS) */
  /*   return err; */
  /* clmpi_fetch_next_clocks=(PNMPIMOD_fetch_next_clocks_t) ((void*)serv.fct); */


  /* /\*Get clock-mpi service*\/ */
  /* err=PNMPI_Service_GetServiceByName(handle_clmpi,"clmpi_get_next_clock","p",&serv); */
  /* if (err!=PNMPI_SUCCESS) */
  /*   return err; */
  /* clmpi_get_next_clock=(PNMPIMOD_get_next_clock_t) ((void*)serv.fct); */

  /* /\*Get clock-mpi service*\/ */
  /* err=PNMPI_Service_GetServiceByName(handle_clmpi,"clmpi_set_next_clock","p",&serv); */
  /* if (err!=PNMPI_SUCCESS) */
  /*   return err; */
  /* clmpi_set_next_clock=(PNMPIMOD_set_next_clock_t) ((void*)serv.fct); */

  /*Load own moduel*/
  err=PNMPI_Service_GetModuleByName(PNMPI_MODULE_TEST, &handle_test);
  if (err!=PNMPI_SUCCESS)
    return err;
  return err;
}

int main(int argc, char *argv[])
{
  int i, my_rank, size;
  size_t next_clock = 0;
  int ranks[1];
  size_t remote_next_clocks[1];
  double start, end;

  /* Init */
  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);

  init_clmpi();
  
  start = MPI_Wtime();
  while(next_clock < MAX_NEXT_CLOCK + 10) {
    /* ranks[0] = (my_rank + 1) % size; */
    /* clmpi_fetch_next_clocks(1, ranks, remote_next_clocks); */
    /* next_clock = remote_next_clocks[0] + 1; */
    /* clmpi_set_next_clock(next_clock); */
    /* clmpi_get_next_clock(&next_clock); */
  }
  end = MPI_Wtime();
  
  MPI_Finalize();
  
  fprintf(stderr, "rank %d: 1 hop: %f, val=%d\n", my_rank, (end - start) * 1000000 /MAX_NEXT_CLOCK, next_clock);

  return 0;

}
