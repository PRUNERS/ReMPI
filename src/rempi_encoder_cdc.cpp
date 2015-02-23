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
/* CLASS rempi_encoder_cdc_input_format */
/* ==================================== */



// bool rempi_encoder_cdc_input_format::compare(
// 					     rempi_event* event1,
// 					     rempi_event* event2)
// {
//   if (event1->get_clock() < event2->get_clock()) {
//     return true;
//   } else if (event1->get_clock() == event2->get_clock()) {
//     return event1->get_source() < event2->get_source();
//   }
//   return false;
// }

bool compare(
	     rempi_event* event1,
	     rempi_event* event2)
{
  if (event1->get_clock() < event2->get_clock()) {
    return true;
  } else if (event1->get_clock() == event2->get_clock()) {
    return event1->get_source() < event2->get_source();
  }
  return false;
}

void rempi_encoder_cdc_input_format::add(rempi_event *event)
{
  bool is_matched_event;
  is_matched_event = (event->get_flag() == 1);
  if (is_matched_event) {
    events_vec.push_back(event);
    if (event->get_is_testsome()) {
      int added_event_index;
      added_event_index = events_vec.size() - 1;
      with_previous_vec.push_back(added_event_index);
    }
    count++;
  } else {
    int next_index_added_event_index;
    next_index_added_event_index = events_vec.size();
    unmatched_events_id_vec.push_back(next_index_added_event_index);
    unmatched_events_count_vec.push_back(event->get_event_counts());
  }
  return;
}

void rempi_encoder_cdc_input_format::format()
{
  vector<rempi_event*> sorted_events_vec(events_vec);
  sort(sorted_events_vec.begin(), sorted_events_vec.end(), compare);
  for (int i = 0; i < sorted_events_vec.size(); i++) {
    sorted_events_vec[i]->clock_order = i;
    matched_events_ordered_map.insert(make_pair(i, sorted_events_vec[i]));
  }
  //  debug_print();
  return;
}

void rempi_encoder_cdc_input_format::debug_print()
{
  REMPI_DBG(  "        count: %d", count);
  for (int i = 0; i < with_previous_vec.size(); i++) {
    REMPI_DBG("with_previous: id:%d val:%d", i, with_previous_vec[i]);
  }
  for (int i = 0; i < unmatched_events_id_vec.size(); i++) {
    REMPI_DBG("    unmatched: id: %d count: %d", unmatched_events_id_vec[i], unmatched_events_count_vec[i]);
  }
  for (int i = 0; i < events_vec.size(); i++) {
    REMPI_DBG("      matched: id: %d  source: %d , clock %d", i, events_vec[i]->get_source(), events_vec[i]->get_clock());
  }
  return;
}

/* ==================================== */
/*      CLASS rempi_encoder_cdc         */
/* ==================================== */


rempi_encoder_cdc::rempi_encoder_cdc(int mode): rempi_encoder(mode)
{}


/*  ==== 3 Step compression ==== 
1. rempi_encoder_cdc.get_encoding_event_sequence(...): [rempi_event_list => rempi_encoding_structure]
     this function translate sequence of event(rempi_event_list) 
     into a data format class(rempi_encoding_structure) whose format 
     is sutable for compression(rempi_encoder_cdc.encode(...))
   

2. repmi_encoder.encode(...): [rempi_encoding_structure => char*]
     encode, i.e., rempi_encoding_structure -> char*
     
3. write_record_file: [char* => "file"]
     write the data(char*)
 =============================== */


bool rempi_encoder_cdc::extract_encoder_input_format_chunk(rempi_event_list<rempi_event*> &events, rempi_encoder_input_format &input_format)
{
  rempi_event *event_dequeued;
  bool is_ready_for_encoding = false;


  while (1) {
    /*Append events to current check as many as possible*/
    if (events.front() == NULL) break;
    bool is_clock_less_than_gmc = (events.front()->get_clock() < events.get_globally_minimal_clock());
    bool is_unmatched           = (events.front()->get_flag() == 0);
    if (is_clock_less_than_gmc || is_unmatched) {
      event_dequeued = events.pop();
      input_format.add(event_dequeued);
      //      event_dequeued->print();
      //      fprintf(stderr, "\n");
    } else {
      break;
    }
  }

  if (input_format.length() == 0) {
    return is_ready_for_encoding;
  }

  if (input_format.length() > MAX_CDC_INPUT_FORMAT_LENGTH || events.is_push_closed_()) {
    /*If got enough chunck size, OR the end of run*/
    input_format.format();
    is_ready_for_encoding = true;
  }

  return is_ready_for_encoding;
}


void rempi_encoder_cdc::encode(rempi_encoder_input_format &input_format)
{
  char  *compressed_buff, *original_buff;
  size_t compressed_size,  original_size;
  
  /*#################################################*/
  /*     Encoding Section                            */
  /*#################################################*/

  /*=== message count ===*/
  /*   Nothing to do     */
  /*=====================*/

  /*=== Compress with_previous ===*/
  input_format.compressed_with_previous = compression_util.compress_by_zero_one_binary(input_format.with_previous_vec, compressed_size);
  input_format.compressed_with_previous_size   = compressed_size;
  input_format.compressed_with_previous_length = input_format.with_previous_vec.size();
  /*====================================*/

  /*=== Compress unmatched_events ===*/
  /* (1) id */
  compression_util.compress_by_linear_prediction(input_format.unmatched_events_id_vec);
  original_buff   = (char*)&input_format.unmatched_events_id_vec[0];
  original_size   = input_format.unmatched_events_id_vec.size() * sizeof(input_format.unmatched_events_id_vec[0]);
  compressed_buff = compression_util.compress_by_zlib(original_buff, original_size, compressed_size);
  /* If compressed data is bigger than oroginal data, use original data*/    
  input_format.compressed_unmatched_events_id           = (compressed_buff == NULL)? original_buff:compressed_buff;
  input_format.compressed_unmatched_events_id_size      = (compressed_buff == NULL)? original_size:compressed_size;
  REMPI_DBG("unmatched(id): zlib %lu to %lu", original_size, compressed_size);
  /* (2) count */
  /*   count will be totally random number, so this function does not do zlib*/
  input_format.compressed_unmatched_events_count      = (char*)&input_format.unmatched_events_count_vec[0];
  input_format.compressed_unmatched_events_count_size = input_format.unmatched_events_count_vec.size() * sizeof(input_format.unmatched_events_count_vec[0]);
  /*=======================================*/


  /*=== Compress matched_events ===*/
  original_buff = rempi_clock_delta_compression::compress(
							  input_format.matched_events_ordered_map, 
							  input_format.events_vec, 
							  original_size);
  if (original_size > 0) {
    compressed_buff = compression_util.compress_by_zlib(original_buff, original_size, compressed_size);
    input_format.compressed_matched_events           = (compressed_buff == NULL)? original_buff:compressed_buff;
    input_format.compressed_matched_events_size      = (compressed_buff == NULL)? original_size:compressed_size;
  }  
  REMPI_DBG("      matched: zlib %lu to %lu", original_size, compressed_size);
  /*================================*/
  return;
}

void rempi_encoder_cdc::write_record_file(rempi_encoder_input_format &input_format)
{
  size_t compressed_size = 0;
  //  REMPI_DBG("size: %lu", sizeof(input_format.count));
  /* Count */
  record_fs.write((char*)&input_format.count, sizeof(input_format.count));
  compressed_size += sizeof(input_format.count);


  /* with_previous */
  record_fs.write(   (char*)&input_format.compressed_with_previous_size, 
		      sizeof(input_format.compressed_with_previous_size));
  compressed_size +=  sizeof(input_format.compressed_with_previous_size);
  record_fs.write(           input_format.compressed_with_previous, 
		             input_format.compressed_with_previous_size);
  compressed_size +=         input_format.compressed_with_previous_size;


  /* unmatched */
  record_fs.write(  (char*)&input_format.compressed_unmatched_events_id_size, 
		     sizeof(input_format.compressed_unmatched_events_id_size));
  compressed_size += sizeof(input_format.compressed_unmatched_events_id_size);  
  record_fs.write(          input_format.compressed_unmatched_events_id,    
			    input_format.compressed_unmatched_events_id_size);
  compressed_size +=        input_format.compressed_unmatched_events_id_size;

  record_fs.write(  (char*)&input_format.compressed_unmatched_events_count_size, 
		     sizeof(input_format.compressed_unmatched_events_count_size));
  compressed_size += sizeof(input_format.compressed_unmatched_events_count_size);  
  record_fs.write(          input_format.compressed_unmatched_events_count, 
			    input_format.compressed_unmatched_events_count_size);
  compressed_size +=        input_format.compressed_unmatched_events_count_size;


  /* matched */
  record_fs.write(  (char*)&input_format.compressed_matched_events_size, 
		     sizeof(input_format.compressed_matched_events_size));
  compressed_size += sizeof(input_format.compressed_matched_events_size);  
  record_fs.write(          input_format.compressed_matched_events, 
			    input_format.compressed_matched_events_size);
  compressed_size +=        input_format.compressed_matched_events_size;
  //TODO: delete encoded_event
  REMPI_DBG("Original size: count:%5d x 8 bytes x 2(matched/unmatched)= %10d bytes,  Compressed size: %10lu bytes", 
	    input_format.count, input_format.count * 8 * 2, compressed_size);
}   


vector<rempi_event*> rempi_encoder_cdc::decode(char *serialized_data, size_t *size)
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





