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
#include "rempi_mutex.h"
#include "rempi_err.h"
#include "rempi_config.h"

rempi_mutex::rempi_mutex()
{
  pthread_mutex_init(&mutex, 0);
}

rempi_mutex::~rempi_mutex()
{
  pthread_mutex_destroy(&mutex);
}

void rempi_mutex::lock()
{
#ifdef REMPI_DBG_REPLAY
  //  REMPI_DBGI(REMPI_DBG_REPLAY, ">>>>> try lock");
#endif
  pthread_mutex_lock(&mutex);
#ifdef REMPI_DBG_REPLAY
  //  REMPI_DBGI(REMPI_DBG_REPLAY, ">>>>> lock");
#endif
}

void rempi_mutex::unlock()
{
  pthread_mutex_unlock(&mutex);
#ifdef REMPI_DBG_REPLAY
  //  REMPI_DBGI(REMPI_DBG_REPLAY, "<<<<<< unlock");
#endif
}

bool rempi_mutex::trylock()
{
  return (pthread_mutex_trylock(&mutex) == 0);
}