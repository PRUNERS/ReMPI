#ifndef __REMPI_SEND_MF_H__
#define __REMPI_SEND_MF_H__

#define MPI_REQUEST_SEND_NULL (MPI_Request)(1)
#define MPI_INDICES_IGNORE (int*)(0x1)

#include <mpi.h>

int REMPI_Send_Wait(MPI_Request *arg_0, MPI_Status *arg_1);
int REMPI_Send_Waitany(int arg_0, MPI_Request *arg_1, int *arg_2, MPI_Status *arg_3);
int REMPI_Send_Waitall(int arg_0, MPI_Request *arg_1, MPI_Status *arg_2);
int REMPI_Send_Waitsome(int arg_0, MPI_Request *arg_1, int *arg_2, int *arg_3, MPI_Status *arg_4);
int REMPI_Send_Test(MPI_Request *arg_0, int *arg_1, MPI_Status *arg_2);
int REMPI_Send_Testany(int arg_0, MPI_Request *arg_1, int *arg_2, int *arg_3, MPI_Status *arg_4);
int REMPI_Send_Testall(int arg_0, MPI_Request *arg_1, int *arg_2, MPI_Status *arg_3);
int REMPI_Send_Testsome(int incount, MPI_Request *array_of_requests, int *outcount, int *array_of_indices, MPI_Status *array_of_statuses);


#endif
