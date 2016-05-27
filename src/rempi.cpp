#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/time.h>
#include <signal.h>

#include "rempi_re.h"
#include "rempi_err.h"
#include "rempi_type.h"
#include "rempi_config.h"
#include "rempi_sig_handler.h"

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


// int PNMPI_RegistrationPoint()
// {
//   int err;
//   // PNMPI_Service_descriptor_t service;
//   // PNMPI_Global_descriptor_t global;

//   /* register this module and its services */  
//   err = PNMPI_Service_RegisterModule(PNMPI_MODULE_REMPI);
//   if (err!=PNMPI_SUCCESS) {
//     return MPI_ERROR_PNMPI;
//   }

//   return err;
// }

void init_rempi() {
  if ((rempi_encode == 0  || rempi_encode == 8) && rempi_lite) {
    rempi_record_replay = new rempi_re();
  } else if (rempi_lite) {
    REMPI_ERR("No such rempi_encode in ReMPI(Lite): %d", rempi_encode);
  } else {
#ifdef REMPI_LITE
    REMPI_ERR("No such rempi_encode: %d", rempi_encode);
#else
    rempi_record_replay = new rempi_re_cdc();
#endif  
  }

  return;
}

static void print_configuration()
{
  REMPI_DBGI(0, "========== ReMPI Configuration ==========");
  REMPI_DBGI(0, "%16s:  %d", REMPI_ENV_NAME_MODE, rempi_mode);
  REMPI_DBGI(0, "%16s:  %s", REMPI_ENV_NAME_DIR,  rempi_record_dir_path.c_str());
  REMPI_DBGI(0, "%16s:  %d", REMPI_ENV_NAME_ENCODE, rempi_encode);
  REMPI_DBGI(0, "%16s:  %d", REMPI_ENV_NAME_GZIP,  rempi_gzip);
  REMPI_DBGI(0, "%16s:  %d", REMPI_ENV_NAME_TEST_ID, rempi_is_test_id);
  REMPI_DBGI(0, "%16s:  %d", REMPI_ENV_NAME_MAX_EVENT_LENGTH, rempi_max_event_length);
  REMPI_DBGI(0, "==========================================");
}

// MPI_Init does all the communicator setup
//

// void abort(void)
// {
//   REMPI_ASSERT(0);
// }

/* ================== C Wrappers for MPI_Init ================== */
_EXTERN_C_ int PMPI_Init(int *arg_0, char ***arg_1);
_EXTERN_C_ int MPI_Init(int *arg_0, char ***arg_1)
{ 
  REMPI_PREPRINT;
  int _wrap_py_return_val = 0;
  rempi_set_configuration(arg_0, arg_1);
  init_rempi();
  signal(SIGSEGV, SIG_DFL);
  _wrap_py_return_val = rempi_record_replay->re_init(arg_0, arg_1);
  //signal(SIGSEGV, SIG_DFL);
  print_configuration();
  REMPI_POSTPRINT;  
  return _wrap_py_return_val;
}

/* ================== C Wrappers for MPI_Init_thread ================== */
_EXTERN_C_ int PMPI_Init_thread(int *arg_0, char ***arg_1, int arg_2, int *arg_3);
_EXTERN_C_ int MPI_Init_thread(int *arg_0, char ***arg_1, int arg_2, int *arg_3)
{ 
  REMPI_PREPRINT;
  int _wrap_py_return_val = 0;
  rempi_set_configuration(arg_0, arg_1);
  init_rempi();
  signal(SIGSEGV, SIG_DFL);
  _wrap_py_return_val = rempi_record_replay->re_init_thread(arg_0, arg_1, arg_2, arg_3);
  print_configuration();
  //  signal(SIGSEGV, SIG_DFL); 
  REMPI_POSTPRINT;
  return _wrap_py_return_val;
}


/* ================== C Wrappers for MPI_Recv ================== */
_EXTERN_C_ int PMPI_Recv(void *arg_0, int arg_1, MPI_Datatype arg_2, int arg_3, int arg_4, MPI_Comm arg_5, MPI_Status *arg_6);
_EXTERN_C_ int MPI_Recv(void *arg_0, int arg_1, MPI_Datatype arg_2, int arg_3, int arg_4, MPI_Comm arg_5, MPI_Status *arg_6) {
  int _wrap_py_return_val = 0;
  {
    REMPI_PREPRINT;
    /*TODO: Implementation witouh using MPI_Irecv*/
    MPI_Request req;
    //    _wrap_py_return_val = MPI_Irecv(arg_0, arg_1, arg_2, arg_3, arg_4, arg_5, &req);
    _wrap_py_return_val = rempi_record_replay->re_irecv(arg_0, arg_1, arg_2, arg_3, arg_4, arg_5, &req);
    //    REMPI_DBGI(9, "request: %p %p at %s", req, &req, __func__);
    //    _wrap_py_return_val = MPI_Wait(&req, arg_6);
    _wrap_py_return_val = rempi_record_replay->re_wait(&req, arg_6);
  } 
  REMPI_POSTPRINT;
  return _wrap_py_return_val;
}

/* ================== C Wrappers for MPI_Irecv ================== */
_EXTERN_C_ int PMPI_Irecv(void *arg_0, int arg_1, MPI_Datatype arg_2, int arg_3, int arg_4, MPI_Comm arg_5, MPI_Request *arg_6);
_EXTERN_C_ int MPI_Irecv(void *arg_0, int arg_1, MPI_Datatype arg_2, int arg_3, int arg_4, MPI_Comm arg_5, MPI_Request *arg_6)
{ 
  REMPI_PREPRINT;
  int _wrap_py_return_val = 0;
  _wrap_py_return_val = rempi_record_replay->re_irecv(arg_0, arg_1, arg_2, arg_3, arg_4, arg_5, arg_6);
  REMPI_POSTPRINT;
  return _wrap_py_return_val;
}

/* ================== C Wrappers for MPI_Test ================== */
_EXTERN_C_ int PMPI_Test(MPI_Request *arg_0, int *arg_1, MPI_Status *arg_2);
_EXTERN_C_ int MPI_Test(MPI_Request *arg_0, int *arg_1, MPI_Status *arg_2)
{ 
  REMPI_PREPRINT;
  int _wrap_py_return_val = 0;
  _wrap_py_return_val = rempi_record_replay->re_test(arg_0, arg_1, arg_2);
  REMPI_POSTPRINT;
  return _wrap_py_return_val;
}

/* ================== C Wrappers for MPI_Testany ================== */
_EXTERN_C_ int PMPI_Testany(int arg_0, MPI_Request *arg_1, int *arg_2, int *arg_3, MPI_Status *arg_4);
_EXTERN_C_ int MPI_Testany(int arg_0, MPI_Request *arg_1, int *arg_2, int *arg_3, MPI_Status *arg_4) {
  REMPI_PREPRINT;
  int _wrap_py_return_val = 0;
  {  
    _wrap_py_return_val = rempi_record_replay->re_testany(arg_0, arg_1, arg_2, arg_3, arg_4);
  }    
  REMPI_POSTPRINT;
  return _wrap_py_return_val;
}

/* kento================== C Wrappers for MPI_Testsome ================== */
_EXTERN_C_ int PMPI_Testsome(int arg_0, MPI_Request *arg_1, int *arg_2, int *arg_3, MPI_Status *arg_4);
_EXTERN_C_ int MPI_Testsome(int arg_0, MPI_Request *arg_1, int *arg_2, int *arg_3, MPI_Status *arg_4)
{ 
  REMPI_PREPRINT;
  int _wrap_py_return_val = 0;
  _wrap_py_return_val = rempi_record_replay->re_testsome(arg_0, arg_1, arg_2, arg_3, arg_4);
  REMPI_POSTPRINT;
  return _wrap_py_return_val;
}


/* ================== C Wrappers for MPI_Testall ================== */
_EXTERN_C_ int PMPI_Testall(int arg_0, MPI_Request *arg_1, int *arg_2, MPI_Status *arg_3);
_EXTERN_C_ int MPI_Testall(int arg_0, MPI_Request *arg_1, int *arg_2, MPI_Status *arg_3) {
  REMPI_PREPRINT;
  int _wrap_py_return_val = 0;
  {
    _wrap_py_return_val = rempi_record_replay->re_testall(arg_0, arg_1, arg_2, arg_3);
  }    
  REMPI_POSTPRINT;
  return _wrap_py_return_val;
}



/* ================== C Wrappers for MPI_Wait ================== */
_EXTERN_C_ int PMPI_Wait(MPI_Request *arg_1, MPI_Status *arg_2);
_EXTERN_C_ int MPI_Wait(MPI_Request *arg_1, MPI_Status *arg_2) {
  REMPI_PREPRINT;
  int _wrap_py_return_val = 0;
  _wrap_py_return_val = rempi_record_replay->re_wait(arg_1, arg_2);
  REMPI_POSTPRINT;
  return _wrap_py_return_val;
}

/* ================== C Wrappers for MPI_Waitany ================== */
_EXTERN_C_ int PMPI_Waitany(int arg_0, MPI_Request *arg_1, int *arg_2, MPI_Status *arg_3);
_EXTERN_C_ int MPI_Waitany(int arg_0, MPI_Request *arg_1, int *arg_2, MPI_Status *arg_3) {
  REMPI_PREPRINT;
  int _wrap_py_return_val = 0;
  {
    _wrap_py_return_val = rempi_record_replay->re_waitany(arg_0, arg_1, arg_2, arg_3);
  } 
  REMPI_POSTPRINT;
  return _wrap_py_return_val;
}

/* ================== C Wrappers for MPI_Waitsome ================== */
_EXTERN_C_ int PMPI_Waitsome(int arg_0, MPI_Request *arg_1, int *arg_2, int *arg_3, MPI_Status *arg_4);
_EXTERN_C_ int MPI_Waitsome(int arg_0, MPI_Request *arg_1, int *arg_2, int *arg_3, MPI_Status *arg_4) {
  REMPI_PREPRINT;
  int _wrap_py_return_val = 0;
  {
    _wrap_py_return_val = rempi_record_replay->re_waitsome(arg_0, arg_1, arg_2, arg_3, arg_4);
  }    
  REMPI_POSTPRINT;
  return _wrap_py_return_val;
}

/* ================== C Wrappers for MPI_Waitall ================== */
_EXTERN_C_ int PMPI_Waitall(int arg_0, MPI_Request *arg_1, MPI_Status *arg_2);
_EXTERN_C_ int MPI_Waitall(int arg_0, MPI_Request *arg_1, MPI_Status *arg_2) {
  REMPI_PREPRINT;
  int _wrap_py_return_val = 0;
  _wrap_py_return_val = rempi_record_replay->re_waitall(arg_0, arg_1, arg_2);
  REMPI_POSTPRINT;
  return _wrap_py_return_val;
}

/* ================== C Wrappers for MPI_Probe ================== */
_EXTERN_C_ int PMPI_Probe(int arg_0, int arg_1, MPI_Comm arg_2, MPI_Status *arg_3);
_EXTERN_C_ int MPI_Probe(int arg_0, int arg_1, MPI_Comm arg_2, MPI_Status *arg_3) {
  REMPI_PREPRINT;
  int _wrap_py_return_val = 0;
  {  
    _wrap_py_return_val = rempi_record_replay->re_probe(arg_0, arg_1, arg_2, arg_3);
  }    
  REMPI_POSTPRINT;
  return _wrap_py_return_val;
}

/* ================== C Wrappers for MPI_Iprobe ================== */
_EXTERN_C_ int PMPI_Iprobe(int arg_0, int arg_1, MPI_Comm arg_2, int *flag, MPI_Status *arg_4);
_EXTERN_C_ int MPI_Iprobe(int arg_0, int arg_1, MPI_Comm arg_2, int *flag, MPI_Status *arg_4) {
  REMPI_PREPRINT;
  int _wrap_py_return_val = 0;
  {   
    _wrap_py_return_val = rempi_record_replay->re_iprobe(arg_0, arg_1, arg_2, flag, arg_4);
  }    
  REMPI_POSTPRINT;
  return _wrap_py_return_val;
}

/* ================== C Wrappers for MPI_Cancel ================== */
_EXTERN_C_ int PMPI_Cancel(MPI_Request *arg_0);
_EXTERN_C_ int MPI_Cancel(MPI_Request *arg_0) {
  REMPI_PREPRINT;
  int _wrap_py_return_val = 0;
  {
    _wrap_py_return_val = rempi_record_replay->re_cancel(arg_0);
  }    
  REMPI_POSTPRINT;
  return _wrap_py_return_val;
}

/* ================== C Wrappers for MPI_Request_free ================== */
_EXTERN_C_ int PMPI_Request_free(MPI_Request *arg_0);
_EXTERN_C_ int MPI_Request_free(MPI_Request *arg_0) {
  REMPI_PREPRINT;
  int _wrap_py_return_val = 0;
  {
    _wrap_py_return_val = rempi_record_replay->re_request_free(arg_0);
  }    
  REMPI_POSTPRINT;
  return _wrap_py_return_val;
}

/* ================== C Wrappers for MPI_Type_commit ================== */
_EXTERN_C_ int PMPI_Type_commit(MPI_Datatype *arg_0);
_EXTERN_C_ int MPI_Type_commit(MPI_Datatype *arg_0) {
  REMPI_PREPRINT;
  int _wrap_py_return_val = 0;
  {   
    _wrap_py_return_val = PMPI_Type_commit(arg_0);
  }    
  REMPI_POSTPRINT;
  return _wrap_py_return_val;
}

/* ================== C Wrappers for MPI_Type_contiguous ================== */
_EXTERN_C_ int PMPI_Type_contiguous(int arg_0, MPI_Datatype arg_1, MPI_Datatype *arg_2);
_EXTERN_C_ int MPI_Type_contiguous(int arg_0, MPI_Datatype arg_1, MPI_Datatype *arg_2) {
  REMPI_PREPRINT;
  int _wrap_py_return_val = 0;
  {     
    _wrap_py_return_val = PMPI_Type_contiguous(arg_0, arg_1, arg_2);
  }    
  REMPI_POSTPRINT;
  return _wrap_py_return_val;
}

/* ================== C Wrappers for MPI_Type_struct ================== */
#if MPI_VERSION == 3 && !defined(OPEN_MPI)
_EXTERN_C_ int PMPI_Type_struct(int arg_0, const int *arg_1, const MPI_Aint *arg_2, const MPI_Datatype *arg_3, MPI_Datatype *arg_4);
_EXTERN_C_ int MPI_Type_struct(int arg_0, const int *arg_1, const MPI_Aint *arg_2, const MPI_Datatype *arg_3, MPI_Datatype *arg_4) {
#else
_EXTERN_C_ int PMPI_Type_struct(int arg_0, int *arg_1, MPI_Aint *arg_2, MPI_Datatype *arg_3, MPI_Datatype *arg_4);
_EXTERN_C_ int MPI_Type_struct(int arg_0, int *arg_1, MPI_Aint *arg_2, MPI_Datatype *arg_3, MPI_Datatype *arg_4) {
#endif
  REMPI_PREPRINT;
  int _wrap_py_return_val = 0;
  {  
   
    _wrap_py_return_val = PMPI_Type_struct(arg_0, arg_1, arg_2, arg_3, arg_4);
  }    return _wrap_py_return_val;
}


/* ================== C Wrappers for MPI_Comm_split ================== */
_EXTERN_C_ int PMPI_Comm_split(MPI_Comm arg_0, int arg_1, int arg_2, MPI_Comm *arg_3);
_EXTERN_C_ int MPI_Comm_split(MPI_Comm arg_0, int arg_1, int arg_2, MPI_Comm *arg_3) {
  REMPI_PREPRINT;
  int _wrap_py_return_val = 0;
  {
    _wrap_py_return_val = rempi_record_replay->re_comm_split(arg_0, arg_1, arg_2, arg_3);
  }    
  REMPI_POSTPRINT;
  return _wrap_py_return_val;
}


/* ================== C Wrappers for MPI_Comm_create ================== */
_EXTERN_C_ int PMPI_Comm_create(MPI_Comm arg_0, MPI_Group arg_1, MPI_Comm *arg_2);
_EXTERN_C_ int MPI_Comm_create(MPI_Comm arg_0, MPI_Group arg_1, MPI_Comm *arg_2) {
  REMPI_PREPRINT;
  int _wrap_py_return_val = 0;
  {
    _wrap_py_return_val = rempi_record_replay->re_comm_create(arg_0, arg_1, arg_2);
    //    _wrap_py_return_val = PMPI_Comm_create(arg_0, arg_1, arg_2);
  }    
  REMPI_POSTPRINT;
  return _wrap_py_return_val;
}

/* ================== C Wrappers for MPI_Comm_free ================== */
 _EXTERN_C_ int PMPI_Comm_free(MPI_Comm *arg_0);
 _EXTERN_C_ int MPI_Comm_free(MPI_Comm *arg_0) {
   REMPI_PREPRINT;
   int _wrap_py_return_val = 0;
   {  

     _wrap_py_return_val = PMPI_Comm_free(arg_0);
   }
   REMPI_POSTPRINT;
   return _wrap_py_return_val;
 }

/* ================== C Wrappers for MPI_Comm_rank ================== */
_EXTERN_C_ int PMPI_Comm_rank(MPI_Comm arg_0, int *arg_1);
_EXTERN_C_ int MPI_Comm_rank(MPI_Comm arg_0, int *arg_1) {
  REMPI_PREPRINT;
  int _wrap_py_return_val = 0;
  {
    _wrap_py_return_val = PMPI_Comm_rank(arg_0, arg_1);
  }
  REMPI_POSTPRINT;
  return _wrap_py_return_val;
}

/* ================== C Wrappers for MPI_Comm_size ================== */
_EXTERN_C_ int PMPI_Comm_size(MPI_Comm arg_0, int *arg_1);
_EXTERN_C_ int MPI_Comm_size(MPI_Comm arg_0, int *arg_1) {
  REMPI_PREPRINT;
  int _wrap_py_return_val = 0;
  {
     _wrap_py_return_val = PMPI_Comm_size(arg_0, arg_1);
  }
  REMPI_POSTPRINT;
  return _wrap_py_return_val;
}




/* ================== C Wrappers for MPI_Comm_dup ================== */
 _EXTERN_C_ int PMPI_Comm_dup(MPI_Comm arg_0, MPI_Comm *arg_1);
 _EXTERN_C_ int MPI_Comm_dup(MPI_Comm arg_0, MPI_Comm *arg_1) {
  REMPI_PREPRINT;
   int _wrap_py_return_val = 0;
   {
     _wrap_py_return_val = PMPI_Comm_dup(arg_0, arg_1);
   }    
   REMPI_POSTPRINT;
   return _wrap_py_return_val;
 }

 /* ================== C Wrappers for MPI_Comm_group ================== */
 _EXTERN_C_ int PMPI_Comm_group(MPI_Comm arg_0, MPI_Group *arg_1);
 _EXTERN_C_ int MPI_Comm_group(MPI_Comm arg_0, MPI_Group *arg_1) {
   REMPI_PREPRINT;
   int _wrap_py_return_val = 0;
   {  
     _wrap_py_return_val = PMPI_Comm_group(arg_0, arg_1);
   }
   REMPI_POSTPRINT;
   return _wrap_py_return_val;
 }
 
 

/* ================== C Wrappers for MPI_Comm_set_name ================== */
_EXTERN_C_ int PMPI_Comm_set_name(MPI_Comm arg_0, mpi_const char *arg_1);
_EXTERN_C_ int MPI_Comm_set_name(MPI_Comm arg_0, mpi_const char *arg_1) {
  REMPI_PREPRINT;
  int _wrap_py_return_val = 0;
  {  
    _wrap_py_return_val = PMPI_Comm_set_name(arg_0, arg_1);
  }
  REMPI_POSTPRINT;
  return _wrap_py_return_val;
}

/* ================== C Wrappers for MPI_Finalize ================== */
_EXTERN_C_ int PMPI_Finalize();
_EXTERN_C_ int MPI_Finalize()
{ 
  REMPI_PREPRINT;
  int _wrap_py_return_val = 0;
  _wrap_py_return_val = rempi_record_replay->re_finalize();
  delete rempi_record_replay;
  REMPI_POSTPRINT;
  return _wrap_py_return_val;
}



/* ================== C Wrappers for MPI_Isend ================== */
_EXTERN_C_ int PMPI_Isend(mpi_const void *arg_0, int arg_1, MPI_Datatype arg_2, int arg_3, int arg_4, MPI_Comm arg_5, MPI_Request *arg_6);
_EXTERN_C_ int MPI_Isend(mpi_const void *arg_0, int arg_1, MPI_Datatype arg_2, int arg_3, int arg_4, MPI_Comm arg_5, MPI_Request *arg_6) {
  REMPI_PREPRINT;
  int _wrap_py_return_val = 0;
  {
    _wrap_py_return_val = rempi_record_replay->re_isend(arg_0, arg_1, arg_2, arg_3, arg_4, arg_5, arg_6);
    //    REMPI_DBG("buf: %p, request: %p %p at %s", arg_0, *arg_6, arg_6, __func__);
  }
  REMPI_POSTPRINT;
  return _wrap_py_return_val;
}

/* ================== C Wrappers for MPI_Irsend ================== */
_EXTERN_C_ int PMPI_Irsend(mpi_const void *arg_0, int arg_1, MPI_Datatype arg_2, int arg_3, int arg_4, MPI_Comm arg_5, MPI_Request *arg_6);
_EXTERN_C_ int MPI_Irsend(mpi_const void *arg_0, int arg_1, MPI_Datatype arg_2, int arg_3, int arg_4, MPI_Comm arg_5, MPI_Request *arg_6) {
  REMPI_PREPRINT;
  int _wrap_py_return_val = 0;
  {
     _wrap_py_return_val = PMPI_Irsend(arg_0, arg_1, arg_2, arg_3, arg_4, arg_5, arg_6);
  }
  REMPI_POSTPRINT;
  return _wrap_py_return_val;
}

/* ================== C Wrappers for MPI_Send ================== */
_EXTERN_C_ int PMPI_Send(mpi_const void *arg_0, int arg_1, MPI_Datatype arg_2, int arg_3, int arg_4, MPI_Comm arg_5);
_EXTERN_C_ int MPI_Send(mpi_const void *arg_0, int arg_1, MPI_Datatype arg_2, int arg_3, int arg_4, MPI_Comm arg_5) {
  REMPI_PREPRINT;
  int _wrap_py_return_val = 0;
  {
    _wrap_py_return_val = PMPI_Send(arg_0, arg_1, arg_2, arg_3, arg_4, arg_5);
  }
  REMPI_POSTPRINT;
  return _wrap_py_return_val;
}


/* ==================================================================== */
/* ================== C Wrappers for MPI_Collectives ================== */
/* ==================================================================== */


/* ================== C Wrappers for MPI_Allreduce ================== */
_EXTERN_C_ int PMPI_Allreduce(mpi_const void *arg_0, void *arg_1, int arg_2, MPI_Datatype arg_3, MPI_Op arg_4, MPI_Comm arg_5);
_EXTERN_C_ int MPI_Allreduce(mpi_const void *arg_0, void *arg_1, int arg_2, MPI_Datatype arg_3, MPI_Op arg_4, MPI_Comm arg_5)
{
  REMPI_PREPRINT;
  int _wrap_py_return_val = 0;
  {
    //fprintf(stderr, "======= %s =======\n", __func__);
    //    _wrap_py_return_val = PMPI_Allreduce(arg_0, arg_1, arg_2, arg_3, arg_4, arg_5);
    _wrap_py_return_val = rempi_record_replay->re_allreduce(arg_0, arg_1, arg_2, arg_3, arg_4, arg_5);
    REMPI_DBGI(0, "MPI_Allreduce");
    //    fprintf(stderr, "======= %s end =======\n", __func__);
  }    
  REMPI_POSTPRINT;
  return _wrap_py_return_val;
}

/* ================== C Wrappers for MPI_Reduce ================== */
_EXTERN_C_ int PMPI_Reduce(mpi_const void *arg_0, void *arg_1, int arg_2, MPI_Datatype arg_3, MPI_Op arg_4, int arg_5, MPI_Comm arg_6);
_EXTERN_C_ int MPI_Reduce(mpi_const void *arg_0, void *arg_1, int arg_2, MPI_Datatype arg_3, MPI_Op arg_4, int arg_5, MPI_Comm arg_6)
{
  REMPI_PREPRINT;
  int _wrap_py_return_val = 0;
  {
    //    _wrap_py_return_val = PMPI_Reduce(arg_0, arg_1, arg_2, arg_3, arg_4, arg_5, arg_6);
    _wrap_py_return_val = rempi_record_replay->re_reduce(arg_0, arg_1, arg_2, arg_3, arg_4, arg_5, arg_6);
  }    
  REMPI_POSTPRINT;
  return _wrap_py_return_val;
}

/* ================== C Wrappers for MPI_Scan ================== */
_EXTERN_C_ int PMPI_Scan(mpi_const void *arg_0, void *arg_1, int arg_2, MPI_Datatype arg_3, MPI_Op arg_4, MPI_Comm arg_5);
_EXTERN_C_ int MPI_Scan(mpi_const void *arg_0, void *arg_1, int arg_2, MPI_Datatype arg_3, MPI_Op arg_4, MPI_Comm arg_5) {
  REMPI_PREPRINT;
  int _wrap_py_return_val = 0;
  {
    //    _wrap_py_return_val = PMPI_Scan(arg_0, arg_1, arg_2, arg_3, arg_4, arg_5);
    _wrap_py_return_val = rempi_record_replay->re_scan(arg_0, arg_1, arg_2, arg_3, arg_4, arg_5);
    
  }    
  REMPI_POSTPRINT;
  return _wrap_py_return_val;
}

/* ================== C Wrappers for MPI_Allgather ================== */
_EXTERN_C_ int PMPI_Allgather(mpi_const void *arg_0, int arg_1, MPI_Datatype arg_2, void *arg_3, int arg_4, MPI_Datatype arg_5, MPI_Comm arg_6);
_EXTERN_C_ int MPI_Allgather(mpi_const void *arg_0, int arg_1, MPI_Datatype arg_2, void *arg_3, int arg_4, MPI_Datatype arg_5, MPI_Comm arg_6)
 {
  REMPI_PREPRINT;
  int _wrap_py_return_val = 0;
  { 
    //    fprintf(stderr, "======= %s =======\n", __func__); 
    //    _wrap_py_return_val = PMPI_Allgather(arg_0, arg_1, arg_2, arg_3, arg_4, arg_5, arg_6);
    _wrap_py_return_val = rempi_record_replay->re_allgather(arg_0, arg_1, arg_2, arg_3, arg_4, arg_5, arg_6);
    //    fprintf(stderr, "======= %s end =======\n", __func__);
  }    
  REMPI_POSTPRINT;
  return _wrap_py_return_val;
}



/* ================== C Wrappers for MPI_Gatherv ================== */
 _EXTERN_C_ int PMPI_Gatherv(mpi_const void *arg_0, int arg_1, MPI_Datatype arg_2, void *arg_3, mpi_const int *arg_4, mpi_const int *arg_5, MPI_Datatype arg_6, int arg_7, MPI_Comm arg_8);
 _EXTERN_C_ int MPI_Gatherv(mpi_const void *arg_0, int arg_1, MPI_Datatype arg_2, void *arg_3, mpi_const int *arg_4, mpi_const int *arg_5, MPI_Datatype arg_6, int arg_7, MPI_Comm arg_8) {
  REMPI_PREPRINT;
   int _wrap_py_return_val = 0;
   {
     //     _wrap_py_return_val = PMPI_Gatherv(arg_0, arg_1, arg_2, arg_3, arg_4, arg_5, arg_6, arg_7, arg_8);
     _wrap_py_return_val = rempi_record_replay->re_gatherv(arg_0, arg_1, arg_2, arg_3, arg_4, arg_5, arg_6, arg_7, arg_8);
   }    
   REMPI_POSTPRINT;
   return _wrap_py_return_val;
 }

 /* ================== C Wrappers for MPI_Reduce_scatter ================== */
 _EXTERN_C_ int PMPI_Reduce_scatter(mpi_const void *arg_0, void *arg_1, mpi_const int *arg_2, MPI_Datatype arg_3, MPI_Op arg_4, MPI_Comm arg_5);
 _EXTERN_C_ int MPI_Reduce_scatter(mpi_const void *arg_0, void *arg_1, mpi_const int *arg_2, MPI_Datatype arg_3, MPI_Op arg_4, MPI_Comm arg_5) {
  REMPI_PREPRINT;
   int _wrap_py_return_val = 0;
   {
     //     _wrap_py_return_val = PMPI_Reduce_scatter(arg_0, arg_1, arg_2, arg_3, arg_4, arg_5);
     _wrap_py_return_val = rempi_record_replay->re_reduce_scatter(arg_0, arg_1, arg_2, arg_3, arg_4, arg_5);
   }    
   REMPI_POSTPRINT;
   return _wrap_py_return_val;
 }

 /* ================== C Wrappers for MPI_Scatterv ================== */
 _EXTERN_C_ int PMPI_Scatterv(mpi_const void *arg_0, mpi_const int *arg_1, mpi_const int *arg_2, MPI_Datatype arg_3, void *arg_4, int arg_5, MPI_Datatype arg_6, int arg_7, MPI_Comm arg_8);
 _EXTERN_C_ int MPI_Scatterv(mpi_const void *arg_0, mpi_const int *arg_1, mpi_const int *arg_2, MPI_Datatype arg_3, void *arg_4, int arg_5, MPI_Datatype arg_6, int arg_7, MPI_Comm arg_8) {
  REMPI_PREPRINT;
   int _wrap_py_return_val = 0;
   {
     //     _wrap_py_return_val = PMPI_Scatterv(arg_0, arg_1, arg_2, arg_3, arg_4, arg_5, arg_6, arg_7, arg_8);
     _wrap_py_return_val = rempi_record_replay->re_scatterv(arg_0, arg_1, arg_2, arg_3, arg_4, arg_5, arg_6, arg_7, arg_8);
   }    
   REMPI_POSTPRINT;
   return _wrap_py_return_val;
 }

 /* ================== C Wrappers for MPI_Allgatherv ================== */
 _EXTERN_C_ int PMPI_Allgatherv(mpi_const void *arg_0, int arg_1, MPI_Datatype arg_2, void *arg_3, mpi_const int *arg_4, mpi_const int *arg_5, MPI_Datatype arg_6, MPI_Comm arg_7);
 _EXTERN_C_ int MPI_Allgatherv(mpi_const void *arg_0, int arg_1, MPI_Datatype arg_2, void *arg_3, mpi_const int *arg_4, mpi_const int *arg_5, MPI_Datatype arg_6, MPI_Comm arg_7) {
  REMPI_PREPRINT;
   int _wrap_py_return_val = 0;
   {
     //     _wrap_py_return_val = PMPI_Allgatherv(arg_0, arg_1, arg_2, arg_3, arg_4, arg_5, arg_6, arg_7);
     _wrap_py_return_val = rempi_record_replay->re_allgatherv(arg_0, arg_1, arg_2, arg_3, arg_4, arg_5, arg_6, arg_7);
   }    
   REMPI_POSTPRINT;
   return _wrap_py_return_val;
 }

 /* ================== C Wrappers for MPI_Scatter ================== */
 _EXTERN_C_ int PMPI_Scatter(mpi_const void *arg_0, int arg_1, MPI_Datatype arg_2, void *arg_3, int arg_4, MPI_Datatype arg_5, int arg_6, MPI_Comm arg_7);
 _EXTERN_C_ int MPI_Scatter(mpi_const void *arg_0, int arg_1, MPI_Datatype arg_2, void *arg_3, int arg_4, MPI_Datatype arg_5, int arg_6, MPI_Comm arg_7) {
  REMPI_PREPRINT;
   int _wrap_py_return_val = 0;
   {
     //     _wrap_py_return_val = PMPI_Scatter(arg_0, arg_1, arg_2, arg_3, arg_4, arg_5, arg_6, arg_7);
     _wrap_py_return_val = rempi_record_replay->re_scatter(arg_0, arg_1, arg_2, arg_3, arg_4, arg_5, arg_6, arg_7);
   }    
   REMPI_POSTPRINT;
   return _wrap_py_return_val;
 }


/* ================== C Wrappers for MPI_Bcast ================== */
_EXTERN_C_ int PMPI_Bcast(void *arg_0, int arg_1, MPI_Datatype arg_2, int arg_3, MPI_Comm arg_4);
_EXTERN_C_ int MPI_Bcast(void *arg_0, int arg_1, MPI_Datatype arg_2, int arg_3, MPI_Comm arg_4) {
  REMPI_PREPRINT;
   int _wrap_py_return_val = 0;
   {
     //     _wrap_py_return_val = PMPI_Bcast(arg_0, arg_1, arg_2, arg_3, arg_4);
     _wrap_py_return_val = rempi_record_replay->re_bcast(arg_0, arg_1, arg_2, arg_3, arg_4);
   }    
   REMPI_POSTPRINT;
   return _wrap_py_return_val;
}

/* ================== C Wrappers for MPI_Alltoall ================== */
 _EXTERN_C_ int PMPI_Alltoall(mpi_const void *arg_0, int arg_1, MPI_Datatype arg_2, void *arg_3, int arg_4, MPI_Datatype arg_5, MPI_Comm arg_6);
 _EXTERN_C_ int MPI_Alltoall(mpi_const void *arg_0, int arg_1, MPI_Datatype arg_2, void *arg_3, int arg_4, MPI_Datatype arg_5, MPI_Comm arg_6) {
  REMPI_PREPRINT;
   int _wrap_py_return_val = 0;
   {
     //     _wrap_py_return_val = PMPI_Alltoall(arg_0, arg_1, arg_2, arg_3, arg_4, arg_5, arg_6);
     _wrap_py_return_val = rempi_record_replay->re_alltoall(arg_0, arg_1, arg_2, arg_3, arg_4, arg_5, arg_6);
   }    
   REMPI_POSTPRINT;
   return _wrap_py_return_val;
 }

 /* ================== C Wrappers for MPI_Gather ================== */
 _EXTERN_C_ int PMPI_Gather(mpi_const void *arg_0, int arg_1, MPI_Datatype arg_2, void *arg_3, int arg_4, MPI_Datatype arg_5, int arg_6, MPI_Comm arg_7);
 _EXTERN_C_ int MPI_Gather(mpi_const void *arg_0, int arg_1, MPI_Datatype arg_2, void *arg_3, int arg_4, MPI_Datatype arg_5, int arg_6, MPI_Comm arg_7) {
  REMPI_PREPRINT;
   int _wrap_py_return_val = 0;
   {
     //     _wrap_py_return_val = PMPI_Gather(arg_0, arg_1, arg_2, arg_3, arg_4, arg_5, arg_6, arg_7);
     _wrap_py_return_val = rempi_record_replay->re_gather(arg_0, arg_1, arg_2, arg_3, arg_4, arg_5, arg_6, arg_7);
   }    
   REMPI_POSTPRINT;
   return _wrap_py_return_val;
 }


/* ================== C Wrappers for MPI_Get_count ================== */
_EXTERN_C_ int PMPI_Get_count(mpi_const MPI_Status *arg_0, MPI_Datatype arg_1, int *arg_2);
_EXTERN_C_ int MPI_Get_count(mpi_const MPI_Status *arg_0, MPI_Datatype arg_1, int *arg_2) {
  REMPI_PREPRINT;
  int _wrap_py_return_val = 0;
  {
     _wrap_py_return_val = PMPI_Get_count(arg_0, arg_1, arg_2);
  }
  REMPI_POSTPRINT;
  return _wrap_py_return_val;
}


/* ================== C Wrappers for MPI_Barrier ================== */
_EXTERN_C_ int PMPI_Barrier(MPI_Comm arg_0);
_EXTERN_C_ int MPI_Barrier(MPI_Comm arg_0) {
  REMPI_PREPRINT;
   int _wrap_py_return_val = 0;
   {
     //     fprintf(stderr, "======= %s =======\n", __func__);
     _wrap_py_return_val = rempi_record_replay->re_barrier(arg_0);
     //     fprintf(stderr, "======= %s end =======\n", __func__);
   }    
   REMPI_POSTPRINT;
   return _wrap_py_return_val;
}



/* ================== C Wrappers for MPI_Recv_init ================== */
 _EXTERN_C_ int PMPI_Recv_init(void *arg_0, int arg_1, MPI_Datatype arg_2, int arg_3, int arg_4, MPI_Comm arg_5, MPI_Request *arg_6);
 _EXTERN_C_ int MPI_Recv_init(void *arg_0, int arg_1, MPI_Datatype arg_2, int arg_3, int arg_4, MPI_Comm arg_5, MPI_Request *arg_6) {
  REMPI_PREPRINT;
   int _wrap_py_return_val = 0;
   {
     _wrap_py_return_val = PMPI_Recv_init(arg_0, arg_1, arg_2, arg_3, arg_4, arg_5, arg_6);
     //     REMPI_DBGI(9, "request: %p at %s", *arg_6, __func__);
   }    
   REMPI_POSTPRINT;
   return _wrap_py_return_val;
 }

/* ================== C Wrappers for MPI_Send_init ================== */
_EXTERN_C_ int PMPI_Send_init(mpi_const void *arg_0, int arg_1, MPI_Datatype arg_2, int arg_3, int arg_4, MPI_Comm arg_5, MPI_Request *arg_6);
_EXTERN_C_ int MPI_Send_init(mpi_const void *arg_0, int arg_1, MPI_Datatype arg_2, int arg_3, int arg_4, MPI_Comm arg_5, MPI_Request *arg_6) {
  REMPI_PREPRINT;
  int _wrap_py_return_val = 0;
  {
     _wrap_py_return_val = PMPI_Send_init(arg_0, arg_1, arg_2, arg_3, arg_4, arg_5, arg_6);
     //     REMPI_DBGI(9, "request: %p at %s", *arg_6, __func__);
  }    
  REMPI_POSTPRINT;
  return _wrap_py_return_val;
}

/* ================== C Wrappers for MPI_Start ================== */
_EXTERN_C_ int PMPI_Start(MPI_Request *arg_0);
_EXTERN_C_ int MPI_Start(MPI_Request *arg_0) {
  REMPI_PREPRINT;
  int _wrap_py_return_val = 0;
  {
    _wrap_py_return_val = PMPI_Start(arg_0);
    //     REMPI_DBGI(9, "request: %p at %s", *arg_0, __func__);
  }    
  REMPI_POSTPRINT;
  return _wrap_py_return_val;
}

/* ================== C Wrappers for MPI_Startall ================== */
 _EXTERN_C_ int PMPI_Startall(int arg_0, MPI_Request *arg_1);
 _EXTERN_C_ int MPI_Startall(int arg_0, MPI_Request *arg_1) {
  REMPI_PREPRINT;
   int _wrap_py_return_val = 0;
   {
     _wrap_py_return_val = PMPI_Startall(arg_0, arg_1);
     //     REMPI_DBGI(9, "request: %p at %s", *arg_1, __func__);
   }    
   REMPI_POSTPRINT;
   return _wrap_py_return_val;
 }



/* ================== C Wrappers for MPI_Abort ================== */
_EXTERN_C_ int PMPI_Abort(MPI_Comm arg_0, int arg_1);
_EXTERN_C_ int MPI_Abort(MPI_Comm arg_0, int arg_1) {
  REMPI_PREPRINT;
  int _wrap_py_return_val = 0;
  {
    if (rempi_mode == 0) {
      rempi_sig_handler_run(12);
    } else {
      _wrap_py_return_val = PMPI_Abort(arg_0, arg_1);
    }
    //
    //    _wrap_py_return_val = PMPI_Abort(arg_0, arg_1);
  }    
  REMPI_POSTPRINT;
  return _wrap_py_return_val;
}


/* ================== C Wrappers for MPI_Attr_put ================== */
 _EXTERN_C_ int PMPI_Attr_put(MPI_Comm arg_0, int arg_1, void *arg_2);
 _EXTERN_C_ int MPI_Attr_put(MPI_Comm arg_0, int arg_1, void *arg_2) {
   REMPI_PREPRINT;
   int _wrap_py_return_val = 0;
   {
     _wrap_py_return_val = PMPI_Attr_put(arg_0, arg_1, arg_2);
   }
   REMPI_POSTPRINT;
   return _wrap_py_return_val;
 }


/* ================== C Wrappers for MPI_Group_incl ================== */
_EXTERN_C_ int PMPI_Group_incl(MPI_Group group, int arg_1, mpi_const int *arg_2, MPI_Group *arg_3);
_EXTERN_C_ int MPI_Group_incl(MPI_Group group, int arg_1, mpi_const int *arg_2, MPI_Group *arg_3) {
  REMPI_PREPRINT;
  int _wrap_py_return_val = 0;
  {
    _wrap_py_return_val = PMPI_Group_incl(group, arg_1, arg_2, arg_3);
  }
  REMPI_POSTPRINT;
  return _wrap_py_return_val;
}

/* ================== C Wrappers for MPI_Group_free ================== */
_EXTERN_C_ int PMPI_Group_free(MPI_Group *arg_0);
_EXTERN_C_ int MPI_Group_free(MPI_Group *arg_0) {
  REMPI_PREPRINT;
  int _wrap_py_return_val = 0;
  {
    _wrap_py_return_val = PMPI_Group_free(arg_0);
  } 
  REMPI_POSTPRINT;
  return _wrap_py_return_val;
}


/* ================== C Wrappers for MPI_Errhandler_set ================== */
_EXTERN_C_ int PMPI_Errhandler_set(MPI_Comm arg_0, MPI_Errhandler arg_1);
_EXTERN_C_ int MPI_Errhandler_set(MPI_Comm arg_0, MPI_Errhandler arg_1) {
  REMPI_PREPRINT;
  int _wrap_py_return_val = 0;
  {
     _wrap_py_return_val = PMPI_Errhandler_set(arg_0, arg_1);
  }
  REMPI_POSTPRINT;
  return _wrap_py_return_val;
}

#if PMPI_Request_c2f != MPI_Fint && MPI_Request_c2f != MPI_Fint

/* ================== C Wrappers for MPI_Errhandler_set ================== */
_EXTERN_C_ MPI_Fint PMPI_Request_c2f(MPI_Request request);
_EXTERN_C_ MPI_Fint MPI_Request_c2f(MPI_Request request)
{
  REMPI_PREPRINT;
  MPI_Fint _wrap_py_return_val = 0;
  {
    _wrap_py_return_val = rempi_record_replay->re_request_c2f(request);
  }
  REMPI_POSTPRINT;
  return _wrap_py_return_val;
}

#endif
