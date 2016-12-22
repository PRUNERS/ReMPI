#ifndef __REMPI_STM_H__
#define __REMPI_STM_H__

#include <stdlib.h>
#include <string.h>

#include <vector>

#include <mpi.h>

#include "rempi_err.h"
#include "rempi_mem.h"
#include "rempi_config.h"
#include "rempi_util.h"
#include "rempi_type.h"

using namespace std;

void rempi_stm_send(int rank, int tag, MPI_Comm comm);
void rempi_stm_mf(int rank, int tag, MPI_Comm comm);

#endif
