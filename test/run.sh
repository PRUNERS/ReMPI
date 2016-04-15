#!/bin/sh
REMPI_MODE=0 REMPI_DIR=/tmp REMPI_ENCODE=0 REMPI_GZIP=0 REMPI_TEST_ID=0 LD_PRELOAD=/g/g90/sato5/repo/rempi/lib/librempilite.so srun -n 64  --io-watchdog --ntasks=64 --nodes=4-4 --ntasks-per-node=16 ./rempi_test_hypre_parasails
#srun -n 64 --ntasks=64 --nodes=4-4 --ntasks-per-node=16 ./rempi_test_hypre_parasails