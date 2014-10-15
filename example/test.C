#include <stdio.h>
#include <mpi.h>
#include <sys/time.h>

int main(int argc, char *argv[])
{

int rank, size;
MPI_Status status;

/* Init */
MPI_Init(&argc, &argv);
MPI_Comm_rank(MPI_COMM_WORLD, &rank);
MPI_Comm_size(MPI_COMM_WORLD, &size);

if (rank != 0) { // Slaves
    int buf;

    if (rank == 1) {
        buf = 1;
        MPI_Send(&buf, 1, MPI_INT, 0, 0, MPI_COMM_WORLD); 
    }
    if (rank == 2) {
        buf = 2;
        MPI_Send(&buf, 1, MPI_INT, 0, 0, MPI_COMM_WORLD); 
    }

}
else { // Master
    int sum = 0;
    int flag = -1, res;
    MPI_Request request;
    MPI_Status status;
    while (1) { 
    if(flag != 0)
    {
        MPI_Irecv(&res, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &request);
        flag = 0;
    }
        MPI_Test(&request, &flag, &status);

        if (flag != 0) { 
            printf("recv : %d, slave : %d\n", res, status.MPI_SOURCE);
            if (status.MPI_SOURCE != -1) 
                sum += res;
        flag = -1;
        }


        if (sum == 3)
            break;
    }

    printf("sum : %d\n", sum);
}

MPI_Finalize();
return 0;

}
