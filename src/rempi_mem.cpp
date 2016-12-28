/* ==========================ReMPI:LICENSE==========================================   
   Copyright (c) 2016, Lawrence Livermore National Security, LLC.                     
   Produced at the Lawrence Livermore National Laboratory.                            
                                                                                       
   Written by Kento Sato, kento@llnl.gov. LLNL-CODE-711357.                           
   All rights reserved.                                                               
                                                                                       
   This file is part of ReMPI. For details, see https://github.com/PRUNER/ReMPI       
   Please also see the LICENSE file for our notice and the LGPL.                      
                                                                                       
   This program is free software; you can redistribute it and/or modify it under the   
   terms of the GNU General Public License (as published by the Free Software         
   Foundation) version 2.1 dated February 1999.                                       
                                                                                       
   This program is distributed in the hope that it will be useful, but WITHOUT ANY    
   WARRANTY; without even the IMPLIED WARRANTY OF MERCHANTABILITY or                  
   FITNESS FOR A PARTICULAR PURPOSE. See the terms and conditions of the GNU          
   General Public License for more details.                                           
                                                                                       
   You should have received a copy of the GNU Lesser General Public License along     
   with this program; if not, write to the Free Software Foundation, Inc., 59 Temple   
   Place, Suite 330, Boston, MA 02111-1307 USA
   ============================ReMPI:LICENSE========================================= */
#include <stdlib.h>
#include <stdio.h>

#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>

#include "rempi_err.h"

unsigned long total_alloc_size = 0;
unsigned long total_alloc_count = 0;

void* rempi_malloc(size_t size) 
{
  void* addr;
  if ((addr = malloc(size)) == NULL) {
    REMPI_DBG("Memory allocation returned NULL (%s:%s:%d)",  __FILE__, __func__, __LINE__);
    REMPI_ASSERT(0);
  }
  total_alloc_count++;

  //TODO: Manage memory consumption
  //  total_alloc_size += size;
  // pid_t tid;
  // tid = syscall(SYS_gettid);
  // REMPI_DBG("malloc: %d, size: %lu", tid, size);
  //  rempi_dbg("malloc: done %d", total_alloc_size);
  return addr;
}

void rempi_free(void* addr) 
{
  free(addr);
  total_alloc_count--;

  //TODO: Manage memory consumption
  //  total_alloc_size -= size;
  return;
}
