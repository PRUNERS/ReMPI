#!/bin/sh

#LD_PRELOAD=./src/.libs/libpmpi.so mpirun -n 2 ./test/rempi_test_units
mpirun -n 2 ./test/rempi_test_units
