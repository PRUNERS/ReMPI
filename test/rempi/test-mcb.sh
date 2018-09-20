#/bin/sh

#prefix=/g/g90/sato5/repo/rempi
#prefix=/tmp
#sprefix=/l/ssd
prefix=/p/lscratchf/sato5/rempi/

if [ $# -ne 2 ]; then
    echo "run.sh <mode> <nprocs>"
    exit 1
fi
mode=$1
num_procs=$2

dir=${prefix}/test/.rempi
mkdir -p ${dir}
#io_watchdog="--io-watchdog"
#librempi=/g/g90/sato5/repo/rempi/install/lib/librempi.so


# ===== MCB (OpenMP enabled) test ========
CORES_PER_NODE=16
THREADS_PER_CORE=2
NUM_NODES=$SLURM_NNODES
# NUM_PART is the total number of Monte Carlo particles summed
# over all processes and threads.
echo "$NUM_NODES nodes, $CORES_PER_NODE cores per node, $THREADS_PER_CORE threads per core"
PROCS_PER_NODE=`expr $num_procs / $NUM_NODES`
CORES_PER_PROC=`expr $CORES_PER_NODE / $PROCS_PER_NODE` # thread per process: 8
echo "$CORES_PER_PROC cores per process"
OMP_NUM_THREADS=`expr $CORES_PER_PROC \* $THREADS_PER_CORE`
TOTAL_NPROCS=`expr $PROCS_PER_NODE \* $NUM_NODES`
echo $TOTAL_NPROCS total processes
NUM_PART=`expr 10000 \* $CORES_PER_NODE \* $NUM_NODES`

cd /g/g90/sato5/repo/MCBdouble/run-decks/
make cleanc

export TSAN_OPTIONS="log_path=data_races.log history_size=7"
CODE="../src/MCBenchmark-linux_x86_64.exe"
srun -N $NUM_NODES -n $TOTAL_NPROCS --ntasks-per-node=$PROCS_PER_NODE --cpus-per-task=$CORES_PER_PROC $CODE --nCores=${CORES_PER_PROC} --nThreadCore=$THREADS_PER_CORE --numParticles=$NUM_PART --nZonesX=400 --nZonesY=400 
cd -
exit

# ===== MCB (OpenMP enabled) test ========
CORES_PER_NODE=16
THREADS_PER_CORE=1
NUM_NODES=$SLURM_NNODES
# NUM_PART is the total number of Monte Carlo particles summed
# over all processes and threads.
echo "$NUM_NODES nodes, $CORES_PER_NODE cores per node, $THREADS_PER_CORE threads per core"
PROCS_PER_NODE=`expr $num_procs / $NUM_NODES`
CORES_PER_PROC=`expr $CORES_PER_NODE / $PROCS_PER_NODE` # thread per process: 8
echo "$CORES_PER_PROC cores per process"
OMP_NUM_THREADS=`expr $CORES_PER_PROC \* $THREADS_PER_CORE`
TOTAL_NPROCS=`expr $PROCS_PER_NODE \* $NUM_NODES`
echo $TOTAL_NPROCS total processes
NUM_PART=`expr 10000 \* $CORES_PER_NODE \* $NUM_NODES`

cd /g/g90/sato5/repo/MCBdouble/run-decks/
make cleanc
CODE="../src/MCBenchmark-linux_x86_64.exe"
librempi=/g/g90/sato5/repo/rempi/src/.libs/librempi.so 
REMPI_MODE=${mode} REMPI_DIR=${dir} LD_PRELOAD=${librempi} srun -N $NUM_NODES -n $TOTAL_NPROCS --ntasks-per-node=$PROCS_PER_NODE --cpus-per-task=$CORES_PER_PROC $CODE --nCores=${CORES_PER_PROC} --nThreadCore=$THREADS_PER_CORE --numParticles=$NUM_PART --nZonesX=400 --nZonesY=400 
cd -
exit


# ===== MCB test ========
par=`expr 80 \* $num_procs`
bin="../src/MCBenchmark-linux_x86_64.exe --nCores=1 --nThreadCore=1 --numParticles=$par --nZonesX=400 --nZonesY=400 --distributedSource --mirrorBoundary --sigmaA 1 --sigmaS 20 "
cd /g/g90/sato5/repo/MCBdouble/run-decks/
make cleanc
#librempi=/g/g90/sato5/repo/rempi/src/.libs/librempi.so
#REMPI_MODE=${mode} REMPI_DIR=${dir} REMPI_ENCODE=0 REMPI_GZIP=1 REMPI_TEST_ID=0 LD_PRELOAD=${librempi} srun ${io_watchdog} -n ${num_procs} ${bin}
librempi=/g/g90/sato5/repo/rempi/src/.libs/librempix.so
REMPI_MODE=${mode} REMPI_DIR=${dir} REMPI_ENCODE=4 REMPI_GZIP=1 REMPI_TEST_ID=1 LD_PRELOAD=${librempi} srun ${io_watchdog} -n ${num_procs} ${bin}
#librempi=/g/g90/sato5/repo/rempi/src/.libs/librempix.so
#REMPI_MODE=${mode} REMPI_DIR=${dir} REMPI_ENCODE=7 REMPI_GZIP=1 REMPI_TEST_ID=1 LD_PRELOAD=${librempi} srun ${io_watchdog} -n ${num_procs} ${bin}
#srun ${io_watchdog} -n ${num_procs} ${bin}
cd -
exit







