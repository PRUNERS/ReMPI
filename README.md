README


*MPI_VERSION == 3 && defined(REMPI_LITE)
	     * Simple ReMPI => librempilite.so

*MPI_VERSION == 3 && !defined(REMPI_LITE)
	     * Clock + CDC ReMPI => libremp.so

*MPI_VERSION != 3 && defined(REMPI_LITE)
	     * Simple ReMPI => librempilite.so

*MPI_VERSION != 3 && !defined(REMPI_LITE)
	     * Clock ReMPI => libremp.so

* Non-piggyback
  * Simple ReMPI:  defined(REMPI_LITE)
* Piggyback
  * Clock  ReMPI: !defined(REMPI_LITE)
  * CDC    ReMPI: !defined(REMPI_LITE) && MPI_VERSION == 3