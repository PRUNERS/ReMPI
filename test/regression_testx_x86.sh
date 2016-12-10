#/bin/sh

num_procs=$1

dir=./rempi_record
librempi=/g/g90/sato5/repo/rempi/src/.libs/librempix.so
mkdir -p $dir

bin="./rempi_test_units matching"
REMPI_MODE=0 REMPI_DIR=${dir} REMPI_ENCODE=4 REMPI_GZIP=1 REMPI_TEST_ID=1 LD_PRELOAD=${librempi} srun -n ${num_procs} ${bin}
REMPI_MODE=1 REMPI_DIR=${dir} REMPI_ENCODE=4 REMPI_GZIP=1 REMPI_TEST_ID=1 LD_PRELOAD=${librempi} srun -n ${num_procs} ${bin}                           
srun rm ${dir}/* 2> /dev/null

# Master worker w/ gzip
bin="./rempi_test_master_worker"
REMPI_MODE=0 REMPI_DIR=${dir} REMPI_ENCODE=4 REMPI_GZIP=1 REMPI_TEST_ID=1 LD_PRELOAD=${librempi} srun -n ${num_procs} ${bin}
REMPI_MODE=1 REMPI_DIR=${dir} REMPI_ENCODE=4 REMPI_GZIP=1 REMPI_TEST_ID=1 LD_PRELOAD=${librempi} srun -n ${num_procs} ${bin}
srun rm ${dir}/* 2> /dev/null

bin="./rempi_test_master_worker"
REMPI_MODE=0 REMPI_DIR=${dir} REMPI_ENCODE=7 REMPI_GZIP=1 REMPI_TEST_ID=1 LD_PRELOAD=${librempi} srun -n ${num_procs} ${bin}
REMPI_MODE=1 REMPI_DIR=${dir} REMPI_ENCODE=7 REMPI_GZIP=1 REMPI_TEST_ID=1 LD_PRELOAD=${librempi} srun -n ${num_procs} ${bin}                   
srun rm ${dir}/* 2> /dev/null

# MCB w/ gzip
par=`expr 800 \* $num_procs`
bin="../src/MCBenchmark-linux_x86_64.exe --nCores=1 --nThreadCore=1 --numParticles=$par --nZonesX=400 --nZonesY=400 --distributedSource --mirrorBoundary --sigmaA 1 --sigmaS 20 "
cd ../../MCBdouble/run-decks/
REMPI_MODE=0 REMPI_DIR=${dir} REMPI_ENCODE=4 REMPI_GZIP=1 REMPI_TEST_ID=0 LD_PRELOAD=${librempi} srun -n ${num_procs} ${bin}
REMPI_MODE=1 REMPI_DIR=${dir} REMPI_ENCODE=4 REMPI_GZIP=1 REMPI_TEST_ID=0 LD_PRELOAD=${librempi} srun -n ${num_procs} ${bin}
srun rm ${dir}/* 2> /dev/null
cd -

# MCB w/ gzip
cd ../../MCBdouble/run-decks/
REMPI_MODE=0 REMPI_DIR=${dir} REMPI_ENCODE=4 REMPI_GZIP=1 REMPI_TEST_ID=1 LD_PRELOAD=${librempi} srun -n ${num_procs} ${bin}
REMPI_MODE=1 REMPI_DIR=${dir} REMPI_ENCODE=4 REMPI_GZIP=1 REMPI_TEST_ID=1 LD_PRELOAD=${librempi} srun -n ${num_procs} ${bin}
srun rm ${dir}/* 2> /dev/null
cd -

cd ../../MCBdouble/run-decks/
REMPI_MODE=0 REMPI_DIR=${dir} REMPI_ENCODE=7 REMPI_GZIP=1 REMPI_TEST_ID=1 LD_PRELOAD=${librempi} srun -n ${num_procs} ${bin}
REMPI_MODE=1 REMPI_DIR=${dir} REMPI_ENCODE=7 REMPI_GZIP=1 REMPI_TEST_ID=1 LD_PRELOAD=${librempi} srun -n ${num_procs} ${bin}
srun rm ${dir}/* 2> /dev/null
cd -

