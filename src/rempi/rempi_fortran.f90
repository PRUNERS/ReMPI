      subroutine GET_MPI_F_STATUS_SIZE___ (CODE)
      INTEGER CODE
      CALL GET_MPI_F_STATUS_SIZE (CODE)
      END
      subroutine GET_MPI_F_STATUS_SIZE__ (CODE)
      INTEGER CODE
      CALL GET_MPI_F_STATUS_SIZE (CODE)
      END
      subroutine GET_MPI_F_STATUS_SIZE_ (CODE)
      INTEGER CODE
      CALL GET_MPI_F_STATUS_SIZE (CODE)
      END
      subroutine GET_MPI_F_STATUS_SIZE (CODE)
      INCLUDE "mpif.h"
      INTEGER CODE
      CODE = MPI_STATUS_SIZE
      RETURN
      END
      
      subroutine GET_MPI_F_IN_PLACE___ (CODE)
      INTEGER CODE
      CALL GET_MPI_F_IN_PLACE (CODE)
      END
      subroutine GET_MPI_F_IN_PLACE__ (CODE)
      INTEGER CODE
      CALL GET_MPI_F_IN_PLACE (CODE)
      END
      subroutine GET_MPI_F_IN_PLACE_ (CODE)
      INTEGER CODE
      CALL GET_MPI_F_IN_PLACE (CODE)
      END
      subroutine GET_MPI_F_IN_PLACE (CODE)
      INCLUDE "mpif.h"
      INTEGER CODE
      CODE = MPI_IN_PLACE
      RETURN
      END
      
      subroutine GET_MPI_F_BOTTOM___ (CODE)
      INTEGER CODE
      CALL GET_MPI_F_BOTTOM (CODE)
      END
      subroutine GET_MPI_F_BOTTOM__ (CODE)
      INTEGER CODE
      CALL GET_MPI_F_BOTTOM (CODE)
      END
      subroutine GET_MPI_F_BOTTOM_ (CODE)
      INTEGER CODE
      CALL GET_MPI_F_BOTTOM (CODE)
      END
      subroutine GET_MPI_F_BOTTOM (CODE)
      INCLUDE "mpif.h"
      INTEGER CODE
      CODE = MPI_BOTTOM
      RETURN
      END
