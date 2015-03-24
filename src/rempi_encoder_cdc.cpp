#include <stdlib.h>
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
  rempi_encoder_input_format_test_table *test_table;
  is_matched_event = (event->get_flag() == 1);
  if (test_tables_map.find(event->get_test_id()) == test_tables_map.end()) {
    test_tables_map[event->get_test_id()] = new rempi_encoder_input_format_test_table();
  }
  test_table = test_tables_map[event->get_test_id()];

  if (is_matched_event) {
    test_table->events_vec.push_back(event);
    if (event->get_is_testsome()) {
      int added_event_index;
      added_event_index = test_table->events_vec.size() - 1;
      test_table->with_previous_vec.push_back(added_event_index);
    }
    test_table->count++;
    total_length++;
  } else {
    int next_index_added_event_index;
    bool not_empty;
    next_index_added_event_index = test_table->events_vec.size();
    not_empty = test_table->unmatched_events_id_vec.size() > 0;
    if (not_empty && test_table->unmatched_events_id_vec.back() == next_index_added_event_index) {
      test_table->unmatched_events_count_vec.back() += 1;
    } else {
      test_table->unmatched_events_id_vec.push_back(next_index_added_event_index);
      test_table->unmatched_events_count_vec.push_back(event->get_event_counts());
      total_length++;
    }
  }

  return;
}

void rempi_encoder_cdc_input_format::format()
{
  map<int, rempi_encoder_input_format_test_table*>::iterator test_tables_map_it;
  for (test_tables_map_it  = test_tables_map.begin();
       test_tables_map_it != test_tables_map.end();
       test_tables_map_it++) {  
    rempi_encoder_input_format_test_table *test_table;
    test_table = test_tables_map_it->second;

    vector<rempi_event*> sorted_events_vec(test_table->events_vec);
    sort(sorted_events_vec.begin(), sorted_events_vec.end(), compare);
    for (int i = 0; i < sorted_events_vec.size(); i++) {
      sorted_events_vec[i]->clock_order = i;
      test_table->matched_events_ordered_map.insert(make_pair(i, sorted_events_vec[i]));
    }
  }
  //  debug_print();
  return;
}

void rempi_encoder_cdc_input_format::debug_print()
{
  map<int, rempi_encoder_input_format_test_table*>::iterator test_tables_map_it;
  for (test_tables_map_it  = test_tables_map.begin();
       test_tables_map_it != test_tables_map.end();
       test_tables_map_it++) {  
    int test_id;
    rempi_encoder_input_format_test_table *test_table;
    test_id    = test_tables_map_it->first;
    REMPI_DBG(  " ====== Test_id: %2d ======", test_id);

    test_table = test_tables_map_it->second;

    REMPI_DBG(  "        count: %d", test_table->count);
    for (int i = 0; i < test_table->with_previous_vec.size(); i++) {
      REMPI_DBG("with_previous: id:%d val:%d", i, test_table->with_previous_vec[i]);
    }
    for (int i = 0; i < test_table->unmatched_events_id_vec.size(); i++) {
      REMPI_DBG("    unmatched: id: %d count: %d", test_table->unmatched_events_id_vec[i], test_table->unmatched_events_count_vec[i]);
    }
    for (int i = 0; i < test_table->events_vec.size(); i++) {
      REMPI_DBG("      matched: id: %d  source: %d , clock %d", i, test_table->events_vec[i]->get_source(), test_table->events_vec[i]->get_clock());
    }

  }
  return;

}

/* ==================================== */
/*      CLASS rempi_encoder_cdc         */
/* ==================================== */


rempi_encoder_cdc::rempi_encoder_cdc(int mode): rempi_encoder(mode)
{
  cdc = new rempi_clock_delta_compression(1);
}

rempi_encoder_input_format* rempi_encoder_cdc::create_encoder_input_format()
{
  return new rempi_encoder_cdc_input_format();
}


void rempi_encoder_cdc::compress_non_matched_events(rempi_encoder_input_format &input_format, rempi_encoder_input_format_test_table *test_table)
{
  char  *compressed_buff, *original_buff;
  size_t compressed_size,  original_size;
    /*=== message count ===*/
    /*   Nothing to do     */
    /*=====================*/

    /*=== Compress with_previous ===*/
    test_table->compressed_with_previous = compression_util.compress_by_zero_one_binary(test_table->with_previous_vec, compressed_size);
    test_table->compressed_with_previous_size   = compressed_size;
    test_table->compressed_with_previous_length = test_table->with_previous_vec.size();

    input_format.write_queue_vec.push_back((char*)&test_table->compressed_with_previous_length);
    input_format.write_size_queue_vec.push_back(sizeof(test_table->compressed_with_previous_length));
    input_format.write_queue_vec.push_back((char*)&test_table->compressed_with_previous_size);
    input_format.write_size_queue_vec.push_back(sizeof(test_table->compressed_with_previous_size));
    input_format.write_queue_vec.push_back((char*)test_table->compressed_with_previous);
    input_format.write_size_queue_vec.push_back(test_table->compressed_with_previous_size);

    // REMPI_DBG("with_previous: %lu bytes (length: %lu)", 
    // 	      test_table->compressed_with_previous_size, 
    // 	      test_table->compressed_with_previous_length);
    /*====================================*/


    /*=== Compress unmatched_events ===*/
    /* (1) id */
    compression_util.compress_by_linear_prediction(test_table->unmatched_events_id_vec);
    test_table->compressed_unmatched_events_id        = (char*)&test_table->unmatched_events_id_vec[0];
    test_table->compressed_unmatched_events_id_size   = test_table->unmatched_events_id_vec.size() * sizeof(test_table->unmatched_events_id_vec[0]);
    input_format.write_queue_vec.push_back((char*)&test_table->compressed_unmatched_events_id_size);
    input_format.write_size_queue_vec.push_back(sizeof(test_table->compressed_unmatched_events_id_size));
    input_format.write_queue_vec.push_back(test_table->compressed_unmatched_events_id);
    input_format.write_size_queue_vec.push_back(test_table->compressed_unmatched_events_id_size);
    
    // compressed_buff = compression_util.compress_by_zlib(original_buff, original_size, compressed_size);
    // /* If compressed data is bigger than oroginal data, use original data*/    
    // test_table->compressed_unmatched_events_id           = (compressed_buff == NULL)? original_buff:compressed_buff;
    // test_table->compressed_unmatched_events_id_size      = (compressed_buff == NULL)? original_size:compressed_size;
    //REMPI_DBG("unmatched(id): %lu bytes (<-zlib- %lu)", test_table->compressed_unmatched_events_id_size, original_size);

    /* (2) count */
    /*   count will be totally random number, so this function does not do zlib*/
    test_table->compressed_unmatched_events_count        = (char*)&test_table->unmatched_events_count_vec[0];
    test_table->compressed_unmatched_events_count_size   = test_table->unmatched_events_count_vec.size() * sizeof(test_table->unmatched_events_count_vec[0]);
    input_format.write_queue_vec.push_back((char*)&test_table->compressed_unmatched_events_count_size);
    input_format.write_size_queue_vec.push_back(sizeof(test_table->compressed_unmatched_events_count));
    input_format.write_queue_vec.push_back(test_table->compressed_unmatched_events_count);
    input_format.write_size_queue_vec.push_back(test_table->compressed_unmatched_events_count_size);
    
    //    compressed_buff = compression_util.compress_by_zlib(original_buff, original_size, compressed_size);
    /* If compressed data is bigger than oroginal data, use original data*/    
    //    test_table->compressed_unmatched_events_count      = (compressed_buff == NULL)? original_buff:compressed_buff;
    //    test_table->compressed_unmatched_events_count_size = (compressed_buff == NULL)? original_size:compressed_size;
    //    REMPI_DBG("unmatched(ct): %lu bytes (<-zlib- %lu)", test_table->compressed_unmatched_events_count_size, original_size);
    /*=======================================*/
    return;
}

void rempi_encoder_cdc::compress_matched_events(rempi_encoder_input_format &input_format, rempi_encoder_input_format_test_table *test_table)
{
  char  *compressed_buff, *original_buff;
  size_t compressed_size,  original_size;
  test_table->compressed_matched_events = cdc->compress(
							  test_table->matched_events_ordered_map, 
							  test_table->events_vec, 
							  original_size);
  test_table->compressed_matched_events_size           = original_size;

  input_format.write_queue_vec.push_back((char*)&test_table->compressed_matched_events_size);
  input_format.write_size_queue_vec.push_back(sizeof(test_table->compressed_matched_events_size));
  input_format.write_queue_vec.push_back(test_table->compressed_matched_events);
  input_format.write_size_queue_vec.push_back(test_table->compressed_matched_events_size);

  // compressed_buff = compression_util.compress_by_zlib(original_buff, original_size, compressed_size);
  // test_table->compressed_matched_events           = (compressed_buff == NULL)? original_buff:compressed_buff;
  // test_table->compressed_matched_events_size      = (compressed_buff == NULL)? original_size:compressed_size;
  // REMPI_DBG("  matched(id): %lu bytes (<-zlib- %lu)", test_table->compressed_matched_events_size, original_size);
  return;
}


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

  //  REMPI_DBG("input_format.length() %d", input_format.length());
  if (input_format.length() == 0) {
    return is_ready_for_encoding; /*false*/
  }
  if (input_format.length() > MAX_CDC_INPUT_FORMAT_LENGTH || events.is_push_closed_()) {
    /*If got enough chunck size, OR the end of run*/
    input_format.format();
    is_ready_for_encoding = true;

  }

  return is_ready_for_encoding; /*true*/
}


void rempi_encoder_cdc::encode(rempi_encoder_input_format &input_format)
{
  char  *compressed_buff, *original_buff;
  size_t compressed_size,  original_size;

  map<int, rempi_encoder_input_format_test_table*>::iterator test_tables_map_it;
  for (test_tables_map_it  = input_format.test_tables_map.begin();
       test_tables_map_it != input_format.test_tables_map.end();
       test_tables_map_it++) {  
    rempi_encoder_input_format_test_table *test_table;
    test_table = test_tables_map_it->second;

    /*#################################################*/
    /*     Encoding Section                            */
    /*#################################################*/

    /*=== message count ===*/    /*=== Compress with_previous ===*/     /*=== Compress unmatched_events ===*/
    compress_non_matched_events(input_format, test_table);
    /*=======================================*/

    /*=== Compress matched_events ===*/
    compress_matched_events(input_format, test_table);
    /*=======================================*/
  }
  return;
}

// void rempi_encoder_cdc::write_record_file(rempi_encoder_input_format &input_format)
// {
//   size_t total_compressed_size = 0;
//   size_t compressed_size = 0;
//   map<int, rempi_encoder_input_format_test_table*>::iterator test_tables_map_it;
//   for (test_tables_map_it  = input_format.test_tables_map.begin();
//        test_tables_map_it != input_format.test_tables_map.end();
//        test_tables_map_it++) {
//     rempi_encoder_input_format_test_table *test_table;
//     test_table = test_tables_map_it->second;

//     compressed_size = 0;
//     /* Count */
//     record_fs.write((char*)&test_table->count, sizeof(test_table->count));
//     compressed_size += sizeof(test_table->count);

//     /* with_previous */
//     record_fs.write(   (char*)&test_table->compressed_with_previous_size, 
// 		        sizeof(test_table->compressed_with_previous_size));
//     compressed_size +=  sizeof(test_table->compressed_with_previous_size);
//     record_fs.write(           test_table->compressed_with_previous, 
// 			       test_table->compressed_with_previous_size);
//     compressed_size +=         test_table->compressed_with_previous_size;


//     /* unmatched */
//     record_fs.write(  (char*)&test_table->compressed_unmatched_events_id_size, 
// 		       sizeof(test_table->compressed_unmatched_events_id_size));
//     compressed_size += sizeof(test_table->compressed_unmatched_events_id_size);  
//     record_fs.write(          test_table->compressed_unmatched_events_id,    
// 			      test_table->compressed_unmatched_events_id_size);
//     compressed_size +=        test_table->compressed_unmatched_events_id_size;

//     record_fs.write(  (char*)&test_table->compressed_unmatched_events_count_size, 
// 		       sizeof(test_table->compressed_unmatched_events_count_size));
//     compressed_size += sizeof(test_table->compressed_unmatched_events_count_size);  
//     record_fs.write(          test_table->compressed_unmatched_events_count, 
// 			      test_table->compressed_unmatched_events_count_size);
//     compressed_size +=        test_table->compressed_unmatched_events_count_size;


//     /* matched */
//     record_fs.write(  (char*)&test_table->compressed_matched_events_size, 
// 		       sizeof(test_table->compressed_matched_events_size));
//     compressed_size += sizeof(test_table->compressed_matched_events_size);  
//     record_fs.write(          test_table->compressed_matched_events, 
// 			      test_table->compressed_matched_events_size);
//     compressed_size +=        test_table->compressed_matched_events_size;
//     //TODO: delete encoded_event

//     // REMPI_DBG("Original size: count:%5d x 8 bytes x 2(matched/unmatched)= %d bytes,  Compressed size: %lu bytes", 
//     // 	      test_table->count, test_table->count * 8 * 2, compressed_size);
//     total_compressed_size += compressed_size;
//     REMPI_DBG("compressed_size: %lu", compressed_size);
//   }
//   REMPI_DBG("xOriginal size: count:%5d(matched/unmatched entry) x 8 bytes= %d bytes,  Compressed size: %lu bytes , %d %lu", 
// 	    input_format.total_length, input_format.total_length * 8 , total_compressed_size, 
// 	    input_format.total_length*8, total_compressed_size);
//   //  REMPI_DBG("TOTAL Original: %d ", total_countb);


// }   

void rempi_encoder_cdc::write_record_file(rempi_encoder_input_format &input_format)
{
  
  size_t total_compressed_size = 0;
  size_t total_original_size   = 0;
  vector<char*> compressed_write_queue_vec;
  vector<size_t> compressed_write_size_queue_vec;

  if (input_format.write_queue_vec.size() != input_format.write_size_queue_vec.size()) {
    REMPI_ERR("Size is different in write_queue_vec & write_size_queue_vec.");
  }

  for (int i = 0; i < input_format.write_size_queue_vec.size(); i++) {
    total_original_size += input_format.write_size_queue_vec[i];
  }
  
  compression_util.compress_by_zlib_vec(input_format.write_queue_vec, input_format.write_size_queue_vec,
					compressed_write_queue_vec, compressed_write_size_queue_vec, total_compressed_size);


  record_fs.write((char*)&total_compressed_size, sizeof(total_compressed_size));
  for (int i = 0; i < compressed_write_queue_vec.size(); i++) {
    record_fs.write(compressed_write_queue_vec[i], compressed_write_size_queue_vec[i]);
    free(compressed_write_queue_vec[i]);
  }

  REMPI_DBG("Event count: %d  ->  %lu %lu",
	    input_format.total_length, total_original_size, total_compressed_size + sizeof(total_compressed_size));
  return;
}   

bool rempi_encoder_cdc::read_record_file(rempi_encoder_input_format &input_format)
{
  size_t chunk_size;
  char* zlib_payload;
  vector<char*>   compressed_payload_vec;
  vector<size_t>  compressed_payload_size_vec;
  vector<char*>   decompressed_payload_vec;
  vector<size_t>  decompressed_payload_size_vec;
  size_t decompressed_size;
  bool is_no_more_record; 

  record_fs.read((char*)&chunk_size, sizeof(chunk_size));
  if (chunk_size == 0) {
    is_no_more_record = true;
    return is_no_more_record;
  }

  zlib_payload = (char*)malloc(chunk_size);
  record_fs.read(zlib_payload, chunk_size);
  compressed_payload_vec.push_back(zlib_payload);
  compressed_payload_size_vec.push_back(chunk_size);

  compression_util.decompress_by_zlib_vec(compressed_payload_vec, compressed_payload_size_vec,
   					  input_format.write_queue_vec, input_format.write_size_queue_vec, decompressed_size);

  input_format.decompressed_size = decompressed_size;
  
  is_no_more_record = false;
  return is_no_more_record;
}


void rempi_encoder_cdc::decompress_non_matched_events(rempi_encoder_input_format &input_format)
{

}

void rempi_encoder_cdc::decompress_matched_events(rempi_encoder_input_format &input_format)
{

}


void rempi_encoder_cdc::decode(rempi_encoder_input_format &input_format)
{
  char* decompressed_record;
  char* decoding_address;
  //TODO: this is redundant memcopy => remove this overhead
  decompressed_record = (char*)malloc(input_format.decompressed_size);
  for (int i = 0; i < input_format.write_queue_vec.size(); i++) {
    memcpy(decompressed_record, input_format.write_queue_vec[i], input_format.write_size_queue_vec[i]);
    decompressed_record += input_format.write_size_queue_vec[i];
  }

  int test_id = 0;
  decoding_address = decompressed_record;
  while (decoding_address < decompressed_record + input_format.decompressed_size) {
    rempi_encoder_input_format_test_table *test_table;
    test_table = new rempi_encoder_input_format_test_table();

    /*=== Decode with_next ===*/
    test_table->compressed_with_previous_length = decompressed_record;
    decompressed_record += sizeof(test_table->compressed_with_previous_length);
    test_table->compressed_with_previous_size = decompressed_record;
    decompressed_record += sizeof(test_table->compressed_with_previous_size);
    test_table->compressed_with_previous = decompressed_record;
    decompressed_record += test_table->compressed_with_previous_size;
    compression_util.decompress_by_zero_one_binary(test_table->compressed_with_previous, test_table->compressed_with_previous_length,
						   test_table->with_previous_vec);
    /*==========================*/

    /*=== Decode unmatched_events ===*/
    /*  (1) id */
    {
      size_t *array; size_t length;
      test_table->compressed_unmatched_events_id_size = decompressed_record;
      decompressed_record += sizeof(test_table->compressed_unmatched_events_id_size);
      test_table->compressed_unmatched_events_id      = decompressed_record;
      decompressed_record += test_table->compressed_unmatched_events_id_size;
      array = (size_t*)test_table->compressed_unmatched_events_id;
      length = test_table->compressed_unmatched_events_id_size / sizeof(size_t);
      test_table->unmatched_events_id_vec.resize(length);
      for (int i = 0; i < length; i++) {
	test_table->unmatched_events_id_vec[i] = array[i];
      }
      compression_util.decompress_by_linear_prediction(test_table->unmatched_events);
      /*  (2) count */

      test_table->compressed_unmatched_events_count_size = decompressed_record;
      decompressed_record += sizeof(test_table->compressed_unmatched_events_count_size);
      test_table->compressed_unmatched_events_count      = decompressed_record;
      decompressed_record += test_table->compressed_unmatched_events_count_size;
      array = (size_t*)test_table->compressed_unmatched_events_count;
      length = test_table->compressed_unmatched_events_count_size / sizeof(size_t);
      test_table->unmatched_events_count_vec.resize(length);
      for (int i = 0; i < length; i++) {
	test_table->unmatched_events_count_vec[i] = array[i];
      }
    }
      /*===============================*/

    
      


    input_format.test_tables_map[test_id++] = test_table;
  }

  if (decoding_address != decompressed_record + input_format.decompressed_size) {
    REMPI_ERR("Inconsistent size");
  }

  REMPI_ERR("This function has not been implemented yet a");
  return;
}

void rempi_encoder_cdc::insert_encoder_input_format_chunk(rempi_event_list<rempi_event*> &recording_events, rempi_event_list<rempi_event*> &replaying_events, rempi_encoder_input_format &input_format)
{
  REMPI_ERR("This function has not been implemented yet");
  return;
}


// vector<rempi_event*> rempi_encoder_cdc::decode(char *serialized_data, size_t *size)
// {
//   vector<rempi_event*> vec;
//   int *mpi_inputs;
//   int i;

//   /*In rempi_envoder, the serialized sequence identicals to one Test event */
//   mpi_inputs = (int*)serialized_data;
//   vec.push_back(
//     new rempi_test_event(mpi_inputs[0], mpi_inputs[1], mpi_inputs[2], 
// 			 mpi_inputs[3], mpi_inputs[4], mpi_inputs[5])
//   );
//   delete mpi_inputs;
//   return vec;
// }





