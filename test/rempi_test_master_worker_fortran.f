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
            call MPI_RECV(inmsg, 1, MPI_INT, MPI_ANY_SOURCE, tag, 
     &           MPI_COMM_WORLD, stat, ierr)
            dest = stat(MPI_SOURCE)
            print *, "rank=", dest, "value=", inmsg
        end do

      else 
         outmsg = rank
         dest = 0
         call MPI_SEND(outmsg, 1, MPI_INT, dest, tag, 
     &        MPI_COMM_WORLD, err)
      endif


      call MPI_FINALIZE(ierr)

      end
