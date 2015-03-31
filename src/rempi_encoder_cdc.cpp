#include <stdlib.h>
#include <string.h>

#include <vector>
#include <algorithm>

#include "rempi_err.h"
#include "rempi_util.h"
#include "rempi_event.h"
#include "rempi_encoder.h"
#include "rempi_config.h"
#include "rempi_clock_delta_compression.h"
#include "rempi_compression_util.h"

#define MAX_CDC_INPUT_FORMAT_LENGTH (1024 * 1024)

/* ==================================== */
/* CLASS rempi_encoder_cdc_input_format_test_table */

bool rempi_encoder_input_format_test_table::is_decoded_all()
{
  bool is_unmatched_finished = true;
  //  REMPI_DBGI(REMPI_DBG_REPLAY, "unamtched size: %d, index: %d", unmatched_events_umap.size(), replayed_matched_event_index);
  if (!unmatched_events_umap.empty()) {
    return false;
  }

  if (replayed_matched_event_index < count) {
    return false;
  }
  
  return true;
}

bool rempi_encoder_input_format_test_table::is_pending_all_rank_msg()
{

// #ifdef REMPI_DBG_REPLAY
//     REMPI_DBGI(REMPI_DBG_REPLAY, "epoch_vec size : %d", epoch_rank_vec.size());
// #endif

  for (int i = 0, n = epoch_rank_vec.size(); i < n ; i++) {
    int rank = epoch_rank_vec[i];
    int count = pending_event_count_umap[rank];
    int last_clock = epoch_clock_vec[i];
    int current_lock = current_epoch_line_umap[rank];
#ifdef REMPI_DBG_REPLAY
    //    REMPI_DBGI(REMPI_DBG_REPLAY, "rank: %d count: %d", rank, count);
#endif

    if (count == 0 && last_clock != current_lock) {
      return false;
    } else if (count < 0) {
      REMPI_ERR("count is < 0");
    }
  }
  return true;
}
/* ==================================== */


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

      /*Update epoch line by using unordered_map*/
      if (test_table->epoch_umap.find(sorted_events_vec[i]->get_source()) == test_table->epoch_umap.end()) {
	test_table->epoch_umap[sorted_events_vec[i]->get_source()] = sorted_events_vec[i]->get_clock();
      } else {
	if (test_table->epoch_umap[sorted_events_vec[i]->get_source()] >= sorted_events_vec[i]->get_clock()) {
	  REMPI_ERR("Later message has smaller clock than earlier message: epoch line clock of rank:%d clock:%d but clock:%d", 
		    sorted_events_vec[i]->get_source(), 
		    test_table->epoch_umap[sorted_events_vec[i]->get_source()],sorted_events_vec[i]->get_clock()
		    );
	}
	test_table->epoch_umap[sorted_events_vec[i]->get_source()] = sorted_events_vec[i]->get_clock();
      }
    }
    /*format epoch line, and puth into vectork*/
    for (unordered_map<size_t, size_t>::iterator epoch_it  = test_table->epoch_umap.begin();
	 epoch_it != test_table->epoch_umap.end();
	 epoch_it++) {
      test_table->epoch_rank_vec.push_back (epoch_it->first );
      test_table->epoch_clock_vec.push_back(epoch_it->second);
    }

    
  }
  debug_print();
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
    for (unordered_map<size_t, size_t>::iterator epoch_it = test_table->epoch_umap.begin();
	 epoch_it != test_table->epoch_umap.end();
	 epoch_it++) {
      REMPI_DBG("        epoch: rk:%d clock:%d", 
		epoch_it->first,
		epoch_it->second);
    }

    for (int i = 0; i < test_table->epoch_rank_vec.size(); i++) {
      REMPI_DBG("        epoch: rk:%d clock:%d", 
		test_table->epoch_rank_vec[i],
		test_table->epoch_clock_vec[i]);
    }

#if 0
    for (int i = 0; i < test_table->with_previous_vec.size(); i++) {
      REMPI_DBG("with_previous:  id:%d", test_table->with_previous_vec[i]);
    }
#endif

    for (int i = 0; i < test_table->with_previous_bool_vec.size(); i++) {
      int bl = 0;
      if (test_table->with_previous_bool_vec[i] == true) { 
	bl = 1;
      }
      REMPI_DBG("with_next: id:%d bol:%d" , i, bl);
    }

    for (int i = 0; i < test_table->unmatched_events_id_vec.size(); i++) {
      REMPI_DBG("    unmatched:  id: %d count: %d", test_table->unmatched_events_id_vec[i], test_table->unmatched_events_count_vec[i]);
    }

    for (unordered_map<size_t, size_t>::iterator unmatched_it = test_table->unmatched_events_umap.begin(),
	   unmatched_it_end = test_table->unmatched_events_umap.end();
	 unmatched_it != unmatched_it_end;
	 unmatched_it++) {
      REMPI_DBG("    unmatched:  id: %d count: %d", unmatched_it->first, unmatched_it->second);
    }

    for (int i = 0; i < test_table->events_vec.size(); i++) {
      REMPI_DBG("      matched: (id: %d) source: %d , clock %d", i, test_table->events_vec[i]->get_source(), test_table->events_vec[i]->get_clock());
    }

    for (int i = 0; i < test_table->matched_events_id_vec.size(); i++) {
      REMPI_DBG("      matched:  id: %d  msg_id: %d, delay: %d", i, 
		test_table->matched_events_id_vec[i], test_table->matched_events_delay_vec[i]);
    }

#if 0
    for (int i = 0; i < test_table->matched_events_square_sizes_vec.size(); i++) {
      REMPI_DBG("  matched(sq):  id: %d, size: %d", i, 
		test_table->matched_events_square_sizes_vec[i]);
    }
#endif

    for (int i = 0; i < test_table->matched_events_permutated_indices_vec.size(); i++) {
      REMPI_DBG("  matched(pr):  id: %d  index: %d", i, 
		test_table->matched_events_permutated_indices_vec[i]);
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

  /*=== Compress with_previous ===*/
  //test_table->compressed_with_previous = (char*)compression_util.compress_by_zero_one_binary(test_table->with_previous_vec, compressed_size);
  compression_util.compress_by_linear_prediction(test_table->with_previous_vec);
  test_table->compressed_with_previous        = (char*)&test_table->with_previous_vec[0];
  //   test_table->compressed_with_previous_size   = compressed_size;
  test_table->compressed_with_previous_size   = test_table->with_previous_vec.size() * sizeof(test_table->with_previous_vec[0]);
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
  //  REMPI_DBGI(3, "matched size---> %lu", test_table->compressed_matched_events_size);
  input_format.write_queue_vec.push_back(test_table->compressed_matched_events);
  input_format.write_size_queue_vec.push_back(test_table->compressed_matched_events_size);
#if 0
  {
    int *a, *b;
    a  = (int*)test_table->compressed_matched_events;
    b  = (int*)test_table->compressed_matched_events;
    b +=  test_table->compressed_matched_events_size /sizeof(int)/  2;
    for (int i = 0; i < test_table->compressed_matched_events_size / sizeof(int) / 2; i++) {
      REMPI_DBG("--> %d %d", a[i], b[i]);
    }
  }
#endif

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

   // REMPI_DBG("Event count: %d  ->  %lu %lu",
   // 	     input_format.total_length, total_original_size, total_compressed_size + sizeof(total_compressed_size));
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
  size_t decompressed_size, read_size;
  bool is_no_more_record; 

  record_fs.read((char*)&chunk_size, sizeof(chunk_size));
  read_size = record_fs.gcount();
  if (read_size == 0) {
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
  //  REMPI_DBG("zlib_payload size: %lu -> %lu", chunk_size, decompressed_size);
  
  is_no_more_record = false;
  return is_no_more_record;
}


void rempi_encoder_cdc::decompress_non_matched_events(rempi_encoder_input_format &input_format)
{

}

void rempi_encoder_cdc::decompress_matched_events(rempi_encoder_input_format &input_format)
{

}


void rempi_copy_vec(size_t* array, size_t length, vector<size_t>&vec)
{
  if (!vec.empty()) {
    REMPI_ERR("vec is not empty");
  }
  vec.resize(length);
  for (int i = 0; i < length; i++) {
    vec[i] = array[i];
  }
  return;
}

void rempi_copy_vec_int(int* array, size_t length, vector<size_t>&vec)
{
  if (!vec.empty()) {
    REMPI_ERR("vec is not empty");
  }
  vec.resize(length);
  for (int i = 0; i < length; i++) {
    vec[i] = (size_t)array[i];
  }
  return;
}

void rempi_encoder_cdc::decode(rempi_encoder_input_format &input_format)
{
  char* decompressed_record;
  char* decoding_address;
  //TODO: this is redundant memcopy => remove this overhead
  decompressed_record = (char*)malloc(input_format.decompressed_size);
  decoding_address = decompressed_record;
  for (int i = 0; i < input_format.write_queue_vec.size(); i++) {
    memcpy(decompressed_record, input_format.write_queue_vec[i], input_format.write_size_queue_vec[i]);
    decompressed_record += input_format.write_size_queue_vec[i];
  }

  int test_id = 0;
  while (decoding_address < decompressed_record) {
    rempi_encoder_input_format_test_table *test_table;

    test_table = new rempi_encoder_input_format_test_table();
    input_format.test_tables_map[test_id++] = test_table;

    /*=== Decode count  ===*/
    test_table->count = *(size_t*)decoding_address;
    decoding_address += sizeof(test_table->count);
    /*==========================*/

    /*=== Decode epoch  ===*/
    {
      size_t *epoch_rank, *epoch_clock;
      /*----------- size ------------*/
      test_table->epoch_size = *(size_t*)decoding_address;
      decoding_address += sizeof(test_table->epoch_size);
      /*----------- rank ------------*/
      epoch_rank             = (size_t*)decoding_address;
      decoding_address += test_table->epoch_size;
      /*----------- clock ------------*/
      epoch_clock             = (size_t*)decoding_address;
      decoding_address += test_table->epoch_size;
      for (int i = 0; i < test_table->epoch_size / sizeof(size_t); i++) {
	test_table->epoch_umap[epoch_rank[i]] = epoch_clock[i];
	test_table->epoch_rank_vec.push_back(epoch_rank[i]);
	test_table->epoch_clock_vec.push_back(epoch_clock[i]);
      }
    }
    /*==========================*/

    /*=== Decode with_next ===*/
    /*----------- length ------------*/
    test_table->compressed_with_previous_length = *(size_t*)decoding_address;
    decoding_address += sizeof(test_table->compressed_with_previous_length);
    /*----------- size   ------------*/
    test_table->compressed_with_previous_size = *(size_t*)decoding_address;
    decoding_address += sizeof(test_table->compressed_with_previous_size);
    /*----------- with_next   ------------*/
    test_table->compressed_with_previous = decoding_address;
    rempi_copy_vec((size_t*)test_table->compressed_with_previous, test_table->compressed_with_previous_size / sizeof(size_t), 
		   test_table->with_previous_vec);
    compression_util.decompress_by_linear_prediction(test_table->with_previous_vec);
    decoding_address += test_table->compressed_with_previous_size;
    {
      test_table->with_previous_bool_vec.resize(test_table->count, false);
      for (int i = 0, n = test_table->with_previous_vec.size(); i < n; i++) {
	int with_next_index = test_table->with_previous_vec[i];
	test_table->with_previous_bool_vec[with_next_index] = true;
      }
    }

    /*==========================*/
    

    /*=== Decode unmatched_events ===*/
    /*===  (1) id */
    /*----------- size ------------*/
    test_table->compressed_unmatched_events_id_size = *(size_t*)decoding_address;
    decoding_address += sizeof(test_table->compressed_unmatched_events_id_size);
    /*----------- id   ------------*/
    test_table->compressed_unmatched_events_id      = decoding_address;
    rempi_copy_vec((size_t*)test_table->compressed_unmatched_events_id, test_table->compressed_unmatched_events_id_size / sizeof(size_t),
		   test_table->unmatched_events_id_vec);
    compression_util.decompress_by_linear_prediction(test_table->unmatched_events_id_vec);
    decoding_address += test_table->compressed_unmatched_events_id_size;
    /*===  (2) count */
    /*----------- size ------------*/
    test_table->compressed_unmatched_events_count_size = *(size_t*)decoding_address;
    decoding_address += sizeof(test_table->compressed_unmatched_events_count_size);
    /*----------- count   ------------*/
    test_table->compressed_unmatched_events_count      = decoding_address;
    rempi_copy_vec((size_t*)test_table->compressed_unmatched_events_count, test_table->compressed_unmatched_events_count_size / sizeof(size_t),
		   test_table->unmatched_events_count_vec);
    decoding_address += test_table->compressed_unmatched_events_count_size;
    {
      for (int i = 0, n = test_table->unmatched_events_id_vec.size(); i < n; i++) {
	size_t id    = test_table->unmatched_events_id_vec[i];
	size_t count = test_table->unmatched_events_count_vec[i];
	test_table->unmatched_events_umap[id] = count;
      }
    }
    test_table->unmatched_events_id_vec.clear();
    test_table->unmatched_events_count_vec.clear();
    /*===============================*/

    /*=== Decode matched_events ===*/
    /*----------- size ------------*/
    test_table->compressed_matched_events_size      = *(size_t*)decoding_address;
    //    REMPI_DBGI(3, "matched size: %lu", test_table->compressed_matched_events_size);
    decoding_address += sizeof(test_table->compressed_matched_events_size);
    /*----------- event ------------*/
    test_table->compressed_matched_events          = decoding_address;
    /*   --  id  --  */
    {
      int* copy_start;
      copy_start = (int*)test_table->compressed_matched_events;
      rempi_copy_vec_int(copy_start,
			 test_table->compressed_matched_events_size / sizeof(int) / 2,
			 test_table->matched_events_id_vec);
      compression_util.decompress_by_linear_prediction(test_table->matched_events_id_vec);
      //    REMPI_DBGI(3, "matched length: %lu", test_table->matched_events_id_vec.size());
      /*   -- delay --  */
      copy_start += 	 test_table->compressed_matched_events_size / sizeof(int) / 2;
      rempi_copy_vec_int(copy_start,
			 test_table->compressed_matched_events_size / sizeof(int) / 2,
			 test_table->matched_events_delay_vec);
      compression_util.decompress_by_linear_prediction(test_table->matched_events_delay_vec);
      
      if (test_table->matched_events_id_vec.size() != test_table->matched_events_delay_vec.size()) {
	REMPI_ERR("matched_id_vec.size != matched_delay_vec.size()");
      }
      cdc_prepare_decode_indices(test_table->count,
				 test_table->matched_events_id_vec,
				 test_table->matched_events_delay_vec,
				 test_table->matched_events_square_sizes_vec,
				 test_table->matched_events_permutated_indices_vec);
    }

    
    /* ======== Several initialization of test_table for replay ==========*/
    for (int i = 0, n = test_table->epoch_rank_vec.size(); i < n ; i++) {
      int rank = test_table->epoch_rank_vec[i];
      test_table->pending_event_count_umap[rank] = 0;
      test_table->current_epoch_line_umap[rank] = 0;
    }
    /* ===================================================================*/


#if 0
    {
      int *a, *b;
      a  = (int*)test_table->compressed_matched_events;
      b  = (int*)test_table->compressed_matched_events;
      b +=  test_table->compressed_matched_events_size /sizeof(int)/  2;
      for (int i = 0; i < test_table->compressed_matched_events_size / sizeof(int) / 2; i++) {
	REMPI_DBG("--> %d %d", a[i], b[i]);
      }
    }
#endif

    /*   ------------ */
    decoding_address += test_table->compressed_matched_events_size;

    /*===============================*/
  }
  
  input_format.debug_print();
  //  exit(0);

  if (decoding_address != decompressed_record) {
    REMPI_ERR("Inconsistent size");
  }

  return;
}

void rempi_encoder_cdc::cdc_prepare_decode_indices(
						   size_t matched_event_count,
						   vector<size_t> &matched_events_id_vec,
						   vector<size_t> &matched_events_delay_vec,
						   vector<int> &matched_events_square_sizes_vec,
						   vector<int> &matched_events_permutated_indices_vec)
{
  /* Detailed algorithm is written my note on a day, March 26th */
  matched_events_permutated_indices_vec.resize(matched_event_count, 0);

  /*permutated_indices*/  
  for (int i = 0, n = matched_events_id_vec.size(); i < n ; i++) {
    int id    = (int)matched_events_id_vec[i];
    int delay = (int)matched_events_delay_vec[i];

    matched_events_permutated_indices_vec[id] = delay;
    int val   = (delay > 0)? (-1):(1);
    for (int j = id + delay; j != id; j += val) {
      matched_events_permutated_indices_vec[j] = val;
    }
  }
  for (int i = 0, n = matched_events_permutated_indices_vec.size(); i < n; i++) {
    matched_events_permutated_indices_vec[i] += i;
  }
  
  /*square_sizes_indices*/
  int init_square_size = 0;
  int max = 0;
  matched_events_square_sizes_vec.push_back(init_square_size);
  for (int i = 0, n = matched_events_permutated_indices_vec.size(); i < n; i++) {
    matched_events_square_sizes_vec.back()++;
    if (max < matched_events_permutated_indices_vec[i]) {
      max = matched_events_permutated_indices_vec[i];
    }
    if (i == max) {
      matched_events_square_sizes_vec.push_back(init_square_size);
    }
  }
  if (matched_events_square_sizes_vec.back() == 0) {
    matched_events_square_sizes_vec.pop_back();
  }
  return;
    
}

// void rempi_encoder_cdc::cdc_decode_ordering(vector<rempi_event*> &event_vec, rempi_encoder_input_format_test_table* test_table, vector<rempi_event*> &replay_event_vec)
// {
//   if (replay_event_vec.size() != 0) {
//     REMPI_ERR("replay_event_vec is not empty, and the replaying events are not passed to replaying_events");
//   }  

//   /*First, check if unmatched replay or not*/
//   if (test_table->unmatched_events_umap.find(test_table->replayed_matched_event_index)
//       != test_table->unmatched_events_umap.end()) {
//     int event_count = test_table->unmatched_events_umap[test_table->replayed_matched_event_index];
//     replay_event_vec.push_back(new rempi_test_event(event_count, 0,0,0,0,0,0,0));
//     test_table->unmatched_events_umap.erase(test_table->replayed_matched_event_index);
//   }

//   /* Detailed algorithm was written in my note on a day, March 27th*/
//   /*
//   unordered_map<size_t, list<rempi_event*>>  pending_events_umap;
//   rempi_event*                       min_clock_event;
//   list<rempi_event*>                 ordered_event_list;
//   */
//   /*1. Put it to hash table*/
//   if (!event_vec.empty()) {
//     for (int i = 0, n = event_vec.size(); i < n; i++) {
//       rempi_event *event;
//       event = event_vec[i];
// #if 0
//       REMPI_DBGI(REMPI_DBG_REPLAY, "Decoded ; (count: %d, with_next: %d, flag: %d, source: %d, tag: %d, clock: %d)",
// 		 event_vec[i]->get_event_counts(), event_vec[i]->get_is_testsome(), event_vec[i]->get_flag(),
// 		 event_vec[i]->get_source(), event_vec[i]->get_tag(), event_vec[i]->get_clock());
// #endif
//       if (test_table->pending_events_umap.find(event->get_source()) == test_table->pending_events_umap.end()) {
// 	test_table->pending_events_umap[event->get_source()] = new list<rempi_event*>;
//       }
//       test_table->pending_events_umap[event->get_source()]->push_back(event);
//     }
//     event_vec.clear();
//   }


//   /*2. update event with  minimal clock*/
//   for (unordered_map<int, list<rempi_event*>*>::iterator events_it = test_table->pending_events_umap.begin(),
// 	 events_it_end = test_table->pending_events_umap.end();
//        events_it != events_it_end;
//        events_it++) {
//     list<rempi_event*> *events;
//     events = events_it->second;
//     if (events->empty()) continue;

//     if (test_table->min_clock_event_in_current_epoch == NULL) {
//       test_table->min_clock_event_in_current_epoch = events->back();
//     } else {
//       rempi_event *event;
//       event = events->back();
//       if (event->get_clock() == test_table->min_clock_event_in_current_epoch->get_clock()) {
// 	if (event->get_source() < test_table->min_clock_event_in_current_epoch->get_source()) {
// 	  test_table->min_clock_event_in_current_epoch = event;
// 	}
//       } else if (event->get_clock() < test_table->min_clock_event_in_current_epoch->get_clock()) {
// 	test_table->min_clock_event_in_current_epoch = event;
//       }      
//     }
//   }
  
//   if (test_table->min_clock_event_in_current_epoch == NULL) return;
  
//   REMPI_DBGI(REMPI_DBG_REPLAY, "min ; (count: %d, with_next: %d, flag: %d, source: %d, tag: %d, clock: %d)",
// 	     test_table->min_clock_event_in_current_epoch->get_event_counts(), 
// 	     test_table->min_clock_event_in_current_epoch->get_is_testsome(), 
// 	     test_table->min_clock_event_in_current_epoch->get_flag(),
// 	     test_table->min_clock_event_in_current_epoch->get_source(), 
// 	     test_table->min_clock_event_in_current_epoch->get_tag(), 
// 	     test_table->min_clock_event_in_current_epoch->get_clock());

//   /*3. move event, which is smaller clock than min_clock_event_in_current_epoch, to ordered_event_list*/
//   for (unordered_map<int, list<rempi_event*>*>::iterator events_it = test_table->pending_events_umap.begin(),
// 	 events_it_end = test_table->pending_events_umap.end();
//        events_it != events_it_end;
//        events_it++) {
//     list<rempi_event*> *event_list;
//     list<rempi_event*>::iterator event_list_it;
//     event_list = events_it->second;
    
//     for (event_list_it = event_list->begin();
// 	 event_list_it != event_list->end();
// 	 event_list_it++) {
//       rempi_event *event;
//       event = *event_list_it;
//       if (event->get_clock() == test_table->min_clock_event_in_current_epoch->get_clock()) {
// 	if (event->get_source() <= test_table->min_clock_event_in_current_epoch->get_source()) {
// 	  test_table->ordered_event_list.push_back(event);
// 	} else {
// 	  break;
// 	}
//       } else if (event->get_clock() <= test_table->min_clock_event_in_current_epoch->get_clock()) {
// 	test_table->ordered_event_list.push_back(event);
//       } else {
// 	break;
//       }
//     }
//     event_list->erase(event_list->begin(), event_list_it);    
//   }
  
  
//   for (rempi_event *e: test_table->ordered_event_list) {
//     REMPI_DBGI(REMPI_DBG_REPLAY, "Decoded ; (count: %d, with_next: %d, flag: %d, source: %d, tag: %d, clock: %d)",
// 	       e->get_event_counts(), e->get_is_testsome(), e->get_flag(),
// 	       e->get_source(), e->get_tag(), e->get_clock());
//   }
  
  

  

//   //#Ifdef REMPI_DBG_REPLAY
// #if 0

//   for (int i = 0; i < event_vec.size(); i++) {
//     REMPI_DBGI(REMPI_DBG_REPLAY, "Decoded ; (count: %d, with_next: %d, flag: %d, source: %d, tag: %d, clock: %d)",
// 	       event_vec[i]->get_event_counts(), event_vec[i]->get_is_testsome(), event_vec[i]->get_flag(),
// 	       event_vec[i]->get_source(), event_vec[i]->get_tag(), event_vec[i]->get_clock());
//   }



//     for (unordered_map<size_t, size_t>::iterator epoch_it = test_table->epoch_umap.begin();
// 	 epoch_it != test_table->epoch_umap.end();
// 	 epoch_it++) {
//       REMPI_DBGI(REMPI_DBG_REPLAY,"        epoch: rk:%d clock:%d", 
// 		epoch_it->first,
// 		epoch_it->second);
//     }

//     for (int i = 0; i < test_table->epoch_rank_vec.size(); i++) {
//       REMPI_DBGI(REMPI_DBG_REPLAY, "        epoch: rk:%d clock:%d", 
// 		test_table->epoch_rank_vec[i],
// 		test_table->epoch_clock_vec[i]);
//     }

//     for (int i = 0; i < test_table->with_previous_vec.size(); i++) {
//       REMPI_DBGI(REMPI_DBG_REPLAY, "with_previous:  id:%d", test_table->with_previous_vec[i]);
//     }

//     for (unordered_map<size_t, size_t>::iterator unmatched_it = test_table->unmatched_events_umap.begin(),
// 	   unmatched_it_end = test_table->unmatched_events_umap.end();
// 	 unmatched_it != unmatched_it_end;
// 	 unmatched_it++) {
//       REMPI_DBGI(REMPI_DBG_REPLAY, "    unmatched:  id: %d count: %d", unmatched_it->first, unmatched_it->second);
//     }





//   for (int i = 0; i < test_table->matched_events_square_sizes_vec.size(); i++) {
//     REMPI_DBGI(REMPI_DBG_REPLAY, "  matched(sq):  id: %d, size: %d", i, 
// 	      test_table->matched_events_square_sizes_vec[i]);
//   }
  
//   for (int i = 0; i < test_table->matched_events_permutated_indices_vec.size(); i++) {
//     REMPI_DBGI(REMPI_DBG_REPLAY, "  matched(pr):  id: %d  index: %d", i, 
// 	      test_table->matched_events_permutated_indices_vec[i]);
//   }
// #endif

// }



bool rempi_encoder_cdc::cdc_decode_ordering(vector<rempi_event*> &event_vec, rempi_encoder_input_format_test_table* test_table, vector<rempi_event*> &replay_event_vec)
{
  if (replay_event_vec.size() != 0) {
    REMPI_ERR("replay_event_vec is not empty, and the replaying events are not passed to replaying_events");
  }  

  /*First, check if unmatched replay or not*/
  if (test_table->unmatched_events_umap.find(test_table->replayed_matched_event_index)
      != test_table->unmatched_events_umap.end()) {
    int event_count = test_table->unmatched_events_umap[test_table->replayed_matched_event_index];
    replay_event_vec.push_back(new rempi_test_event(event_count, 0,0,0,0,0,0,0));
    test_table->unmatched_events_umap.erase(test_table->replayed_matched_event_index);
    return test_table->is_decoded_all();
  }
  

  /* Detailed algorithm was written in my note on a day, March 30th*/
  /*1. Put events to list */
  if (!event_vec.empty()) {
    for (int i = 0, n = event_vec.size(); i < n; i++) {
      rempi_event *event;
      event = event_vec[i];
      test_table->ordered_event_list.push_back(event);
      /*2. update pending_msg_count_umap, epoch_line_umap */
      test_table->pending_event_count_umap[event->get_source()]++;
      test_table->current_epoch_line_umap[event->get_source()] = event->get_clock();
#if REMPI_DBG_REPLAY
      REMPI_DBGI(REMPI_DBG_REPLAY, "RCQ -> PQ ; (count: %d, with_next: %d, flag: %d, source: %d, tag: %d, clock: %d): list size: %d",
		 event_vec[i]->get_event_counts(), event_vec[i]->get_is_testsome(), event_vec[i]->get_flag(),
		 event_vec[i]->get_source(), event_vec[i]->get_tag(), event_vec[i]->get_clock(), 
		 test_table->ordered_event_list.size());

      for (int i = 0, n = test_table->epoch_rank_vec.size(); i < n ; i++) {
	int rank = test_table->epoch_rank_vec[i];
	int count = test_table->pending_event_count_umap[rank];
	
#ifdef REMPI_DBG_REPLAY
	REMPI_DBGI(REMPI_DBG_REPLAY, "rank: %d count: %d", rank, count);
#endif
      }


#endif
    }
    event_vec.clear();
  }

  /*3. If 2-(a), and satisfy (square_sizes), sort*/
  if (!test_table->is_pending_all_rank_msg()) { 
#if REMPI_DBG_REPLAY
    //    REMPI_DBGI(REMPI_DBG_REPLAY, "is_pending_all_rank_msg is false");
#endif
    return test_table->is_decoded_all();
  }
  


  // for (rempi_event *e: test_table->ordered_event_list) {
  //   REMPI_DBGI(REMPI_DBG_REPLAY, "Before Sort  ; (count: %d, with_next: %d, flag: %d, source: %d, tag: %d, clock: %d)",
  // 	       e->get_event_counts(), e->get_is_testsome(), e->get_flag(),
  // 	       e->get_source(), e->get_tag(), e->get_clock());
  // }
  test_table->ordered_event_list.sort(compare);
  // for (rempi_event *e: test_table->ordered_event_list) {
  //   REMPI_DBGI(REMPI_DBG_REPLAY, "After Sort  ; (count: %d, with_next: %d, flag: %d, source: %d, tag: %d, clock: %d)",
  // 	       e->get_event_counts(), e->get_is_testsome(), e->get_flag(),
  // 	       e->get_source(), e->get_tag(), e->get_clock());
  // }
  

  /* Compute outcount */
  int outcount = 1;
  for (int i = test_table->replayed_matched_event_index; test_table->with_previous_bool_vec[i] == true; i++) {
    outcount++;
  }
  replay_event_vec.resize(outcount, NULL);
  

  int i = 0;
  for (list<rempi_event*>::iterator 
  	 ordered_event_list_it = test_table->ordered_event_list.begin(), 
  	 ordered_event_list_it_end = test_table->ordered_event_list.end();
       ordered_event_list_it != ordered_event_list_it_end;
       ordered_event_list_it++, i++) {
    rempi_event *replaying_event;
    replaying_event = *ordered_event_list_it;

    int permutated_index = test_table->matched_events_permutated_indices_vec[i + test_table->replayed_matched_event_index] 
      - test_table->replayed_matched_event_index;

#ifdef REMPI_DBG_REPLAY
     REMPI_DBGI(4, "PQ -> RPQv ; (count: %d, with_next: %d, flag: %d, source: %d, tag: %d, clock: %d): index: %d, count: %d, list_size: %d",
      	       replaying_event->get_event_counts(), replaying_event->get_is_testsome(), replaying_event->get_flag(),
 	       replaying_event->get_source(), replaying_event->get_tag(), replaying_event->get_clock(),
		permutated_index, outcount, test_table->ordered_event_list.size());
#endif
     if (permutated_index >= replay_event_vec.size()) continue;
    
    replay_event_vec[permutated_index] = replaying_event;
  }
  
  /*If there is hanuke, we abort this replay*/
  for (int i = 0, n = replay_event_vec.size(); i < n; i++) {
    if (replay_event_vec[i] == NULL) {
      replay_event_vec.resize(0);
      return test_table->is_decoded_all();
    } 
    replay_event_vec[i]->set_with_next(1);
  }
  replay_event_vec.back()->set_with_next(0);

  for (int i = 0, n = replay_event_vec.size(); i < n; i++) {
    test_table->pending_event_count_umap[replay_event_vec[i]->get_source()]--;
    test_table->ordered_event_list.remove(replay_event_vec[i]);
  }

  test_table->replayed_matched_event_index += replay_event_vec.size();

  
#ifdef REMPI_DBG_REPLAY
  for (int i = 0, n = replay_event_vec.size(); i < n; i++) {
    REMPI_DBGI(REMPI_DBG_REPLAY, "PQ -> RPQv ; (count: %d, with_next: %d, flag: %d, source: %d, tag: %d, clock: %d)",
	       replay_event_vec[i]->get_event_counts(), replay_event_vec[i]->get_is_testsome(), replay_event_vec[i]->get_flag(),
	       replay_event_vec[i]->get_source(), replay_event_vec[i]->get_tag(), replay_event_vec[i]->get_clock());
  }
#endif  
  return test_table->is_decoded_all();
}


void rempi_encoder_cdc::insert_encoder_input_format_chunk(rempi_event_list<rempi_event*> &recording_events, rempi_event_list<rempi_event*> &replaying_events, rempi_encoder_input_format &input_format)
{
  // for (map<int, rempi_encoder_input_format_test_table*>::iterator test_table_it = input_format.test_table_map.begin();
  //      test_table_it != input_format.test_table.end(); test_table_it++) {
  //   int test_id = test_table_it->first;
  //   rempi_encoder_input_format_test_table *test_table = test_table->second;
  // }
  unordered_map<int, vector<rempi_event*>*> matched_events_vec_umap;
  vector<rempi_event*> replay_event_vec;
  bool is_finished = false;
  int finished_testsome_count  = 0;

  for (int i = 0; i < input_format.test_tables_map.size(); i++) {
    matched_events_vec_umap[i] = new vector<rempi_event*>();
  }

  int sleep_counter = input_format.test_tables_map.size();
  for (int test_id = 0, n = input_format.test_tables_map.size(); 
       finished_testsome_count < n; 
       test_id = ++test_id % n) {

    if (recording_events.front_replay(test_id) != NULL) {      
      rempi_event *matched_event;
      int event_list_status;
      matched_event = recording_events.dequeue_replay(test_id, event_list_status);
      matched_events_vec_umap[test_id]->push_back(matched_event);
    } else {
      sleep_counter--;
      if (sleep_counter <= 0) {
	sleep_counter = input_format.test_tables_map.size();
      }      
    }
    is_finished = cdc_decode_ordering(*matched_events_vec_umap[test_id], input_format.test_tables_map[test_id], replay_event_vec);
    
    if (is_finished) {
      finished_testsome_count++;
      is_finished = false;
    }
    
    for (int i = 0; i < replay_event_vec.size(); i++) {
#ifdef REMPI_DBG_REPLAY
      REMPI_DBGI(REMPI_DBG_REPLAY, "RPQv -> RPQ ; (count: %d, with_next: %d, flag: %d, source: %d, tag: %d, clock: %d)",
		 replay_event_vec[i]->get_event_counts(), replay_event_vec[i]->get_is_testsome(), replay_event_vec[i]->get_flag(),
		 replay_event_vec[i]->get_source(), replay_event_vec[i]->get_tag(), replay_event_vec[i]->get_clock());
#endif
      replaying_events.enqueue_replay(replay_event_vec[i], test_id);
    }
    replay_event_vec.clear();
  }

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





