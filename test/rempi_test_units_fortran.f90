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

module global
  include 'mpif.h'
  integer rank
  integer numtasks
end module global

subroutine test_master_worker()
  use global
  implicit none;
  integer dest, source, count, tag, ierr, err, i
  integer stat(MPI_STATUS_SIZE)
  integer inmsg, outmsg

  tag = 1
  if (rank .eq. 0) then
     do i=1,numtasks-1
        call MPI_RECV(inmsg, 1, MPI_INTEGER, MPI_ANY_SOURCE, tag, MPI_COMM_WORLD, stat, ierr)
        dest = stat(MPI_SOURCE)
     end do
  else 
     outmsg = rank
     dest = 0
     call MPI_SEND(outmsg, 1, MPI_INTEGER, dest, tag, MPI_COMM_WORLD, err)
  endif

end subroutine test_master_worker

!subroutine test_in_place()
!  use global
!  implicit none;
!
!  integer rank_in_place, ierr
!  integer rank_sum
!
!  rank_in_place = rank
!
!  print *, 'Rank: ', rank
!  call MPI_Allreduce(rank, rank_sum, 1, MPI_INTEGER, MPI_SUM, MPI_COMM_WORLD, ierr)
!  call MPI_Allreduce(MPI_F_IN_PLACE, rank_in_place, 1, MPI_INTEGER, MPI_SUM, MPI_COMM_WORLD, ierr)
!  print *, 'Sum: ', rank_sum, ' - ', rank_in_place, ' in_place: ', MPI_F_IN_PLACE
!
!end subroutine test_in_place


program main
  use global
  implicit none
  integer ierr


  call MPI_INIT(ierr)

!  print *, MPI_IN_PLACE, MPI_BOTTOM
!  call MPI_COMM_RANK(MPI_COMM_WORLD, MPI_F_IN_PLACE, ierr)
  call MPI_COMM_RANK(MPI_COMM_WORLD, rank, ierr)
  call MPI_COMM_SIZE(MPI_COMM_WORLD, numtasks, ierr)

  call test_master_worker()
!  call test_in_place()
  

  call MPI_FINALIZE(ierr)
  stop
end program main



