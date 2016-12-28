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
#include "rempi_thread.h"
#include "rempi_config.h"
#include "rempi_err.h"

rempi_thread::~rempi_thread()
{}

void * rempi_thread::dispatch(void * ptr)
{
  if (!ptr) {
    REMPI_ERR("thread object failed: ptr = %p", ptr);
    return 0;
  }
  static_cast<rempi_thread *>(ptr)->run();
  pthread_exit(ptr);
  return 0;
}

void rempi_thread::start()
{
  int ret;
  ret = pthread_create(&thread, 0, rempi_thread::dispatch, this);
  if (ret != 0) REMPI_ERR("pthread_create failed: ret = %d", ret);
}

void rempi_thread::join()
{
  pthread_join(thread, 0);
}
