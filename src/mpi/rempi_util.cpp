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
#include <unistd.h>
#include <sys/time.h>

#include "rempi_util.h"
#include "rempi_err.h"

double rempi_get_time(void)
{
  double t;
  struct timeval tv;
  gettimeofday(&tv, NULL);
  t = ((double)(tv.tv_sec) + (double)(tv.tv_usec) * 0.001 * 0.001);
  //  rempi_dbg(" -== > %f", t);
  return t;
}

void rempi_dbg_sleep_sec(int sec)
{
  rempi_dbg("Sleep: %d sec (%s:%s:%d)", sec, __FILE__, __func__, __LINE__);
  sleep(sec);
  return;
}
void rempi_sleep_sec(int sec)
{
  sleep(sec);
  return;
}

void rempi_dbg_sleep_usec(int usec)
{
  rempi_dbg("Sleep: %d usec (%s:%s:%d)\n", usec, __FILE__, __func__, __LINE__);
  usleep(usec);
  return;
}

void rempi_sleep_usec(int usec)
{
  usleep(usec);
  return;
}


static int hash = 837;
int rempi_compute_hash(void* buff, int size)
{ 
  char* buff_c = (char*)buff;
  for (int i = 0; i < size; i++) {
    hash += buff_c[i] * 7 + hash;
  }
  return hash;
}


unsigned int rempi_hash(unsigned int original_val, unsigned int new_val) {
  return ((original_val << 5) + original_val) + new_val;
}

