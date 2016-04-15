#ifndef __REMPI_TYPE_H__
#define __REMPI_TYPE_H__

#if MPI_VERSION == 1 || MPI_VERSION == 2
#define mpi_const
#else
#define mpi_const const
#endif

//#define rempi_mpi_version_void void
//#define rempi_mpi_version_int  int

#endif
