#!/bin/sh

#LD_PRELOAD=./src/.libs/libpmpi.so mpirun -n 2 ./test/rempi_test_units
#mpirun -n 2 ./test/rempi_test_units
set -x
export REMPI_RUN=mpirun
cd ./test
./rempi_unit_test.sh 16
