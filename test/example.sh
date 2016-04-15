#/bin/sh

num_procs=$1

# MCB
dir=/l/ssd
par=`expr 800 \* $num_procs`
librempi="../../../../lib/librempi.so"
bin="../src/MCBenchmark.exe --nCores=1 --nThreadCore=1 --numParticles=$par --nZonesX=400 --nZonesY=400 --distributedSource --mirrorBoundary --sigmaA 1 --sigmaS 20 "
cd ./external/mcb/run-decks/
REMPI_MODE=0 REMPI_DIR=${dir} REMPI_ENCODE=0 REMPI_GZIP=1 REMPI_TEST_ID=0 LD_PRELOAD=${librempi} srun -n ${num_procs} ${bin}
srun rm ${dir}/* 2> /dev/null

REMPI_MODE=0 REMPI_DIR=${dir} REMPI_ENCODE=1 REMPI_GZIP=1 REMPI_TEST_ID=0 LD_PRELOAD=${librempi} srun -n ${num_procs} ${bin}
srun rm ${dir}/* 2> /dev/null

REMPI_MODE=0 REMPI_DIR=${dir} REMPI_ENCODE=4 REMPI_GZIP=1 REMPI_TEST_ID=0 LD_PRELOAD=${librempi} srun -n ${num_procs} ${bin}
srun rm ${dir}/* 2> /dev/null

REMPI_MODE=0 REMPI_DIR=${dir} REMPI_ENCODE=4 REMPI_GZIP=1 REMPI_TEST_ID=1 LD_PRELOAD=${librempi} srun -n ${num_procs} ${bin}
#srun rm ${dir}/* 2> /dev/null

cd -