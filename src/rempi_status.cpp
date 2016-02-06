#include <mpi.h>
#include <rempi_status.h>
#include <rempi_mem.h>

using namespace std;

MPI_Status *rempi_status_allocate(int incount, MPI_Status *statuses, int *flag)
{
  MPI_Status *new_statuses;
  if (statuses == NULL || statuses == MPI_STATUS_IGNORE) {
    new_statuses = (MPI_Status*)rempi_malloc(incount * sizeof(MPI_Status));
    *flag  = 1;
  } else {
    new_statuses = statuses;
    *flag = 0;
  }
  return new_statuses;
}

void rempi_status_free(MPI_Status *statuses)
{
  rempi_free(statuses);
  return;
}
