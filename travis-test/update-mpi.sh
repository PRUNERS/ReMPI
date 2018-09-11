#!/bin/sh
set -e
case $1 in
  mpich)   set -x
    update-alternatives --set mpi /usr/include/mpich;;
  openmpi) set -x
    update-alternatives --set mpi /usr/lib/openmpi/include;;
  *)
    echo "Unknown MPI implementation:" $1; exit 1;;
esac
update-alternatives --set mpirun /usr/bin/mpirun.$1
