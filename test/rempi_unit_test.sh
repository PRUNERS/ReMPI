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

# ===== Unit test fortran ======== 
bin="./rempi_test_units_fortran"
REMPI_MODE=0 REMPI_DIR=${dir} LD_PRELOAD=${librempi} srun -n ${num_procs} ${bin}
REMPI_MODE=1 REMPI_DIR=${dir} LD_PRELOAD=${librempi} srun -n ${num_procs} ${bin}
srun rm -rf ${dir} 2> /dev/null

# ===== Unit test ======== 
bin="./rempi_test_units matching probe isend init_sendrecv start null_status sendrecv_req comm_dup request_null zero_incount late_irecv clock_wait"
#bin="./rempi_test_units"
REMPI_MODE=0 REMPI_DIR=${dir} LD_PRELOAD=${librempi} srun -n ${num_procs} ${bin}
REMPI_MODE=1 REMPI_DIR=${dir} LD_PRELOAD=${librempi} srun -n ${num_procs} ${bin}
srun rm -rf ${dir} 2> /dev/null

# ===== Unit test 2======== 
bin="./rempi_test_units test_canceled"
REMPI_MODE=0 REMPI_DIR=${dir} LD_PRELOAD=${librempi} srun -n ${num_procs} ${bin}
REMPI_MODE=1 REMPI_DIR=${dir} LD_PRELOAD=${librempi} srun -n ${num_procs} ${bin}
srun rm ${dir}/* 2> /dev/null



