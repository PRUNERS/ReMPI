#!/bin/sh

mode=$1
num_procs=$2

dir=${prefix}/
mkdir -p ${dir}
totalview=totalview
#librempi=/g/g90/sato5/repo/WMPI/mpi_only_init/libmpi_init.so
librempi=/g/g90/sato5/repo/rempi/install_x86-64/lib/librempi.so

# ======== Totalview =================
bin="./rempi_test_units"

REMPI_MODE=${mode} \
REMPI_DIR=${prefix} \
REMPI_ENCODE=0 \
#${totalview} srun -a -n ${num_procs} ${bin}
${totalview} -e "export LD_PRELOAD=${librempi}" srun -a -n ${num_procs} ${bin}
#${totalview} -args env LD_PRELOAD=${librempi} srun -n ${num_procs} ${bin}
exit




