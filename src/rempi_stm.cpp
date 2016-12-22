#ifndef __REMPI_STM_H__
#define __REMPI_STM_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <vector>
#include <string>

#include <mpi.h>

#include "rempi_err.h"
#include "rempi_mem.h"
#include "rempi_config.h"
#include "rempi_util.h"
#include "rempi_type.h"

using namespace std;

//#define ENABLE_STM
#define LEN (1024 * 128)

int fd = -1;
char output[LEN];
char path[1024];

void rempi_stm_send(int rank, int tag, MPI_Comm comm)
{
#ifdef ENABLE_STM
  if (fd == -1) {
    sprintf(path, "/p/lscratche/sato5/rempi/stm/rank_%d.stm", rempi_my_rank);
    fd = open(path, O_CREAT|O_WRONLY, S_IRWXU);
  }
  string btrace = rempi_btrace_string();
  sprintf(output, "STM:Send:%d:%d:%s\n", rank, tag, btrace.c_str());
  write(fd, output, strlen(output));
  fsync(fd);
  //  REMPI_DBG("STM:Send:%d:%d:%s", rank, tag, btrace.c_str());
#endif
}

void rempi_stm_mf(int rank, int tag, MPI_Comm comm)
{
#ifdef ENABLE_STM
  if (fd == -1) {
    sprintf(path, "/p/lscratche/sato5/rempi/stm/rank_%d.stm", rempi_my_rank);
    fd = open(path, O_CREAT| O_WRONLY, S_IRWXU);
  }
  string btrace = rempi_btrace_string();
  sprintf(output, "STM:MF:%d:%d:%s\n", rank, tag, btrace.c_str());
  write(fd, output, strlen(output));
  fsync(fd);
  //  REMPI_DBG("STM:MF:%d:%d:%s", rank, tag, btrace.c_str());
#endif
}

#endif
