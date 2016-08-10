#/bin/sh

prefix=/g/g90/sato5/repo/rempi

mode=$1
num_procs=$2


dir=${prefix}/test/.rempi
mkdir ${dir}
#io_watchdog="--io-watchdog"

#memcheck="memcheck"


#memcheck="memcheck --xml-file=/tmp/rempi.mc"



bin="./rempi_test_units matching"
librempi="../lib/librempi.so"
REMPI_MODE=$mode REMPI_DIR=${dir} REMPI_ENCODE=4 REMPI_GZIP=1 REMPI_TEST_ID=1 LD_PRELOAD=${librempi} srun -n ${num_procs} ${bin}
exit


par=1000
bin="../src/MCBenchmark-linux_x86_64.exe --nCores=1 --nThreadCore=1 --numParticles=$par --nZonesX=400 --nZonesY=400 --distributedSource --mirrorBoundary --sigmaA 1 --sigmaS 20 --weakScaling"
cd ./external/mcb/run-decks/
make cleanc
librempi="../../../../lib/librempi.so"
REMPI_MODE=${mode} REMPI_DIR=${dir} REMPI_ENCODE=4 REMPI_GZIP=1 REMPI_TEST_ID=1 REMPI_MAX=128 LD_PRELOAD=${librempi} srun -n ${num_procs} ${memcheck} ${bin}
cd -
exit

bin="./rempi_test_mini_mcb 10 0 20"
librempi="../lib/librempi.so"
REMPI_MODE=${mode} REMPI_DIR=${dir} REMPI_ENCODE=4 REMPI_GZIP=1 REMPI_TEST_ID=1 REMPI_MAX=16 LD_PRELOAD=${librempi} srun -n ${num_procs} ${memcheck} ${bin}
exit




















bin="./rempi_test_master_worker"
librempi="../lib/librempi.so"
REMPI_MODE=${mode} REMPI_DIR=${dir} REMPI_ENCODE=4 REMPI_GZIP=1 REMPI_TEST_ID=1 REMPI_MAX=50 LD_PRELOAD=${librempi} srun -n ${num_procs} ${bin}
exit





par=`expr 800 \* $num_procs`
bin="../src/MCBenchmark.exe --nCores=1 --nThreadCore=1 --numParticles=$par --nZonesX=400 --nZonesY=400 --distributedSource --mirrorBoundary --sigmaA 1 --sigmaS 20 "
cd ./external/mcb/run-decks/
make cleanc
#librempi="../../../../lib/librempilite.so"
#REMPI_MODE=${mode} REMPI_DIR=${dir} REMPI_ENCODE=0 REMPI_GZIP=1 REMPI_TEST_ID=0 LD_PRELOAD=${librempi} srun ${io_watchdog} -n ${num_procs} ${memcheck} ${bin}
librempi="../../../../lib/librempi.so"
REMPI_MODE=${mode} REMPI_DIR=${dir} REMPI_ENCODE=4 REMPI_GZIP=1 REMPI_TEST_ID=1 LD_PRELOAD=${librempi} srun -n ${num_procs} ${bin}
cd -
exit














#bin="./rempi_test_mini_mcb 10 1 1000"
bin="./rempi_test_mini_mcb 2 1 2"
librempi="../lib/librempi.so"
REMPI_MODE=${mode} REMPI_DIR=${dir} REMPI_ENCODE=4 REMPI_GZIP=1 REMPI_TEST_ID=0 LD_PRELOAD=${librempi} srun -n ${num_procs} ${bin}
#librempi="../lib/librempilite.so"
#REMPI_MODE=${mode} REMPI_DIR=${dir} REMPI_ENCODE=0 REMPI_GZIP=1 REMPI_TEST_ID=0 LD_PRELOAD=${librempi} srun -n ${num_procs} ${bin}
exit





par=`expr 800000 \* $num_procs`
bin="../src/MCBenchmark.exe --nCores=1 --nThreadCore=1 --numParticles=$par --nZonesX=400 --nZonesY=400 --distributedSource --mirrorBoundary --sigmaA 1 --sigmaS 20 "
cd ./external/mcb/run-decks/
make cleanc
dir=${prefix}/test/.rempi.b_gzip
mkdir ${dir}
librempi="../../../../lib/librempilite.so"
REMPI_MODE=${mode} REMPI_DIR=${dir} REMPI_ENCODE=0 REMPI_GZIP=1 REMPI_TEST_ID=0 LD_PRELOAD=${librempi} srun -n ${num_procs} ${bin}
dir=${prefix}/test/.rempi.cdc_0_gzip
mkdir ${dir}
librempi="../../../../lib/librempi.so"
REMPI_MODE=${mode} REMPI_DIR=${dir} REMPI_ENCODE=4 REMPI_GZIP=1 REMPI_TEST_ID=0 LD_PRELOAD=${librempi} srun -n ${num_procs} ${bin}
dir=${prefix}/test/.rempi.cdc_1_gzip
mkdir ${dir}
librempi="../../../../lib/librempi.so"
REMPI_MODE=${mode} REMPI_DIR=${dir} REMPI_ENCODE=4 REMPI_GZIP=1 REMPI_TEST_ID=1 LD_PRELOAD=${librempi} srun -n ${num_procs} ${bin}
cd -
exit




librempi="../lib/librempilite.so"
bin="./rempi_test_units"
REMPI_MODE=${mode} REMPI_DIR=${dir} REMPI_ENCODE=0 REMPI_GZIP=1 REMPI_TEST_ID=0 REMPI_MAX=16 LD_PRELOAD=${librempi} srun -n ${num_procs} ${memcheck} ${bin}
exit





librempi="../lib/librempilite.so"
bin="./rempi_test_msg_race 0 1 10000 2 0"
REMPI_MODE=${mode} REMPI_DIR=${dir} REMPI_ENCODE=0 REMPI_GZIP=1 REMPI_TEST_ID=0 REMPI_MAX=16 LD_PRELOAD=${librempi} srun --io-watchdog=conf=.io-watchdogrc -n ${num_procs} ${bin}
exit


bin="./rempi_test_msg_race 0 3 10000 2 0"
REMPI_MODE=${mode} REMPI_DIR=${dir} REMPI_ENCODE=0 REMPI_GZIP=1 REMPI_TEST_ID=0 REMPI_MAX=16 LD_PRELOAD=${librempi} srun --io-watchdog=conf=.io-watchdogrc -n ${num_procs} ${bin}
exit


bin="./rempi_test_msg_race 0 2 10000 2 0"
REMPI_MODE=${mode} REMPI_DIR=${dir} REMPI_ENCODE=0 REMPI_GZIP=1 REMPI_TEST_ID=0 REMPI_MAX=16 LD_PRELOAD=${librempi} srun --io-watchdog=conf=.io-watchdogrc -n ${num_procs} ${bin}
exit






