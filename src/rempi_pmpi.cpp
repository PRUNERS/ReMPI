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


#ifndef MPI_F_STATUS_SIZE
#if defined(MPI_STATUS_SIZE) && MPI_STATUS_SIZE > 0
#define MPI_F_STATUS_SIZE MPI_STATUS_SIZE
#else
#define MPI_F_STATUS_SIZE sizeof(MPI_Status) //Kento replaced
//inline int __get_f_status_size(){int size; get_f_status_size(&size); return size;}
//#define MPI_F_STATUS_SIZE __get_f_status_size()
#endif
#endif


  void char_p_f2c(const char* fstr, int len, char** cstr)
{
  const char* end;
  int i;

  /* Leading and trailing blanks are discarded. */

  end = fstr + len - 1;

  for (i = 0; (i < len) && (' ' == *fstr); ++i, ++fstr) {
    continue;
  }

  if (i >= len) {
    len = 0;
  } else {
    for (; (end > fstr) && (' ' == *end); --end) {
      continue;
    }

    len = end - fstr + 1;
  }

  /* Allocate space for the C string, if necessary. */

  if (*cstr == NULL) {
    if ((*cstr = (char*)malloc(len + 1)) == NULL) {
      return;
    }
  }

  /* Copy F77 string into C string and NULL terminate it. */

  if (len > 0) {
    strncpy(*cstr, fstr, len);
  }
  (*cstr)[len] = '\0';
}


void char_p_c2f(const char* cstr, char* fstr, int len)
{
  int i;

  strncpy(fstr, cstr, len);
  for (i = strnlen(cstr, len); i < len; ++i) {
    fstr[i] = ' ';
  }
}

#if defined(__GNUC__) || defined(__INTEL_COMPILER) || defined(__PGI) || defined(_CRAYC)
#if defined(__GNUC__)
#define WEAK_POSTFIX __attribute__ ((weak))
#else
#define WEAK_POSTFIX
#define USE_WEAK_PRAGMA
#endif
/* OpenMPI */
_EXTERN_C_ MPI_Fint mpi_fortran_in_place WEAK_POSTFIX;
_EXTERN_C_ MPI_Fint MPI_FORTRAN_IN_PLACE WEAK_POSTFIX;
_EXTERN_C_ MPI_Fint mpi_fortran_in_place_ WEAK_POSTFIX;
_EXTERN_C_ MPI_Fint MPI_FORTRAN_IN_PLACE_ WEAK_POSTFIX;
_EXTERN_C_ MPI_Fint mpi_fortran_in_place__ WEAK_POSTFIX;
_EXTERN_C_ MPI_Fint MPI_FORTRAN_IN_PLACE__ WEAK_POSTFIX;
_EXTERN_C_ MPI_Fint mpi_fortran_bottom WEAK_POSTFIX;
_EXTERN_C_ MPI_Fint MPI_FORTRAN_BOTTOM WEAK_POSTFIX;
_EXTERN_C_ MPI_Fint mpi_fortran_bottom_ WEAK_POSTFIX;
_EXTERN_C_ MPI_Fint MPI_FORTRAN_BOTTOM_ WEAK_POSTFIX;
_EXTERN_C_ MPI_Fint mpi_fortran_bottom__ WEAK_POSTFIX;
_EXTERN_C_ MPI_Fint MPI_FORTRAN_BOTTOM__ WEAK_POSTFIX;
/* MPICH 2 */
_EXTERN_C_ MPI_Fint MPIFCMB3 WEAK_POSTFIX;
_EXTERN_C_ MPI_Fint mpifcmb3 WEAK_POSTFIX;
_EXTERN_C_ MPI_Fint MPIFCMB3_ WEAK_POSTFIX;
_EXTERN_C_ MPI_Fint mpifcmb3_ WEAK_POSTFIX;
_EXTERN_C_ MPI_Fint MPIFCMB3__ WEAK_POSTFIX;
_EXTERN_C_ MPI_Fint mpifcmb3__ WEAK_POSTFIX;
_EXTERN_C_ MPI_Fint MPIFCMB4 WEAK_POSTFIX;
_EXTERN_C_ MPI_Fint mpifcmb4 WEAK_POSTFIX;
_EXTERN_C_ MPI_Fint MPIFCMB4_ WEAK_POSTFIX;
_EXTERN_C_ MPI_Fint mpifcmb4_ WEAK_POSTFIX;
_EXTERN_C_ MPI_Fint MPIFCMB4__ WEAK_POSTFIX;
_EXTERN_C_ MPI_Fint mpifcmb4__ WEAK_POSTFIX;
/* Argonne Fortran MPI wrappers */
_EXTERN_C_ void *MPIR_F_MPI_BOTTOM WEAK_POSTFIX;
_EXTERN_C_ void *MPIR_F_MPI_IN_PLACE WEAK_POSTFIX;
_EXTERN_C_ void *MPI_F_MPI_BOTTOM WEAK_POSTFIX;
_EXTERN_C_ void *MPI_F_MPI_IN_PLACE WEAK_POSTFIX;


/* MPICH 2 requires no special handling - MPI_BOTTOM may (must!) be passed through as-is. */
#define IsBottom(x) ((x) == (void *) &mpi_fortran_bottom || (x) == (void *) &MPI_FORTRAN_BOTTOM || (x) == (void *) &mpi_fortran_bottom_ ||    (x) == (void *) &MPI_FORTRAN_BOTTOM_ ||    (x) == (void *) & mpi_fortran_bottom__ ||    (x) == (void *) &MPI_FORTRAN_BOTTOM__)
#define IsInPlace(x) ((x) == (void *) &mpi_fortran_in_place ||     (x) == (void *) &MPI_FORTRAN_IN_PLACE ||      (x) == (void *) &mpi_fortran_in_place_ ||     (x) == (void *) &MPI_FORTRAN_IN_PLACE_ ||     (x) == (void *) &mpi_fortran_in_place__ ||     (x) == (void *) &MPI_FORTRAN_IN_PLACE__ ||     (x) == (void *) &MPIFCMB4 ||     (x) == (void *) &mpifcmb4 ||     (x) == (void *) &MPIFCMB4_ || (x) == (void *) &mpifcmb4_ ||     (x) == (void *) &MPIFCMB4__ ||     (x) == (void *) &mpifcmb4__ ||  (x) == MPIR_F_MPI_IN_PLACE ||     (x) == MPI_F_MPI_IN_PLACE)

#ifdef USE_WEAK_PRAGMA
#pragma weak mpi_fortran_in_place
#pragma weak MPI_FORTRAN_IN_PLACE
#pragma weak mpi_fortran_in_place_
#pragma weak MPI_FORTRAN_IN_PLACE_
#pragma weak mpi_fortran_in_place__
#pragma weak MPI_FORTRAN_IN_PLACE__
#pragma weak mpi_fortran_bottom
#pragma weak MPI_FORTRAN_BOTTOM
#pragma weak mpi_fortran_bottom_
#pragma weak MPI_FORTRAN_BOTTOM_
#pragma weak mpi_fortran_bottom__
#pragma weak MPI_FORTRAN_BOTTOM__
/* MPICH 2 */
#pragma weak MPIFCMB3
#pragma weak mpifcmb3
#pragma weak MPIFCMB3_
#pragma weak mpifcmb3_
#pragma weak MPIFCMB3__
#pragma weak mpifcmb3__
#pragma weak MPIFCMB4
#pragma weak mpifcmb4
#pragma weak MPIFCMB4_
#pragma weak mpifcmb4_
#pragma weak MPIFCMB4__
#pragma weak mpifcmb4__
/* Argonne Fortran MPI wrappers */
#pragma weak MPIR_F_MPI_BOTTOM
#pragma weak MPIR_F_MPI_IN_PLACE
#pragma weak MPI_F_MPI_BOTTOM
#pragma weak MPI_F_MPI_IN_PLACE
#endif

#if defined(MPICH_NAME) && (MPICH_NAME == 1) /* MPICH has no MPI_IN_PLACE */
//#define BufferC2F(x) (IsBottom(x) ? MPI_BOTTOM : (x))
#define BufferC2F(x) (x)
#else
//#define BufferC2F(x) (IsBottom(x) ? MPI_BOTTOM : (IsInPlace(x) ? MPI_IN_PLACE : (x)))
#define BufferC2F(x) (x)
#endif /* defined(MPICH_NAME) && (MPICH_NAME == 1) */

#else
#define BufferC2F(x) (x)
#endif /* defined(__GNUC__) || defined(__INTEL_COMPILER) || defined(__PGI) || defined(_CRAYC) */



#if (defined(PIC) || defined(__PIC__))
/* For shared libraries, declare these weak and figure out which one was linked                                                              
   based on which init wrapper was called.  See mpi_init wrappers.  */
#pragma weak pmpi_init
#pragma weak PMPI_INIT
#pragma weak pmpi_init_
#pragma weak pmpi_init__
#pragma weak pmpi_init_thread
#pragma weak PMPI_INIT_THREAD
#pragma weak pmpi_init_thread_
#pragma weak pmpi_init_thread__
#endif /* PIC */

_EXTERN_C_ void pmpi_init(MPI_Fint *ierr);
_EXTERN_C_ void PMPI_INIT(MPI_Fint *ierr);
_EXTERN_C_ void pmpi_init_(MPI_Fint *ierr);
_EXTERN_C_ void pmpi_init__(MPI_Fint *ierr);
_EXTERN_C_ void pmpi_init_thread(MPI_Fint *required, MPI_Fint *provided, MPI_Fint *ierr);
_EXTERN_C_ void PMPI_INIT_THREAD(MPI_Fint *required, MPI_Fint *provided, MPI_Fint *ierr);
_EXTERN_C_ void pmpi_init_thread_(MPI_Fint *required, MPI_Fint *provided, MPI_Fint *ierr);
_EXTERN_C_ void pmpi_init_thread__(MPI_Fint *required, MPI_Fint *provided, MPI_Fint *ierr);




#define PNMPI_MODULE_REMPI "rempi"

#define NOT_SUPPORTED						\
  do {								\
    REMPI_DBG("%s is not supported in this version", __func__); \
  } while(0)



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

static int is_printed = 0;
static void print_configuration()
{
  if (!is_printed) {
    is_printed = 1;
    REMPI_DBGI(0, "========== ReMPI Configuration ==========");
    REMPI_DBGI(0, "%16s:  %d", REMPI_ENV_NAME_MODE, rempi_mode);
    REMPI_DBGI(0, "%16s:  %s", REMPI_ENV_NAME_DIR,  rempi_record_dir_path.c_str());
    REMPI_DBGI(0, "%16s:  %d", REMPI_ENV_NAME_ENCODE, rempi_encode);
    REMPI_DBGI(0, "%16s:  %d", REMPI_ENV_NAME_GZIP,  rempi_gzip);
    REMPI_DBGI(0, "%16s:  %d", REMPI_ENV_NAME_TEST_ID, rempi_is_test_id);
    REMPI_DBGI(0, "%16s:  %d", REMPI_ENV_NAME_MAX_EVENT_LENGTH, rempi_max_event_length);
    REMPI_DBGI(0, "==========================================");
  }
}

// MPI_Init does all the communicator setup
//

// void abort(void)
// {
//   REMPI_ASSERT(0);
// }

static int fortran_init = 0;
/* ================== C Wrappers for MPI_Init ================== */
_EXTERN_C_ int PMPI_Init(int *arg_0, char ***arg_1);
_EXTERN_C_ int MPI_Init(int *arg_0, char ***arg_1)
{ 
  REMPI_PREPRINT;

  int _wrap_py_return_val = 0;
  rempi_set_configuration(arg_0, arg_1);
  init_rempi();
  signal(SIGSEGV, SIG_DFL);
  _wrap_py_return_val = rempi_record_replay->re_init(arg_0, arg_1, fortran_init);
  //signal(SIGSEGV, SIG_DFL);
  print_configuration();
  REMPI_POSTPRINT;  
  return _wrap_py_return_val;
}


/* =============== Fortran Wrappers for MPI_Init =============== */
static void MPI_Init_fortran_wrapper(MPI_Fint *ierr, int argv_len) {
  int argc = 0;
  char ** argv = NULL;
  int _wrap_py_return_val = 0;
  _wrap_py_return_val = MPI_Init(&argc, &argv);
  *ierr = _wrap_py_return_val;
}

_EXTERN_C_ void MPI_INIT(MPI_Fint *ierr, int argv_len) {
  fortran_init = 1;
  MPI_Init_fortran_wrapper(ierr, argv_len);
}

_EXTERN_C_ void mpi_init(MPI_Fint *ierr, int argv_len) {
  fortran_init = 2;
  MPI_Init_fortran_wrapper(ierr, argv_len);
}

_EXTERN_C_ void mpi_init_(MPI_Fint *ierr, int argv_len) {
  fortran_init = 3;
  MPI_Init_fortran_wrapper(ierr, argv_len);
}

_EXTERN_C_ void mpi_init__(MPI_Fint *ierr, int argv_len) {
  fortran_init = 4;
  MPI_Init_fortran_wrapper(ierr, argv_len);
}

/* ================= End Wrappers for MPI_Init ================= */




static int fortran_init_thread = 0;
/* ================== C Wrappers for MPI_Init_thread ================== */
_EXTERN_C_ int PMPI_Init_thread(int *arg_0, char ***arg_1, int arg_2, int *arg_3);
_EXTERN_C_ int MPI_Init_thread(int *arg_0, char ***arg_1, int arg_2, int *arg_3)
{ 
  REMPI_PREPRINT;
  int _wrap_py_return_val = 0;
  rempi_set_configuration(arg_0, arg_1);
  init_rempi();
  signal(SIGSEGV, SIG_DFL);
  _wrap_py_return_val = rempi_record_replay->re_init_thread(arg_0, arg_1, arg_2, arg_3, fortran_init_thread);
  print_configuration();
  //  signal(SIGSEGV, SIG_DFL); 
  REMPI_POSTPRINT;
  return _wrap_py_return_val;
}

/* =============== Fortran Wrappers for MPI_Init_thread =============== */
static void MPI_Init_thread_fortran_wrapper(MPI_Fint *required, MPI_Fint *provided, MPI_Fint *ierr, int argv_len) {
  int argc = 0;
  char ** argv = NULL;
  int _wrap_py_return_val = 0;
  _wrap_py_return_val = MPI_Init_thread(&argc, &argv, *required, provided);
  *ierr = _wrap_py_return_val;
}

_EXTERN_C_ void MPI_INIT_THREAD(MPI_Fint *required, MPI_Fint *provided, MPI_Fint *ierr, int argv_len) {
  fortran_init_thread = 1;
  MPI_Init_thread_fortran_wrapper(required, provided, ierr, argv_len);
}

_EXTERN_C_ void mpi_init_thread(MPI_Fint *required, MPI_Fint *provided, MPI_Fint *ierr, int argv_len) {
  fortran_init_thread = 2;
  MPI_Init_thread_fortran_wrapper(required, provided, ierr, argv_len);
}

_EXTERN_C_ void mpi_init_thread_(MPI_Fint *required, MPI_Fint *provided, MPI_Fint *ierr, int argv_len) {
  fortran_init_thread = 3;
  MPI_Init_thread_fortran_wrapper(required, provided, ierr, argv_len);
}

_EXTERN_C_ void mpi_init_thread__(MPI_Fint *required, MPI_Fint *provided, MPI_Fint *ierr, int argv_len) {
  fortran_init_thread = 4;
  MPI_Init_thread_fortran_wrapper(required, provided, ierr, argv_len);
}

/* ================= End Wrappers for MPI_Init_thread ================= */



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


/* =============== Fortran Wrappers for MPI_Recv =============== */
static void MPI_Recv_fortran_wrapper(MPI_Fint *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *source, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *status, MPI_Fint *ierr) {
  int _wrap_py_return_val = 0;
#if (!defined(MPICH_HAS_C2F) && defined(MPICH_NAME) && (MPICH_NAME == 1)) /* MPICH test */
  _wrap_py_return_val = MPI_Recv(BufferC2F((void*)buf), *count, (MPI_Datatype)(*datatype), *source, *tag, (MPI_Comm)(*comm), (MPI_Status*) status);
#else /* MPI-2 safe call */
  MPI_Status temp_status;
# if defined(MPICH_NAME) && (MPICH_NAME == 1) /* MPICH test */
  MPI_Status_f2c(status, &temp_status);
  _wrap_py_return_val = MPI_Recv(BufferC2F((void*)buf), *count, MPI_Type_f2c(*datatype), *source, *tag, MPI_Comm_f2c(*comm), &temp_status)  ;
  MPI_Status_c2f(&temp_status, status);
# else /* MPI-2 safe call */
  if (status != MPI_F_STATUS_IGNORE) MPI_Status_f2c(status, &temp_status);
  _wrap_py_return_val = MPI_Recv(BufferC2F((void*)buf), *count, MPI_Type_f2c(*datatype), *source, *tag, MPI_Comm_f2c(*comm), ((status == MPI_F_STATUS_IGNORE) ? MPI_STATUS_IGNORE : &temp_status));
  if (status != MPI_F_STATUS_IGNORE) MPI_Status_c2f(&temp_status, status);
# endif /* MPICH test */
#endif /* MPICH test */
  *ierr = _wrap_py_return_val;
}

_EXTERN_C_ void MPI_RECV(MPI_Fint *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *source, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *status, MPI_Fint *ierr) {
  MPI_Recv_fortran_wrapper(buf, count, datatype, source, tag, comm, status, ierr);
}

_EXTERN_C_ void mpi_recv(MPI_Fint *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *source, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *status, MPI_Fint *ierr) {
  MPI_Recv_fortran_wrapper(buf, count, datatype, source, tag, comm, status, ierr);
}

_EXTERN_C_ void mpi_recv_(MPI_Fint *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *source, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *status, MPI_Fint *ierr) {
  MPI_Recv_fortran_wrapper(buf, count, datatype, source, tag, comm, status, ierr);
}

_EXTERN_C_ void mpi_recv__(MPI_Fint *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *source, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *status, MPI_Fint *ierr) {
  MPI_Recv_fortran_wrapper(buf, count, datatype, source, tag, comm, status, ierr);
}

/* ================= End Wrappers for MPI_Recv ================= */







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

/* =============== Fortran Wrappers for MPI_Irecv =============== */
static void MPI_Irecv_fortran_wrapper(MPI_Fint *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *source, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr) {
  int _wrap_py_return_val = 0;
#if (!defined(MPICH_HAS_C2F) && defined(MPICH_NAME) && (MPICH_NAME == 1)) /* MPICH test */
  _wrap_py_return_val = MPI_Irecv(BufferC2F((void*)buf), *count, (MPI_Datatype)(*datatype), *source, *tag, (MPI_Comm)(*comm), (MPI_Request*)request);
#else /* MPI-2 safe call */
  MPI_Request temp_request;
  _wrap_py_return_val = MPI_Irecv(BufferC2F((void*)buf), *count, MPI_Type_f2c(*datatype), *source, *tag, MPI_Comm_f2c(*comm), &temp_request);
  *request = MPI_Request_c2f(temp_request);
#endif /* MPICH test */
  *ierr = _wrap_py_return_val;
}

_EXTERN_C_ void MPI_IRECV(MPI_Fint *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *source, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr) {
  MPI_Irecv_fortran_wrapper(buf, count, datatype, source, tag, comm, request, ierr);
}

_EXTERN_C_ void mpi_irecv(MPI_Fint *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *source, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr) {
  MPI_Irecv_fortran_wrapper(buf, count, datatype, source, tag, comm, request, ierr);
}

_EXTERN_C_ void mpi_irecv_(MPI_Fint *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *source, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr) {
  MPI_Irecv_fortran_wrapper(buf, count, datatype, source, tag, comm, request, ierr);
}

_EXTERN_C_ void mpi_irecv__(MPI_Fint *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *source, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr) {
  MPI_Irecv_fortran_wrapper(buf, count, datatype, source, tag, comm, request, ierr);
}

/* ================= End Wrappers for MPI_Irecv ================= */






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


/* =============== Fortran Wrappers for MPI_Test =============== */
static void MPI_Test_fortran_wrapper(MPI_Fint *request, MPI_Fint *flag, MPI_Fint *status, MPI_Fint *ierr) {
  int _wrap_py_return_val = 0;
#if (!defined(MPICH_HAS_C2F) && defined(MPICH_NAME) && (MPICH_NAME == 1)) /* MPICH test */
  _wrap_py_return_val = MPI_Test((MPI_Request*)request, (int*)BufferC2F((int*)flag), (MPI_Status*)status);
#else /* MPI-2 safe call */
  MPI_Request temp_request;
  MPI_Status temp_status;
# if defined(MPICH_NAME) && (MPICH_NAME == 1) /* MPICH test */
  temp_request = MPI_Request_f2c(*request);
  MPI_Status_f2c(status, &temp_status);
  _wrap_py_return_val = MPI_Test(&temp_request, (int*)BufferC2F((int*)flag), &temp_status);
  *request = MPI_Request_c2f(temp_request);
  MPI_Status_c2f(&temp_status, status);
# else /* MPI-2 safe call */
  temp_request = MPI_Request_f2c(*request);
  if (status != MPI_F_STATUS_IGNORE) MPI_Status_f2c(status, &temp_status);
  _wrap_py_return_val = MPI_Test(&temp_request, (int*)BufferC2F((int*)flag), ((status == MPI_F_STATUS_IGNORE) ? MPI_STATUS_IGNORE : &temp_status));
  *request = MPI_Request_c2f(temp_request);
  if (status != MPI_F_STATUS_IGNORE) MPI_Status_c2f(&temp_status, status);
# endif /* MPICH test */
#endif /* MPICH test */
  *ierr = _wrap_py_return_val;
}

_EXTERN_C_ void MPI_TEST(MPI_Fint *request, MPI_Fint *flag, MPI_Fint *status, MPI_Fint *ierr) {
  MPI_Test_fortran_wrapper(request, flag, status, ierr);
}

_EXTERN_C_ void mpi_test(MPI_Fint *request, MPI_Fint *flag, MPI_Fint *status, MPI_Fint *ierr) {
  MPI_Test_fortran_wrapper(request, flag, status, ierr);
}

_EXTERN_C_ void mpi_test_(MPI_Fint *request, MPI_Fint *flag, MPI_Fint *status, MPI_Fint *ierr) {
  MPI_Test_fortran_wrapper(request, flag, status, ierr);
}

_EXTERN_C_ void mpi_test__(MPI_Fint *request, MPI_Fint *flag, MPI_Fint *status, MPI_Fint *ierr) {
  MPI_Test_fortran_wrapper(request, flag, status, ierr);
}

/* ================= End Wrappers for MPI_Test ================= */




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

/* =============== Fortran Wrappers for MPI_Testany =============== */
static void MPI_Testany_fortran_wrapper(MPI_Fint *count, MPI_Fint array_of_requests[], MPI_Fint *index, MPI_Fint *flag, MPI_Fint *status, MPI_Fint *ierr) {
  int _wrap_py_return_val = 0;
#if (!defined(MPICH_HAS_C2F) && defined(MPICH_NAME) && (MPICH_NAME == 1)) /* MPICH test */
  _wrap_py_return_val = MPI_Testany(*count, (MPI_Request*)array_of_requests, index, (int*)BufferC2F((int*)flag), (MPI_Status*)status);
#else /* MPI-2 safe call */
  MPI_Request* temp_array_of_requests;
  MPI_Status temp_status;
  int i;
# if defined(MPICH_NAME) && (MPICH_NAME == 1) /* MPICH test */
  temp_array_of_requests = (MPI_Request*)malloc(sizeof(MPI_Request) * *count);
  for (i=0; i < *count; i++)
    temp_array_of_requests[i] = MPI_Request_f2c(array_of_requests[i]);
  MPI_Status_f2c(status, &temp_status);
  _wrap_py_return_val = MPI_Testany(*count, temp_array_of_requests, index, (int*)BufferC2F((int*)flag), &temp_status);
  if (*index != MPI_UNDEFINED){
    ++(*index);
    MPI_Status_c2f(&temp_status, status);
  }
# else /* MPI-2 safe call */
  temp_array_of_requests = (MPI_Request*)malloc(sizeof(MPI_Request) * *count);
  for (i=0; i < *count; i++)
    temp_array_of_requests[i] = MPI_Request_f2c(array_of_requests[i]);
  if (status != MPI_F_STATUS_IGNORE) MPI_Status_f2c(status, &temp_status);
  _wrap_py_return_val = MPI_Testany(*count, temp_array_of_requests, index, (int*)BufferC2F((int*)flag), ((status == MPI_F_STATUS_IGNORE) ? MPI_STATUS_IGNORE : &temp_status));
  if (*index != MPI_UNDEFINED){
    ++(*index);
    if (status != MPI_F_STATUS_IGNORE) MPI_Status_c2f(&temp_status, status);
  }
# endif /* MPICH test */
  if(temp_array_of_requests) free(temp_array_of_requests);
#endif /* MPICH test */
  *ierr = _wrap_py_return_val;
}

_EXTERN_C_ void MPI_TESTANY(MPI_Fint *count, MPI_Fint array_of_requests[], MPI_Fint *index, MPI_Fint *flag, MPI_Fint *status, MPI_Fint *ierr) {
  MPI_Testany_fortran_wrapper(count, array_of_requests, index, flag, status, ierr);
}

_EXTERN_C_ void mpi_testany(MPI_Fint *count, MPI_Fint array_of_requests[], MPI_Fint *index, MPI_Fint *flag, MPI_Fint *status, MPI_Fint *ierr) {
  MPI_Testany_fortran_wrapper(count, array_of_requests, index, flag, status, ierr);
}

_EXTERN_C_ void mpi_testany_(MPI_Fint *count, MPI_Fint array_of_requests[], MPI_Fint *index, MPI_Fint *flag, MPI_Fint *status, MPI_Fint *ierr) {
  MPI_Testany_fortran_wrapper(count, array_of_requests, index, flag, status, ierr);
}

_EXTERN_C_ void mpi_testany__(MPI_Fint *count, MPI_Fint array_of_requests[], MPI_Fint *index, MPI_Fint *flag, MPI_Fint *status, MPI_Fint *ierr) {
  MPI_Testany_fortran_wrapper(count, array_of_requests, index, flag, status, ierr);
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

/* =============== Fortran Wrappers for MPI_Testsome =============== */
static void MPI_Testsome_fortran_wrapper(MPI_Fint *incount, MPI_Fint array_of_requests[], MPI_Fint *outcount, MPI_Fint array_of_indices[], MPI_Fint array_of_statuses[], MPI_Fint *ierr) {
  int _wrap_py_return_val = 0;
#if (!defined(MPICH_HAS_C2F) && defined(MPICH_NAME) && (MPICH_NAME == 1)) /* MPICH test */
  _wrap_py_return_val = MPI_Testsome(*incount, (MPI_Request*)array_of_requests, (int*)BufferC2F((int*)outcount), array_of_indices, (MPI_Status*)array_of_statuses);
#else /* MPI-2 safe call */
  MPI_Status* temp_array_of_statuses;
  MPI_Request* temp_array_of_requests;
  int i;
# if defined(MPICH_NAME) && (MPICH_NAME == 1) /* MPICH test */
  temp_array_of_requests = (MPI_Request*)malloc(sizeof(MPI_Request) * *incount);
  for (i=0; i < *incount; i++)
    temp_array_of_requests[i] = MPI_Request_f2c(array_of_requests[i]);
  temp_array_of_statuses = (MPI_Status*)malloc(sizeof(MPI_Status) * *incount);
  _wrap_py_return_val = MPI_Testsome(*incount, temp_array_of_requests, (int*)BufferC2F((int*)outcount), array_of_indices, temp_array_of_statuses);
  if (*outcount != MPI_UNDEFINED){
    for (i=0; i < *outcount; ++i)
      ++array_of_indices[i];
    for (i=0; i < *incount; i++)
      MPI_Status_c2f(&temp_array_of_statuses[i], &array_of_statuses[i * MPI_F_STATUS_SIZE]);
  }
# else /* MPI-2 safe call */
  temp_array_of_requests = (MPI_Request*)malloc(sizeof(MPI_Request) * *incount);
  for (i=0; i < *incount; i++)
    temp_array_of_requests[i] = MPI_Request_f2c(array_of_requests[i]);
  temp_array_of_statuses = (MPI_Status*)malloc(sizeof(MPI_Status) * *incount);
  _wrap_py_return_val = MPI_Testsome(*incount, temp_array_of_requests, (int*)BufferC2F((int*)outcount), array_of_indices, ((array_of_statuses == MPI_F_STATUSES_IGNORE) ? MPI_STATUSES_IGNORE : temp_array_of_statuses));
  if (*outcount != MPI_UNDEFINED){
    for (i=0; i < *outcount; ++i)
      ++array_of_indices[i];
    if (array_of_statuses != MPI_F_STATUSES_IGNORE)
      for (i=0; i < *incount; i++)
        MPI_Status_c2f(&temp_array_of_statuses[i], &array_of_statuses[i * MPI_F_STATUS_SIZE]);
  }
# endif /* MPICH test */
  if(temp_array_of_requests) free(temp_array_of_requests);
  if(temp_array_of_statuses) free(temp_array_of_statuses);
#endif /* MPICH test */
  *ierr = _wrap_py_return_val;
}

_EXTERN_C_ void MPI_TESTSOME(MPI_Fint *incount, MPI_Fint array_of_requests[], MPI_Fint *outcount, MPI_Fint array_of_indices[], MPI_Fint array_of_statuses[], MPI_Fint *ierr) {
  MPI_Testsome_fortran_wrapper(incount, array_of_requests, outcount, array_of_indices, array_of_statuses, ierr);
}

_EXTERN_C_ void mpi_testsome(MPI_Fint *incount, MPI_Fint array_of_requests[], MPI_Fint *outcount, MPI_Fint array_of_indices[], MPI_Fint array_of_statuses[], MPI_Fint *ierr) {
  MPI_Testsome_fortran_wrapper(incount, array_of_requests, outcount, array_of_indices, array_of_statuses, ierr);
}

_EXTERN_C_ void mpi_testsome_(MPI_Fint *incount, MPI_Fint array_of_requests[], MPI_Fint *outcount, MPI_Fint array_of_indices[], MPI_Fint array_of_statuses[], MPI_Fint *ierr) {
  MPI_Testsome_fortran_wrapper(incount, array_of_requests, outcount, array_of_indices, array_of_statuses, ierr);
}

_EXTERN_C_ void mpi_testsome__(MPI_Fint *incount, MPI_Fint array_of_requests[], MPI_Fint *outcount, MPI_Fint array_of_indices[], MPI_Fint array_of_statuses[], MPI_Fint *ierr) {
  MPI_Testsome_fortran_wrapper(incount, array_of_requests, outcount, array_of_indices, array_of_statuses, ierr);
}

/* ================= End Wrappers for MPI_Testsome ================= */



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

/* =============== Fortran Wrappers for MPI_Testall =============== */
static void MPI_Testall_fortran_wrapper(MPI_Fint *count, MPI_Fint array_of_requests[], MPI_Fint *flag, MPI_Fint array_of_statuses[], MPI_Fint *ierr) {
  int _wrap_py_return_val = 0;
#if (!defined(MPICH_HAS_C2F) && defined(MPICH_NAME) && (MPICH_NAME == 1)) /* MPICH test */
  _wrap_py_return_val = MPI_Testall(*count, (MPI_Request*)array_of_requests, (int*)BufferC2F((int*)flag), (MPI_Status*)array_of_statuses);
#else /* MPI-2 safe call */
  MPI_Status* temp_array_of_statuses;
  MPI_Request* temp_array_of_requests;
  int i;
# if defined(MPICH_NAME) && (MPICH_NAME == 1) /* MPICH test */
  temp_array_of_requests = (MPI_Request*)malloc(sizeof(MPI_Request) * *count);
  for (i=0; i < *count; i++)
    temp_array_of_requests[i] = MPI_Request_f2c(array_of_requests[i]);
  temp_array_of_statuses = (MPI_Status*)malloc(sizeof(MPI_Status) * *count);
  _wrap_py_return_val = MPI_Testall(*count, temp_array_of_requests, (int*)BufferC2F((int*)flag), temp_array_of_statuses);
  for (i=0; i < *count; i++)
    MPI_Status_c2f(&temp_array_of_statuses[i], &array_of_statuses[i * MPI_F_STATUS_SIZE]);
# else /* MPI-2 safe call */
  temp_array_of_requests = (MPI_Request*)malloc(sizeof(MPI_Request) * *count);
  for (i=0; i < *count; i++)
    temp_array_of_requests[i] = MPI_Request_f2c(array_of_requests[i]);
  temp_array_of_statuses = (MPI_Status*)malloc(sizeof(MPI_Status) * *count);
  _wrap_py_return_val = MPI_Testall(*count, temp_array_of_requests, (int*)BufferC2F((int*)flag), ((array_of_statuses == MPI_F_STATUSES_IGNORE) ? MPI_STATUSES_IGNORE : temp_array_of_statuses));
  if (array_of_statuses != MPI_F_STATUSES_IGNORE)
    for (i=0; i < *count; i++)
      MPI_Status_c2f(&temp_array_of_statuses[i], &array_of_statuses[i * MPI_F_STATUS_SIZE]);
# endif /* MPICH test */
  if(temp_array_of_requests) free(temp_array_of_requests);
  if(temp_array_of_statuses) free(temp_array_of_statuses);
#endif /* MPICH test */
  *ierr = _wrap_py_return_val;
}

_EXTERN_C_ void MPI_TESTALL(MPI_Fint *count, MPI_Fint array_of_requests[], MPI_Fint *flag, MPI_Fint array_of_statuses[], MPI_Fint *ierr) {
  MPI_Testall_fortran_wrapper(count, array_of_requests, flag, array_of_statuses, ierr);
}

_EXTERN_C_ void mpi_testall(MPI_Fint *count, MPI_Fint array_of_requests[], MPI_Fint *flag, MPI_Fint array_of_statuses[], MPI_Fint *ierr) {
  MPI_Testall_fortran_wrapper(count, array_of_requests, flag, array_of_statuses, ierr);
}

_EXTERN_C_ void mpi_testall_(MPI_Fint *count, MPI_Fint array_of_requests[], MPI_Fint *flag, MPI_Fint array_of_statuses[], MPI_Fint *ierr) {
  MPI_Testall_fortran_wrapper(count, array_of_requests, flag, array_of_statuses, ierr);
}

_EXTERN_C_ void mpi_testall__(MPI_Fint *count, MPI_Fint array_of_requests[], MPI_Fint *flag, MPI_Fint array_of_statuses[], MPI_Fint *ierr) {
  MPI_Testall_fortran_wrapper(count, array_of_requests, flag, array_of_statuses, ierr);
}

/* ================= End Wrappers for MPI_Testall ================= */



/* ================== C Wrappers for MPI_Wait ================== */
_EXTERN_C_ int PMPI_Wait(MPI_Request *arg_1, MPI_Status *arg_2);
_EXTERN_C_ int MPI_Wait(MPI_Request *arg_1, MPI_Status *arg_2) {
  REMPI_PREPRINT;
  int _wrap_py_return_val = 0;
  _wrap_py_return_val = rempi_record_replay->re_wait(arg_1, arg_2);
  REMPI_POSTPRINT;
  return _wrap_py_return_val;
}


/* =============== Fortran Wrappers for MPI_Wait =============== */
static void MPI_Wait_fortran_wrapper(MPI_Fint *request, MPI_Fint *status, MPI_Fint *ierr) {
  int _wrap_py_return_val = 0;
#if (!defined(MPICH_HAS_C2F) && defined(MPICH_NAME) && (MPICH_NAME == 1)) /* MPICH test */
  _wrap_py_return_val = MPI_Wait((MPI_Request*)request, (MPI_Status*)status);
#else /* MPI-2 safe call */
  MPI_Request temp_request;
  MPI_Status temp_status;
# if defined(MPICH_NAME) && (MPICH_NAME == 1) /* MPICH test */
  temp_request = MPI_Request_f2c(*request);
  MPI_Status_f2c(status, &temp_status);
  _wrap_py_return_val = MPI_Wait(&temp_request, &temp_status);
  *request = MPI_Request_c2f(temp_request);
  MPI_Status_c2f(&temp_status, status);
# else /* MPI-2 safe call */
  temp_request = MPI_Request_f2c(*request);
  if (status != MPI_F_STATUS_IGNORE) MPI_Status_f2c(status, &temp_status);
  _wrap_py_return_val = MPI_Wait(&temp_request, ((status == MPI_F_STATUS_IGNORE) ? MPI_STATUS_IGNORE : &temp_status));
  *request = MPI_Request_c2f(temp_request);
  if (status != MPI_F_STATUS_IGNORE) MPI_Status_c2f(&temp_status, status);
# endif /* MPICH test */
#endif /* MPICH test */
  *ierr = _wrap_py_return_val;
}

_EXTERN_C_ void MPI_WAIT(MPI_Fint *request, MPI_Fint *status, MPI_Fint *ierr) {
  MPI_Wait_fortran_wrapper(request, status, ierr);
}

_EXTERN_C_ void mpi_wait(MPI_Fint *request, MPI_Fint *status, MPI_Fint *ierr) {
  MPI_Wait_fortran_wrapper(request, status, ierr);
}

_EXTERN_C_ void mpi_wait_(MPI_Fint *request, MPI_Fint *status, MPI_Fint *ierr) {
  MPI_Wait_fortran_wrapper(request, status, ierr);
}

_EXTERN_C_ void mpi_wait__(MPI_Fint *request, MPI_Fint *status, MPI_Fint *ierr) {
  MPI_Wait_fortran_wrapper(request, status, ierr);
}

/* ================= End Wrappers for MPI_Wait ================= */




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



/* =============== Fortran Wrappers for MPI_Waitany =============== */
static void MPI_Waitany_fortran_wrapper(MPI_Fint *count, MPI_Fint array_of_requests[], MPI_Fint *index, MPI_Fint *status, MPI_Fint *ierr) {
  int _wrap_py_return_val = 0;
#if (!defined(MPICH_HAS_C2F) && defined(MPICH_NAME) && (MPICH_NAME == 1)) /* MPICH test */
  _wrap_py_return_val = MPI_Waitany(*count, (MPI_Request*)array_of_requests, index, (MPI_Status*)status);
#else /* MPI-2 safe call */
  MPI_Request* temp_array_of_requests;
  MPI_Status temp_status;
  int i;
# if defined(MPICH_NAME) && (MPICH_NAME == 1) /* MPICH test */
  temp_array_of_requests = (MPI_Request*)malloc(sizeof(MPI_Request) * *count);
  for (i=0; i < *count; i++)
    temp_array_of_requests[i] = MPI_Request_f2c(array_of_requests[i]);
  MPI_Status_f2c(status, &temp_status);
  _wrap_py_return_val = MPI_Waitany(*count, temp_array_of_requests, index, &temp_status);
  if (*index != MPI_UNDEFINED){
    ++(*index);
    MPI_Status_c2f(&temp_status, status);
  }
# else /* MPI-2 safe call */
  temp_array_of_requests = (MPI_Request*)malloc(sizeof(MPI_Request) * *count);
  for (i=0; i < *count; i++)
    temp_array_of_requests[i] = MPI_Request_f2c(array_of_requests[i]);
  if (status != MPI_F_STATUS_IGNORE) MPI_Status_f2c(status, &temp_status);
  _wrap_py_return_val = MPI_Waitany(*count, temp_array_of_requests, index, ((status == MPI_F_STATUS_IGNORE) ? MPI_STATUS_IGNORE : &temp_status));
  if (*index != MPI_UNDEFINED){
    ++(*index);
    if (status != MPI_F_STATUS_IGNORE) MPI_Status_c2f(&temp_status, status);
  }
# endif /* MPICH test */
  if(temp_array_of_requests) free(temp_array_of_requests);
#endif /* MPICH test */
  *ierr = _wrap_py_return_val;
}

_EXTERN_C_ void MPI_WAITANY(MPI_Fint *count, MPI_Fint array_of_requests[], MPI_Fint *index, MPI_Fint *status, MPI_Fint *ierr) {
  MPI_Waitany_fortran_wrapper(count, array_of_requests, index, status, ierr);
}

_EXTERN_C_ void mpi_waitany(MPI_Fint *count, MPI_Fint array_of_requests[], MPI_Fint *index, MPI_Fint *status, MPI_Fint *ierr) {
  MPI_Waitany_fortran_wrapper(count, array_of_requests, index, status, ierr);
}

_EXTERN_C_ void mpi_waitany_(MPI_Fint *count, MPI_Fint array_of_requests[], MPI_Fint *index, MPI_Fint *status, MPI_Fint *ierr) {
  MPI_Waitany_fortran_wrapper(count, array_of_requests, index, status, ierr);
}

_EXTERN_C_ void mpi_waitany__(MPI_Fint *count, MPI_Fint array_of_requests[], MPI_Fint *index, MPI_Fint *status, MPI_Fint *ierr) {
  MPI_Waitany_fortran_wrapper(count, array_of_requests, index, status, ierr);
}

/* ================= End Wrappers for MPI_Waitany ================= */



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

/* =============== Fortran Wrappers for MPI_Waitsome =============== */
static void MPI_Waitsome_fortran_wrapper(MPI_Fint *incount, MPI_Fint array_of_requests[], MPI_Fint *outcount, MPI_Fint array_of_indices[], MPI_Fint array_of_statuses[], MPI_Fint *ierr) {
  int _wrap_py_return_val = 0;
#if (!defined(MPICH_HAS_C2F) && defined(MPICH_NAME) && (MPICH_NAME == 1)) /* MPICH test */
  _wrap_py_return_val = MPI_Waitsome(*incount, (MPI_Request*)array_of_requests, (int*)BufferC2F((int*)outcount), array_of_indices, (MPI_Status*) array_of_statuses);
#else /* MPI-2 safe call */
  MPI_Status* temp_array_of_statuses;
  MPI_Request* temp_array_of_requests;
  int i;
# if defined(MPICH_NAME) && (MPICH_NAME == 1) /* MPICH test */
  temp_array_of_requests = (MPI_Request*)malloc(sizeof(MPI_Request) * *incount);
  for (i=0; i < *incount; i++)
    temp_array_of_requests[i] = MPI_Request_f2c(array_of_requests[i]);
  temp_array_of_statuses = (MPI_Status*)malloc(sizeof(MPI_Status) * *incount);
  _wrap_py_return_val = MPI_Waitsome(*incount, temp_array_of_requests, (int*)BufferC2F((int*)outcount), array_of_indices, temp_array_of_statuses);
  if (*outcount != MPI_UNDEFINED){
    for (i=0; i < *outcount; ++i)
      ++array_of_indices[i];
    for (i=0; i < *incount; i++)
      MPI_Status_c2f(&temp_array_of_statuses[i], &array_of_statuses[i * MPI_F_STATUS_SIZE]);
  }
# else /* MPI-2 safe call */
  temp_array_of_requests = (MPI_Request*)malloc(sizeof(MPI_Request) * *incount);
  for (i=0; i < *incount; i++)
    temp_array_of_requests[i] = MPI_Request_f2c(array_of_requests[i]);
  temp_array_of_statuses = (MPI_Status*)malloc(sizeof(MPI_Status) * *incount);
  _wrap_py_return_val = MPI_Waitsome(*incount, temp_array_of_requests, (int*)BufferC2F((int*)outcount), array_of_indices, ((array_of_statuses == MPI_F_STATUSES_IGNORE) ? MPI_STATUSES_IGNORE : temp_array_of_statuses));
  if (*outcount != MPI_UNDEFINED){
    for (i=0; i < *outcount; ++i)
      ++array_of_indices[i];
    if (array_of_statuses != MPI_F_STATUSES_IGNORE)
      for (i=0; i < *incount; i++)
        MPI_Status_c2f(&temp_array_of_statuses[i], &array_of_statuses[i * MPI_F_STATUS_SIZE]);
  }
# endif /* MPICH test */
  if(temp_array_of_requests) free(temp_array_of_requests);
  if(temp_array_of_statuses) free(temp_array_of_statuses);
#endif /* MPICH test */
  *ierr = _wrap_py_return_val;
}

_EXTERN_C_ void MPI_WAITSOME(MPI_Fint *incount, MPI_Fint array_of_requests[], MPI_Fint *outcount, MPI_Fint array_of_indices[], MPI_Fint array_of_statuses[], MPI_Fint *ierr) {
  MPI_Waitsome_fortran_wrapper(incount, array_of_requests, outcount, array_of_indices, array_of_statuses, ierr);
}

_EXTERN_C_ void mpi_waitsome(MPI_Fint *incount, MPI_Fint array_of_requests[], MPI_Fint *outcount, MPI_Fint array_of_indices[], MPI_Fint array_of_statuses[], MPI_Fint *ierr) {
  MPI_Waitsome_fortran_wrapper(incount, array_of_requests, outcount, array_of_indices, array_of_statuses, ierr);
}

_EXTERN_C_ void mpi_waitsome_(MPI_Fint *incount, MPI_Fint array_of_requests[], MPI_Fint *outcount, MPI_Fint array_of_indices[], MPI_Fint array_of_statuses[], MPI_Fint *ierr) {
  MPI_Waitsome_fortran_wrapper(incount, array_of_requests, outcount, array_of_indices, array_of_statuses, ierr);
}

_EXTERN_C_ void mpi_waitsome__(MPI_Fint *incount, MPI_Fint array_of_requests[], MPI_Fint *outcount, MPI_Fint array_of_indices[], MPI_Fint array_of_statuses[], MPI_Fint *ierr) {
  MPI_Waitsome_fortran_wrapper(incount, array_of_requests, outcount, array_of_indices, array_of_statuses, ierr);
}

/* ================= End Wrappers for MPI_Waitsome ================= */



/* ================== C Wrappers for MPI_Waitall ================== */
_EXTERN_C_ int PMPI_Waitall(int arg_0, MPI_Request *arg_1, MPI_Status *arg_2);
_EXTERN_C_ int MPI_Waitall(int arg_0, MPI_Request *arg_1, MPI_Status *arg_2) {
  REMPI_PREPRINT;
  int _wrap_py_return_val = 0;
  _wrap_py_return_val = rempi_record_replay->re_waitall(arg_0, arg_1, arg_2);
  REMPI_POSTPRINT;
  return _wrap_py_return_val;
}

/* =============== Fortran Wrappers for MPI_Waitall =============== */
static void MPI_Waitall_fortran_wrapper(MPI_Fint *count, MPI_Fint array_of_requests[], MPI_Fint *array_of_statuses, MPI_Fint *ierr) {
  int _wrap_py_return_val = 0;
#if (!defined(MPICH_HAS_C2F) && defined(MPICH_NAME) && (MPICH_NAME == 1)) /* MPICH test */
  _wrap_py_return_val = MPI_Waitall(*count, (MPI_Request*)array_of_requests, (MPI_Status*)array_of_statuses);
#else /* MPI-2 safe call */
  MPI_Status* temp_array_of_statuses;
  MPI_Request* temp_array_of_requests;
  int i;
# if defined(MPICH_NAME) && (MPICH_NAME == 1) /* MPICH test */
  temp_array_of_requests = (MPI_Request*)malloc(sizeof(MPI_Request) * *count);
  for (i=0; i < *count; i++)
    temp_array_of_requests[i] = MPI_Request_f2c(array_of_requests[i]);
  temp_array_of_statuses = (MPI_Status*)malloc(sizeof(MPI_Status) * *count);
  _wrap_py_return_val = MPI_Waitall(*count, temp_array_of_requests, temp_array_of_statuses);
  for (i=0; i < *count; i++)
    MPI_Status_c2f(&temp_array_of_statuses[i], &array_of_statuses[i * MPI_F_STATUS_SIZE]);
# else /* MPI-2 safe call */
  temp_array_of_requests = (MPI_Request*)malloc(sizeof(MPI_Request) * *count);
  for (i=0; i < *count; i++)
    temp_array_of_requests[i] = MPI_Request_f2c(array_of_requests[i]);
  temp_array_of_statuses = (MPI_Status*)malloc(sizeof(MPI_Status) * *count);
  _wrap_py_return_val = MPI_Waitall(*count, temp_array_of_requests, ((array_of_statuses == MPI_F_STATUSES_IGNORE) ? MPI_STATUSES_IGNORE : temp_array_of_statuses));
  if (array_of_statuses != MPI_F_STATUSES_IGNORE)
    for (i=0; i < *count; i++)
      MPI_Status_c2f(&temp_array_of_statuses[i], &array_of_statuses[i * MPI_F_STATUS_SIZE]);
# endif /* MPICH test */
  if(temp_array_of_requests) free(temp_array_of_requests);
  if(temp_array_of_statuses) free(temp_array_of_statuses);
#endif /* MPICH test */
  *ierr = _wrap_py_return_val;
}

_EXTERN_C_ void MPI_WAITALL(MPI_Fint *count, MPI_Fint array_of_requests[], MPI_Fint *array_of_statuses, MPI_Fint *ierr) {
  MPI_Waitall_fortran_wrapper(count, array_of_requests, array_of_statuses, ierr);
}

_EXTERN_C_ void mpi_waitall(MPI_Fint *count, MPI_Fint array_of_requests[], MPI_Fint *array_of_statuses, MPI_Fint *ierr) {
  MPI_Waitall_fortran_wrapper(count, array_of_requests, array_of_statuses, ierr);
}

_EXTERN_C_ void mpi_waitall_(MPI_Fint *count, MPI_Fint array_of_requests[], MPI_Fint *array_of_statuses, MPI_Fint *ierr) {
  MPI_Waitall_fortran_wrapper(count, array_of_requests, array_of_statuses, ierr);
}

_EXTERN_C_ void mpi_waitall__(MPI_Fint *count, MPI_Fint array_of_requests[], MPI_Fint *array_of_statuses, MPI_Fint *ierr) {
  MPI_Waitall_fortran_wrapper(count, array_of_requests, array_of_statuses, ierr);
}

/* ================= End Wrappers for MPI_Waitall ================= */




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


/* =============== Fortran Wrappers for MPI_Probe =============== */
static void MPI_Probe_fortran_wrapper(MPI_Fint *source, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *status, MPI_Fint *ierr) {
  int _wrap_py_return_val = 0;
#if (!defined(MPICH_HAS_C2F) && defined(MPICH_NAME) && (MPICH_NAME == 1)) /* MPICH test */
  _wrap_py_return_val = MPI_Probe(*source, *tag, (MPI_Comm)(*comm), (MPI_Status*)status);
#else /* MPI-2 safe call */
  MPI_Status temp_status;
# if defined(MPICH_NAME) && (MPICH_NAME == 1) /* MPICH test */
  MPI_Status_f2c(status, &temp_status);
  _wrap_py_return_val = MPI_Probe(*source, *tag, MPI_Comm_f2c(*comm), &temp_status);
  MPI_Status_c2f(&temp_status, status);
# else /* MPI-2 safe call */
  if (status != MPI_F_STATUS_IGNORE) MPI_Status_f2c(status, &temp_status);
  _wrap_py_return_val = MPI_Probe(*source, *tag, MPI_Comm_f2c(*comm), ((status == MPI_F_STATUS_IGNORE) ? MPI_STATUS_IGNORE : &temp_status) );
  if (status != MPI_F_STATUS_IGNORE) MPI_Status_c2f(&temp_status, status);
# endif /* MPICH test */
#endif /* MPICH test */
  *ierr = _wrap_py_return_val;
}

_EXTERN_C_ void MPI_PROBE(MPI_Fint *source, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *status, MPI_Fint *ierr) {
  MPI_Probe_fortran_wrapper(source, tag, comm, status, ierr);
}

_EXTERN_C_ void mpi_probe(MPI_Fint *source, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *status, MPI_Fint *ierr) {
  MPI_Probe_fortran_wrapper(source, tag, comm, status, ierr);
} 

_EXTERN_C_ void mpi_probe_(MPI_Fint *source, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *status, MPI_Fint *ierr) {
  MPI_Probe_fortran_wrapper(source, tag, comm, status, ierr);
}

_EXTERN_C_ void mpi_probe__(MPI_Fint *source, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *status, MPI_Fint *ierr) {
  MPI_Probe_fortran_wrapper(source, tag, comm, status, ierr);
}

/* ================= End Wrappers for MPI_Probe ================= */





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

/* =============== Fortran Wrappers for MPI_Iprobe =============== */
static void MPI_Iprobe_fortran_wrapper(MPI_Fint *source, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *flag, MPI_Fint *status, MPI_Fint *ierr) {
  int _wrap_py_return_val = 0;
#if (!defined(MPICH_HAS_C2F) && defined(MPICH_NAME) && (MPICH_NAME == 1)) /* MPICH test */
  _wrap_py_return_val = MPI_Iprobe(*source, *tag, (MPI_Comm)(*comm), (int*)BufferC2F((int*)flag), (MPI_Status*)status);
#else /* MPI-2 safe call */
  MPI_Status temp_status;
# if defined(MPICH_NAME) && (MPICH_NAME == 1) /* MPICH test */
  MPI_Status_f2c(status, &temp_status);
  _wrap_py_return_val = MPI_Iprobe(*source, *tag, MPI_Comm_f2c(*comm), (int*)BufferC2F((int*)flag), &temp_status);
  MPI_Status_c2f(&temp_status, status);
# else /* MPI-2 safe call */
  if (status != MPI_F_STATUS_IGNORE) MPI_Status_f2c(status, &temp_status);
  _wrap_py_return_val = MPI_Iprobe(*source, *tag, MPI_Comm_f2c(*comm), (int*)BufferC2F((int*)flag), ((status == MPI_F_STATUS_IGNORE) ? MPI_STATUS_IGNORE : &temp_status));
  if (status != MPI_F_STATUS_IGNORE) MPI_Status_c2f(&temp_status, status);
# endif /* MPICH test */
#endif /* MPICH test */
  *ierr = _wrap_py_return_val;
}

_EXTERN_C_ void MPI_IPROBE(MPI_Fint *source, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *flag, MPI_Fint *status, MPI_Fint *ierr) {
  MPI_Iprobe_fortran_wrapper(source, tag, comm, flag, status, ierr);
}

_EXTERN_C_ void mpi_iprobe(MPI_Fint *source, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *flag, MPI_Fint *status, MPI_Fint *ierr) {
  MPI_Iprobe_fortran_wrapper(source, tag, comm, flag, status, ierr);
}

_EXTERN_C_ void mpi_iprobe_(MPI_Fint *source, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *flag, MPI_Fint *status, MPI_Fint *ierr) {
  MPI_Iprobe_fortran_wrapper(source, tag, comm, flag, status, ierr);
}

_EXTERN_C_ void mpi_iprobe__(MPI_Fint *source, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *flag, MPI_Fint *status, MPI_Fint *ierr) {
  MPI_Iprobe_fortran_wrapper(source, tag, comm, flag, status, ierr);
}

/* ================= End Wrappers for MPI_Iprobe ================= */





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

/* =============== Fortran Wrappers for MPI_Cancel =============== */
static void MPI_Cancel_fortran_wrapper(MPI_Fint *request, MPI_Fint *ierr) {
  int _wrap_py_return_val = 0;
#if (!defined(MPICH_HAS_C2F) && defined(MPICH_NAME) && (MPICH_NAME == 1)) /* MPICH test */
  _wrap_py_return_val = MPI_Cancel((MPI_Request*)request);
#else /* MPI-2 safe call */
  MPI_Request temp_request;
  _wrap_py_return_val = MPI_Cancel(&temp_request);
  *request = MPI_Request_c2f(temp_request);
#endif /* MPICH test */
  *ierr = _wrap_py_return_val;
}

_EXTERN_C_ void MPI_CANCEL(MPI_Fint *request, MPI_Fint *ierr) {
  MPI_Cancel_fortran_wrapper(request, ierr);
}

_EXTERN_C_ void mpi_cancel(MPI_Fint *request, MPI_Fint *ierr) {
  MPI_Cancel_fortran_wrapper(request, ierr);
}

_EXTERN_C_ void mpi_cancel_(MPI_Fint *request, MPI_Fint *ierr) {
  MPI_Cancel_fortran_wrapper(request, ierr);
}

_EXTERN_C_ void mpi_cancel__(MPI_Fint *request, MPI_Fint *ierr) {
  MPI_Cancel_fortran_wrapper(request, ierr);
}

/* ================= End Wrappers for MPI_Cancel ================= */



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

/* =============== Fortran Wrappers for MPI_Request_free =============== */
static void MPI_Request_free_fortran_wrapper(MPI_Fint *request, MPI_Fint *ierr) {
  int _wrap_py_return_val = 0;
#if (!defined(MPICH_HAS_C2F) && defined(MPICH_NAME) && (MPICH_NAME == 1)) /* MPICH test */
  _wrap_py_return_val = MPI_Request_free((MPI_Request*)request);
#else /* MPI-2 safe call */
  MPI_Request temp_request;
  temp_request = MPI_Request_f2c(*request);
  _wrap_py_return_val = MPI_Request_free(&temp_request);
  *request = MPI_Request_c2f(temp_request);
#endif /* MPICH test */
  *ierr = _wrap_py_return_val;
}

_EXTERN_C_ void MPI_REQUEST_FREE(MPI_Fint *request, MPI_Fint *ierr) {
  MPI_Request_free_fortran_wrapper(request, ierr);
}

_EXTERN_C_ void mpi_request_free(MPI_Fint *request, MPI_Fint *ierr) {
  MPI_Request_free_fortran_wrapper(request, ierr);
}

_EXTERN_C_ void mpi_request_free_(MPI_Fint *request, MPI_Fint *ierr) {
  MPI_Request_free_fortran_wrapper(request, ierr);
}

_EXTERN_C_ void mpi_request_free__(MPI_Fint *request, MPI_Fint *ierr) {
  MPI_Request_free_fortran_wrapper(request, ierr);
}

/* ================= End Wrappers for MPI_Request_free ================= */



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

/* =============== Fortran Wrappers for MPI_Type_commit =============== */
static void MPI_Type_commit_fortran_wrapper(MPI_Fint *type, MPI_Fint *ierr) {
  int _wrap_py_return_val = 0;
#if (!defined(MPICH_HAS_C2F) && defined(MPICH_NAME) && (MPICH_NAME == 1)) /* MPICH test */
  _wrap_py_return_val = MPI_Type_commit((MPI_Datatype*)type);
#else /* MPI-2 safe call */
  MPI_Datatype temp_type;
  temp_type = MPI_Type_f2c(*type);
  _wrap_py_return_val = MPI_Type_commit(&temp_type);
  *type = MPI_Type_c2f(temp_type);
#endif /* MPICH test */
  *ierr = _wrap_py_return_val;
}

_EXTERN_C_ void MPI_TYPE_COMMIT(MPI_Fint *type, MPI_Fint *ierr) {
  MPI_Type_commit_fortran_wrapper(type, ierr);
}

_EXTERN_C_ void mpi_type_commit(MPI_Fint *type, MPI_Fint *ierr) {
  MPI_Type_commit_fortran_wrapper(type, ierr);
}

_EXTERN_C_ void mpi_type_commit_(MPI_Fint *type, MPI_Fint *ierr) {
  MPI_Type_commit_fortran_wrapper(type, ierr);
}

_EXTERN_C_ void mpi_type_commit__(MPI_Fint *type, MPI_Fint *ierr) {
  MPI_Type_commit_fortran_wrapper(type, ierr);
}

/* ================= End Wrappers for MPI_Type_commit ================= */



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

/* =============== Fortran Wrappers for MPI_Type_contiguous =============== */
static void MPI_Type_contiguous_fortran_wrapper(MPI_Fint *count, MPI_Fint *oldtype, MPI_Fint *newtype, MPI_Fint *ierr) {
  int _wrap_py_return_val = 0;
#if (!defined(MPICH_HAS_C2F) && defined(MPICH_NAME) && (MPICH_NAME == 1)) /* MPICH test */
  _wrap_py_return_val = MPI_Type_contiguous(*count, (MPI_Datatype)(*oldtype), (MPI_Datatype*)newtype);
#else /* MPI-2 safe call */
  MPI_Datatype temp_newtype;
  _wrap_py_return_val = MPI_Type_contiguous(*count, MPI_Type_f2c(*oldtype), &temp_newtype);
  *newtype = MPI_Type_c2f(temp_newtype);
#endif /* MPICH test */
  *ierr = _wrap_py_return_val;
}

_EXTERN_C_ void MPI_TYPE_CONTIGUOUS(MPI_Fint *count, MPI_Fint *oldtype, MPI_Fint *newtype, MPI_Fint *ierr) {
  MPI_Type_contiguous_fortran_wrapper(count, oldtype, newtype, ierr);
}

_EXTERN_C_ void mpi_type_contiguous(MPI_Fint *count, MPI_Fint *oldtype, MPI_Fint *newtype, MPI_Fint *ierr) {
  MPI_Type_contiguous_fortran_wrapper(count, oldtype, newtype, ierr);
}

_EXTERN_C_ void mpi_type_contiguous_(MPI_Fint *count, MPI_Fint *oldtype, MPI_Fint *newtype, MPI_Fint *ierr) {
  MPI_Type_contiguous_fortran_wrapper(count, oldtype, newtype, ierr);
}

_EXTERN_C_ void mpi_type_contiguous__(MPI_Fint *count, MPI_Fint *oldtype, MPI_Fint *newtype, MPI_Fint *ierr) {
  MPI_Type_contiguous_fortran_wrapper(count, oldtype, newtype, ierr);
}

/* ================= End Wrappers for MPI_Type_contiguous ================= */



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

  /* =============== Fortran Wrappers for MPI_Type_struct =============== */
  static void MPI_Type_struct_fortran_wrapper(MPI_Fint *count, MPI_Fint array_of_blocklengths[], MPI_Aint array_of_displacements[], MPI_Fint array_of_types[], MPI_Fint *newtype, MPI_Fint *ierr) {
    int _wrap_py_return_val = 0;
#if (!defined(MPICH_HAS_C2F) && defined(MPICH_NAME) && (MPICH_NAME == 1)) /* MPICH test */
    _wrap_py_return_val = MPI_Type_struct(*count, (int*)BufferC2F((int*)array_of_blocklengths), (MPI_Aint*))BufferC2F((MPI_Aint*)array_of_displacements), (MPI_Datatype*)array_of_types, (MPI_Datatype*)newtype);
#else /* MPI-2 safe call */
  MPI_Datatype temp_newtype;
  MPI_Datatype* temp_array_of_types;
  int i;
  temp_array_of_types = (MPI_Datatype*)malloc(sizeof(MPI_Datatype) * *count);
  for (i=0; i < *count; i++)
    temp_array_of_types[i] = MPI_Type_f2c(array_of_types[i]);
  _wrap_py_return_val = MPI_Type_struct(*count, (int*)BufferC2F((int*)array_of_blocklengths), (MPI_Aint*)BufferC2F((MPI_Aint*)array_of_displacements), temp_array_of_types, &temp_newtype);
  *newtype = MPI_Type_c2f(temp_newtype);
  if(temp_array_of_types) free(temp_array_of_types);
#endif /* MPICH test */
  *ierr = _wrap_py_return_val;
}

_EXTERN_C_ void MPI_TYPE_STRUCT(MPI_Fint *count, MPI_Fint array_of_blocklengths[], MPI_Aint array_of_displacements[], MPI_Fint array_of_types[], MPI_Fint *newtype, MPI_Fint *ierr) {
  MPI_Type_struct_fortran_wrapper(count, array_of_blocklengths, array_of_displacements, array_of_types, newtype, ierr);
}

_EXTERN_C_ void mpi_type_struct(MPI_Fint *count, MPI_Fint array_of_blocklengths[], MPI_Aint array_of_displacements[], MPI_Fint array_of_types[], MPI_Fint *newtype, MPI_Fint *ierr) {
  MPI_Type_struct_fortran_wrapper(count, array_of_blocklengths, array_of_displacements, array_of_types, newtype, ierr);
}

_EXTERN_C_ void mpi_type_struct_(MPI_Fint *count, MPI_Fint array_of_blocklengths[], MPI_Aint array_of_displacements[], MPI_Fint array_of_types[], MPI_Fint *newtype, MPI_Fint *ierr) {
  MPI_Type_struct_fortran_wrapper(count, array_of_blocklengths, array_of_displacements, array_of_types, newtype, ierr);
}

_EXTERN_C_ void mpi_type_struct__(MPI_Fint *count, MPI_Fint array_of_blocklengths[], MPI_Aint array_of_displacements[], MPI_Fint array_of_types[], MPI_Fint *newtype, MPI_Fint *ierr) {
  MPI_Type_struct_fortran_wrapper(count, array_of_blocklengths, array_of_displacements, array_of_types, newtype, ierr);
}

/* ================= End Wrappers for MPI_Type_struct ================= */




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

/* =============== Fortran Wrappers for MPI_Comm_split =============== */
static void MPI_Comm_split_fortran_wrapper(MPI_Fint *comm, MPI_Fint *color, MPI_Fint *key, MPI_Fint *newcomm, MPI_Fint *ierr) {
  int _wrap_py_return_val = 0;
#if (!defined(MPICH_HAS_C2F) && defined(MPICH_NAME) && (MPICH_NAME == 1)) /* MPICH test */
  _wrap_py_return_val = MPI_Comm_split((MPI_Comm)(*comm), *color, *key, (MPI_Comm*)newcomm);
#else /* MPI-2 safe call */
  MPI_Comm temp_newcomm;
  _wrap_py_return_val = MPI_Comm_split(MPI_Comm_f2c(*comm), *color, *key, &temp_newcomm);
  *newcomm = MPI_Comm_c2f(temp_newcomm);
#endif /* MPICH test */
  *ierr = _wrap_py_return_val;
}

_EXTERN_C_ void MPI_COMM_SPLIT(MPI_Fint *comm, MPI_Fint *color, MPI_Fint *key, MPI_Fint *newcomm, MPI_Fint *ierr) {
  MPI_Comm_split_fortran_wrapper(comm, color, key, newcomm, ierr);
}

_EXTERN_C_ void mpi_comm_split(MPI_Fint *comm, MPI_Fint *color, MPI_Fint *key, MPI_Fint *newcomm, MPI_Fint *ierr) {
  MPI_Comm_split_fortran_wrapper(comm, color, key, newcomm, ierr);
}

_EXTERN_C_ void mpi_comm_split_(MPI_Fint *comm, MPI_Fint *color, MPI_Fint *key, MPI_Fint *newcomm, MPI_Fint *ierr) {
  MPI_Comm_split_fortran_wrapper(comm, color, key, newcomm, ierr);
}

_EXTERN_C_ void mpi_comm_split__(MPI_Fint *comm, MPI_Fint *color, MPI_Fint *key, MPI_Fint *newcomm, MPI_Fint *ierr) {
  MPI_Comm_split_fortran_wrapper(comm, color, key, newcomm, ierr);
}

/* ================= End Wrappers for MPI_Comm_split ================= */


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
/* =============== Fortran Wrappers for MPI_Comm_create =============== */
static void MPI_Comm_create_fortran_wrapper(MPI_Fint *comm, MPI_Fint *group, MPI_Fint *newcomm, MPI_Fint *ierr) {
  int _wrap_py_return_val = 0;
#if (!defined(MPICH_HAS_C2F) && defined(MPICH_NAME) && (MPICH_NAME == 1)) /* MPICH test */
  _wrap_py_return_val = MPI_Comm_create((MPI_Comm)(*comm), (MPI_Group)(*group), (MPI_Comm*)newcomm);
#else /* MPI-2 safe call */
  MPI_Comm temp_newcomm;
  _wrap_py_return_val = MPI_Comm_create(MPI_Comm_f2c(*comm), MPI_Group_f2c(*group), &temp_newcomm);
  *newcomm = MPI_Comm_c2f(temp_newcomm);
#endif /* MPICH test */
  *ierr = _wrap_py_return_val;
}

_EXTERN_C_ void MPI_COMM_CREATE(MPI_Fint *comm, MPI_Fint *group, MPI_Fint *newcomm, MPI_Fint *ierr) {
  MPI_Comm_create_fortran_wrapper(comm, group, newcomm, ierr);
}

_EXTERN_C_ void mpi_comm_create(MPI_Fint *comm, MPI_Fint *group, MPI_Fint *newcomm, MPI_Fint *ierr) {
  MPI_Comm_create_fortran_wrapper(comm, group, newcomm, ierr);
}

_EXTERN_C_ void mpi_comm_create_(MPI_Fint *comm, MPI_Fint *group, MPI_Fint *newcomm, MPI_Fint *ierr) {
  MPI_Comm_create_fortran_wrapper(comm, group, newcomm, ierr);
}

_EXTERN_C_ void mpi_comm_create__(MPI_Fint *comm, MPI_Fint *group, MPI_Fint *newcomm, MPI_Fint *ierr) {
  MPI_Comm_create_fortran_wrapper(comm, group, newcomm, ierr);
}

/* ================= End Wrappers for MPI_Comm_create ================= */






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



/* =============== Fortran Wrappers for MPI_Comm_free =============== */
static void MPI_Comm_free_fortran_wrapper(MPI_Fint *comm, MPI_Fint *ierr) {
  int _wrap_py_return_val = 0;
#if (!defined(MPICH_HAS_C2F) && defined(MPICH_NAME) && (MPICH_NAME == 1)) /* MPICH test */
  _wrap_py_return_val = MPI_Comm_free((MPI_Comm*)comm);
#else /* MPI-2 safe call */
  MPI_Comm temp_comm;
  temp_comm = MPI_Comm_f2c(*comm);
  _wrap_py_return_val = MPI_Comm_free(&temp_comm);
  *comm = MPI_Comm_c2f(temp_comm);
#endif /* MPICH test */
  *ierr = _wrap_py_return_val;
}

_EXTERN_C_ void MPI_COMM_FREE(MPI_Fint *comm, MPI_Fint *ierr) {
  MPI_Comm_free_fortran_wrapper(comm, ierr);
}

_EXTERN_C_ void mpi_comm_free(MPI_Fint *comm, MPI_Fint *ierr) {
  MPI_Comm_free_fortran_wrapper(comm, ierr);
}

_EXTERN_C_ void mpi_comm_free_(MPI_Fint *comm, MPI_Fint *ierr) {
  MPI_Comm_free_fortran_wrapper(comm, ierr);
}

_EXTERN_C_ void mpi_comm_free__(MPI_Fint *comm, MPI_Fint *ierr) {
  MPI_Comm_free_fortran_wrapper(comm, ierr);
}

/* ================= End Wrappers for MPI_Comm_free ================= */




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


/* =============== Fortran Wrappers for MPI_Comm_rank =============== */
static void MPI_Comm_rank_fortran_wrapper(MPI_Fint *comm, MPI_Fint *rank, MPI_Fint *ierr) {
  int _wrap_py_return_val = 0;
#if (!defined(MPICH_HAS_C2F) && defined(MPICH_NAME) && (MPICH_NAME == 1)) /* MPICH test */
  _wrap_py_return_val = MPI_Comm_rank((MPI_Comm)(*comm), (int*)BufferC2F((int*)rank));
#else /* MPI-2 safe call */
  _wrap_py_return_val = MPI_Comm_rank(MPI_Comm_f2c(*comm), (int*)BufferC2F((int*)rank));
#endif /* MPICH test */
  *ierr = _wrap_py_return_val;
}

_EXTERN_C_ void MPI_COMM_RANK(MPI_Fint *comm, MPI_Fint *rank, MPI_Fint *ierr) {
  MPI_Comm_rank_fortran_wrapper(comm, rank, ierr);
}

_EXTERN_C_ void mpi_comm_rank(MPI_Fint *comm, MPI_Fint *rank, MPI_Fint *ierr) {
  MPI_Comm_rank_fortran_wrapper(comm, rank, ierr);
}

_EXTERN_C_ void mpi_comm_rank_(MPI_Fint *comm, MPI_Fint *rank, MPI_Fint *ierr) {
  MPI_Comm_rank_fortran_wrapper(comm, rank, ierr);
}

_EXTERN_C_ void mpi_comm_rank__(MPI_Fint *comm, MPI_Fint *rank, MPI_Fint *ierr) {
  MPI_Comm_rank_fortran_wrapper(comm, rank, ierr);
}

/* ================= End Wrappers for MPI_Comm_rank ================= */



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

/* =============== Fortran Wrappers for MPI_Comm_size =============== */
static void MPI_Comm_size_fortran_wrapper(MPI_Fint *comm, MPI_Fint *size, MPI_Fint *ierr) {
  int _wrap_py_return_val = 0;
#if (!defined(MPICH_HAS_C2F) && defined(MPICH_NAME) && (MPICH_NAME == 1)) /* MPICH test */
  _wrap_py_return_val = MPI_Comm_size((MPI_Comm)(*comm), (int*)BufferC2F((int*)size));
#else /* MPI-2 safe call */
  _wrap_py_return_val = MPI_Comm_size(MPI_Comm_f2c(*comm), (int*)BufferC2F((int*)size));
#endif /* MPICH test */
  *ierr = _wrap_py_return_val;
}

_EXTERN_C_ void MPI_COMM_SIZE(MPI_Fint *comm, MPI_Fint *size, MPI_Fint *ierr) {
  MPI_Comm_size_fortran_wrapper(comm, size, ierr);
}

_EXTERN_C_ void mpi_comm_size(MPI_Fint *comm, MPI_Fint *size, MPI_Fint *ierr) {
  MPI_Comm_size_fortran_wrapper(comm, size, ierr);
}

_EXTERN_C_ void mpi_comm_size_(MPI_Fint *comm, MPI_Fint *size, MPI_Fint *ierr) {
  MPI_Comm_size_fortran_wrapper(comm, size, ierr);
}

_EXTERN_C_ void mpi_comm_size__(MPI_Fint *comm, MPI_Fint *size, MPI_Fint *ierr) {
  MPI_Comm_size_fortran_wrapper(comm, size, ierr);
}

/* ================= End Wrappers for MPI_Comm_size ================= */





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

/* =============== Fortran Wrappers for MPI_Comm_dup =============== */
static void MPI_Comm_dup_fortran_wrapper(MPI_Fint *comm, MPI_Fint *newcomm, MPI_Fint *ierr) {
  int _wrap_py_return_val = 0;
#if (!defined(MPICH_HAS_C2F) && defined(MPICH_NAME) && (MPICH_NAME == 1)) /* MPICH test */
  _wrap_py_return_val = MPI_Comm_dup((MPI_Comm)(*comm), (MPI_Comm*)newcomm);
#else /* MPI-2 safe call */
  MPI_Comm temp_newcomm;
  _wrap_py_return_val = MPI_Comm_dup(MPI_Comm_f2c(*comm), &temp_newcomm);
  *newcomm = MPI_Comm_c2f(temp_newcomm);
#endif /* MPICH test */
  *ierr = _wrap_py_return_val;
}

_EXTERN_C_ void MPI_COMM_DUP(MPI_Fint *comm, MPI_Fint *newcomm, MPI_Fint *ierr) {
  MPI_Comm_dup_fortran_wrapper(comm, newcomm, ierr);
}

_EXTERN_C_ void mpi_comm_dup(MPI_Fint *comm, MPI_Fint *newcomm, MPI_Fint *ierr) {
  MPI_Comm_dup_fortran_wrapper(comm, newcomm, ierr);
}

_EXTERN_C_ void mpi_comm_dup_(MPI_Fint *comm, MPI_Fint *newcomm, MPI_Fint *ierr) {
  MPI_Comm_dup_fortran_wrapper(comm, newcomm, ierr);
}

_EXTERN_C_ void mpi_comm_dup__(MPI_Fint *comm, MPI_Fint *newcomm, MPI_Fint *ierr) {
  MPI_Comm_dup_fortran_wrapper(comm, newcomm, ierr);
}

/* ================= End Wrappers for MPI_Comm_dup ================= */

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
 
/* =============== Fortran Wrappers for MPI_Comm_group =============== */
static void MPI_Comm_group_fortran_wrapper(MPI_Fint *comm, MPI_Fint *group, MPI_Fint *ierr) {
  int _wrap_py_return_val = 0;
#if (!defined(MPICH_HAS_C2F) && defined(MPICH_NAME) && (MPICH_NAME == 1)) /* MPICH test */
  _wrap_py_return_val = MPI_Comm_group((MPI_Comm)(*comm), (MPI_Group*)group);
#else /* MPI-2 safe call */
  MPI_Group temp_group;
  _wrap_py_return_val = MPI_Comm_group(MPI_Comm_f2c(*comm), &temp_group);
  *group = MPI_Group_c2f(temp_group);
#endif /* MPICH test */
  *ierr = _wrap_py_return_val;
}

_EXTERN_C_ void MPI_COMM_GROUP(MPI_Fint *comm, MPI_Fint *group, MPI_Fint *ierr) {
  MPI_Comm_group_fortran_wrapper(comm, group, ierr);
}

_EXTERN_C_ void mpi_comm_group(MPI_Fint *comm, MPI_Fint *group, MPI_Fint *ierr) {
  MPI_Comm_group_fortran_wrapper(comm, group, ierr);
}

_EXTERN_C_ void mpi_comm_group_(MPI_Fint *comm, MPI_Fint *group, MPI_Fint *ierr) {
  MPI_Comm_group_fortran_wrapper(comm, group, ierr);
}

_EXTERN_C_ void mpi_comm_group__(MPI_Fint *comm, MPI_Fint *group, MPI_Fint *ierr) {
  MPI_Comm_group_fortran_wrapper(comm, group, ierr);
}

/* ================= End Wrappers for MPI_Comm_group ================= */


 

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

/* =============== Fortran Wrappers for MPI_Comm_set_name =============== */
static void MPI_Comm_set_name_fortran_wrapper(MPI_Fint *comm, MPI_Fint *comm_name, MPI_Fint *ierr) {
  int _wrap_py_return_val = 0;
#if (!defined(MPICH_HAS_C2F) && defined(MPICH_NAME) && (MPICH_NAME == 1)) /* MPICH test */
  _wrap_py_return_val = MPI_Comm_set_name((MPI_Comm)(*comm), (mpi_const char*)BufferC2F((mpi_const char*)comm_name));
#else /* MPI-2 safe call */
  _wrap_py_return_val = MPI_Comm_set_name(MPI_Comm_f2c(*comm), (mpi_const char*)BufferC2F((mpi_const char*)comm_name));
#endif /* MPICH test */
  *ierr = _wrap_py_return_val;
}

_EXTERN_C_ void MPI_COMM_SET_NAME(MPI_Fint *comm, MPI_Fint *comm_name, MPI_Fint *ierr) {
  MPI_Comm_set_name_fortran_wrapper(comm, comm_name, ierr);
}

_EXTERN_C_ void mpi_comm_set_name(MPI_Fint *comm, MPI_Fint *comm_name, MPI_Fint *ierr) {
  MPI_Comm_set_name_fortran_wrapper(comm, comm_name, ierr);
}

_EXTERN_C_ void mpi_comm_set_name_(MPI_Fint *comm, MPI_Fint *comm_name, MPI_Fint *ierr) {
  MPI_Comm_set_name_fortran_wrapper(comm, comm_name, ierr);
}

_EXTERN_C_ void mpi_comm_set_name__(MPI_Fint *comm, MPI_Fint *comm_name, MPI_Fint *ierr) {
  MPI_Comm_set_name_fortran_wrapper(comm, comm_name, ierr);
}

/* ================= End Wrappers for MPI_Comm_set_name ================= */


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


/* =============== Fortran Wrappers for MPI_Finalize =============== */
static void MPI_Finalize_fortran_wrapper(MPI_Fint *ierr) {
  int _wrap_py_return_val = 0;
  _wrap_py_return_val = MPI_Finalize();
  *ierr = _wrap_py_return_val;
}

_EXTERN_C_ void MPI_FINALIZE(MPI_Fint *ierr) {
  MPI_Finalize_fortran_wrapper(ierr);
}

_EXTERN_C_ void mpi_finalize(MPI_Fint *ierr) {
  MPI_Finalize_fortran_wrapper(ierr);
}

_EXTERN_C_ void mpi_finalize_(MPI_Fint *ierr) {
  MPI_Finalize_fortran_wrapper(ierr);
}

_EXTERN_C_ void mpi_finalize__(MPI_Fint *ierr) {
  MPI_Finalize_fortran_wrapper(ierr);
}

/* ================= End Wrappers for MPI_Finalize ================= */



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

/* =============== Fortran Wrappers for MPI_Isend =============== */
static void MPI_Isend_fortran_wrapper(MPI_Fint *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr) {
  int _wrap_py_return_val = 0;
#if (!defined(MPICH_HAS_C2F) && defined(MPICH_NAME) && (MPICH_NAME == 1)) /* MPICH test */
  _wrap_py_return_val = MPI_Isend(BufferC2F((mpi_const void*)buf), *count, (MPI_Datatype)(*datatype), *dest, *tag, (MPI_Comm)(*comm), (MPI_Request*)request);
#else /* MPI-2 safe call */
  MPI_Request temp_request;
  _wrap_py_return_val = MPI_Isend(BufferC2F((mpi_const void*)buf), *count, MPI_Type_f2c(*datatype), *dest, *tag, MPI_Comm_f2c(*comm), &temp_request);
  *request = MPI_Request_c2f(temp_request);
#endif /* MPICH test */
  *ierr = _wrap_py_return_val;
}

_EXTERN_C_ void MPI_ISEND(MPI_Fint *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr) {
  MPI_Isend_fortran_wrapper(buf, count, datatype, dest, tag, comm, request, ierr);
}

_EXTERN_C_ void mpi_isend(MPI_Fint *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr) {
  MPI_Isend_fortran_wrapper(buf, count, datatype, dest, tag, comm, request, ierr);
}

_EXTERN_C_ void mpi_isend_(MPI_Fint *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr) {
  MPI_Isend_fortran_wrapper(buf, count, datatype, dest, tag, comm, request, ierr);
}

_EXTERN_C_ void mpi_isend__(MPI_Fint *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr) {
  MPI_Isend_fortran_wrapper(buf, count, datatype, dest, tag, comm, request, ierr);
}

/* ================= End Wrappers for MPI_Isend ================= */

/* ================== C Wrappers for MPI_Ibsend ================== */
_EXTERN_C_ int PMPI_Ibsend(mpi_const void *arg_0, int arg_1, MPI_Datatype arg_2, int arg_3, int arg_4, MPI_Comm arg_5, MPI_Request *arg_6);
_EXTERN_C_ int MPI_Ibsend(mpi_const void *arg_0, int arg_1, MPI_Datatype arg_2, int arg_3, int arg_4, MPI_Comm arg_5, MPI_Request *arg_6) {
  REMPI_PREPRINT;
  int _wrap_py_return_val = 0;
  {
    //    _wrap_py_return_val = rempi_record_replay->re_isend(arg_0, arg_1, arg_2, arg_3, arg_4, arg_5, arg_6);
  }
  REMPI_POSTPRINT;
  return _wrap_py_return_val;
}

/* =============== Fortran Wrappers for MPI_Ibsend =============== */
static void MPI_Ibsend_fortran_wrapper(MPI_Fint *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr) {
  int _wrap_py_return_val = 0;
#if (!defined(MPICH_HAS_C2F) && defined(MPICH_NAME) && (MPICH_NAME == 1)) /* MPICH test */
  _wrap_py_return_val = MPI_Ibsend(BufferC2F((mpi_const void*)buf), *count, (MPI_Datatype)(*datatype), *dest, *tag, (MPI_Comm)(*comm), (MPI_Request*)request);
#else /* MPI-2 safe call */
  MPI_Request temp_request;
  _wrap_py_return_val = MPI_Ibsend(BufferC2F((mpi_const void*)buf), *count, MPI_Type_f2c(*datatype), *dest, *tag, MPI_Comm_f2c(*comm), &temp_request);
  *request = MPI_Request_c2f(temp_request);
#endif /* MPICH test */
  *ierr = _wrap_py_return_val;
}

_EXTERN_C_ void MPI_IBSEND(MPI_Fint *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr) {
  MPI_Ibsend_fortran_wrapper(buf, count, datatype, dest, tag, comm, request, ierr);
}

_EXTERN_C_ void mpi_ibsend(MPI_Fint *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr) {
  MPI_Ibsend_fortran_wrapper(buf, count, datatype, dest, tag, comm, request, ierr);
}

_EXTERN_C_ void mpi_ibsend_(MPI_Fint *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr) {
  MPI_Ibsend_fortran_wrapper(buf, count, datatype, dest, tag, comm, request, ierr);
}

_EXTERN_C_ void mpi_ibsend__(MPI_Fint *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr) {
  MPI_Ibsend_fortran_wrapper(buf, count, datatype, dest, tag, comm, request, ierr);
}

/* ================= End Wrappers for MPI_Ibsend ================= */





/* ================== C Wrappers for MPI_Irsend ================== */
_EXTERN_C_ int PMPI_Irsend(mpi_const void *arg_0, int arg_1, MPI_Datatype arg_2, int arg_3, int arg_4, MPI_Comm arg_5, MPI_Request *arg_6);
_EXTERN_C_ int MPI_Irsend(mpi_const void *arg_0, int arg_1, MPI_Datatype arg_2, int arg_3, int arg_4, MPI_Comm arg_5, MPI_Request *arg_6) {
  REMPI_PREPRINT;
  int _wrap_py_return_val = 0;
  {
    _wrap_py_return_val = rempi_record_replay->re_isend(arg_0, arg_1, arg_2, arg_3, arg_4, arg_5, arg_6);
  }
  REMPI_POSTPRINT;
  return _wrap_py_return_val;
}

/* =============== Fortran Wrappers for MPI_Irsend =============== */
static void MPI_Irsend_fortran_wrapper(MPI_Fint *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr) {
  int _wrap_py_return_val = 0;
#if (!defined(MPICH_HAS_C2F) && defined(MPICH_NAME) && (MPICH_NAME == 1)) /* MPICH test */
  _wrap_py_return_val = MPI_Irsend(BufferC2F((mpi_const void*)buf), *count, (MPI_Datatype)(*datatype), *dest, *tag, (MPI_Comm)(*comm), (MPI_Request*)request);
#else /* MPI-2 safe call */
  MPI_Request temp_request;
  _wrap_py_return_val = MPI_Irsend(BufferC2F((mpi_const void*)buf), *count, MPI_Type_f2c(*datatype), *dest, *tag, MPI_Comm_f2c(*comm), &temp_request);
  *request = MPI_Request_c2f(temp_request);
#endif /* MPICH test */
  *ierr = _wrap_py_return_val;
}

_EXTERN_C_ void MPI_IRSEND(MPI_Fint *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr) {
  MPI_Irsend_fortran_wrapper(buf, count, datatype, dest, tag, comm, request, ierr);
}

_EXTERN_C_ void mpi_irsend(MPI_Fint *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr) {
  MPI_Irsend_fortran_wrapper(buf, count, datatype, dest, tag, comm, request, ierr);
}

_EXTERN_C_ void mpi_irsend_(MPI_Fint *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr) {
  MPI_Irsend_fortran_wrapper(buf, count, datatype, dest, tag, comm, request, ierr);
}

_EXTERN_C_ void mpi_irsend__(MPI_Fint *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr) {
  MPI_Irsend_fortran_wrapper(buf, count, datatype, dest, tag, comm, request, ierr);
}

/* ================= End Wrappers for MPI_Irsend ================= */




/* ================== C Wrappers for MPI_Issend ================== */
_EXTERN_C_ int PMPI_Issend(mpi_const void *arg_0, int arg_1, MPI_Datatype arg_2, int arg_3, int arg_4, MPI_Comm arg_5, MPI_Request *arg_6);
_EXTERN_C_ int MPI_Issend(mpi_const void *arg_0, int arg_1, MPI_Datatype arg_2, int arg_3, int arg_4, MPI_Comm arg_5, MPI_Request *arg_6) {
  REMPI_PREPRINT;
  int _wrap_py_return_val = 0;
  {
    _wrap_py_return_val = rempi_record_replay->re_isend(arg_0, arg_1, arg_2, arg_3, arg_4, arg_5, arg_6);
  }
  REMPI_POSTPRINT;
  return _wrap_py_return_val;
}


/* =============== Fortran Wrappers for MPI_Issend =============== */
static void MPI_Issend_fortran_wrapper(MPI_Fint *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr) {
  int _wrap_py_return_val = 0;
#if (!defined(MPICH_HAS_C2F) && defined(MPICH_NAME) && (MPICH_NAME == 1)) /* MPICH test */
  _wrap_py_return_val = MPI_Issend(BufferC2F((mpi_const void*)buf), *count, (MPI_Datatype)(*datatype), *dest, *tag, (MPI_Comm)(*comm), (MPI_Request*)request);
#else /* MPI-2 safe call */
  MPI_Request temp_request;
  _wrap_py_return_val = MPI_Issend(BufferC2F((mpi_const void*)buf), *count, MPI_Type_f2c(*datatype), *dest, *tag, MPI_Comm_f2c(*comm), &temp_request);
  *request = MPI_Request_c2f(temp_request);
#endif /* MPICH test */
  *ierr = _wrap_py_return_val;
}

_EXTERN_C_ void MPI_ISSEND(MPI_Fint *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr) {
  MPI_Issend_fortran_wrapper(buf, count, datatype, dest, tag, comm, request, ierr);
}

_EXTERN_C_ void mpi_issend(MPI_Fint *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr) {
  MPI_Issend_fortran_wrapper(buf, count, datatype, dest, tag, comm, request, ierr);
}

_EXTERN_C_ void mpi_issend_(MPI_Fint *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr) {
  MPI_Issend_fortran_wrapper(buf, count, datatype, dest, tag, comm, request, ierr);
}

_EXTERN_C_ void mpi_issend__(MPI_Fint *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr) {
  MPI_Issend_fortran_wrapper(buf, count, datatype, dest, tag, comm, request, ierr);
}

/* ================= End Wrappers for MPI_Issend ================= */



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

/* =============== Fortran Wrappers for MPI_Send =============== */
static void MPI_Send_fortran_wrapper(MPI_Fint *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *ierr) {
  int _wrap_py_return_val = 0;
#if (!defined(MPICH_HAS_C2F) && defined(MPICH_NAME) && (MPICH_NAME == 1)) /* MPICH test */
  _wrap_py_return_val = MPI_Send(BufferC2F((mpi_const void*)buf), *count, (MPI_Datatype)(*datatype), *dest, *tag, (MPI_Comm)(*comm));
#else /* MPI-2 safe call */
  _wrap_py_return_val = MPI_Send(BufferC2F((mpi_const void*)buf), *count, MPI_Type_f2c(*datatype), *dest, *tag, MPI_Comm_f2c(*comm));
#endif /* MPICH test */
  *ierr = _wrap_py_return_val;
}

_EXTERN_C_ void MPI_SEND(MPI_Fint *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *ierr)   {
  MPI_Send_fortran_wrapper(buf, count, datatype, dest, tag, comm, ierr);
}

_EXTERN_C_ void mpi_send(MPI_Fint *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *ierr)   {
  MPI_Send_fortran_wrapper(buf, count, datatype, dest, tag, comm, ierr);
}

_EXTERN_C_ void mpi_send_(MPI_Fint *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *ierr)  {
  MPI_Send_fortran_wrapper(buf, count, datatype, dest, tag, comm, ierr);
}

_EXTERN_C_ void mpi_send__(MPI_Fint *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *ierr) {
  MPI_Send_fortran_wrapper(buf, count, datatype, dest, tag, comm, ierr);
}

/* ================= End Wrappers for MPI_Send ================= */


/* ================== C Wrappers for MPI_Send ================== */
_EXTERN_C_ int PMPI_Bsend(mpi_const void *arg_0, int arg_1, MPI_Datatype arg_2, int arg_3, int arg_4, MPI_Comm arg_5);
_EXTERN_C_ int MPI_Bsend(mpi_const void *arg_0, int arg_1, MPI_Datatype arg_2, int arg_3, int arg_4, MPI_Comm arg_5) {
  REMPI_PREPRINT;
  int _wrap_py_return_val = 0;
  {
    _wrap_py_return_val = PMPI_Send(arg_0, arg_1, arg_2, arg_3, arg_4, arg_5);
  }
  REMPI_POSTPRINT;
  return _wrap_py_return_val;
}

/* =============== Fortran Wrappers for MPI_Bsend =============== */
static void MPI_Bsend_fortran_wrapper(MPI_Fint *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *ierr) {
  int _wrap_py_return_val = 0;
#if (!defined(MPICH_HAS_C2F) && defined(MPICH_NAME) && (MPICH_NAME == 1)) /* MPICH test */
  _wrap_py_return_val = MPI_Bsend(BufferC2F((mpi_const void*)buf), *count, (MPI_Datatype)(*datatype), *dest, *tag, (MPI_Comm)(*comm));
#else /* MPI-2 safe call */
  _wrap_py_return_val = MPI_Bsend(BufferC2F((mpi_const void*)buf), *count, MPI_Type_f2c(*datatype), *dest, *tag, MPI_Comm_f2c(*comm));
#endif /* MPICH test */
  *ierr = _wrap_py_return_val;
}

_EXTERN_C_ void MPI_BSEND(MPI_Fint *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *ierr)
{
  MPI_Bsend_fortran_wrapper(buf, count, datatype, dest, tag, comm, ierr);
}

_EXTERN_C_ void mpi_bsend(MPI_Fint *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *ierr)
{
  MPI_Bsend_fortran_wrapper(buf, count, datatype, dest, tag, comm, ierr);
}

_EXTERN_C_ void mpi_bsend_(MPI_Fint *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *ierr) {
  MPI_Bsend_fortran_wrapper(buf, count, datatype, dest, tag, comm, ierr);
}

_EXTERN_C_ void mpi_bsend__(MPI_Fint *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *ierr) {
  MPI_Bsend_fortran_wrapper(buf, count, datatype, dest, tag, comm, ierr);
}

/* ================= End Wrappers for MPI_Bsend ================= */


/* ================== C Wrappers for MPI_Send ================== */
_EXTERN_C_ int PMPI_Rsend(mpi_const void *arg_0, int arg_1, MPI_Datatype arg_2, int arg_3, int arg_4, MPI_Comm arg_5);
_EXTERN_C_ int MPI_Rsend(mpi_const void *arg_0, int arg_1, MPI_Datatype arg_2, int arg_3, int arg_4, MPI_Comm arg_5) {
  REMPI_PREPRINT;
  int _wrap_py_return_val = 0;
  {
    _wrap_py_return_val = PMPI_Send(arg_0, arg_1, arg_2, arg_3, arg_4, arg_5);
  }
  REMPI_POSTPRINT;
  return _wrap_py_return_val;
}

/* =============== Fortran Wrappers for MPI_Rsend =============== */
static void MPI_Rsend_fortran_wrapper(MPI_Fint *ibuf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *ierr) {
  int _wrap_py_return_val = 0;
#if (!defined(MPICH_HAS_C2F) && defined(MPICH_NAME) && (MPICH_NAME == 1)) /* MPICH test */
  _wrap_py_return_val = MPI_Rsend(BufferC2F((mpi_const void*)ibuf), *count, (MPI_Datatype)(*datatype), *dest, *tag, (MPI_Comm)(*comm));
#else /* MPI-2 safe call */
  _wrap_py_return_val = MPI_Rsend(BufferC2F((mpi_const void*)ibuf), *count, MPI_Type_f2c(*datatype), *dest, *tag, MPI_Comm_f2c(*comm));
#endif /* MPICH test */
  *ierr = _wrap_py_return_val;
}

_EXTERN_C_ void MPI_RSEND(MPI_Fint *ibuf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *ierr) {
  MPI_Rsend_fortran_wrapper(ibuf, count, datatype, dest, tag, comm, ierr);
}

_EXTERN_C_ void mpi_rsend(MPI_Fint *ibuf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *ierr) {
  MPI_Rsend_fortran_wrapper(ibuf, count, datatype, dest, tag, comm, ierr);
}

_EXTERN_C_ void mpi_rsend_(MPI_Fint *ibuf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *ierr) {
  MPI_Rsend_fortran_wrapper(ibuf, count, datatype, dest, tag, comm, ierr);
}

_EXTERN_C_ void mpi_rsend__(MPI_Fint *ibuf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *ierr) {
  MPI_Rsend_fortran_wrapper(ibuf, count, datatype, dest, tag, comm, ierr);
}

/* ================= End Wrappers for MPI_Rsend ================= */


/* ================== C Wrappers for MPI_Send ================== */
_EXTERN_C_ int PMPI_Ssend(mpi_const void *arg_0, int arg_1, MPI_Datatype arg_2, int arg_3, int arg_4, MPI_Comm arg_5);
_EXTERN_C_ int MPI_Ssend(mpi_const void *arg_0, int arg_1, MPI_Datatype arg_2, int arg_3, int arg_4, MPI_Comm arg_5) {
  REMPI_PREPRINT;
  int _wrap_py_return_val = 0;
  {
    _wrap_py_return_val = PMPI_Send(arg_0, arg_1, arg_2, arg_3, arg_4, arg_5);
  }
  REMPI_POSTPRINT;
  return _wrap_py_return_val;
}

/* =============== Fortran Wrappers for MPI_Ssend =============== */
static void MPI_Ssend_fortran_wrapper(MPI_Fint *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *ierr) {
  int _wrap_py_return_val = 0;
#if (!defined(MPICH_HAS_C2F) && defined(MPICH_NAME) && (MPICH_NAME == 1)) /* MPICH test */
  _wrap_py_return_val = MPI_Ssend(BufferC2F((mpi_const void*)buf), *count, (MPI_Datatype)(*datatype), *dest, *tag, (MPI_Comm)(*comm));
#else /* MPI-2 safe call */
  _wrap_py_return_val = MPI_Ssend(BufferC2F((mpi_const void*)buf), *count, MPI_Type_f2c(*datatype), *dest, *tag, MPI_Comm_f2c(*comm));
#endif /* MPICH test */
  *ierr = _wrap_py_return_val;
}

_EXTERN_C_ void MPI_SSEND(MPI_Fint *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *ierr)
{
  MPI_Ssend_fortran_wrapper(buf, count, datatype, dest, tag, comm, ierr);
}

_EXTERN_C_ void mpi_ssend(MPI_Fint *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *ierr)
{
  MPI_Ssend_fortran_wrapper(buf, count, datatype, dest, tag, comm, ierr);
}

_EXTERN_C_ void mpi_ssend_(MPI_Fint *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *ierr) {
  MPI_Ssend_fortran_wrapper(buf, count, datatype, dest, tag, comm, ierr);
}

_EXTERN_C_ void mpi_ssend__(MPI_Fint *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *ierr) {
  MPI_Ssend_fortran_wrapper(buf, count, datatype, dest, tag, comm, ierr);
}

/* ================= End Wrappers for MPI_Ssend ================= */



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
    //    REMPI_DBGI(0, "%s", __func__);
    //    fprintf(stderr, "======= %s end =======\n", __func__);
  }    
  REMPI_POSTPRINT;
  return _wrap_py_return_val;
}

/* =============== Fortran Wrappers for MPI_Allreduce =============== */
static void MPI_Allreduce_fortran_wrapper(MPI_Fint *sendbuf, MPI_Fint *recvbuf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm, MPI_Fint *ierr) {
  int _wrap_py_return_val = 0;
#if (!defined(MPICH_HAS_C2F) && defined(MPICH_NAME) && (MPICH_NAME == 1)) /* MPICH test */
  _wrap_py_return_val = MPI_Allreduce(BufferC2F((mpi_const void*)sendbuf), BufferC2F((void*)recvbuf), *count, (MPI_Datatype)(*datatype), (MPI_Op)(*op), (MPI_Comm)(*comm));
#else /* MPI-2 safe call */
  _wrap_py_return_val = MPI_Allreduce(BufferC2F((mpi_const void*)sendbuf), BufferC2F((void*)recvbuf), *count, MPI_Type_f2c(*datatype), MPI_Op_f2c(*op), MPI_Comm_f2c(*comm));
#endif /* MPICH test */
  *ierr = _wrap_py_return_val;
}

_EXTERN_C_ void MPI_ALLREDUCE(MPI_Fint *sendbuf, MPI_Fint *recvbuf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm, MPI_Fint *ierr) {
  MPI_Allreduce_fortran_wrapper(sendbuf, recvbuf, count, datatype, op, comm, ierr);
}

_EXTERN_C_ void mpi_allreduce(MPI_Fint *sendbuf, MPI_Fint *recvbuf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm, MPI_Fint *ierr) {
  MPI_Allreduce_fortran_wrapper(sendbuf, recvbuf, count, datatype, op, comm, ierr);
}

_EXTERN_C_ void mpi_allreduce_(MPI_Fint *sendbuf, MPI_Fint *recvbuf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm, MPI_Fint *ierr) {
  MPI_Allreduce_fortran_wrapper(sendbuf, recvbuf, count, datatype, op, comm, ierr);
}

_EXTERN_C_ void mpi_allreduce__(MPI_Fint *sendbuf, MPI_Fint *recvbuf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm, MPI_Fint *ierr) {
  MPI_Allreduce_fortran_wrapper(sendbuf, recvbuf, count, datatype, op, comm, ierr);
}

/* ================= End Wrappers for MPI_Allreduce ================= */


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


/* =============== Fortran Wrappers for MPI_Reduce =============== */
static void MPI_Reduce_fortran_wrapper(MPI_Fint *sendbuf, MPI_Fint *recvbuf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *ierr) {
  int _wrap_py_return_val = 0;
#if (!defined(MPICH_HAS_C2F) && defined(MPICH_NAME) && (MPICH_NAME == 1)) /* MPICH test */
  _wrap_py_return_val = MPI_Reduce(BufferC2F((mpi_const void*)sendbuf), BufferC2F((void*)recvbuf), *count, (MPI_Datatype)(*datatype), (MPI_Op) (*op), *root, (MPI_Comm)(*comm));
#else /* MPI-2 safe call */
  _wrap_py_return_val = MPI_Reduce(BufferC2F((mpi_const void*)sendbuf), BufferC2F((void*)recvbuf), *count, MPI_Type_f2c(*datatype), MPI_Op_f2c(*op), *root, MPI_Comm_f2c(*comm));
#endif /* MPICH test */
  *ierr = _wrap_py_return_val;
}

_EXTERN_C_ void MPI_REDUCE(MPI_Fint *sendbuf, MPI_Fint *recvbuf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *ierr) {
  MPI_Reduce_fortran_wrapper(sendbuf, recvbuf, count, datatype, op, root, comm, ierr);
}

_EXTERN_C_ void mpi_reduce(MPI_Fint *sendbuf, MPI_Fint *recvbuf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *ierr) {
  MPI_Reduce_fortran_wrapper(sendbuf, recvbuf, count, datatype, op, root, comm, ierr);
}

_EXTERN_C_ void mpi_reduce_(MPI_Fint *sendbuf, MPI_Fint *recvbuf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *ierr) {
  MPI_Reduce_fortran_wrapper(sendbuf, recvbuf, count, datatype, op, root, comm, ierr);
}

_EXTERN_C_ void mpi_reduce__(MPI_Fint *sendbuf, MPI_Fint *recvbuf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *ierr) {
  MPI_Reduce_fortran_wrapper(sendbuf, recvbuf, count, datatype, op, root, comm, ierr);
}

/* ================= End Wrappers for MPI_Reduce ================= */

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

/* =============== Fortran Wrappers for MPI_Scan =============== */
static void MPI_Scan_fortran_wrapper(MPI_Fint *sendbuf, MPI_Fint *recvbuf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm, MPI_Fint *ierr) {
  int _wrap_py_return_val = 0;
#if (!defined(MPICH_HAS_C2F) && defined(MPICH_NAME) && (MPICH_NAME == 1)) /* MPICH test */
  _wrap_py_return_val = MPI_Scan(BufferC2F((mpi_const void*)sendbuf), BufferC2F((void*)recvbuf), *count, (MPI_Datatype)(*datatype), (MPI_Op)(*op), (MPI_Comm)(*comm));
#else /* MPI-2 safe call */
  _wrap_py_return_val = MPI_Scan(BufferC2F((mpi_const void*)sendbuf), BufferC2F((void*)recvbuf), *count, MPI_Type_f2c(*datatype), MPI_Op_f2c(*op), MPI_Comm_f2c(*comm));
#endif /* MPICH test */
  *ierr = _wrap_py_return_val;
}

_EXTERN_C_ void MPI_SCAN(MPI_Fint *sendbuf, MPI_Fint *recvbuf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm, MPI_Fint *ierr) {
  MPI_Scan_fortran_wrapper(sendbuf, recvbuf, count, datatype, op, comm, ierr);
}

_EXTERN_C_ void mpi_scan(MPI_Fint *sendbuf, MPI_Fint *recvbuf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm, MPI_Fint *ierr) {
  MPI_Scan_fortran_wrapper(sendbuf, recvbuf, count, datatype, op, comm, ierr);
}

_EXTERN_C_ void mpi_scan_(MPI_Fint *sendbuf, MPI_Fint *recvbuf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm, MPI_Fint *ierr) {
  MPI_Scan_fortran_wrapper(sendbuf, recvbuf, count, datatype, op, comm, ierr);
}

_EXTERN_C_ void mpi_scan__(MPI_Fint *sendbuf, MPI_Fint *recvbuf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm, MPI_Fint *ierr) {
  MPI_Scan_fortran_wrapper(sendbuf, recvbuf, count, datatype, op, comm, ierr);
}

/* ================= End Wrappers for MPI_Scan ================= */


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


/* =============== Fortran Wrappers for MPI_Allgather =============== */
static void MPI_Allgather_fortran_wrapper(MPI_Fint *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *ierr) {
  int _wrap_py_return_val = 0;
#if (!defined(MPICH_HAS_C2F) && defined(MPICH_NAME) && (MPICH_NAME == 1)) /* MPICH test */
  _wrap_py_return_val = MPI_Allgather(BufferC2F((mpi_const void*)sendbuf), *sendcount, (MPI_Datatype)(*sendtype), BufferC2F((void*)recvbuf), *recvcount, (MPI_Datatype)(*recvtype), (MPI_Comm)(*comm));
#else /* MPI-2 safe call */
  _wrap_py_return_val = MPI_Allgather(BufferC2F((mpi_const void*)sendbuf), *sendcount, MPI_Type_f2c(*sendtype), BufferC2F((void*)recvbuf), *recvcount, MPI_Type_f2c(*recvtype), MPI_Comm_f2c(*comm));
#endif /* MPICH test */
  *ierr = _wrap_py_return_val;
}

_EXTERN_C_ void MPI_ALLGATHER(MPI_Fint *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *ierr) {
  MPI_Allgather_fortran_wrapper(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm, ierr);
}

_EXTERN_C_ void mpi_allgather(MPI_Fint *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *ierr) {
  MPI_Allgather_fortran_wrapper(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm, ierr);
}

_EXTERN_C_ void mpi_allgather_(MPI_Fint *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *ierr) {
  MPI_Allgather_fortran_wrapper(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm, ierr);
}

_EXTERN_C_ void mpi_allgather__(MPI_Fint *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *ierr) {
  MPI_Allgather_fortran_wrapper(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm, ierr);
}

/* ================= End Wrappers for MPI_Allgather ================= */



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


/* =============== Fortran Wrappers for MPI_Gatherv =============== */
static void MPI_Gatherv_fortran_wrapper(MPI_Fint *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint recvcounts[], MPI_Fint displs[], MPI_Fint *recvtype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *ierr) {
  int _wrap_py_return_val = 0;
#if (!defined(MPICH_HAS_C2F) && defined(MPICH_NAME) && (MPICH_NAME == 1)) /* MPICH test */
  _wrap_py_return_val = MPI_Gatherv(BufferC2F((mpi_const void*)sendbuf), *sendcount, (MPI_Datatype)(*sendtype), BufferC2F((void*)recvbuf), (mpi_const int*)BufferC2F((mpi_const int*)recvcounts), (mpi_const int*)BufferC2F((mpi_const int*)displs), (MPI_Datatype)(*recvtype), *root, (MPI_Comm)(*comm));
#else /* MPI-2 safe call */
  _wrap_py_return_val = MPI_Gatherv(BufferC2F((mpi_const void*)sendbuf), *sendcount, MPI_Type_f2c(*sendtype), BufferC2F((void*)recvbuf), (mpi_const int*)BufferC2F((mpi_const int*)recvcounts), (mpi_const int*)BufferC2F((mpi_const int*)displs), MPI_Type_f2c(*recvtype), *root, MPI_Comm_f2c(*comm));
#endif /* MPICH test */
  *ierr = _wrap_py_return_val;
}

_EXTERN_C_ void MPI_GATHERV(MPI_Fint *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint recvcounts[], MPI_Fint displs[], MPI_Fint *recvtype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *ierr) {
  MPI_Gatherv_fortran_wrapper(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, root, comm, ierr);
}

_EXTERN_C_ void mpi_gatherv(MPI_Fint *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint recvcounts[], MPI_Fint displs[], MPI_Fint *recvtype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *ierr) {
  MPI_Gatherv_fortran_wrapper(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, root, comm, ierr);
}

_EXTERN_C_ void mpi_gatherv_(MPI_Fint *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint recvcounts[], MPI_Fint displs[], MPI_Fint *recvtype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *ierr) {
  MPI_Gatherv_fortran_wrapper(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, root, comm, ierr);
}

_EXTERN_C_ void mpi_gatherv__(MPI_Fint *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint recvcounts[], MPI_Fint displs[], MPI_Fint *recvtype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *ierr) {
  MPI_Gatherv_fortran_wrapper(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, root, comm, ierr);
}

/* ================= End Wrappers for MPI_Gatherv ================= */


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

/* =============== Fortran Wrappers for MPI_Reduce_scatter =============== */
static void MPI_Reduce_scatter_fortran_wrapper(MPI_Fint *sendbuf, MPI_Fint *recvbuf, MPI_Fint recvcounts[], MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm, MPI_Fint *ierr) {
  int _wrap_py_return_val = 0;
#if (!defined(MPICH_HAS_C2F) && defined(MPICH_NAME) && (MPICH_NAME == 1)) /* MPICH test */
  _wrap_py_return_val = MPI_Reduce_scatter(BufferC2F((mpi_const void*)sendbuf), BufferC2F((void*)recvbuf), (mpi_const int*)BufferC2F((mpi_const int*)recvcounts), (MPI_Datatype)(*datatype), (MPI_Op)(*op), (MPI_Comm)(*comm));
#else /* MPI-2 safe call */
  _wrap_py_return_val = MPI_Reduce_scatter(BufferC2F((mpi_const void*)sendbuf), BufferC2F((void*)recvbuf), (mpi_const int*)BufferC2F((mpi_const int*)recvcounts), MPI_Type_f2c(*datatype), MPI_Op_f2c(*op), MPI_Comm_f2c(*comm));
#endif /* MPICH test */
  *ierr = _wrap_py_return_val;
}

_EXTERN_C_ void MPI_REDUCE_SCATTER(MPI_Fint *sendbuf, MPI_Fint *recvbuf, MPI_Fint recvcounts[], MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm, MPI_Fint *ierr) {
  MPI_Reduce_scatter_fortran_wrapper(sendbuf, recvbuf, recvcounts, datatype, op, comm, ierr);
}

_EXTERN_C_ void mpi_reduce_scatter(MPI_Fint *sendbuf, MPI_Fint *recvbuf, MPI_Fint recvcounts[], MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm, MPI_Fint *ierr) {
  MPI_Reduce_scatter_fortran_wrapper(sendbuf, recvbuf, recvcounts, datatype, op, comm, ierr);
}

_EXTERN_C_ void mpi_reduce_scatter_(MPI_Fint *sendbuf, MPI_Fint *recvbuf, MPI_Fint recvcounts[], MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm, MPI_Fint *ierr) {
  MPI_Reduce_scatter_fortran_wrapper(sendbuf, recvbuf, recvcounts, datatype, op, comm, ierr);
}

_EXTERN_C_ void mpi_reduce_scatter__(MPI_Fint *sendbuf, MPI_Fint *recvbuf, MPI_Fint recvcounts[], MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm, MPI_Fint *ierr) {
  MPI_Reduce_scatter_fortran_wrapper(sendbuf, recvbuf, recvcounts, datatype, op, comm, ierr);
}

/* ================= End Wrappers for MPI_Reduce_scatter ================= */




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

/* =============== Fortran Wrappers for MPI_Scatterv =============== */
static void MPI_Scatterv_fortran_wrapper(MPI_Fint *sendbuf, MPI_Fint sendcounts[], MPI_Fint displs[], MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *ierr) {
  int _wrap_py_return_val = 0;
#if (!defined(MPICH_HAS_C2F) && defined(MPICH_NAME) && (MPICH_NAME == 1)) /* MPICH test */
  _wrap_py_return_val = MPI_Scatterv(BufferC2F((mpi_const void*)sendbuf), (mpi_const int*)BufferC2F((mpi_const int*)sendcounts), (mpi_const int*)BufferC2F((mpi_const int*)displs), (MPI_Datatype)(*sendtype), BufferC2F((void*)recvbuf), *recvcount, (MPI_Datatype)(*recvtype), *root, (MPI_Comm)(*comm));
#else /* MPI-2 safe call */
  _wrap_py_return_val = MPI_Scatterv(BufferC2F((mpi_const void*)sendbuf), (mpi_const int*)BufferC2F((mpi_const int*)sendcounts), (mpi_const int*)BufferC2F((mpi_const int*)displs), MPI_Type_f2c(*sendtype), BufferC2F((void*)recvbuf), *recvcount, MPI_Type_f2c(*recvtype), *root, MPI_Comm_f2c(*comm));
#endif /* MPICH test */
  *ierr = _wrap_py_return_val;
}

_EXTERN_C_ void MPI_SCATTERV(MPI_Fint *sendbuf, MPI_Fint sendcounts[], MPI_Fint displs[], MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *ierr) {
  MPI_Scatterv_fortran_wrapper(sendbuf, sendcounts, displs, sendtype, recvbuf, recvcount, recvtype, root, comm, ierr);
}

_EXTERN_C_ void mpi_scatterv(MPI_Fint *sendbuf, MPI_Fint sendcounts[], MPI_Fint displs[], MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *ierr) {
  MPI_Scatterv_fortran_wrapper(sendbuf, sendcounts, displs, sendtype, recvbuf, recvcount, recvtype, root, comm, ierr);
}

_EXTERN_C_ void mpi_scatterv_(MPI_Fint *sendbuf, MPI_Fint sendcounts[], MPI_Fint displs[], MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *ierr) {
  MPI_Scatterv_fortran_wrapper(sendbuf, sendcounts, displs, sendtype, recvbuf, recvcount, recvtype, root, comm, ierr);
}

_EXTERN_C_ void mpi_scatterv__(MPI_Fint *sendbuf, MPI_Fint sendcounts[], MPI_Fint displs[], MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *ierr) {
  MPI_Scatterv_fortran_wrapper(sendbuf, sendcounts, displs, sendtype, recvbuf, recvcount, recvtype, root, comm, ierr);
}

/* ================= End Wrappers for MPI_Scatterv ================= */



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


/* =============== Fortran Wrappers for MPI_Allgatherv =============== */
static void MPI_Allgatherv_fortran_wrapper(MPI_Fint *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint recvcounts[], MPI_Fint displs[], MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *ierr) {
  int _wrap_py_return_val = 0;
#if (!defined(MPICH_HAS_C2F) && defined(MPICH_NAME) && (MPICH_NAME == 1)) /* MPICH test */
  _wrap_py_return_val = MPI_Allgatherv(BufferC2F((mpi_const void*)sendbuf), *sendcount, (MPI_Datatype)(*sendtype), BufferC2F((void*)recvbuf), (mpi_const int*)BufferC2F((mpi_const int*)recvcounts), (mpi_const int*)BufferC2F((mpi_const int*)displs), (MPI_Datatype)(*recvtype), (MPI_Comm)(*comm));
#else /* MPI-2 safe call */
  _wrap_py_return_val = MPI_Allgatherv(BufferC2F((mpi_const void*)sendbuf), *sendcount, MPI_Type_f2c(*sendtype), BufferC2F((void*)recvbuf), (mpi_const int*)BufferC2F((mpi_const int*)recvcounts), (mpi_const int*)BufferC2F((mpi_const int*)displs), MPI_Type_f2c(*recvtype), MPI_Comm_f2c(*comm));
#endif /* MPICH test */
  *ierr = _wrap_py_return_val;
}

_EXTERN_C_ void MPI_ALLGATHERV(MPI_Fint *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint recvcounts[], MPI_Fint displs[], MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *ierr) {
  MPI_Allgatherv_fortran_wrapper(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, comm, ierr);
}

_EXTERN_C_ void mpi_allgatherv(MPI_Fint *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint recvcounts[], MPI_Fint displs[], MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *ierr) {
  MPI_Allgatherv_fortran_wrapper(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, comm, ierr);
}

_EXTERN_C_ void mpi_allgatherv_(MPI_Fint *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint recvcounts[], MPI_Fint displs[], MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *ierr) {
  MPI_Allgatherv_fortran_wrapper(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, comm, ierr);
}

_EXTERN_C_ void mpi_allgatherv__(MPI_Fint *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint recvcounts[], MPI_Fint displs[], MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *ierr) {
  MPI_Allgatherv_fortran_wrapper(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, comm, ierr);
}

/* ================= End Wrappers for MPI_Allgatherv ================= */



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

/* =============== Fortran Wrappers for MPI_Scatter =============== */
static void MPI_Scatter_fortran_wrapper(MPI_Fint *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *ierr) {
  int _wrap_py_return_val = 0;
#if (!defined(MPICH_HAS_C2F) && defined(MPICH_NAME) && (MPICH_NAME == 1)) /* MPICH test */
  _wrap_py_return_val = MPI_Scatter(BufferC2F((mpi_const void*)sendbuf), *sendcount, (MPI_Datatype)(*sendtype), BufferC2F((void*)recvbuf), *recvcount, (MPI_Datatype)(*recvtype), *root, (MPI_Comm)(*comm));
#else /* MPI-2 safe call */
  _wrap_py_return_val = MPI_Scatter(BufferC2F((mpi_const void*)sendbuf), *sendcount, MPI_Type_f2c(*sendtype), BufferC2F((void*)recvbuf), *recvcount, MPI_Type_f2c(*recvtype), *root, MPI_Comm_f2c(*comm));
#endif /* MPICH test */
  *ierr = _wrap_py_return_val;
}

_EXTERN_C_ void MPI_SCATTER(MPI_Fint *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *ierr) {
  MPI_Scatter_fortran_wrapper(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm, ierr);
}

_EXTERN_C_ void mpi_scatter(MPI_Fint *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *ierr) {
  MPI_Scatter_fortran_wrapper(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm, ierr);
}

_EXTERN_C_ void mpi_scatter_(MPI_Fint *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *ierr) {
  MPI_Scatter_fortran_wrapper(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm, ierr);
}

_EXTERN_C_ void mpi_scatter__(MPI_Fint *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *ierr) {
  MPI_Scatter_fortran_wrapper(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm, ierr);
}

/* ================= End Wrappers for MPI_Scatter ================= */





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



/* =============== Fortran Wrappers for MPI_Bcast =============== */
static void MPI_Bcast_fortran_wrapper(MPI_Fint *buffer, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *ierr)
{
  int _wrap_py_return_val = 0;
#if (!defined(MPICH_HAS_C2F) && defined(MPICH_NAME) && (MPICH_NAME == 1)) /* MPICH test */
  _wrap_py_return_val = MPI_Bcast(BufferC2F((void*)buffer), *count, (MPI_Datatype)(*datatype), *root, (MPI_Comm)(*comm));
#else /* MPI-2 safe call */
  _wrap_py_return_val = MPI_Bcast(BufferC2F((void*)buffer), *count, MPI_Type_f2c(*datatype), *root, MPI_Comm_f2c(*comm));
#endif /* MPICH test */
  *ierr = _wrap_py_return_val;
}

_EXTERN_C_ void MPI_BCAST(MPI_Fint *buffer, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *ierr) {
  MPI_Bcast_fortran_wrapper(buffer, count, datatype, root, comm, ierr);
}

_EXTERN_C_ void mpi_bcast(MPI_Fint *buffer, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *ierr) {
  MPI_Bcast_fortran_wrapper(buffer, count, datatype, root, comm, ierr);
}

_EXTERN_C_ void mpi_bcast_(MPI_Fint *buffer, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *ierr) {
  MPI_Bcast_fortran_wrapper(buffer, count, datatype, root, comm, ierr);
}

_EXTERN_C_ void mpi_bcast__(MPI_Fint *buffer, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *ierr) {
  MPI_Bcast_fortran_wrapper(buffer, count, datatype, root, comm, ierr);
}

/* ================= End Wrappers for MPI_Bcast ================= */



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


/* =============== Fortran Wrappers for MPI_Alltoall =============== */
static void MPI_Alltoall_fortran_wrapper(MPI_Fint *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *ierr) {
  int _wrap_py_return_val = 0;
#if (!defined(MPICH_HAS_C2F) && defined(MPICH_NAME) && (MPICH_NAME == 1)) /* MPICH test */
  _wrap_py_return_val = MPI_Alltoall(BufferC2F((mpi_const void*)sendbuf), *sendcount, (MPI_Datatype)(*sendtype), BufferC2F((void*)recvbuf), *recvcount, (MPI_Datatype)(*recvtype), (MPI_Comm)(*comm));
#else /* MPI-2 safe call */
  _wrap_py_return_val = MPI_Alltoall(BufferC2F((mpi_const void*)sendbuf), *sendcount, MPI_Type_f2c(*sendtype), BufferC2F((void*)recvbuf), *recvcount, MPI_Type_f2c(*recvtype), MPI_Comm_f2c(*comm));
#endif /* MPICH test */
  *ierr = _wrap_py_return_val;
}

_EXTERN_C_ void MPI_ALLTOALL(MPI_Fint *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *ierr) {
  MPI_Alltoall_fortran_wrapper(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm, ierr);
}

_EXTERN_C_ void mpi_alltoall(MPI_Fint *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *ierr) {
  MPI_Alltoall_fortran_wrapper(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm, ierr);
}

_EXTERN_C_ void mpi_alltoall_(MPI_Fint *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *ierr) {
  MPI_Alltoall_fortran_wrapper(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm, ierr);
}

_EXTERN_C_ void mpi_alltoall__(MPI_Fint *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *ierr) {
  MPI_Alltoall_fortran_wrapper(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm, ierr);
}

/* ================= End Wrappers for MPI_Alltoall ================= */

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

/* =============== Fortran Wrappers for MPI_Gather =============== */
static void MPI_Gather_fortran_wrapper(MPI_Fint *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *ierr) {
  int _wrap_py_return_val = 0;
#if (!defined(MPICH_HAS_C2F) && defined(MPICH_NAME) && (MPICH_NAME == 1)) /* MPICH test */
  _wrap_py_return_val = MPI_Gather(BufferC2F((mpi_const void*)sendbuf), *sendcount, (MPI_Datatype)(*sendtype), BufferC2F((void*)recvbuf), *recvcount, (MPI_Datatype)(*recvtype), *root, (MPI_Comm)(*comm));
#else /* MPI-2 safe call */
  _wrap_py_return_val = MPI_Gather(BufferC2F((mpi_const void*)sendbuf), *sendcount, MPI_Type_f2c(*sendtype), BufferC2F((void*)recvbuf), *recvcount, MPI_Type_f2c(*recvtype), *root, MPI_Comm_f2c(*comm));
#endif /* MPICH test */
  *ierr = _wrap_py_return_val;
}

_EXTERN_C_ void MPI_GATHER(MPI_Fint *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *ierr) {
  MPI_Gather_fortran_wrapper(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm, ierr);
}

_EXTERN_C_ void mpi_gather(MPI_Fint *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *ierr) {
  MPI_Gather_fortran_wrapper(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm, ierr);
}

_EXTERN_C_ void mpi_gather_(MPI_Fint *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *ierr) {
  MPI_Gather_fortran_wrapper(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm, ierr);
}

_EXTERN_C_ void mpi_gather__(MPI_Fint *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *ierr) {
  MPI_Gather_fortran_wrapper(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm, ierr);
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


/* =============== Fortran Wrappers for MPI_Get_count =============== */
static void MPI_Get_count_fortran_wrapper(MPI_Fint *status, MPI_Fint *datatype, MPI_Fint *count, MPI_Fint *ierr) {
  int _wrap_py_return_val = 0;
#if (!defined(MPICH_HAS_C2F) && defined(MPICH_NAME) && (MPICH_NAME == 1)) /* MPICH test */
  _wrap_py_return_val = MPI_Get_count((mpi_const MPI_Status*)BufferC2F((mpi_const MPI_Status*)status), (MPI_Datatype)(*datatype), (int*)BufferC2F((int*)count));
#else /* MPI-2 safe call */
  _wrap_py_return_val = MPI_Get_count((mpi_const MPI_Status*)BufferC2F((mpi_const MPI_Status*)status), MPI_Type_f2c(*datatype), (int*)BufferC2F((int*)count));
#endif /* MPICH test */
  *ierr = _wrap_py_return_val;
}

_EXTERN_C_ void MPI_GET_COUNT(MPI_Fint *status, MPI_Fint *datatype, MPI_Fint *count, MPI_Fint *ierr) {
  MPI_Get_count_fortran_wrapper(status, datatype, count, ierr);
}

_EXTERN_C_ void mpi_get_count(MPI_Fint *status, MPI_Fint *datatype, MPI_Fint *count, MPI_Fint *ierr) {
  MPI_Get_count_fortran_wrapper(status, datatype, count, ierr);
}

_EXTERN_C_ void mpi_get_count_(MPI_Fint *status, MPI_Fint *datatype, MPI_Fint *count, MPI_Fint *ierr) {
  MPI_Get_count_fortran_wrapper(status, datatype, count, ierr);
}

_EXTERN_C_ void mpi_get_count__(MPI_Fint *status, MPI_Fint *datatype, MPI_Fint *count, MPI_Fint *ierr) {
  MPI_Get_count_fortran_wrapper(status, datatype, count, ierr);
}

/* ================= End Wrappers for MPI_Get_count ================= */



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


/* =============== Fortran Wrappers for MPI_Barrier =============== */
static void MPI_Barrier_fortran_wrapper(MPI_Fint *comm, MPI_Fint *ierr) {
  int _wrap_py_return_val = 0;
#if (!defined(MPICH_HAS_C2F) && defined(MPICH_NAME) && (MPICH_NAME == 1)) /* MPICH test */
  _wrap_py_return_val = MPI_Barrier((MPI_Comm)(*comm));
#else /* MPI-2 safe call */
  _wrap_py_return_val = MPI_Barrier(MPI_Comm_f2c(*comm));
#endif /* MPICH test */
  *ierr = _wrap_py_return_val;
}

_EXTERN_C_ void MPI_BARRIER(MPI_Fint *comm, MPI_Fint *ierr) {
  MPI_Barrier_fortran_wrapper(comm, ierr);
}

_EXTERN_C_ void mpi_barrier(MPI_Fint *comm, MPI_Fint *ierr) {
  MPI_Barrier_fortran_wrapper(comm, ierr);
}

_EXTERN_C_ void mpi_barrier_(MPI_Fint *comm, MPI_Fint *ierr) {
  MPI_Barrier_fortran_wrapper(comm, ierr);
}

_EXTERN_C_ void mpi_barrier__(MPI_Fint *comm, MPI_Fint *ierr) {
  MPI_Barrier_fortran_wrapper(comm, ierr);
}

/* ================= End Wrappers for MPI_Barrier ================= */



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


/* =============== Fortran Wrappers for MPI_Recv_init =============== */
static void MPI_Recv_init_fortran_wrapper(MPI_Fint *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *source, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr) {
  int _wrap_py_return_val = 0;
#if (!defined(MPICH_HAS_C2F) && defined(MPICH_NAME) && (MPICH_NAME == 1)) /* MPICH test */
  _wrap_py_return_val = MPI_Recv_init(BufferC2F((void*)buf), *count, (MPI_Datatype)(*datatype), *source, *tag, (MPI_Comm)(*comm), (MPI_Request*)request);
#else /* MPI-2 safe call */
  MPI_Request temp_request;
  _wrap_py_return_val = MPI_Recv_init(BufferC2F((void*)buf), *count, MPI_Type_f2c(*datatype), *source, *tag, MPI_Comm_f2c(*comm), &temp_request);
  *request = MPI_Request_c2f(temp_request);
#endif /* MPICH test */
  *ierr = _wrap_py_return_val;
}

_EXTERN_C_ void MPI_RECV_INIT(MPI_Fint *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *source, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr) {
  MPI_Recv_init_fortran_wrapper(buf, count, datatype, source, tag, comm, request, ierr);
}

_EXTERN_C_ void mpi_recv_init(MPI_Fint *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *source, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr) {
  MPI_Recv_init_fortran_wrapper(buf, count, datatype, source, tag, comm, request, ierr);
}

_EXTERN_C_ void mpi_recv_init_(MPI_Fint *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *source, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr) {
  MPI_Recv_init_fortran_wrapper(buf, count, datatype, source, tag, comm, request, ierr);
}

_EXTERN_C_ void mpi_recv_init__(MPI_Fint *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *source, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr) {
  MPI_Recv_init_fortran_wrapper(buf, count, datatype, source, tag, comm, request, ierr);
}

/* ================= End Wrappers for MPI_Recv_init ================= */


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

/* =============== Fortran Wrappers for MPI_Send_init =============== */
static void MPI_Send_init_fortran_wrapper(MPI_Fint *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr) {
  int _wrap_py_return_val = 0;
#if (!defined(MPICH_HAS_C2F) && defined(MPICH_NAME) && (MPICH_NAME == 1)) /* MPICH test */
  _wrap_py_return_val = MPI_Send_init(BufferC2F((mpi_const void*)buf), *count, (MPI_Datatype)(*datatype), *dest, *tag, (MPI_Comm)(*comm), (MPI_Request*)request);
#else /* MPI-2 safe call */
  MPI_Request temp_request;
  _wrap_py_return_val = MPI_Send_init(BufferC2F((mpi_const void*)buf), *count, MPI_Type_f2c(*datatype), *dest, *tag, MPI_Comm_f2c(*comm), &temp_request);
  *request = MPI_Request_c2f(temp_request);
#endif /* MPICH test */
  *ierr = _wrap_py_return_val;
}

_EXTERN_C_ void MPI_SEND_INIT(MPI_Fint *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr) {
  MPI_Send_init_fortran_wrapper(buf, count, datatype, dest, tag, comm, request, ierr);
}

_EXTERN_C_ void mpi_send_init(MPI_Fint *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr) {
  MPI_Send_init_fortran_wrapper(buf, count, datatype, dest, tag, comm, request, ierr);
}

_EXTERN_C_ void mpi_send_init_(MPI_Fint *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr) {
  MPI_Send_init_fortran_wrapper(buf, count, datatype, dest, tag, comm, request, ierr);
}

_EXTERN_C_ void mpi_send_init__(MPI_Fint *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr) {
  MPI_Send_init_fortran_wrapper(buf, count, datatype, dest, tag, comm, request, ierr);
}

/* ================= End Wrappers for MPI_Send_init ================= */

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


/* =============== Fortran Wrappers for MPI_Start =============== */
static void MPI_Start_fortran_wrapper(MPI_Fint *request, MPI_Fint *ierr) {
  int _wrap_py_return_val = 0;
#if (!defined(MPICH_HAS_C2F) && defined(MPICH_NAME) && (MPICH_NAME == 1)) /* MPICH test */
  _wrap_py_return_val = MPI_Start((MPI_Request*)request);
#else /* MPI-2 safe call */
  MPI_Request temp_request;
  temp_request = MPI_Request_f2c(*request);
  _wrap_py_return_val = MPI_Start(&temp_request);
  *request = MPI_Request_c2f(temp_request);
#endif /* MPICH test */
  *ierr = _wrap_py_return_val;
}

_EXTERN_C_ void MPI_START(MPI_Fint *request, MPI_Fint *ierr) {
  MPI_Start_fortran_wrapper(request, ierr);
}

_EXTERN_C_ void mpi_start(MPI_Fint *request, MPI_Fint *ierr) {
  MPI_Start_fortran_wrapper(request, ierr);
}

_EXTERN_C_ void mpi_start_(MPI_Fint *request, MPI_Fint *ierr) {
  MPI_Start_fortran_wrapper(request, ierr);
}

_EXTERN_C_ void mpi_start__(MPI_Fint *request, MPI_Fint *ierr) {
  MPI_Start_fortran_wrapper(request, ierr);
}

/* ================= End Wrappers for MPI_Start ================= */

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

/* =============== Fortran Wrappers for MPI_Startall =============== */
static void MPI_Startall_fortran_wrapper(MPI_Fint *count, MPI_Fint array_of_requests[], MPI_Fint *ierr) {
  int _wrap_py_return_val = 0;
#if (!defined(MPICH_HAS_C2F) && defined(MPICH_NAME) && (MPICH_NAME == 1)) /* MPICH test */
  _wrap_py_return_val = MPI_Startall(*count, (MPI_Request*)array_of_requests);
#else /* MPI-2 safe call */
  MPI_Request* temp_array_of_requests;
  int i;
  temp_array_of_requests = (MPI_Request*)malloc(sizeof(MPI_Request) * *count);
  for (i=0; i < *count; i++)
    temp_array_of_requests[i] = MPI_Request_f2c(array_of_requests[i]);
  _wrap_py_return_val = MPI_Startall(*count, temp_array_of_requests);
  if(temp_array_of_requests) free(temp_array_of_requests);
#endif /* MPICH test */
  *ierr = _wrap_py_return_val;
}

_EXTERN_C_ void MPI_STARTALL(MPI_Fint *count, MPI_Fint array_of_requests[], MPI_Fint *ierr) {
  MPI_Startall_fortran_wrapper(count, array_of_requests, ierr);
}

_EXTERN_C_ void mpi_startall(MPI_Fint *count, MPI_Fint array_of_requests[], MPI_Fint *ierr) {
  MPI_Startall_fortran_wrapper(count, array_of_requests, ierr);
}

_EXTERN_C_ void mpi_startall_(MPI_Fint *count, MPI_Fint array_of_requests[], MPI_Fint *ierr) {
  MPI_Startall_fortran_wrapper(count, array_of_requests, ierr);
}

_EXTERN_C_ void mpi_startall__(MPI_Fint *count, MPI_Fint array_of_requests[], MPI_Fint *ierr) {
  MPI_Startall_fortran_wrapper(count, array_of_requests, ierr);
}

/* ================= End Wrappers for MPI_Startall ================= */





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


/* =============== Fortran Wrappers for MPI_Abort =============== */
static void MPI_Abort_fortran_wrapper(MPI_Fint *comm, MPI_Fint *errorcode, MPI_Fint *ierr) {
  int _wrap_py_return_val = 0;
#if (!defined(MPICH_HAS_C2F) && defined(MPICH_NAME) && (MPICH_NAME == 1)) /* MPICH test */
  _wrap_py_return_val = MPI_Abort((MPI_Comm)(*comm), *errorcode);
#else /* MPI-2 safe call */
  _wrap_py_return_val = MPI_Abort(MPI_Comm_f2c(*comm), *errorcode);
#endif /* MPICH test */
  *ierr = _wrap_py_return_val;
}

_EXTERN_C_ void MPI_ABORT(MPI_Fint *comm, MPI_Fint *errorcode, MPI_Fint *ierr) {
  MPI_Abort_fortran_wrapper(comm, errorcode, ierr);
}

_EXTERN_C_ void mpi_abort(MPI_Fint *comm, MPI_Fint *errorcode, MPI_Fint *ierr) {
  MPI_Abort_fortran_wrapper(comm, errorcode, ierr);
}

_EXTERN_C_ void mpi_abort_(MPI_Fint *comm, MPI_Fint *errorcode, MPI_Fint *ierr) {
  MPI_Abort_fortran_wrapper(comm, errorcode, ierr);
}

_EXTERN_C_ void mpi_abort__(MPI_Fint *comm, MPI_Fint *errorcode, MPI_Fint *ierr) {
  MPI_Abort_fortran_wrapper(comm, errorcode, ierr);
}

/* ================= End Wrappers for MPI_Abort ================= */




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

/* =============== Fortran Wrappers for MPI_Attr_put =============== */
static void MPI_Attr_put_fortran_wrapper(MPI_Fint *comm, MPI_Fint *keyval, MPI_Fint *attribute_val, MPI_Fint *ierr) {
  int _wrap_py_return_val = 0;
#if (!defined(MPICH_HAS_C2F) && defined(MPICH_NAME) && (MPICH_NAME == 1)) /* MPICH test */
  _wrap_py_return_val = MPI_Attr_put((MPI_Comm)(*comm), *keyval, BufferC2F((void*)attribute_val));
#else /* MPI-2 safe call */
  _wrap_py_return_val = MPI_Attr_put(MPI_Comm_f2c(*comm), *keyval, BufferC2F((void*)attribute_val));
#endif /* MPICH test */
  *ierr = _wrap_py_return_val;
}

_EXTERN_C_ void MPI_ATTR_PUT(MPI_Fint *comm, MPI_Fint *keyval, MPI_Fint *attribute_val, MPI_Fint *ierr) {
  MPI_Attr_put_fortran_wrapper(comm, keyval, attribute_val, ierr);
}

_EXTERN_C_ void mpi_attr_put(MPI_Fint *comm, MPI_Fint *keyval, MPI_Fint *attribute_val, MPI_Fint *ierr) {
  MPI_Attr_put_fortran_wrapper(comm, keyval, attribute_val, ierr);
}

_EXTERN_C_ void mpi_attr_put_(MPI_Fint *comm, MPI_Fint *keyval, MPI_Fint *attribute_val, MPI_Fint *ierr) {
  MPI_Attr_put_fortran_wrapper(comm, keyval, attribute_val, ierr);
}

_EXTERN_C_ void mpi_attr_put__(MPI_Fint *comm, MPI_Fint *keyval, MPI_Fint *attribute_val, MPI_Fint *ierr) {
  MPI_Attr_put_fortran_wrapper(comm, keyval, attribute_val, ierr);
}

/* ================= End Wrappers for MPI_Attr_put ================= */



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

/* =============== Fortran Wrappers for MPI_Group_incl =============== */
static void MPI_Group_incl_fortran_wrapper(MPI_Fint *group, MPI_Fint *n, MPI_Fint ranks[], MPI_Fint *newgroup, MPI_Fint *ierr) {
  int _wrap_py_return_val = 0;
#if (!defined(MPICH_HAS_C2F) && defined(MPICH_NAME) && (MPICH_NAME == 1)) /* MPICH test */
  _wrap_py_return_val = MPI_Group_incl((MPI_Group)(*group), *n, (mpi_const int*)BufferC2F((mpi_const int*)ranks), (MPI_Group*)newgroup);
#else /* MPI-2 safe call */
  MPI_Group temp_newgroup;
  _wrap_py_return_val = MPI_Group_incl(MPI_Group_f2c(*group), *n, (mpi_const int*)BufferC2F((mpi_const int*)ranks), &temp_newgroup);
  *newgroup = MPI_Group_c2f(temp_newgroup);
#endif /* MPICH test */
  *ierr = _wrap_py_return_val;
}

_EXTERN_C_ void MPI_GROUP_INCL(MPI_Fint *group, MPI_Fint *n, MPI_Fint ranks[], MPI_Fint *newgroup, MPI_Fint *ierr) {
  MPI_Group_incl_fortran_wrapper(group, n, ranks, newgroup, ierr);
}

_EXTERN_C_ void mpi_group_incl(MPI_Fint *group, MPI_Fint *n, MPI_Fint ranks[], MPI_Fint *newgroup, MPI_Fint *ierr) {
  MPI_Group_incl_fortran_wrapper(group, n, ranks, newgroup, ierr);
}

_EXTERN_C_ void mpi_group_incl_(MPI_Fint *group, MPI_Fint *n, MPI_Fint ranks[], MPI_Fint *newgroup, MPI_Fint *ierr) {
  MPI_Group_incl_fortran_wrapper(group, n, ranks, newgroup, ierr);
}

_EXTERN_C_ void mpi_group_incl__(MPI_Fint *group, MPI_Fint *n, MPI_Fint ranks[], MPI_Fint *newgroup, MPI_Fint *ierr) {
  MPI_Group_incl_fortran_wrapper(group, n, ranks, newgroup, ierr);
}

/* ================= End Wrappers for MPI_Group_incl ================= */

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


/* =============== Fortran Wrappers for MPI_Group_free =============== */
static void MPI_Group_free_fortran_wrapper(MPI_Fint *group, MPI_Fint *ierr) {
  int _wrap_py_return_val = 0;
#if (!defined(MPICH_HAS_C2F) && defined(MPICH_NAME) && (MPICH_NAME == 1)) /* MPICH test */
  _wrap_py_return_val = MPI_Group_free((MPI_Group*)group);
#else /* MPI-2 safe call */
  MPI_Group temp_group;
  temp_group = MPI_Group_f2c(*group);
  _wrap_py_return_val = MPI_Group_free(&temp_group);
  *group = MPI_Group_c2f(temp_group);
#endif /* MPICH test */
  *ierr = _wrap_py_return_val;
}

_EXTERN_C_ void MPI_GROUP_FREE(MPI_Fint *group, MPI_Fint *ierr) {
  MPI_Group_free_fortran_wrapper(group, ierr);
}

_EXTERN_C_ void mpi_group_free(MPI_Fint *group, MPI_Fint *ierr) {
  MPI_Group_free_fortran_wrapper(group, ierr);
}

_EXTERN_C_ void mpi_group_free_(MPI_Fint *group, MPI_Fint *ierr) {
  MPI_Group_free_fortran_wrapper(group, ierr);
}

_EXTERN_C_ void mpi_group_free__(MPI_Fint *group, MPI_Fint *ierr) {
  MPI_Group_free_fortran_wrapper(group, ierr);
}

/* ================= End Wrappers for MPI_Group_free ================= */


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

/* =============== Fortran Wrappers for MPI_Errhandler_set =============== */
static void MPI_Errhandler_set_fortran_wrapper(MPI_Fint *comm, MPI_Fint *errhandler, MPI_Fint *ierr) {
  int _wrap_py_return_val = 0;
#if (!defined(MPICH_HAS_C2F) && defined(MPICH_NAME) && (MPICH_NAME == 1)) /* MPICH test */
  _wrap_py_return_val = MPI_Errhandler_set((MPI_Comm)(*comm), (MPI_Errhandler)(*errhandler));
#else /* MPI-2 safe call */
  _wrap_py_return_val = MPI_Errhandler_set(MPI_Comm_f2c(*comm), MPI_Errhandler_f2c(*errhandler));
#endif /* MPICH test */
  *ierr = _wrap_py_return_val;
}

_EXTERN_C_ void MPI_ERRHANDLER_SET(MPI_Fint *comm, MPI_Fint *errhandler, MPI_Fint *ierr) {
  MPI_Errhandler_set_fortran_wrapper(comm, errhandler, ierr);
}

_EXTERN_C_ void mpi_errhandler_set(MPI_Fint *comm, MPI_Fint *errhandler, MPI_Fint *ierr) {
  MPI_Errhandler_set_fortran_wrapper(comm, errhandler, ierr);
}

_EXTERN_C_ void mpi_errhandler_set_(MPI_Fint *comm, MPI_Fint *errhandler, MPI_Fint *ierr) {
  MPI_Errhandler_set_fortran_wrapper(comm, errhandler, ierr);
}

_EXTERN_C_ void mpi_errhandler_set__(MPI_Fint *comm, MPI_Fint *errhandler, MPI_Fint *ierr) {
  MPI_Errhandler_set_fortran_wrapper(comm, errhandler, ierr);
}

/* ================= End Wrappers for MPI_Errhandler_set ================= */



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
