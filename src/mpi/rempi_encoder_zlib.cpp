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

#if MPI_VERSION == 3 && !defined(REMPI_LITE)
#include <vector>
#include <algorithm>

#include "rempi_err.h"
#include "rempi_event.h"
#include "rempi_encoder.h"
#include "rempi_config.h"
#include "rempi_clock_delta_compression.h"
#include "rempi_compression_util.h"

#define MAX_CDC_INPUT_FORMAT_LENGTH (1024 * 1024)


/* ==================================== */
/*      CLASS rempi_encoder_zlib         */
/* ==================================== */


rempi_encoder_zlib::rempi_encoder_zlib(int mode): rempi_encoder_cdc(mode)
{}


void rempi_encoder_zlib::compress_matched_events(rempi_encoder_input_format &input_format, rempi_encoder_input_format_test_table *test_table)
{
  char  *compressed_buff, *original_buff;
  size_t compressed_size,  original_size;
  vector<size_t> events_id_vec;

  for (int i = 0; i < test_table->events_vec.size(); i++) {
    size_t source, clock;
    source = test_table->events_vec[i]->get_source();
    clock  = test_table->events_vec[i]->get_clock();
    events_id_vec.push_back(source);
    events_id_vec.push_back(clock);
    //    REMPI_DBGI(0, "vall: %lu", source);
  }
  original_buff   = (char*)&events_id_vec[0];
  original_size   = events_id_vec.size() * sizeof(events_id_vec[0]);

  // compressed_buff = compression_util.compress_by_zlib(original_buff, original_size, compressed_size);
  // test_table->compressed_matched_events           = compressed_buff;
  // test_table->compressed_matched_events_size      = compressed_size;

  input_format.write_queue_vec.push_back((char*)&original_size);
  input_format.write_size_queue_vec.push_back(sizeof(original_size));
  input_format.write_queue_vec.push_back(original_buff);
  input_format.write_size_queue_vec.push_back(original_size);
  return;
}




vector<rempi_event*> rempi_encoder_zlib::decode(char *serialized_data, size_t *size)
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




