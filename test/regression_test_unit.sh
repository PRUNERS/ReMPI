#/bin/sh

num_procs=$1

dir=/l/ssd


librempi="../lib/librempilite.so"
bin=./rempi_test_units
REMPI_MODE=0 REMPI_DIR=${dir} REMPI_ENCODE=0 REMPI_GZIP=0 REMPI_TEST_ID=0 LD_PRELOAD=${librempi} srun -n ${num_procs} ${bin}
REMPI_MODE=1 REMPI_DIR=${dir} REMPI_ENCODE=0 REMPI_GZIP=0 REMPI_TEST_ID=0 LD_PRELOAD=${librempi} srun -n ${num_procs} ${bin}

