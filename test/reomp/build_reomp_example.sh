#!/bin/bash


set -x
#########################
#  Build without ReOMP  #
#########################
bin=reomp_example_without_reomp
TSAN_CFLAGS=""
REOMP_CFLAGS=""
clang++ ${TSAN_CFLAGS} ${REOMP_CFLAGS} -O3 -fopenmp -o ${bin} reomp_example.cpp
./${bin} 16
./${bin} 16

#############################
#  Build/Run without ReOMP  #
#############################
bin=reomp_example_with_reomp
TSAN_CFLAGS=""
REOMP_CFLAGS="-Xclang -load -Xclang ../../src/reomp/.libs/libreompir.so -L../../src/reomp/.libs/ -lreomp"
clang++ ${TSAN_CFLAGS} ${REOMP_CFLAGS} -O3 -fopenmp -o ${bin} reomp_example.cpp
export LD_LIBRARY_PATH=../../src/reomp/.libs/
REOMP_MODE=0 ./${bin} 16
REOMP_MODE=1 ./${bin} 16

#####################################
#  Build/Run with thread sanitizer  #
#####################################
rm reomp_tsan.log.*
bin=reomp_example_with_tsan
TSAN_CFLAGS="-g -fomit-frame-pointer -fsanitize=thread"
REOMP_CFLAGS=""
clang++ ${TSAN_CFLAGS} ${REOMP_CFLAGS} -O3 -fopenmp -o ${bin} reomp_example.cpp
export TSAN_OPTIONS="log_path=reomp_tsan.log history_size=7"
echo "This may take a few minutes ..."
./${bin} 2

###############################################
#  Build/Run with ReOMP (+ data race support)  #
###############################################
bin=reomp_example_with_reomp_data_race
TSAN_CFLAGS=""
REOMP_CFLAGS="-Xclang -load -Xclang ../../src/reomp/.libs/libreompir.so -L../../src/reomp/.libs/ -lreomp"
clang++ ${TSAN_CFLAGS} ${REOMP_CFLAGS} -O3 -fopenmp -o ${bin} reomp_example.cpp
#clang++ -emit-llvm -S ${REOMP_CFLAGS} -O3 -fopenmp -o reomp_example_with_reomp_data_race.ll reomp_example.cpp
export LD_LIBRARY_PATH=../../src/reomp/.libs/
export TSAN_OPTIONS="log_path=reomp_tsan.log"
REOMP_MODE=0 ./${bin} 16
REOMP_MODE=1 ./${bin} 16
echo ""
echo "One more replay ... "
echo ""
sleep 5
REOMP_MODE=1 ./${bin} 16





