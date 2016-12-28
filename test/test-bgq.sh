#!/bin/sh
####################################################################################
# Copyright (c) 2016, Lawrence Livermore National Security, LLC.                     
# Produced at the Lawrence Livermore National Laboratory.                            
#                                                                                    
# Written by Kento Sato, kento@llnl.gov. LLNL-CODE-711357.                           
# All rights reserved.                                                               
#                                                                                    
# This file is part of ReMPI. For details, see https://github.com/PRUNER/ReMPI       
# Please also see the LICENSE file for our notice and the LGPL.                      
#                                                                                    
# This program is free software; you can redistribute it and/or modify it under the 
# terms of the GNU General Public License (as published by the Free Software         
# Foundation) version 2.1 dated February 1999.                                       
#                                                                                    
# This program is distributed in the hope that it will be useful, but WITHOUT ANY    
# WARRANTY; without even the IMPLIED WARRANTY OF MERCHANTABILITY or                  
# FITNESS FOR A PARTICULAR PURPOSE. See the terms and conditions of the GNU          
# General Public License for more details.                                           
#                                                                                    
# You should have received a copy of the GNU Lesser General Public License along     
# with this program; if not, write to the Free Software Foundation, Inc., 59 Temple 
# Place, Suite 330, Boston, MA 02111-1307 USA                                
####################################################################################

export BG_COREDUMPBINARY="*"

#prefix=/g/g90/sato5/repo/rempi
prefix=/p/lscratchv/sato5/

mode=$1
num_procs=$2


dir=${prefix}/test/.rempi
#dir=/tmp/
#mkdir ${dir}/*

#io_watchdog="--io-watchdog"
#memcheck="memcheck  --xml-file=/tmp/unit.cab687.0.mc"


par=1000
bin="../src/MCBenchmark.exe --nCores=1 --nThreadCore=1 --numParticles=$par --nZonesX=400 --nZonesY=400 --distributedSource --weakScaling"
#bin="../src/MCBenchmark.exe --nCores=1 --nThreadCore=1 --numParticles=$par --nZonesX=400 --nZonesY=400 --distributedSource --mirrorBoundary --sigmaA 1 --sigmaS 20"
cd ../../MCBdouble/run-decks/
make cleanc
REMPI_MODE=${mode} REMPI_DIR=${dir} REMPI_ENCODE=7 REMPI_GZIP=0 REMPI_TEST_ID=1 REMPI_MAX=10000000 LD_PRELOAD=${librempi} srun -n ${num_procs} ${bin}
#REMPI_MODE=${mode} REMPI_DIR=${dir} REMPI_ENCODE=4 REMPI_GZIP=1 REMPI_TEST_ID=1 REMPI_MAX=10000000 LD_PRELOAD=${librempi} srun -n ${num_procs} ${bin}
#REMPI_MODE=${mode} REMPI_DIR=${dir} REMPI_ENCODE=4 REMPI_GZIP=1 REMPI_TEST_ID=1 REMPI_MAX=50 LD_PRELOAD=${librempi} srun -n ${num_procs} ${bin}
cd -
exit


bin="./rempi_test_master_workerx"
REMPI_MODE=${mode} REMPI_DIR=${dir} REMPI_ENCODE=7 REMPI_GZIP=1 REMPI_TEST_ID=1 REMPI_MAX=10000000 srun -n ${num_procs} ${bin}
exit





bin="./rempi_test_unitsx matching"
REMPI_MODE=${mode} REMPI_DIR=${dir} REMPI_ENCODE=4 REMPI_GZIP=1 REMPI_TEST_ID=1 srun -n ${num_procs} ${memcheck} ${bin}
exit








bin="./rempi_test_mini_mcb 10 0 1000"
REMPI_MODE=${mode} REMPI_DIR=${dir} REMPI_ENCODE=0 REMPI_GZIP=1 REMPI_TEST_ID=1 REMPI_MAX=10000000 LD_PRELOAD=${librempi} srun -n ${num_procs} ${bin}
#REMPI_MODE=${mode} REMPI_DIR=${dir} REMPI_ENCODE=4 REMPI_GZIP=1 REMPI_TEST_ID=1 REMPI_MAX=10000000 LD_PRELOAD=${librempi} srun -n ${num_procs} ${bin}
exit



























