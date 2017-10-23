/* ==========================ReMPI:LICENSE==========================================   
   Copyright (c) 2016, Lawrence Livermore National Security, LLC.                     
   Produced at the Lawrence Livermore National Laboratory.                            
                                                                                       
   Written by Kento Sato, kento@llnl.gov. LLNL-CODE-711357.                           
   All rights reserved.                                                               
                                                                                       
   This file is part of ReMPI. For details, see https://github.com/PRUNER/ReMPI       
   Please also see the LICENSE file for our notice and the LGPL.                      
                                                                                       
   This program is free software; you can redistribute it and/or modify it under the   
   terms of the GNU General Public License (as published by the Free Software         
   Foundation) version 2.1 dated February 1999.                                       
                                                                                       
   This program is distributed in the hope that it will be useful, but WITHOUT ANY    
   WARRANTY; without even the IMPLIED WARRANTY OF MERCHANTABILITY or                  
   FITNESS FOR A PARTICULAR PURPOSE. See the terms and conditions of the GNU          
   General Public License for more details.                                           
                                                                                       
   You should have received a copy of the GNU Lesser General Public License along     
   with this program; if not, write to the Free Software Foundation, Inc., 59 Temple   
   Place, Suite 330, Boston, MA 02111-1307 USA
   ============================ReMPI:LICENSE========================================= */
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
