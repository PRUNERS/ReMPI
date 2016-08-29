#/bin/sh

prefix=/g/g90/sato5/repo/rempi

mode=$1
num_procs=$2


dir=${prefix}/test/.rempi
#dir=/tmp/
#mkdir ${dir}/*

#io_watchdog="--io-watchdog"
#memcheck="memcheck  --xml-file=/tmp/unit.cab687.0.mc"




bin="./rempi_test_mini_mcb 10 0 1000"
REMPI_MODE=${mode} REMPI_DIR=${dir} REMPI_ENCODE=0 REMPI_GZIP=1 REMPI_TEST_ID=1 REMPI_MAX=10000000 LD_PRELOAD=${librempi} srun -n ${num_procs} ${bin}
#REMPI_MODE=${mode} REMPI_DIR=${dir} REMPI_ENCODE=4 REMPI_GZIP=1 REMPI_TEST_ID=1 REMPI_MAX=10000000 LD_PRELOAD=${librempi} srun -n ${num_procs} ${bin}
exit

par=1000
bin="../src/MCBenchmark.exe --nCores=1 --nThreadCore=1 --numParticles=$par --nZonesX=400 --nZonesY=400 --distributedSource --mirrorBoundary --sigmaA 1 --sigmaS 20 --weakScaling"
cd ./external/mcb/run-decks/
make cleanc
REMPI_MODE=${mode} REMPI_DIR=${dir} REMPI_ENCODE=0 REMPI_GZIP=1 REMPI_TEST_ID=1 REMPI_MAX=10000000 LD_PRELOAD=${librempi} srun -n ${num_procs} ${bin}
#REMPI_MODE=${mode} REMPI_DIR=${dir} REMPI_ENCODE=4 REMPI_GZIP=1 REMPI_TEST_ID=1 REMPI_MAX=10000000 LD_PRELOAD=${librempi} srun -n ${num_procs} ${bin}
#REMPI_MODE=${mode} REMPI_DIR=${dir} REMPI_ENCODE=4 REMPI_GZIP=1 REMPI_TEST_ID=1 REMPI_MAX=50 LD_PRELOAD=${librempi} srun -n ${num_procs} ${bin}
cd -
exit

bin="./rempi_test_master_worker"
REMPI_MODE=${mode} REMPI_DIR=${dir} REMPI_ENCODE=0 REMPI_GZIP=1 REMPI_TEST_ID=1 REMPI_MAX=10000000 LD_PRELOAD=${librempi} srun -n ${num_procs} ${bin}
exit
























