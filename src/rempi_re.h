#ifndef __REMPI_RE_H__
#define __REMPI_RE_H__

#define REVERT1

#include <stdlib.h>
#include <string.h>

#include <mpi.h>

#include "rempi_type.h"
#include "rempi_err.h"
#include "rempi_mem.h"
#include "rempi_recorder.h"

#if !defined(REMPI_LITE)
#include "clmpi.h"
#endif


using namespace std;


#define REMPI_FUNCTIONS \
  virtual int re_init(int *argc, char ***argv, int fortran_init);			\
  virtual int re_init_thread(int *argc, char ***argv, int required, int *provided, int fortran_init_thraed); \
  virtual int re_isend(mpi_const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request); \
  virtual int re_irecv(void *buf, int count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm, MPI_Request *request); \
  virtual int re_cancel(MPI_Request *request); \
  virtual int re_test(MPI_Request *request, int *flag, MPI_Status *status); \
  virtual int re_testany(int count, MPI_Request array_of_requests[], int *index, int *flag, MPI_Status *status); \
  virtual int re_testsome(int incount, MPI_Request array_of_requests[], int *outcount, int array_of_indices[], MPI_Status array_of_statuses[]); \
  virtual int re_testall(int count, MPI_Request array_of_requests[], int *flag, MPI_Status array_of_statuses[]); \
  virtual int re_wait(MPI_Request *request, MPI_Status *status); \
  virtual int re_waitany(int count, MPI_Request array_of_requests[], int *index, MPI_Status *status); \
  virtual int re_waitsome(int incount, MPI_Request array_of_requests[], int *outcount, int array_of_indices[], MPI_Status array_of_statuses[]); \
  virtual int re_waitall(int incount, MPI_Request array_of_requests[], MPI_Status array_of_statuses[]); \
  virtual int re_probe(int source, int tag, MPI_Comm comm, MPI_Status *status); \
  virtual int re_iprobe(int source, int tag, MPI_Comm comm, int *flag, MPI_Status *status); \
  virtual int re_comm_split(MPI_Comm arg_0, int arg_1, int arg_2, MPI_Comm *arg_3); \
  virtual int re_comm_create(MPI_Comm arg_0, MPI_Group arg_1, MPI_Comm *arg_2); \
  virtual int re_comm_dup(MPI_Comm arg_0, MPI_Comm *arg_2); \
  virtual int re_allreduce(mpi_const void *arg_0, void *arg_1, int arg_2, MPI_Datatype arg_3, MPI_Op arg_4, MPI_Comm arg_5); \
  virtual int re_reduce(mpi_const void *arg_0, void *arg_1, int arg_2, MPI_Datatype arg_3, MPI_Op arg_4, int arg_5, MPI_Comm arg_6); \
  virtual int re_scan(mpi_const void *arg_0, void *arg_1, int arg_2, MPI_Datatype arg_3, MPI_Op arg_4, MPI_Comm arg_5); \
  virtual int re_allgather(mpi_const void *arg_0, int arg_1, MPI_Datatype arg_2, void *arg_3, int arg_4, MPI_Datatype arg_5, MPI_Comm arg_6); \
  virtual int re_gatherv(mpi_const void *arg_0, int arg_1, MPI_Datatype arg_2, void *arg_3, mpi_const int *arg_4, mpi_const int *arg_5, MPI_Datatype arg_6, int arg_7, MPI_Comm arg_8); \
  virtual int re_reduce_scatter(mpi_const void *arg_0, void *arg_1, mpi_const int *arg_2, MPI_Datatype arg_3, MPI_Op arg_4, MPI_Comm arg_5); \
  virtual int re_scatterv(mpi_const void *arg_0, mpi_const int *arg_1, mpi_const int *arg_2, MPI_Datatype arg_3, void *arg_4, int arg_5, MPI_Datatype arg_6, int arg_7, MPI_Comm arg_8); \
  virtual int re_allgatherv(mpi_const void *arg_0, int arg_1, MPI_Datatype arg_2, void *arg_3, mpi_const int *arg_4, mpi_const int *arg_5, MPI_Datatype arg_6, MPI_Comm arg_7); \
  virtual int re_scatter(mpi_const void *arg_0, int arg_1, MPI_Datatype arg_2, void *arg_3, int arg_4, MPI_Datatype arg_5, int arg_6, MPI_Comm arg_7); \
  virtual int re_bcast(void *arg_0, int arg_1, MPI_Datatype arg_2, int arg_3, MPI_Comm arg_4); \
  virtual int re_alltoall(mpi_const void *arg_0, int arg_1, MPI_Datatype arg_2, void *arg_3, int arg_4, MPI_Datatype arg_5, MPI_Comm arg_6); \
  virtual int re_gather(mpi_const void *arg_0, int arg_1, MPI_Datatype arg_2, void *arg_3, int arg_4, MPI_Datatype arg_5, int arg_6, MPI_Comm arg_7); \
  virtual int re_barrier(MPI_Comm arg_0); \
  virtual MPI_Fint re_request_c2f(MPI_Request request); \
  virtual int re_request_free(MPI_Request *request); \
  virtual int re_finalize(); 

//  virtaul int re_waitsome(int incount, MPI_Request array_of_requests[], int *outcount, int array_of_indices[], MPI_Status array_of_statuses[]); 


class rempi_re
{
 private:
  rempi_recorder *recorder;  
 protected:
  int my_rank; // = -1;
  int init_after_pmpi_init(int *argc, char ***argv);
 public:
 rempi_re()
   : my_rank(-1)
    {
      recorder = new rempi_recorder();
    };

  ~rempi_re()
    {
      delete recorder;
    };

  REMPI_FUNCTIONS
  
};

#if MPI_VERSION == 3 && !defined(REMPI_LITE)

class rempi_re_cdc : public rempi_re
{
 private:
  PNMPIMOD_register_recv_clocks_t clmpi_register_recv_clocks;
  PNMPIMOD_get_local_clock_t clmpi_get_local_clock;
  PNMPIMOD_collective_sync_clock_t clmpi_collective_sync_clock;
  //  PNMPIMOD_get_recv_clocks_t clmpi_get_recv_clocks;
  int init_clmpi();
  rempi_recorder *recorder;  
#ifdef REVERT1
  unordered_map<string, int> test_ids_map;
#else
  unordered_map<MPI_Request*, int> test_ids_map;
#endif
 protected:
  int collective_sync_clock(MPI_Comm comm);
 public:
  rempi_re_cdc() 
    {
    recorder = new rempi_recorder_cdc();
  };

  ~rempi_re_cdc() {
    free(recorder);
  }
  
  REMPI_FUNCTIONS

};

#endif


/* class rempi_re_no_comp : public rempi_re */
/* { */
/*  protected: */
/*   PNMPIMOD_register_recv_clocks_t clmpi_register_recv_clocks; */
/*   PNMPIMOD_get_local_clock_t clmpi_get_local_clock; */
/*   PNMPIMOD_collective_sync_clock_t clmpi_collective_sync_clock; */
/*   //  PNMPIMOD_get_recv_clocks_t clmpi_get_recv_clocks; */
/*   int init_clmpi(); */
/*   rempi_recorder *recorder;   */
/* #ifdef REVERT1 */
/*   unordered_map<string, int> test_ids_map; */
/* #else */
/*   unordered_map<MPI_Request*, int> test_ids_map; */
/* #endif */
/*   int next_test_id_to_assign;// = 0; */
/*  protected: */
/* #ifdef REVERT1 */
/*   virtual int get_test_id(MPI_Request *request, int count); */
/* #else */
/*   virtual int get_test_id(MPI_Request *requests); */
/* #endif */
/*  public: */
/*   rempi_re_no_comp() */
/*     : rempi_re() */
/*     , next_test_id_to_assign(0) { */
/*       recorder = new rempi_recorder(); */
/*   } */

/*   virtual int re_init(int *argc, char ***argv); */

/*   virtual int re_init_thread( */
/* 			     int *argc, char ***argv, */
/* 			     int required, int *provided); */

/*   virtual int re_isend( */
/* 		       void *buf, */
/* 		       int count, */
/* 		       MPI_Datatype datatype, */
/* 		       int dest, */
/* 		       int tag, */
/* 		       MPI_Comm comm, */
/* 		       MPI_Request *request); */

/*   virtual int re_irecv( */
/* 		       void *buf, */
/* 		       int count, */
/* 		       MPI_Datatype datatype, */
/* 		       int source, */
/* 		       int tag, */
/* 		       MPI_Comm comm, */
/* 		       MPI_Request *request); */

/*   virtual int re_cancel(MPI_Request *request); */
  
/*   virtual int re_test( */
/* 		      MPI_Request *request,  */
/* 		      int *flag,  */
/* 		      MPI_Status *status); */

/*   virtual int re_testsome( */
/* 			  int incount,  */
/* 			  MPI_Request array_of_requests[], */
/* 			  int *outcount,  */
/* 			  int array_of_indices[],  */
/* 			  MPI_Status array_of_statuses[]); */

/*   virtual int re_waitall( */
/* 			  int incount,  */
/* 			  MPI_Request array_of_requests[], */
/* 			  MPI_Status array_of_statuses[]); */

/*   virtual int re_comm_split(MPI_Comm arg_0, int arg_1, int arg_2, MPI_Comm *arg_3); */
/*   virtual int re_comm_create(MPI_Comm arg_0, MPI_Group arg_1, MPI_Comm *arg_2); */
/*   virtual int re_comm_dup(MPI_Comm arg_0, MPI_Comm *arg_2); */

/*   virtual int re_allreduce(const void *arg_0, void *arg_1, int arg_2, MPI_Datatype arg_3, MPI_Op arg_4, MPI_Comm arg_5); */
/*   virtual int re_reduce(const void *arg_0, void *arg_1, int arg_2, MPI_Datatype arg_3, MPI_Op arg_4, int arg_5, MPI_Comm arg_6); */
/*   virtual int re_scan(const void *arg_0, void *arg_1, int arg_2, MPI_Datatype arg_3, MPI_Op arg_4, MPI_Comm arg_5); */
/*   virtual int re_allgather(const void *arg_0, int arg_1, MPI_Datatype arg_2, void *arg_3, int arg_4, MPI_Datatype arg_5, MPI_Comm arg_6); */
/*   virtual int re_gatherv(const void *arg_0, int arg_1, MPI_Datatype arg_2, void *arg_3, const int *arg_4, const int *arg_5, MPI_Datatype arg_6, int arg_7, MPI_Comm arg_8); */
/*   virtual int re_reduce_scatter(const void *arg_0, void *arg_1, const int *arg_2, MPI_Datatype arg_3, MPI_Op arg_4, MPI_Comm arg_5); */
/*   virtual int re_scatterv(const void *arg_0, const int *arg_1, const int *arg_2, MPI_Datatype arg_3, void *arg_4, int arg_5, MPI_Datatype arg_6, int arg_7, MPI_Comm arg_8); */
/*   virtual int re_allgatherv(const void *arg_0, int arg_1, MPI_Datatype arg_2, void *arg_3, const int *arg_4, const int *arg_5, MPI_Datatype arg_6, MPI_Comm arg_7); */
/*   virtual int re_scatter(const void *arg_0, int arg_1, MPI_Datatype arg_2, void *arg_3, int arg_4, MPI_Datatype arg_5, int arg_6, MPI_Comm arg_7); */
/*   virtual int re_bcast(void *arg_0, int arg_1, MPI_Datatype arg_2, int arg_3, MPI_Comm arg_4); */
/*   virtual int re_alltoall(const void *arg_0, int arg_1, MPI_Datatype arg_2, void *arg_3, int arg_4, MPI_Datatype arg_5, MPI_Comm arg_6); */
/*   virtual int re_gather(const void *arg_0, int arg_1, MPI_Datatype arg_2, void *arg_3, int arg_4, MPI_Datatype arg_5, int arg_6, MPI_Comm arg_7); */
/*   virtual int re_barrier(MPI_Comm arg_0); */

/*   virtual int re_finalize(); */
/* }; */





#endif
