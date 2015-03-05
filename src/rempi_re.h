#ifndef __REMPI_RE_H__
#define __REMPI_RE_H__

#include <stdlib.h>
#include <string.h>

#include <mpi.h>

#include "rempi_err.h"
#include "rempi_mem.h"
#include "rempi_recorder.h"
#include "clmpi.h"

#define REMPI_COMM_ID_LENGTH (128)

using namespace std;

class rempi_re
{
 private:
 protected:
  int my_rank = -1;
  int init_after_pmpi_init(int *argc, char ***argv);
  virtual int get_test_id();
 public:
  rempi_re();
  virtual int re_init(int *argc, char ***argv);

  virtual int re_init_thread(
				   int *argc, char ***argv,
				   int required, int *provided);
  virtual int re_irecv(
		 void *buf,
		 int count,
		 MPI_Datatype datatype,
		 int source,
		 int tag,
		 MPI_Comm comm,
		 MPI_Request *request);
  
  virtual int re_test(
		      MPI_Request *request, 
		      int *flag, 
		      MPI_Status *status);
  virtual int re_testsome(
			  int incount, 
			  MPI_Request array_of_requests[],
			  int *outcount, 
			  int array_of_indices[], 
			  MPI_Status array_of_statuses[]);

  virtual int re_finalize();
};


class rempi_re_cdc : public rempi_re
{
 private:
  PNMPIMOD_register_recv_clocks_t clmpi_register_recv_clocks;
  //  PNMPIMOD_get_recv_clocks_t clmpi_get_recv_clocks;
  int init_clmpi();
  rempi_recorder *recorder;  
  unordered_map<string, int> test_ids_map;
  int next_test_id_to_assign = 0;
 protected:
  virtual int get_test_id();
 public:
  rempi_re_cdc();
  virtual int re_init(int *argc, char ***argv);

  virtual int re_init_thread(
			     int *argc, char ***argv,
			     int required, int *provided);
  virtual int re_irecv(
		       void *buf,
		       int count,
		       MPI_Datatype datatype,
		       int source,
		       int tag,
		       MPI_Comm comm,
		       MPI_Request *request);
  
  virtual int re_test(
		      MPI_Request *request, 
		      int *flag, 
		      MPI_Status *status);

  virtual int re_testsome(
			  int incount, 
			  MPI_Request array_of_requests[],
			  int *outcount, 
			  int array_of_indices[], 
			  MPI_Status array_of_statuses[]);

  virtual int re_finalize();
};


#endif
