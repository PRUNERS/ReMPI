#include <mpi.h>
#if MPI_VERSION == 3 && !defined(REMPI_LITE)

#include <stdlib.h>
#include <string.h>

#include <vector>
#include <algorithm>
#include <unordered_set>

#include "rempi_err.h"
#include "rempi_mem.h"
#include "rempi_util.h"
#include "rempi_event.h"
#include "rempi_encoder.h"
#include "rempi_config.h"
#include "rempi_clock_delta_compression.h"
#include "rempi_compression_util.h"
#include "rempi_cp.h"



/* ==================================== */
/*      CLASS rempi_encoder_rep         */
/* ==================================== */



void rempi_encoder_rep::compress_non_matched_events(rempi_encoder_input_format &input_format, rempi_encoder_input_format_test_table *test_table)
{
  char *write_buff;
  size_t write_size;

  /*=== message count ===*/
  input_format.write_queue_vec.push_back((char*)&test_table->count);
  input_format.write_size_queue_vec.push_back(sizeof(test_table->count));
  /*=====================*/

  /*=== Compress Epoch   ===*/
  /* (0) size*/
  if (test_table->epoch_rank_vec.size() != test_table->epoch_clock_vec.size()) {
    REMPI_ERR("Epoch size is inconsistent");
  }
  test_table->epoch_size = test_table->epoch_rank_vec.size() * sizeof(test_table->epoch_rank_vec[0]);
  input_format.write_queue_vec.push_back((char*)&test_table->epoch_size);
  input_format.write_size_queue_vec.push_back(sizeof(test_table->epoch_size));
  /* (1) rank*/
  input_format.write_queue_vec.push_back((char*)&test_table->epoch_rank_vec[0]);
  input_format.write_size_queue_vec.push_back(test_table->epoch_size);
  /* (2) clock*/
  input_format.write_queue_vec.push_back((char*)&test_table->epoch_clock_vec[0]);
  input_format.write_size_queue_vec.push_back(test_table->epoch_size);
  /*====================================*/

  /*=== Compress with_next ===*/
  test_table->compressed_with_previous_length = 0;
  test_table->compressed_with_previous_size   = 0;
  test_table->compressed_with_previous        = NULL;
  input_format.write_queue_vec.push_back((char*)&test_table->compressed_with_previous_length);
  input_format.write_size_queue_vec.push_back(sizeof(test_table->compressed_with_previous_length));
  input_format.write_queue_vec.push_back((char*)&test_table->compressed_with_previous_size);
  input_format.write_size_queue_vec.push_back(sizeof(test_table->compressed_with_previous_size));
  /*====================================*/

  /*=== Compress unmatched_events ===*/
  /* (1) id */
  test_table->compressed_unmatched_events_id        = NULL;
  test_table->compressed_unmatched_events_id_size   = 0;
  input_format.write_queue_vec.push_back((char*)&test_table->compressed_unmatched_events_id_size);
  input_format.write_size_queue_vec.push_back(sizeof(test_table->compressed_unmatched_events_id_size));
  //  input_format.write_queue_vec.push_back(test_table->compressed_unmatched_events_id);
  //  input_format.write_size_queue_vec.push_back(test_table->compressed_unmatched_events_id_size);
  /* (2) count */
  /*   count will be totally random number, so this function does not do zlib*/
  test_table->compressed_unmatched_events_count        = NULL;
  test_table->compressed_unmatched_events_count_size   = 0;
  input_format.write_queue_vec.push_back((char*)&test_table->compressed_unmatched_events_count_size);
  input_format.write_size_queue_vec.push_back(sizeof(test_table->compressed_unmatched_events_count));
  //  input_format.write_queue_vec.push_back(test_table->compressed_unmatched_events_count);
  //  input_format.write_size_queue_vec.push_back(test_table->compressed_unmatched_events_count_size);
  /*=======================================*/
  return;
}


void rempi_encoder_rep::compress_matched_events(rempi_encoder_input_format &input_format, rempi_encoder_input_format_test_table *test_table)
{
  size_t compressed_size,  original_size;
  test_table->compressed_matched_events_size           = 0;
  input_format.write_queue_vec.push_back((char*)&test_table->compressed_matched_events_size);
  input_format.write_size_queue_vec.push_back(sizeof(test_table->compressed_matched_events_size));

  return;
}




// void rempi_encoder_rep::encode(rempi_encoder_input_format &input_format)
// { 
//   map<int, rempi_encoder_input_format_test_table*>::iterator test_tables_map_it;
//   for (test_tables_map_it  = input_format.test_tables_map.begin();
//        test_tables_map_it != input_format.test_tables_map.end();
//        test_tables_map_it++) {
//     rempi_encoder_input_format_test_table *test_table;
//     test_table = test_tables_map_it->second;
//     /*#################################################*/
//     /*     Encoding Section                            */
//     /*#################################################*/
//     /*=== message count ===*/    /*=== Compress with_next  ===*/  /*=== Compress unmatched_events ===*/
//     compress_non_matched_events(input_format, test_table);
//     /*=======================================*/
//   }
// return;
// }


#endif



