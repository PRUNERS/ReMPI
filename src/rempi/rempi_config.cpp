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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <iostream>

#include "rempi_err.h"
#include "rempi_config.h"

using namespace std;

int rempi_lite;
int rempi_mode = 0;
string rempi_record_dir_path = ".";
int rempi_encode;
int rempi_gzip;
int rempi_is_test_id;
int rempi_max_event_length;

void rempi_set_configuration(int *argc, char ***argv)
{
  char* env;
  int env_int;

#ifdef REMPI_LITE
  rempi_lite = 1;
#else
  rempi_lite = 0;
#endif

  if (NULL == (env = getenv(REMPI_ENV_NAME_MODE))) {
    rempi_dbg("getenv failed: Please specify REMPI_MODE (%s:%s:%d)", __FILE__, __func__, __LINE__);
    exit(0);
  }
  env_int = atoi(env);
  if (env_int == 0) {
    rempi_mode = 0;
  } else if (env_int == 1) {
    rempi_mode = 1;
  } else {
    rempi_dbg("Invalid value for REMPI_MODE: %s (%s:%s:%d)", env, __FILE__, __func__, __LINE__);
    exit(0);
  }

  if (NULL == (env = getenv(REMPI_ENV_NAME_DIR))) {
    // defoult: current dir
    rempi_record_dir_path =  ".";
  } else {
    rempi_record_dir_path = env;
    mkdir(env, S_IRWXU);
  }


  if (NULL == (env = getenv(REMPI_ENV_NAME_ENCODE))) {
    //    rempi_dbg("Apply default value (%d) for %s (%s:%s:%d)", rempi_encode, REMPI_ENV_NAME_ENCODE, __FILE__, __func__, __LINE__);
    rempi_encode = 0;
  } else {
    env_int = atoi(env);
    rempi_encode = env_int;
   }

  if (NULL == (env = getenv(REMPI_ENV_NAME_GZIP))) {
    //    rempi_dbg("Apply default value (%d) for %s (%s:%s:%d)", rempi_gzip, REMPI_ENV_NAME_GZIP, __FILE__, __func__, __LINE__);
    rempi_gzip = 0;
  } else {
    env_int = atoi(env);
    rempi_gzip = env_int;
  }

  if (NULL == (env = getenv(REMPI_ENV_NAME_TEST_ID))) {
    //    rempi_dbg("Apply default value (%d) for %s (%s:%s:%d)", rempi_is_test_id, REMPI_ENV_NAME_TEST_ID, __FILE__, __func__, __LINE__);
    rempi_is_test_id = 1;
  } else {
    env_int = atoi(env);
    rempi_is_test_id = env_int;
  }

  if (NULL == (env = getenv(REMPI_ENV_NAME_MAX_EVENT_LENGTH))) {
    rempi_max_event_length = REMPI_DEFAULT_MAX_EVENT_LENGTH;
  } else {
    env_int = atoi(env);
    rempi_max_event_length = env_int;
 }
  
  switch(rempi_encode) {
  case REMPI_ENV_REMPI_ENCODE_BASIC:
    //    REMPI_DBG("Ignoring %s", REMPI_ENV_NAME_TEST_ID);
    rempi_is_test_id = 0;
    break;
  case REMPI_ENV_REMPI_ENCODE_CDC:
    break;
  case REMPI_ENV_REMPI_ENCODE_REP:
    //    REMPI_DBG("Ignoring %s", REMPI_ENV_NAME_GZIP);
    rempi_gzip = 0;
    break;
  }

  // if (rempi_mode == 1 && rempi_is_test_id == 1) {
  //   REMPI_DBG("REMPI_MODE=1 & REMPI_IS_TEST_IS=1 is not supported yet");
  // }


  return;
}


