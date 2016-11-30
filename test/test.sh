#/bin/sh

#prefix=/g/g90/sato5/repo/rempi
#prefix=/tmp
#prefix=/l/ssd
#prefix=/p/lscratchf/sato5/rempi/
prefix=./rempi_record/

mode=$1
num_procs=$2

dir=${prefix}/
mkdir -p ${dir}
#io_watchdog="--io-watchdog"
#librempi=/g/g90/sato5/repo/rempi/install/lib/librempi.so
#memcheck="valgrind --tool=memcheck --xml=yes --xml-file=`echo $$`.mc --partial-loads-ok=yes --error-limit=no --leak-check=full --show-reachable=yes --max-stackframe=16777216 --num-callers=20 --child-silent-after-fork=yes --track-origins=yes"

bin="./rempi_test_units zero_incount"
librempi=../src/.libs/librempi.so
#REMPI_MODE=$mode REMPI_DIR=${dir} REMPI_ENCODE=7 REMPI_GZIP=1 REMPI_TEST_ID=1 LD_PRELOAD=${librempi} srun -n ${num_procs} ${bin}
#REMPI_MODE=$mode REMPI_DIR=${dir} REMPI_ENCODE=4 REMPI_GZIP=1 REMPI_TEST_ID=1 LD_PRELOAD=${librempi} srun -n ${num_procs} ${bin}
REMPI_MODE=$mode REMPI_DIR=${dir} REMPI_ENCODE=0 REMPI_GZIP=1 REMPI_TEST_ID=0 LD_PRELOAD=${librempi} srun -n ${num_procs} ${bin}
exit

# ===== MCB test ========
par=`expr 80 \* $num_procs`
bin="../src/MCBenchmark-linux_x86_64.exe --nCores=1 --nThreadCore=1 --numParticles=$par --nZonesX=400 --nZonesY=400 --distributedSource --mirrorBoundary --sigmaA 1 --sigmaS 20 "
cd /g/g90/sato5/repo/MCBdouble/run-decks/
make cleanc
#librempi=/g/g90/sato5/repo/rempi/src/.libs/librempi.so
#REMPI_MODE=${mode} REMPI_DIR=${dir} REMPI_ENCODE=0 REMPI_GZIP=1 REMPI_TEST_ID=0 LD_PRELOAD=${librempi} srun ${io_watchdog} -n ${num_procs} ${bin}
#librempi=/g/g90/sato5/repo/rempi/src/.libs/librempix.so
#REMPI_MODE=${mode} REMPI_DIR=${dir} REMPI_ENCODE=4 REMPI_GZIP=1 REMPI_TEST_ID=1 LD_PRELOAD=${librempi} srun ${io_watchdog} -n ${num_procs} ${bin}
librempi=/g/g90/sato5/repo/rempi/src/.libs/librempix.so
REMPI_MODE=${mode} REMPI_DIR=${dir} REMPI_ENCODE=7 REMPI_GZIP=0 REMPI_TEST_ID=1 LD_PRELOAD=${librempi} srun ${io_watchdog} -n ${num_procs} ${memcheck}  ${bin}
#srun ${io_watchdog} -n ${num_procs} ${bin}
cd -
exit


#bin="./rempi_test_mini_mcb 10 1 1000"
bin="./rempi_test_mini_mcb 2 1 1"
librempi=/g/g90/sato5/repo/rempi/src/.libs/librempix.so
REMPI_MODE=${mode} REMPI_DIR=${dir} REMPI_ENCODE=7 REMPI_GZIP=0 REMPI_TEST_ID=1 LD_PRELOAD=${librempi} srun -n ${num_procs} ${bin}
exit






bin="./rempi_test_master_worker"
librempi=../src/.libs/librempix.so
REMPI_MODE=${mode} REMPI_DIR=${dir} REMPI_ENCODE=7 REMPI_GZIP=0 REMPI_TEST_ID=1 LD_PRELOAD=${librempi} srun -n ${num_procs} ${bin}
#REMPI_MODE=${mode} REMPI_DIR=${dir} REMPI_ENCODE=4 REMPI_GZIP=1 REMPI_TEST_ID=1 LD_PRELOAD=${librempi} srun -n ${num_procs} ${bin}
exit


























par=1000
bin="../src/MCBenchmark-linux_x86_64.exe --nCores=1 --nThreadCore=1 --numParticles=$par --nZonesX=400 --nZonesY=400 --distributedSource --mirrorBoundary --sigmaA 1 --sigmaS 20 --weakScaling"
cd /g/g90/sato5/repo/MCBdouble/run-decks/
make cleanc
librempi=/g/g90/sato5/repo/rempi/src/.libs/librempix.so
REMPI_MODE=${mode} REMPI_DIR=${dir} REMPI_ENCODE=4 REMPI_GZIP=1 REMPI_TEST_ID=1 LD_PRELOAD=${librempi} srun ${io_watchdog} -n ${num_procs} ${bin}
cd -
exit










bin="./rempi_test_master_worker"
librempi=../src/.libs/librempix.so
REMPI_MODE=${mode} REMPI_DIR=${dir} REMPI_ENCODE=4 REMPI_GZIP=1 REMPI_TEST_ID=1 LD_PRELOAD=${librempi} srun -n ${num_procs} ${bin}
#REMPI_MODE=${mode} REMPI_DIR=${dir} REMPI_ENCODE=4 REMPI_GZIP=1 REMPI_TEST_ID=1 LD_PRELOAD=${librempi} srun -n ${num_procs} ${bin}
exit

exit



bin="./rempi_test_mini_mcb 3 0 20"
librempi=../src/.libs/librempix.so
REMPI_MODE=${mode} REMPI_DIR=${dir} REMPI_ENCODE=4 REMPI_GZIP=1 REMPI_TEST_ID=1 LD_PRELOAD=${librempi} srun -n ${num_procs} ${memcheck} ${bin}
exit







bin="./rempi_test_msg_race 0 1 10000 2 0"
REMPI_MODE=${mode} REMPI_DIR=${dir} REMPI_ENCODE=0 REMPI_GZIP=1 REMPI_TEST_ID=0 LD_PRELOAD=${librempi} srun --io-watchdog=conf=.io-watchdogrc -n ${num_procs} ${bin}
exit




#memcheck="memcheck"
#memcheck="memcheck --xml-file=/tmp/rempi.mc"



bin="./rempi_test_master_worker_fortran"
librempi="../lib/librempilite.so"
REMPI_MODE=${mode} REMPI_DIR=${dir} REMPI_ENCODE=0 REMPI_GZIP=1 REMPI_TEST_ID=0 LD_PRELOAD=${librempi} srun -n ${num_procs} ${bin}
exit


bin="./rempi_test_units matching"
librempi="../lib/librempi.so"
REMPI_MODE=$mode REMPI_DIR=${dir} REMPI_ENCODE=4 REMPI_GZIP=1 REMPI_TEST_ID=1 LD_PRELOAD=${librempi} srun -n ${num_procs} ${bin}
exit





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






