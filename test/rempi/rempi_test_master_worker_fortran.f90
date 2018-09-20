!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
! Copyright (c) 2016, Lawrence Livermore National Security, LLC.                     
! Produced at the Lawrence Livermore National Laboratory.                            
!                                                                                    
! Written by Kento Sato, kento@llnl.gov. LLNL-CODE-711357.                           
! All rights reserved.                                                               
!                                                                                    
! This file is part of ReMPI. For details, see https://github.com/PRUNER/ReMPI       
! Please also see the LICENSE file for our notice and the LGPL.                      
!                                                                                    
! This program is free software; you can redistribute it and/or modify it under the 
! terms of the GNU General Public License (as published by the Free Software         
! Foundation) version 2.1 dated February 1999.                                       
!                                                                                    
! This program is distributed in the hope that it will be useful, but WITHOUT ANY    
! WARRANTY; without even the IMPLIED WARRANTY OF MERCHANTABILITY or                  
! FITNESS FOR A PARTICULAR PURPOSE. See the terms and conditions of the GNU          
! General Public License for more details.                                           
!                                                                                    
! You should have received a copy of the GNU Lesser General Public License along     
! with this program; if not, write to the Free Software Foundation, Inc., 59 Temple 
! Place, Suite 330, Boston, MA 02111-1307 USA                                
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

      program ping
      include 'mpif.h'

      integer numtasks, rank, dest, source, count, tag, ierr
      integer stat(MPI_STATUS_SIZE)
      integer inmsg, outmsg
      tag = 1


      call MPI_INIT(ierr)
      call MPI_COMM_RANK(MPI_COMM_WORLD, rank, ierr)
      call MPI_COMM_SIZE(MPI_COMM_WORLD, numtasks, ierr)
 

      if (rank .eq. 0) then
         do i=1,numtasks-1
            call MPI_RECV(inmsg, 1, MPI_INT, MPI_ANY_SOURCE, tag, MPI_COMM_WORLD, stat, ierr)
            dest = stat(MPI_SOURCE)
            print *, "rank=", dest, "value=", inmsg
        end do

      else 
         outmsg = rank
         dest = 0
         call MPI_SEND(outmsg, 1, MPI_INT, dest, tag, MPI_COMM_WORLD, err)
      endif


      call MPI_FINALIZE(ierr)

      end
