#!/bin/bash -x

export DYLD_LIBRARY_PATH=$DYLD_LIBRARY_PATH:/Users/lagunaperalt1/projects/PRUNER/ReMPI/code/src

mpirun -n 4 ./test

