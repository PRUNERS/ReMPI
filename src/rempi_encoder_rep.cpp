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



