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
/*      CLASS rempi_encoder_cdc_row_wise_diff         */
/* ============================================== */


rempi_encoder_cdc_row_wise_diff::rempi_encoder_cdc_row_wise_diff(int mode, string record_path): rempi_encoder_cdc(mode, record_path)
{}


void rempi_encoder_cdc_row_wise_diff::compress_matched_events(rempi_encoder_input_format &input_format, rempi_encoder_input_format_test_table *test_table)
{
  char  *compressed_buff, *original_buff;
  size_t compressed_size,  original_size;
  int num_of_recording_vals = 2;

  original_size = sizeof(size_t) * test_table->events_vec.size() * num_of_recording_vals;
  size_t *original_buff_int = (size_t*)malloc(original_size);
  map<int, rempi_event*>::iterator matched_events_ordered_map_it;
  int index = 0;
  for (matched_events_ordered_map_it  = test_table->matched_events_ordered_map.begin();
       matched_events_ordered_map_it != test_table->matched_events_ordered_map.end();
       matched_events_ordered_map_it++) {
    rempi_event *observed_event, *clock_ordered_event;
    observed_event = test_table->events_vec[index];
    clock_ordered_event = matched_events_ordered_map_it->second;
    bool is_source_same = observed_event->get_source() == clock_ordered_event->get_source();
    bool is_clock_same  = observed_event->get_clock()  == clock_ordered_event->get_clock();
    if (is_source_same && is_clock_same) {
      for (int i = 0; i < num_of_recording_vals; i++) {
	original_buff_int[index * num_of_recording_vals + i] = 0;
      }
    } else {
      original_buff_int[index * num_of_recording_vals + 0] = observed_event->get_source();
      original_buff_int[index * num_of_recording_vals + 1] = observed_event->get_clock();
    }
    //    REMPI_DBGI(0, "vall: %lu X %lu = %lu", test_table->events_vec[index]->get_source(), clock_ordered_event->get_source(), original_buff_int[index]);
    index++;
  }
  original_buff   = (char*)original_buff_int;
  compressed_buff = compression_util.compress_by_zlib(original_buff, original_size, compressed_size);
  if (compressed_buff == NULL && compressed_size != 0) {
    REMPI_ERR("zlib error");
  }
  test_table->compressed_matched_events           = (compressed_buff == NULL)? original_buff:compressed_buff;
  test_table->compressed_matched_events_size      = (compressed_buff == NULL)? original_size:compressed_size;
  REMPI_DBG("  matched(id): %lu bytes (<-zlib- %lu)", test_table->compressed_matched_events_size, original_size);

  return;
}



vector<rempi_event*> rempi_encoder_cdc_row_wise_diff::decode(char *serialized_data, size_t *size)
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



