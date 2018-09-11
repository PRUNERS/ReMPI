#!/bin/sh

# take no command-line arguments
# expects the following environment variables to be set
# - MPI:            one of (mpich, openmpi)
# - CLANG_VERSION:  a version number, e.g. 3.9

set -e
set -u
set -x

SCRIPT="`readlink -f "$0"`"
SCRIPT_DIR="`dirname "$SCRIPT"`"

cd "$SCRIPT_DIR/.."

sudo sh travis-test/update-mpi.sh $MPI

mpicc --version
mpicxx --version
mpif77 --version
mpirun --version

./autogen.sh
./configure
make
sh travis-test/test.sh
