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
#include <mpi.h>

#if MPI_VERSION == 3  && !defined(REMPI_LITE)

#include <string.h>

#include <vector>
#include <algorithm>

#include "rempi_err.h"
#include "rempi_event.h"
#include "rempi_encoder.h"
#include "rempi_config.h"
#include "rempi_clock_delta_compression.h"
#include "rempi_compression_util.h"

#define MAX_CDC_INPUT_FORMAT_LENGTH (1024 * 1024)


/* ============================================== */
/*      CLASS rempi_encoder_cdc_permutation_diff         */
/* ============================================== */


rempi_encoder_cdc_permutation_diff::rempi_encoder_cdc_permutation_diff(int mode, string record_path): rempi_encoder_cdc(mode, record_path)
{
  cdc = new rempi_clock_delta_compression_2(0);
}

void rempi_encoder_cdc_permutation_diff::compress_matched_events(rempi_encoder_input_format &input_format, rempi_encoder_input_format_test_table *test_table)
{
  char  *compressed_buff, *original_buff;
  size_t compressed_size,  original_size;
  int *cdc_buff;

  cdc_buff  = (int*)cdc->compress(
				 test_table->matched_events_ordered_map,
				 test_table->events_vec,
				 original_size);
  size_t original_buff_int_size = sizeof(unsigned int) * test_table->events_vec.size();
  int *original_buff_int = (int*)malloc(original_buff_int_size);
  memset(original_buff_int, 0, original_buff_int_size);

  int cdc_buff_id_length = original_size / sizeof(int) /2;
  // REMPI_DBG("cdc_buff_id_length  %d", cdc_buff_id_length);
  for (int i = 0; i < cdc_buff_id_length; i++) {
    int index = cdc_buff[i];
    int val   = cdc_buff[i + cdc_buff_id_length];
    original_buff_int[index] = val;
    //    REMPI_DBGI(0, "index: %d, val: %d", index, val);
  }
  free((char*)cdc_buff); //TODO: free
      
  original_buff = (char*)original_buff_int;
  original_size = original_buff_int_size;
  compressed_buff = compression_util.compress_by_zlib(original_buff, original_size, compressed_size);
  test_table->compressed_matched_events           = (compressed_buff == NULL)? original_buff:compressed_buff;
  test_table->compressed_matched_events_size      = (compressed_buff == NULL)? original_size:compressed_size;
  REMPI_DBG("  matched(id): %lu bytes (<-zlib- %lu)", test_table->compressed_matched_events_size, original_size);

  return;
}



vector<rempi_event*> rempi_encoder_cdc_permutation_diff::decode(char *serialized_data, size_t *size)
{
  vector<rempi_event*> vec;
  int *mpi_inputs;
  int i;

  /*In rempi_envoder, the serialized sequence identicals to one Test event */
  mpi_inputs = (int*)serialized_data;
  vec.push_back(
    rempi_event::create_test_event(mpi_inputs[0], mpi_inputs[1], mpi_inputs[2], 
			 mpi_inputs[3], mpi_inputs[4], mpi_inputs[5], mpi_inputs[6])
  );
  delete mpi_inputs;
  return vec;
}

#endif





