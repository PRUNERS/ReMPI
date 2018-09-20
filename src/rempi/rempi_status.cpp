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
