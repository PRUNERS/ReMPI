#!/bin/bash


bin="./reomp_test_units_native"

function usage {
    echo "./run_reomp.sh <# of threads per proc> <test case> <ReOMP mode> <ReOMP dir> <ReOMP method> <ReOMP multi-clock>"
}

if [ $# -eq 0 ]; then
    usage
    exit 1
fi

srun="srun -n 1"


if [ $# -ge 1 ]; then
    export OMP_NUM_THREADS=$1
#        option="$OMP_NUM_THREADS 0.2"
    option="$OMP_NUM_THREADS 1"
fi

if [ $# -ge 2 ]; then
    test_case=$2
fi

if [ $# -ge 3 ]; then
    if [ $# -ne 6 ]; then
	usage
	echo $#
	exit 1
    fi
    export REOMP_MODE=$3
    export LD_PRELOAD=../../src/reomp/.libs/libreomp.so
    bin="./reomp_test_units"
fi

if [ $# -ge 3 ]; then
    export REOMP_DIR="$4"
    export REOMP_METHOD="$5"
    export REOMP_MULTI_CLOCK="$6"
fi

#export CALI_SERVICES_ENABLE=pthread:event:trace:recorder:libpfm:symbollookup
#export CALI_RECORDER_FILENAME=test.cali
#export CALI_LIBPFM_EVENT_LIST=MEM_TRANS_RETIRED:LATENCY_ABOVE_THRESHOLD
#export CALI_LIBPFM_PERIOD=1000 #1000(best)
#export CALI_LIBPFM_PRECISE_IP=2 #2(best)
#export CALI_LIBPFM_CONFIG1=5 #latency #5 (best)
#export CALI_LIBPFM_SAMPLE_ATTRIBUTES=ip,id,stream_id,time,tid,cpu,period,transaction,addr,weight,data_src

set -x
time -p $srun $bin $option $test_case

