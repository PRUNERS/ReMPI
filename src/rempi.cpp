#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/time.h>

#include "rempi_re.h"
#include "rempi_err.h"
#include "pnmpimod.h"

#ifndef _EXTERN_C_
#ifdef __cplusplus
#define _EXTERN_C_ extern "C"
#else /* __cplusplus */
#define _EXTERN_C_
#endif /* __cplusplus */
#endif /* _EXTERN_C_ */

#ifdef MPICH_HAS_C2F
_EXTERN_C_ void *MPIR_ToPointer(int);
#endif // MPICH_HAS_C2F

#ifdef PIC
/* For shared libraries, declare these weak and figure out which one was linked
   based on which init wrapper was called.  See mpi_init wrappers.  */
#pragma weak pmpi_init
#pragma weak PMPI_INIT
#pragma weak pmpi_init_
#pragma weak pmpi_init__
#endif /* PIC */

_EXTERN_C_ void pmpi_init(MPI_Fint *ierr);
_EXTERN_C_ void PMPI_INIT(MPI_Fint *ierr);
_EXTERN_C_ void pmpi_init_(MPI_Fint *ierr);
_EXTERN_C_ void pmpi_init__(MPI_Fint *ierr);

#define PNMPI_MODULE_REMPI "rempi"

rempi_re *rempi_record_replay;


// int PNMPIMOD_get_recv_clocks () {
//   return 1;
// }

int PNMPI_RegistrationPoint()
{
  int err;
  // PNMPI_Service_descriptor_t service;
  // PNMPI_Global_descriptor_t global;

  /* register this module and its services */  
  err = PNMPI_Service_RegisterModule(PNMPI_MODULE_REMPI);
  if (err!=PNMPI_SUCCESS) {
    return MPI_ERROR_PNMPI;
  }

  return err;
}

void init_rempi() {
  rempi_record_replay = new rempi_re_cdc();
  return;
}
// MPI_Init does all the communicator setup
//
/* ================== C Wrappers for MPI_Init ================== */
_EXTERN_C_ int PMPI_Init(int *arg_0, char ***arg_1);
_EXTERN_C_ int MPI_Init(int *arg_0, char ***arg_1)
{ 
  int _wrap_py_return_val = 0;
  init_rempi();

  _wrap_py_return_val = rempi_record_replay->re_init(arg_0, arg_1);
  return _wrap_py_return_val;
}

/* ================== C Wrappers for MPI_Init_thread ================== */
_EXTERN_C_ int PMPI_Init_thread(int *arg_0, char ***arg_1, int arg_2, int *arg_3);
_EXTERN_C_ int MPI_Init_thread(int *arg_0, char ***arg_1, int arg_2, int *arg_3)
{ 
  int _wrap_py_return_val = 0;
  init_rempi();
  _wrap_py_return_val = rempi_record_replay->re_init_thread(arg_0, arg_1, arg_2, arg_3);
  return _wrap_py_return_val;
}

/* ================== C Wrappers for MPI_Irecv ================== */
_EXTERN_C_ int PMPI_Irecv(void *arg_0, int arg_1, MPI_Datatype arg_2, int arg_3, int arg_4, MPI_Comm arg_5, MPI_Request *arg_6);
_EXTERN_C_ int MPI_Irecv(void *arg_0, int arg_1, MPI_Datatype arg_2, int arg_3, int arg_4, MPI_Comm arg_5, MPI_Request *arg_6)
{ 
  int _wrap_py_return_val = 0;
  _wrap_py_return_val = rempi_record_replay->re_irecv(arg_0, arg_1, arg_2, arg_3, arg_4, arg_5, arg_6);
  // int rank;
  // PMPI_Comm_rank(MPI_COMM_WORLD, &rank);
  //  fprintf(stderr, "rank %d: irecv request: %p\n", rank, *arg_6);
  return _wrap_py_return_val;
}

/* ================== C Wrappers for MPI_Test ================== */
_EXTERN_C_ int PMPI_Test(MPI_Request *arg_0, int *arg_1, MPI_Status *arg_2);
_EXTERN_C_ int MPI_Test(MPI_Request *arg_0, int *arg_1, MPI_Status *arg_2)
{ 
  int _wrap_py_return_val = 0;
  _wrap_py_return_val = rempi_record_replay->re_test(arg_0, arg_1, arg_2);
  return _wrap_py_return_val;
}

/* kento================== C Wrappers for MPI_Testsome ================== */
_EXTERN_C_ int PMPI_Testsome(int arg_0, MPI_Request *arg_1, int *arg_2, int *arg_3, MPI_Status *arg_4);
_EXTERN_C_ int MPI_Testsome(int arg_0, MPI_Request *arg_1, int *arg_2, int *arg_3, MPI_Status *arg_4)
{ 
  int _wrap_py_return_val = 0;
  _wrap_py_return_val = rempi_record_replay->re_testsome(arg_0, arg_1, arg_2, arg_3, arg_4);
  return _wrap_py_return_val;
}

/* ================== C Wrappers for MPI_Waitall ================== */
_EXTERN_C_ int PMPI_Wait(MPI_Request *arg_1, MPI_Status *arg_2);
_EXTERN_C_ int MPI_Wait(MPI_Request *arg_1, MPI_Status *arg_2) {
  int _wrap_py_return_val = 0;
  _wrap_py_return_val = rempi_record_replay->re_waitall(1, arg_1, arg_2);
  return _wrap_py_return_val;
}

/* ================== C Wrappers for MPI_Waitall ================== */
_EXTERN_C_ int PMPI_Waitall(int arg_0, MPI_Request *arg_1, MPI_Status *arg_2);
_EXTERN_C_ int MPI_Waitall(int arg_0, MPI_Request *arg_1, MPI_Status *arg_2) {
  int _wrap_py_return_val = 0;
  REMPI_DBG("MPI_Waitall called");
  _wrap_py_return_val = rempi_record_replay->re_waitall(arg_0, arg_1, arg_2);
  return _wrap_py_return_val;
}


/* ================== C Wrappers for MPI_Cancel ================== */
_EXTERN_C_ int PMPI_Cancel(MPI_Request *arg_0);
_EXTERN_C_ int MPI_Cancel(MPI_Request *arg_0) {
  int _wrap_py_return_val = 0;
  {
    /*Message pooling is needed, and arg_0 is not used internal. so ignore this cancel*/
    //    REMPI_ERR("MPI_Cancel called");
    REMPI_DBG("MPI_Cancel called");
    _wrap_py_return_val = rempi_record_replay->re_cancel(arg_0);
  }    return _wrap_py_return_val;
}

/* ================== C Wrappers for MPI_Type_commit ================== */
_EXTERN_C_ int PMPI_Type_commit(MPI_Datatype *arg_0);
_EXTERN_C_ int MPI_Type_commit(MPI_Datatype *arg_0) {
  int _wrap_py_return_val = 0;
  {
   
    _wrap_py_return_val = PMPI_Type_commit(arg_0);
  }    return _wrap_py_return_val;
}

/* ================== C Wrappers for MPI_Type_contiguous ================== */
_EXTERN_C_ int PMPI_Type_contiguous(int arg_0, MPI_Datatype arg_1, MPI_Datatype *arg_2);
_EXTERN_C_ int MPI_Type_contiguous(int arg_0, MPI_Datatype arg_1, MPI_Datatype *arg_2) {
  int _wrap_py_return_val = 0;
  {  
   
    _wrap_py_return_val = PMPI_Type_contiguous(arg_0, arg_1, arg_2);
  }    return _wrap_py_return_val;
}

/* ================== C Wrappers for MPI_Type_struct ================== */
#if MPI_VERSION == 3 && !defined(OPEN_MPI)
_EXTERN_C_ int PMPI_Type_struct(int arg_0, const int *arg_1, const MPI_Aint *arg_2, const MPI_Datatype *arg_3, MPI_Datatype *arg_4);
_EXTERN_C_ int MPI_Type_struct(int arg_0, const int *arg_1, const MPI_Aint *arg_2, const MPI_Datatype *arg_3, MPI_Datatype *arg_4) {
#else
_EXTERN_C_ int PMPI_Type_struct(int arg_0, int *arg_1, MPI_Aint *arg_2, MPI_Datatype *arg_3, MPI_Datatype *arg_4);
_EXTERN_C_ int MPI_Type_struct(int arg_0, int *arg_1, MPI_Aint *arg_2, MPI_Datatype *arg_3, MPI_Datatype *arg_4) {
#endif
  int _wrap_py_return_val = 0;
  {  
   
    _wrap_py_return_val = PMPI_Type_struct(arg_0, arg_1, arg_2, arg_3, arg_4);
  }    return _wrap_py_return_val;
}


/* ================== C Wrappers for MPI_Comm_split ================== */
_EXTERN_C_ int PMPI_Comm_split(MPI_Comm arg_0, int arg_1, int arg_2, MPI_Comm *arg_3);
_EXTERN_C_ int MPI_Comm_split(MPI_Comm arg_0, int arg_1, int arg_2, MPI_Comm *arg_3) {
  int _wrap_py_return_val = 0;
  {

    _wrap_py_return_val = PMPI_Comm_split(arg_0, arg_1, arg_2, arg_3);
  }    return _wrap_py_return_val;
}


/* ================== C Wrappers for MPI_Comm_create ================== */
_EXTERN_C_ int PMPI_Comm_create(MPI_Comm arg_0, MPI_Group arg_1, MPI_Comm *arg_2);
_EXTERN_C_ int MPI_Comm_create(MPI_Comm arg_0, MPI_Group arg_1, MPI_Comm *arg_2) {
  int _wrap_py_return_val = 0;
  {
    _wrap_py_return_val = PMPI_Comm_create(arg_0, arg_1, arg_2);
  }    return _wrap_py_return_val;
}


/* ================== C Wrappers for MPI_Finalize ================== */
_EXTERN_C_ int PMPI_Finalize();
_EXTERN_C_ int MPI_Finalize()
{ 
  int _wrap_py_return_val = 0;
  _wrap_py_return_val = rempi_record_replay->re_finalize();
  return _wrap_py_return_val;
}



/* ================== C Wrappers for MPI_Isend ================== */
#if MPI_VERSION == 3
_EXTERN_C_ int PMPI_Isend(const void *arg_0, int arg_1, MPI_Datatype arg_2, int arg_3, int arg_4, MPI_Comm arg_5, MPI_Request *arg_6);
_EXTERN_C_ int MPI_Isend(const void *arg_0, int arg_1, MPI_Datatype arg_2, int arg_3, int arg_4, MPI_Comm arg_5, MPI_Request *arg_6) {
#else
_EXTERN_C_ int PMPI_Isend(void *arg_0, int arg_1, MPI_Datatype arg_2, int arg_3, int arg_4, MPI_Comm arg_5, MPI_Request *arg_6);
_EXTERN_C_ int MPI_Isend(void *arg_0, int arg_1, MPI_Datatype arg_2, int arg_3, int arg_4, MPI_Comm arg_5, MPI_Request *arg_6) {
#endif

  int _wrap_py_return_val = 0;
  {
    _wrap_py_return_val = rempi_record_replay->re_isend((void *)arg_0, arg_1, arg_2, arg_3, arg_4, arg_5, arg_6);
    // int rank;
    //  PMPI_Comm_rank(MPI_COMM_WORLD, &rank);
    //  if (rank == 1) {
    //    fprintf(stderr, "rank %d: source: %d, tag: %d, isend request: %p\n", rank, arg_3, arg_4, *arg_6);
    //  }
  }    return _wrap_py_return_val;
}

/* ================== C Wrappers for MPI_Send ================== */
#if MPI_VERSION == 3
_EXTERN_C_ int PMPI_Send(const void *arg_0, int arg_1, MPI_Datatype arg_2, int arg_3, int arg_4, MPI_Comm arg_5);
_EXTERN_C_ int MPI_Send(const void *arg_0, int arg_1, MPI_Datatype arg_2, int arg_3, int arg_4, MPI_Comm arg_5) {
#else 
_EXTERN_C_ int PMPI_Send(void *arg_0, int arg_1, MPI_Datatype arg_2, int arg_3, int arg_4, MPI_Comm arg_5);
_EXTERN_C_ int MPI_Send(void *arg_0, int arg_1, MPI_Datatype arg_2, int arg_3, int arg_4, MPI_Comm arg_5) {
#endif
  int _wrap_py_return_val = 0;
  {
    _wrap_py_return_val = PMPI_Send(arg_0, arg_1, arg_2, arg_3, arg_4, arg_5);
  }
  return _wrap_py_return_val;
}











