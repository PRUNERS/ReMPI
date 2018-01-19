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
#include <stdio.h>
#include <mpi.h>
#include <sys/time.h>
//#include <signal.h>
#include <stdlib.h>
//#include <unistd.h>
//#include <string.h>
//#include <sys/mman.h>


int main(int argc, char *argv[])
{
  int i, dest;
  int my_rank, size;
  int buf = 0;
  MPI_Status status;

  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);

  srand((int)MPI_Wtime());

  for (dest = 0; dest < size; dest++) {
    if (my_rank == dest) {
      for (i = 0; i < size-1; i++) {
	MPI_Recv(&buf, 1, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &status);
	fprintf(stderr, "Rank %d: status.MPI_SOURCE = %d\n", my_rank, status.MPI_SOURCE);
      }
    } else {
      usleep(rand() % 10 * 10000);
      MPI_Send(&buf, 1, MPI_INT, dest, 0, MPI_COMM_WORLD);
    }
    MPI_Barrier(MPI_COMM_WORLD);
  }

  MPI_Finalize();

  return 0;
}
