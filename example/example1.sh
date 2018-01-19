#!/bin/sh

rempi_record srun -n 4 ./a.out 
rempi_replay srun -n 4 ./a.out 

rempi_record REMPI_DIR=/tmp srun -n 4 ./a.out 
rempi_replay REMPI_DIR=/tmp srun -n 4 ./a.out 

rempi_record REMPI_DIR=/tmp REMPI_GZIP=1 srun -n 4 ./a.out 
rempi_replay REMPI_DIR=/tmp REMPI_GZIP=1 srun -n 4 ./a.out 

rempi_record srun -n 4 ./a.out 
rempi_replay totalview -args srun -n 4 ./a.out 

