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

#include "rempi_mf.h"
#include "rempi_config.h"
#include "rempi_err.h"

int rempi_mpi_isend(mpi_const void *buf,
		     int count,
		     MPI_Datatype datatype,
		     int dest,
		     int tag,
		     MPI_Comm comm,
		     MPI_Request *request,
		     int send_function_type)
{
  switch(send_function_type) {
  case REMPI_MPI_ISEND:
    return PMPI_Isend(buf, count, datatype, dest, tag, comm, request);
  case REMPI_MPI_IBSEND:
    return PMPI_Ibsend(buf, count, datatype, dest, tag, comm, request);
  case REMPI_MPI_IRSEND:
    return PMPI_Irsend(buf, count, datatype, dest, tag, comm, request);
  case REMPI_MPI_ISSEND:
    return PMPI_Issend(buf, count, datatype, dest, tag, comm, request);
  default:
    REMPI_ERR("No such Send: %d", send_function_type);
  }
  return MPI_ERR_UNKNOWN;
}

int rempi_mpi_send_init(mpi_const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request, int send_function_type)
{
  switch(send_function_type) {
  case REMPI_MPI_ISEND:
    return PMPI_Send_init(buf, count, datatype, dest, tag, comm, request);
  case REMPI_MPI_IBSEND:
    return PMPI_Bsend_init(buf, count, datatype, dest, tag, comm, request);
  case REMPI_MPI_IRSEND:
    return PMPI_Rsend_init(buf, count, datatype, dest, tag, comm, request);
  case REMPI_MPI_ISSEND:
    return PMPI_Ssend_init(buf, count, datatype, dest, tag, comm, request);
  default:
    REMPI_ERR("No such Send: %d", send_function_type);
  }
  return MPI_ERR_UNKNOWN;
}


int rempi_mpi_mf(int incount,
		 MPI_Request array_of_requests[],
		 int *outcount,
		 int array_of_indices[],
		 MPI_Status array_of_statuses[],
		 int matching_function_type)
{
  switch(matching_function_type) {
  case REMPI_MPI_TEST: 
    return PMPI_Test(array_of_requests, outcount, array_of_statuses);
    break;
  case REMPI_MPI_TESTANY:
    return PMPI_Testany(incount, array_of_requests, array_of_indices, outcount, array_of_statuses);
    break;
  case REMPI_MPI_TESTSOME:
    return PMPI_Testsome(incount, array_of_requests, outcount, array_of_indices, array_of_statuses);
    break;
  case REMPI_MPI_TESTALL: 
    return PMPI_Testall(incount, array_of_requests, outcount, array_of_statuses);
    break;
  case REMPI_MPI_WAIT:
    //    REMPI_DBG("Wait(rempi_mpi_mf): request: %p, status: %p", array_of_requests[0], array_of_statuses[0]);
    return PMPI_Wait(array_of_requests, array_of_statuses);
    break;
  case REMPI_MPI_WAITANY:
    return PMPI_Waitany(incount, array_of_requests, array_of_indices, array_of_statuses);
    break;
  case REMPI_MPI_WAITSOME:
    return PMPI_Waitsome(incount, array_of_requests, outcount, array_of_indices, array_of_statuses);
    break;
  case REMPI_MPI_WAITALL:
    return PMPI_Waitall(incount, array_of_requests, array_of_statuses);
    break;
  default:
    REMPI_ERR("No such MF: %d", matching_function_type);
  }
  return MPI_ERR_UNKNOWN;
}


int rempi_mpi_get_matched_count(int incount, int *outcount, int nullcount, int matching_function_type)
{
  int matched_count;
  if (incount == 0 || incount == nullcount) return 0;

  switch(matching_function_type) {
  case REMPI_MPI_TEST:
    matched_count = *outcount;
    break;
  case REMPI_MPI_TESTANY:
    matched_count = *outcount;
    break;
  case REMPI_MPI_TESTSOME:
    matched_count = *outcount;
    break;
  case REMPI_MPI_TESTALL:
    matched_count = (*outcount == 0)? 0:incount;
    break;
  case REMPI_MPI_WAIT:
    matched_count = 1;
    break;
  case REMPI_MPI_WAITANY:
    matched_count = 1;
    break;
  case REMPI_MPI_WAITSOME:
    matched_count = *outcount;
    break;
  case REMPI_MPI_WAITALL:
    matched_count = incount;
    break;
  default:
    REMPI_ERR("No such matching function type: %d", matching_function_type);
  }
  return matched_count;
}


int rempi_mpi_pf(int source,
		 int tag,
		 MPI_Comm comm,
		 int *flag,
		 MPI_Status *status,
		 int probe_function_type)
{
  int ret;
  switch(probe_function_type) {
  case REMPI_MPI_PROBE:
    ret = PMPI_Probe(source, tag, comm, status);
    break;
  case REMPI_MPI_IPROBE:
    ret = PMPI_Iprobe(source, tag, comm, flag, status);
    break;
  default:
    REMPI_ERR("No such PF: %d", probe_function_type);
  }
  return ret; 
}


