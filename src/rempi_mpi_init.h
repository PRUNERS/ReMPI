#ifndef __REMPI_MPI_INIT__
#define __REMPI_MPI_INIT__

int rempi_mpi_init(int *argc, char ***argv, int fortran_init);
int rempi_mpi_init_thread(int *argc, char ***argv, int required, int *provided, int fortran_init_thread);
int rempi_mpi_comm_get_id(MPI_Comm comm);
size_t rempi_mpi_get_msg_id(MPI_Comm comm, int tag);

#endif
