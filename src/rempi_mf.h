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
#ifndef __REMPI_MF_H__
#define __REMPI_MF_H__

#include "rempi_type.h"

int rempi_mpi_isend(mpi_const void *buf,
		    int count,
		    MPI_Datatype datatype,
		    int dest,
		    int tag,
		    MPI_Comm comm,
		    MPI_Request *request,
		    int send_function_type);

int rempi_mpi_send_init(mpi_const void *arg_0, 
			int arg_1, 
			MPI_Datatype arg_2, 
			int arg_3, 
			int arg_4, 
			MPI_Comm arg_5, 
			MPI_Request *arg_6, 
			int send_function_type);


int rempi_mpi_mf(int incount,
	     MPI_Request array_of_requests[],
	     int *outcount,
	     int array_of_indices[],
	     MPI_Status array_of_statuses[],
	     int matching_function_type);

int rempi_mpi_get_matched_count(int incount, int *outcount, int nullcount, int matching_function_type);

int rempi_mpi_pf(int source,
             int tag,
             MPI_Comm comm,
             int *flag,
             MPI_Status *status,
             int prove_function_type);

#endif
