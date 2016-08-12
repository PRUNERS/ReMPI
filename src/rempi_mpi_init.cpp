#include <mpi.h>
#include "rempi_err.h"

#ifndef _EXTERN_C_
#ifdef __cplusplus
#define _EXTERN_C_ extern "C"
#else /* __cplusplus */
#define _EXTERN_C_
#endif /* __cplusplus */
#endif /* _EXTERN_C_ */

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

static int is_init_called = 0;

/* ================== C Wrappers for MPI_Init ================== */
int rempi_mpi_init(int *argc, char ***argv, int fortran_init)
{
  int _wrap_py_return_val = 0;

  {
    if (fortran_init && !is_init_called) {
      is_init_called = 1;
#if (defined(PIC) || defined(__PIC__)) && !defined(STATIC)
      if (!PMPI_INIT && !pmpi_init && !pmpi_init_ && !pmpi_init__) {
        REMPI_ERR("Couldn't find fortran pmpi_init function.  Link against static library instead.\n");
        exit(1);
      }        
      switch (fortran_init) {
      case 1: PMPI_INIT(&_wrap_py_return_val);   break;
      case 2: pmpi_init(&_wrap_py_return_val);   break;
      case 3: pmpi_init_(&_wrap_py_return_val);  break;
      case 4: pmpi_init__(&_wrap_py_return_val); break;
      default:
	REMPI_ERR("NO SUITABLE FORTRAN MPI_INIT BINDING\n");
        break;
      }
#else /* !PIC */
      pmpi_init_(&_wrap_py_return_val);
#endif /* !PIC */
    } else {
      _wrap_py_return_val = PMPI_Init(argc, argv);
    }
  }
  return _wrap_py_return_val;
}


/* ================== C Wrappers for MPI_Init_thread ================== */
int rempi_mpi_init_thread(int *argc, char ***argv, int required, int *provided, int fortran_init_thread)
{ 
  int _wrap_py_return_val = 0;

  {
    if (fortran_init_thread && !is_init_called) {
      is_init_called = 1;
#if (defined(PIC) || defined(__PIC__)) && !defined(STATIC)
      if (!PMPI_INIT_THREAD && !pmpi_init_thread && !pmpi_init_thread_ && !pmpi_init_thread__) {
	REMPI_ERR("Couldn't find fortran pmpi_init_thread function.  Link against static library instead.\n");
	exit(1);
      }
      switch (fortran_init_thread) {
      case 1: PMPI_INIT_THREAD(&required, provided, &_wrap_py_return_val);   break;
      case 2: pmpi_init_thread(&required, provided, &_wrap_py_return_val);   break;
      case 3: pmpi_init_thread_(&required, provided, &_wrap_py_return_val);  break;
      case 4: pmpi_init_thread__(&required, provided, &_wrap_py_return_val); break;
      default:
	REMPI_ERR("NO SUITABLE FORTRAN MPI_INIT_THREAD BINDING\n");
	break;
      }
#else /* !PIC */
      pmpi_init_thread_(&required, provided, &_wrap_py_return_val);
#endif /* !PIC */
    } else {
      _wrap_py_return_val = PMPI_Init_thread(argc, argv, required, provided);
    }

  }
  return _wrap_py_return_val;
}
