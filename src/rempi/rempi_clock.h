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
#ifndef __REMPI_CLOCK_H__
#define __REMPI_CLOCK_H__

#include <mpi.h>
#include "rempi_type.h"
#include "rempi_event_list.h"

#define REMPI_CLOCK_INITIAL_CLOCK (10)
#define REMPI_CLOCK_COLLECTIVE_CLOCK (3)

void rempi_clock_init();
int rempi_clock_register_recv_clocks(size_t *clocks, int length);
int rempi_clock_get_local_clock(size_t*);
int rempi_clock_sync_clock(size_t clock, int request_type);
int rempi_clock_sync_clock(int matched_count,
                           int array_of_indices[],
                           size_t *clock, int* request_info,
			   int matching_function_type);


int rempi_clock_collective_sync_clock(MPI_Comm comm);

int rempi_clock_mpi_isend(mpi_const void *buf,
			  int count,
			  MPI_Datatype datatype,
			  int dest,
			  int tag,
			  MPI_Comm comm,
			  MPI_Request *request,
			  int send_function_type);

/* #define REMPI_CLOCK_STOP  (0) */
/* #define REMPI_CLOCK_START (1) */
/* int rempi_clock_control(int); */






#endif

