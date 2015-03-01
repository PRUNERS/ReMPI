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


rempi_encoder_cdc_row_wise_diff::rempi_encoder_cdc_row_wise_diff(int mode): rempi_encoder_cdc(mode)
{}


void rempi_encoder_cdc_row_wise_diff::compress_matched_events(rempi_encoder_input_format_test_table *test_table)
{
  char  *compressed_buff, *original_buff;
  size_t compressed_size,  original_size;

  original_size = sizeof(size_t) * test_table->events_vec.size();
  size_t *original_buff_int = (size_t*)malloc(original_size);
  map<int, rempi_event*>::iterator matched_events_ordered_map_it;
  int index = 0;
  for (matched_events_ordered_map_it  = test_table->matched_events_ordered_map.begin();
       matched_events_ordered_map_it != test_table->matched_events_ordered_map.end();
       matched_events_ordered_map_it++) {
    rempi_event *observed_event, *clock_ordered_event;
    observed_event = test_table->events_vec[index];
    clock_ordered_event = matched_events_ordered_map_it->second;
    if (observed_event->get_source() != clock_ordered_event->get_source()) {
      original_buff_int[index] = observed_event->get_source() + 1;
    } else {
      original_buff_int[index] = 0;
    }
    REMPI_DBGI(0, "vall: %lu X %lu = %lu", test_table->events_vec[index]->get_source(), clock_ordered_event->get_source(), original_buff_int[index]);
    index++;
  }

  original_buff   = (char*)original_buff_int;
  compressed_buff = compression_util.compress_by_zlib(original_buff, original_size, compressed_size);
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
    new rempi_test_event(mpi_inputs[0], mpi_inputs[1], mpi_inputs[2], 
			 mpi_inputs[3], mpi_inputs[4], mpi_inputs[5])
  );
  delete mpi_inputs;
  return vec;
}





