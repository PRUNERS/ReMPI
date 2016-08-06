#/bin/sh

num_procs=$1

dir=.rempi
mkdir ${dir}
librempi="../lib/librempilite.so"




bin="./rempi_test_msg_race 0 3 10000 2 0"
REMPI_MODE=0 REMPI_DIR=${dir} REMPI_ENCODE=0 REMPI_GZIP=1 REMPI_TEST_ID=0 LD_PRELOAD=${librempi} srun --io-watchdog=conf=.io-watchdogrc -n ${num_procs} ${bin}
REMPI_MODE=1 REMPI_DIR=${dir} REMPI_ENCODE=0 REMPI_GZIP=1 REMPI_TEST_ID=0 LD_PRELOAD=${librempi} srun --io-watchdog=conf=.io-watchdogrc -n ${num_procs} ${bin}
srun rm ${dir}/* 2> /dev/null
exit

#========== 

bin=./rempi_test_units
REMPI_MODE=0 REMPI_DIR=${dir} REMPI_ENCODE=0 REMPI_GZIP=0 REMPI_TEST_ID=0 LD_PRELOAD=${librempi} srun -n ${num_procs} ${bin}
REMPI_MODE=1 REMPI_DIR=${dir} REMPI_ENCODE=0 REMPI_GZIP=0 REMPI_TEST_ID=0 LD_PRELOAD=${librempi} srun -n ${num_procs} ${bin}
srun rm ${dir}/* 2> /dev/null

bin="./rempi_test_msg_race 0 1 10000 2 0"
REMPI_MODE=0 REMPI_DIR=${dir} REMPI_ENCODE=0 REMPI_GZIP=1 REMPI_TEST_ID=0 LD_PRELOAD=${librempi} srun --io-watchdog=conf=.io-watchdogrc -n ${num_procs} ${bin}
REMPI_MODE=1 REMPI_DIR=${dir} REMPI_ENCODE=0 REMPI_GZIP=1 REMPI_TEST_ID=0 LD_PRELOAD=${librempi} srun --io-watchdog=conf=.io-watchdogrc -n ${num_procs} ${bin}
srun rm ${dir}/* 2> /dev/null

bin="./rempi_test_msg_race 0 2 10000 2 0"
REMPI_MODE=0 REMPI_DIR=${dir} REMPI_ENCODE=0 REMPI_GZIP=1 REMPI_TEST_ID=0 LD_PRELOAD=${librempi} srun --io-watchdog=conf=.io-watchdogrc -n ${num_procs} ${bin}
REMPI_MODE=1 REMPI_DIR=${dir} REMPI_ENCODE=0 REMPI_GZIP=1 REMPI_TEST_ID=0 LD_PRELOAD=${librempi} srun --io-watchdog=conf=.io-watchdogrc -n ${num_procs} ${bin}
srun rm ${dir}/* 2> /dev/null

bin="./rempi_test_msg_race 0 3 10000 2 0"
REMPI_MODE=0 REMPI_DIR=${dir} REMPI_ENCODE=0 REMPI_GZIP=1 REMPI_TEST_ID=0 LD_PRELOAD=${librempi} srun --io-watchdog=conf=.io-watchdogrc -n ${num_procs} ${bin}
REMPI_MODE=1 REMPI_DIR=${dir} REMPI_ENCODE=0 REMPI_GZIP=1 REMPI_TEST_ID=0 LD_PRELOAD=${librempi} srun --io-watchdog=conf=.io-watchdogrc -n ${num_procs} ${bin}
srun rm ${dir}/* 2> /dev/null



librempi="../../../../lib/librempilite.so"
par=`expr 80 \* $num_procs`
bin="../src/MCBenchmark-linux_x86_64.exe --nCores=1 --nThreadCore=1 --numParticles=$par --nZonesX=400 --nZonesY=400 --distributedSource --mirrorBoundary --sigmaA 1 --sigmaS 20 "
cd ./external/mcb/run-decks/
make cleanc
REMPI_MODE=0 REMPI_DIR=${dir} REMPI_ENCODE=0 REMPI_GZIP=0 REMPI_TEST_ID=0 LD_PRELOAD=${librempi} srun ${io_watchdog} -n ${num_procs} ${bin}
REMPI_MODE=1 REMPI_DIR=${dir} REMPI_ENCODE=0 REMPI_GZIP=0 REMPI_TEST_ID=0 LD_PRELOAD=${librempi} srun -n ${num_procs} ${bin}
srun rm ${dir}/* 2> /dev/null

REMPI_MODE=0 REMPI_DIR=${dir} REMPI_ENCODE=0 REMPI_GZIP=1 REMPI_TEST_ID=0 LD_PRELOAD=${librempi} srun ${io_watchdog} -n ${num_procs} ${bin}
REMPI_MODE=1 REMPI_DIR=${dir} REMPI_ENCODE=0 REMPI_GZIP=1 REMPI_TEST_ID=0 LD_PRELOAD=${librempi} srun -n ${num_procs} ${bin}
srun rm ${dir}/* 2> /dev/null

REMPI_MODE=0 REMPI_DIR=${dir} REMPI_ENCODE=0 REMPI_GZIP=1 REMPI_TEST_ID=0 REMPI_MAX=16 LD_PRELOAD=${librempi} srun ${io_watchdog} -n ${num_procs} ${bin}
REMPI_MODE=1 REMPI_DIR=${dir} REMPI_ENCODE=0 REMPI_GZIP=1 REMPI_TEST_ID=0 REMPI_MAX=16 LD_PRELOAD=${librempi} srun -n ${num_procs} ${bin}
srun rm ${dir}/* 2> /dev/null
cd -
exit

