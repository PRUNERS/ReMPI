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

#include <mpi.h>
#include <stddef.h>

#if MPI_VERSION == 3 && !defined(REMPI_LITE)

#include <list>
#include <unordered_set>
#include <unordered_map>

#include "rempi_msg_buffer.h"
#include "rempi_request_mg.h"
#include "rempi_err.h"
#include "rempi_request_mg.h"
#include "rempi_event.h"
#include "rempi_event_list.h"
#include "rempi_cp.h"
#include "rempi_config.h"
#include "rempi_recorder.h"
#include "rempi_util.h"
#include "rempi_mf.h"
#include "rempi_clock.h"
#include "clmpi.h"

static size_t pre_allocated_array_of_clocks[PRE_ALLOCATED_REQUEST_LENGTH];
static size_t local_clock = REMPI_CLOCK_INITIAL_CLOCK;

void rempi_clock_init()
{
  CLMPI_clock_control(0);
  CLMPI_set_pb_clock(&local_clock);
  return;
}

int rempi_clock_register_recv_clocks(size_t *clocks, int length)
{
  PNMPIMOD_register_recv_clocks(clocks, length);
  return 0;
}

int rempi_clock_get_local_clock(size_t* clock)
{
  *clock = local_clock;
  return 0;
}

int rempi_clock_sync_clock(size_t clock, int request_type)
{
  if (request_type == REMPI_RECV_REQUEST) {
    if (local_clock < clock) {
      local_clock = clock;
    }
    local_clock++;
  }
  return 0;
}

int rempi_clock_sync_clock(int outcount, 
			   int array_of_indices[],
			   size_t *clock, int* request_info, int matching_function_type)
{
  int matched_request_index;
  /*
    MPI_Test/Testall/Wait/Waitall ==> array_of_indices==NULL
   */
  for (int i = 0; i < outcount; i++) {
    matched_request_index = (array_of_indices == NULL)? i:array_of_indices[i];
    rempi_clock_sync_clock(clock[i], request_info[matched_request_index]);
  }
  
  if (rempi_encode == REMPI_ENV_REMPI_ENCODE_REP && outcount == 0) { 
    local_clock++;
  }
  
  // size_t c;
  // PNMPIMOD_get_local_clock(&c);
  // if (c != local_clock) {
  //   REMPI_DBG("dfferent: %lu and %lu", c, local_clock);
  // }
  
  return 0;
}

int rempi_clock_collective_sync_clock(MPI_Comm comm)
{
  PNMPIMOD_collective_sync_clock(comm);
  return 0;
}


int rempi_clock_mpi_isend(mpi_const void *buf,
			  int count,
			  MPI_Datatype datatype,
			  int dest,
			  int tag,
			  MPI_Comm comm,
			  MPI_Request *request,
			  int send_function_type)
{
  int ret;
  local_clock++;
  return ret;
}

#else

int rempi_clock_get_local_clock(size_t* clock)
{
   return 0;
}

#endif
