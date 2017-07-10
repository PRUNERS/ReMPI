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

num_procs=$1

dir=.rempi
librempi=../src/.libs/librempi.so

echo $REMPI_RUN
if [ -z $REMPI_RUN ]; then
    rempi_run=srun
else
    rempi_run=$REMPI_RUN
fi

# ===== Unit test fortran ======== 
bin="./rempi_test_units_fortran"
REMPI_MODE=0 REMPI_DIR=${dir} LD_PRELOAD=${librempi} $rempi_run -n ${num_procs} ${bin} 2> record.log
REMPI_MODE=1 REMPI_DIR=${dir} LD_PRELOAD=${librempi} $rempi_run -n ${num_procs} ${bin} 2> replay.log
$rempi_run rm -rf ${dir} 2> /dev/null

# ===== Unit test ======== 
#bin="./rempi_test_units"
#bin="./rempi_test_units matching probe isend init_sendrecv start null_status sendrecv_req comm_dup request_null zero_incount late_irecv clock_wait"
bin="./rempi_test_units matching probe isend init_sendrecv start sendrecv_req request_null zero_incount clock_wait"
REMPI_MODE=0 REMPI_DIR=${dir} LD_PRELOAD=${librempi} $rempi_run -n ${num_procs} ${bin} 2>> record.log
REMPI_MODE=1 REMPI_DIR=${dir} LD_PRELOAD=${librempi} $rempi_run -n ${num_procs} ${bin} 2>> replay.log
$rempi_run rm -rf ${dir} 2> /dev/null

# ===== Unit test 2======== 
bin="./rempi_test_units test_canceled"
REMPI_MODE=0 REMPI_DIR=${dir} LD_PRELOAD=${librempi} $rempi_run -n ${num_procs} ${bin} 2>> record.log
REMPI_MODE=1 REMPI_DIR=${dir} LD_PRELOAD=${librempi} $rempi_run -n ${num_procs} ${bin} 2>> replay.log
$rempi_run rm -rf ${dir} 2> /dev/null

grep "Global validation code" record.log > record_v.log
grep "Global validation code" replay.log > replay_v.log

diff_count=`diff record_v.log replay_v.log | wc -c`

if [ $diff_count -eq 0 ]; then
  echo "OK"
  cat record.log
  exit 0
else
  echo "ERROR"
  diff record.log replay.log
  exit 1
fi



