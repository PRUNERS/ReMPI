#/bin/sh
set -x

mode=$1
encode=$2
num_procs=$3

#bin=./rempi_test_neighbor
#bin=./rempi_test_neighbor-tree
#bin=./rempi_test_neighbor-collective
#bin=./rempi_test_units
#bin="./rempi_test_hypre_parasails 1000000"
#bin="./rempi_test_hypre_parasails"
#dir=./.rempi
#dir=/tmp
dir=/l/ssd
#srun rm ${dir}/* 2> /dev/null
#librempi=/g/g90/sato5/repo/rempi/lib/librempilite.so:/usr/local/tools/ic-14.0.174/lib/libintlc.so
librempi=/g/g90/sato5/repo/rempi/lib/librempi.so
#librempi=/g/g90/sato5/repo/rempi/lib/librempilite.so
#memcheck_all=memcheck #"memcheck --xml-file=/l/ssd/memcheck.mc" #memcheck_all
#io_watchdog="--io-watchdog"

#cp ./rempi_test_units /tmp/
#cd /tmp
REMPI_MODE=${mode} REMPI_DIR=${dir} REMPI_ENCODE=${encode} REMPI_GZIP=0 REMPI_TEST_ID=0 LD_PRELOAD=${librempi} srun ${io_watchdog} -n ${num_procs} ${memcheck_all} ${bin} 
exit


REMPI_MODE=$1 REMPI_DIR=./.rempi REMPI_ENCODE=4 REMPI_GZIP=1 REMPI_TEST_ID=0 LD_PRELOAD=/g/g90/sato5/opt/lib/libpnmpi.so PNMPI_CONF=./.pnmpi-conf-test PNMPI_LIB_PATH=/g/g90/sato5/opt/lib/pnmpi-modules/ srun -n $2 $bin
exit

