#include <stdlib.h>
#include <stdio.h>

#include <vector>

#include "rempi_err.h"
#include "rempi_event.h"
#include "rempi_encoder.h"
#include "rempi_config.h"
#include "rempi_compression_util.h"



/* ============================================= */
/*  CLASS rempi_encoder_input_format_test_table  */
/* ============================================= */

void rempi_encoder_input_format_test_table::clear()
{
  for (int i = 0; i < events_vec.size(); i++) {
    delete events_vec[i];
  }
  events_vec.clear();
  
  /*With previous*/
  with_previous_vec.clear();
  compressed_with_previous_length = 0;
  compressed_with_previous_size   = 0;
  if (compressed_with_previous != NULL) {
    free(compressed_with_previous);
    compressed_with_previous = NULL;
  }

  /*Unmatched */
  unmatched_events_id_vec.clear();
  compressed_unmatched_events_id_size = 0;
  if (compressed_unmatched_events_id  != NULL) {
    free(compressed_unmatched_events_id);
    compressed_unmatched_events_id = NULL;
  }
  unmatched_events_count_vec.clear();
  compressed_unmatched_events_count_size = 0;
  if (compressed_unmatched_events_count  != NULL) {
    free(compressed_unmatched_events_count);
    compressed_unmatched_events_count = NULL;
  }

  /*Matched*/
  compressed_matched_events_size = 0;
  if (compressed_matched_events != NULL) {
    free(compressed_matched_events);
    compressed_matched_events = NULL;
  }

  return;
}

/* ==================================== */
/*  CLASS rempi_encoder_input_format    */
/* ==================================== */

size_t rempi_encoder_input_format::length()
{
  return total_length;
}

void rempi_encoder_input_format::add(rempi_event *event)
{
  rempi_encoder_input_format_test_table *test_table;
  if (test_tables_map.find(0) == test_tables_map.end()) {
    test_tables_map[0] = new rempi_encoder_input_format_test_table();
  }
  test_table = test_tables_map[0];
  test_table->events_vec.push_back(event);
  total_length++;
  return;
}

void rempi_encoder_input_format::add(rempi_event *event, int test_id)
{
  rempi_encoder_input_format_test_table *test_table;
  if (test_tables_map.find(test_id) == test_tables_map.end()) {
    test_tables_map[test_id] = new rempi_encoder_input_format_test_table();
  }
  test_table = test_tables_map[test_id];
  test_table->events_vec.push_back(event);
  total_length++;
  return;
}

void rempi_encoder_input_format::format()
{
  return;
}

void rempi_encoder_input_format::clear()
{
  rempi_encoder_input_format_test_table *test_table;
  map<int, rempi_encoder_input_format_test_table*>::iterator test_tables_map_it;
  for (test_tables_map_it  = test_tables_map.begin();
       test_tables_map_it != test_tables_map.end();
       test_tables_map_it++) {
    test_table = test_tables_map_it->second;
    test_table->clear();
    delete test_table;
  }
  write_queue_vec.clear();
  write_size_queue_vec.clear();
  return;
}

void rempi_encoder_input_format::debug_print()
{
  return;
}


/* ==================================== */
/*      CLASS rempi_encoder             */
/* ==================================== */

bool rempi_encoder::is_event_pooled(rempi_event* replaying_event)
{
  list<rempi_event*>::iterator matched_event_pool_it;
  for (matched_event_pool_it  = matched_event_pool.begin();
       matched_event_pool_it != matched_event_pool.end();
       matched_event_pool_it++) {
    rempi_event *pooled_event;
    pooled_event = *matched_event_pool_it;
    bool is_source = (pooled_event->get_source() == replaying_event->get_source());
    bool is_clock  = (pooled_event->get_clock()  == replaying_event->get_clock());
    if (is_source && is_clock) return true;
  }
  return false;
}

rempi_event* rempi_encoder::pop_event_pool(rempi_event* replaying_event)
{
  list<rempi_event*>::iterator matched_event_pool_it;
  for (matched_event_pool_it  = matched_event_pool.begin();
       matched_event_pool_it != matched_event_pool.end();
       matched_event_pool_it++) {
    rempi_event *pooled_event;
    pooled_event = *matched_event_pool_it;
    bool is_source = (pooled_event->get_source() == replaying_event->get_source());
    bool is_clock  = (pooled_event->get_clock()  == replaying_event->get_clock());
    if (is_source && is_clock) {
      matched_event_pool.erase(matched_event_pool_it);
      return pooled_event;
    }
  }
  return NULL;
}

void rempi_encoder::open_record_file(string record_path)
{
  if (mode == REMPI_ENV_REMPI_MODE_RECORD) {
    record_fs.open(record_path.c_str(), ios::out);
  } else if (mode == REMPI_ENV_REMPI_MODE_REPLAY) {
    record_fs.open(record_path.c_str(), ios::in);
  } else {
    REMPI_ERR("Unknown record replay mode");
  }

  if(!record_fs.is_open()) {
    REMPI_ERR("Record file open failed: %s", record_path.c_str());
  }

}

rempi_encoder_input_format* rempi_encoder::create_encoder_input_format()
{ 
  return new rempi_encoder_input_format();
}


/*  ==== 3 Step compression ==== 
    1. rempi_encoder.get_encoding_event_sequence(...): [rempi_event_list => rempi_encoding_structure]
    this function translate sequence of event(rempi_event_list) 
    into a data format class(rempi_encoding_structure) whose format 
    is sutable for compression(rempi_encoder.encode(...))
   

    2. repmi_encoder.encode(...): [rempi_encoding_structure => char*]
    encode, i.e., rempi_encoding_structure -> char*
     
    3. write_record_file: [char* => "file"]
    write the data(char*)
    =============================== */

// bool rempi_encoder::extract_encoder_input_format_chunk(rempi_event_list<rempi_event*> &events, rempi_encoder_input_format &input_format)
// {
//   rempi_event *event;
//   rempi_encoder_input_format_test_table *test_table;
//   bool is_extracted = false;
//   event = events.pop();
//   /*encoding_event_sequence get only one event*/
//   if (event != NULL) {
//     //    rempi_dbgi(0, "pusu event: %p", event);                                                              
//     input_format.add(event, 0);
//     is_extracted = true;
//   }
//   return is_extracted;
// }

bool rempi_encoder::extract_encoder_input_format_chunk(rempi_event_list<rempi_event*> &events, rempi_encoder_input_format &input_format)
{
  rempi_event *event_dequeued;
  bool is_ready_for_encoding = false;

  while (1) {
    /*Append events to current check as many as possible*/
    if (events.front() == NULL) break;
    event_dequeued = events.pop();
    input_format.add(event_dequeued);
  }

  //  REMPI_DBG("input_format.length() %d", input_format.length());
  if (input_format.length() == 0) {
    return is_ready_for_encoding; /*false*/
  }

  if (input_format.length() > REMPI_MAX_INPUT_FORMAT_LENGTH || events.is_push_closed_()) {
    /*If got enough chunck size, OR the end of run*/
    input_format.format();
    is_ready_for_encoding = true;
    //    REMPI_DBGI(0, "========= formating simple");
  }
  return is_ready_for_encoding; /*true*/
}




void rempi_encoder::encode(rempi_encoder_input_format &input_format)
{
  size_t original_size, compressed_size;
  rempi_event *encoding_event;
  rempi_encoder_input_format_test_table *test_table;
  size_t *original_buff;
  int recoding_inputs_num = rempi_event::record_num; // count, flag, rank, with_next and clock

  test_table = input_format.test_tables_map[0];

  original_size = sizeof(size_t) * recoding_inputs_num * test_table->events_vec.size();
  original_buff = (size_t*)malloc(original_size);
  for (int i = 0; i < test_table->events_vec.size(); i++) {
    int encodign_buff_head_index = i * recoding_inputs_num;
    encoding_event = test_table->events_vec[i];    
    original_buff[encodign_buff_head_index + 0] = encoding_event->get_event_counts();
    original_buff[encodign_buff_head_index + 1] = encoding_event->get_is_testsome();
    original_buff[encodign_buff_head_index + 2] = encoding_event->get_flag();
    original_buff[encodign_buff_head_index + 3] = encoding_event->get_source();
    original_buff[encodign_buff_head_index + 4] = encoding_event->get_clock();
#ifdef REMPI_DBG_REPLAY
    // REMPI_DBGI(REMPI_DBG_REPLAY, "Encoded  : (count: %d, with_next: %d, flag: %d, source: %d, tag: %d, clock: %d)", 
    // 	       encoding_event->get_event_counts(), encoding_event->get_is_testsome(), encoding_event->get_flag(), 
    // 	       encoding_event->get_source(), encoding_event->get_tag(), encoding_event->get_clock());
    REMPI_DBG("Encoded  : (count: %d, with_next: %d, flag: %d, source: %d, tag: %d, clock: %d)", 
	       encoding_event->get_event_counts(), encoding_event->get_is_testsome(), encoding_event->get_flag(), 
	       encoding_event->get_source(), encoding_event->get_tag(), encoding_event->get_clock());
#endif 
  }
  //  rempi_dbgi(0, "-> encoding: %p, size: %d: count: %d", encoding_event, encoding_event_sequence.size(), count++);                                     
  if (rempi_gzip) {
    test_table->compressed_matched_events      = compression_util.compress_by_zlib((char*)original_buff, original_size, compressed_size);
    test_table->compressed_matched_events_size = compressed_size;
  } else {
    test_table->compressed_matched_events      = (char*)original_buff;
    test_table->compressed_matched_events_size = original_size;
  }
  
  // REMPI_DBG("xOriginal size: count:%5d(matched/unmatched entry) x 4 bytes x 5 = %d bytes,  Compressed size: %lu bytes , %d %lu", 
  //  	    input_format.total_length, original_size , compressed_size, 
  // 	    original_size, compressed_size)

  // REMPI_DBG("length: %5d, Size: %lu, %lu", 
  // 	    input_format.total_length, test_table->compressed_matched_events_size, original_size);

  return;
}

void rempi_encoder::write_record_file(rempi_encoder_input_format &input_format)
{
  char* whole_data;
  size_t whole_data_size;
  rempi_event *encoding_event;
  rempi_encoder_input_format_test_table *test_table;

  if (input_format.test_tables_map.size() > 1) {
    REMPI_ERR("test_table_size is %d", input_format.test_tables_map.size());
  }

  test_table = input_format.test_tables_map[0];
  whole_data      = test_table->compressed_matched_events;
  whole_data_size = test_table->compressed_matched_events_size;
  record_fs.write(whole_data, whole_data_size);
  write_size_vec.push_back(whole_data_size);

  /*Free all recorded data*/
  input_format.clear();

  return;
}

void rempi_encoder::close_record_file()
{
  size_t total_write_size = 0;
  for (int i = 0, n = write_size_vec.size(); i < n; i++) {
    total_write_size += write_size_vec[i];
  }
  //  REMPI_DBG("EVAL Total write size: |%lu|", total_write_size);
  record_fs.close();
}

/*File to vector */
bool rempi_encoder::read_record_file(rempi_encoder_input_format &input_format)
{
  char* decoding_event_sequence;
  size_t* decoding_event_sequence_int;
  rempi_event* decoded_event;
  size_t size;
  bool is_no_more_record = false;

  decoding_event_sequence = (char*)malloc(rempi_event::record_num * rempi_event::record_element_size);
  record_fs.read(decoding_event_sequence, rempi_event::record_num * rempi_event::record_element_size);
  size = record_fs.gcount();
  if (size == 0) {
    delete decoding_event_sequence;
    is_no_more_record = true;
    return is_no_more_record;
  }
  decoding_event_sequence_int = (size_t*)decoding_event_sequence;
  
  // for (int i = 0; i < 8; i++) {
  //   fprintf(stderr, "int: %d   ==== ", decoding_event_sequence_int[i]);
  // }
  // REMPI_DBG("");

  decoded_event = new rempi_test_event(
				       (int)decoding_event_sequence_int[0],
				       (int)decoding_event_sequence_int[1],
					-1,
				       (int)decoding_event_sequence_int[2],
				       (int)decoding_event_sequence_int[3],
				       -1,
				       (int)decoding_event_sequence_int[4],
					0
					);
#if 0
  REMPI_DBGI(0, "Read    ; (count: %d, with_next: %d, flag: %d, source: %d, clock: %d)", 
	    (int)decoding_event_sequence_int[0],
	    (int)decoding_event_sequence_int[1],
	    (int)decoding_event_sequence_int[2],
	    (int)decoding_event_sequence_int[3],
	    (int)decoding_event_sequence_int[4]);
#endif
  input_format.add(decoded_event, 0);
  is_no_more_record = false;
  return is_no_more_record;
}


void rempi_encoder::decode(rempi_encoder_input_format &input_format)
{
  /*Do nothing*/
  return;
}

/*vector to event_list*/
void rempi_encoder::insert_encoder_input_format_chunk(rempi_event_list<rempi_event*> &recording_events, rempi_event_list<rempi_event*> &replaying_events, rempi_encoder_input_format &input_format)
{
  size_t size;
  rempi_event *decoded_event;
  rempi_encoder_input_format_test_table *test_table;

  /*Get decoded event to replay*/
  test_table = input_format.test_tables_map[0];
  decoded_event = test_table->events_vec[0];
  test_table->events_vec.pop_back();


#ifdef REMPI_DBG_REPLAY
  REMPI_DBGI(REMPI_DBG_REPLAY, "Decoded ; (count: %d, with_next: %d, flag: %d, source: %d, tag: %d, clock: %d)",
	     decoded_event->get_event_counts(), decoded_event->get_is_testsome(), decoded_event->get_flag(),
	     decoded_event->get_source(), decoded_event->get_tag(), decoded_event->get_clock());
#endif

  if (decoded_event->get_flag() == 0) {
    /*If this is flag == 0, simply enqueue it*/
    replaying_events.enqueue_replay(decoded_event, 0);
    return;
  }

  
  rempi_event *matched_replaying_event;
  if (decoded_event->get_flag()) {
    while ((matched_replaying_event = pop_event_pool(decoded_event)) == NULL) { /*Pop from matched_event_pool*/
      /*If this event is matched test, wait until this message really arrive*/
      rempi_event *matched_event;
      int event_list_status;
      while (recording_events.front_replay(0) == NULL) { 
	/*Wait until any message arraves */
	usleep(100);
      }
      matched_event = recording_events.dequeue_replay(0, event_list_status);
      matched_event_pool.push_back(matched_event);
#ifdef REMPI_DBG_REPLAY
      REMPI_DBGI(REMPI_DBG_REPLAY, "RCQ->PL ; (count: %d, with_next: %d, flag: %d, source: %d, tag: %d, clock: %d, msg_count: %d %p)",
		 matched_event->get_event_counts(), matched_event->get_is_testsome(), matched_event->get_flag(),
		 matched_event->get_source(), matched_event->get_tag(), matched_event->get_clock(), matched_event->msg_count, 
		 matched_event);
#endif
    }
  } 

  /* matched_replaying_event == decoded_event: this two event is same*/

  decoded_event->msg_count = matched_replaying_event->msg_count;
#ifdef REMPI_DBG_REPLAY
  REMPI_DBGI(REMPI_DBG_REPLAY, "PL->RPQ ; (count: %d, with_next: %d, flag: %d, source: %d, tag: %d, clock: %d, msg_count: %d)",
	     decoded_event->get_event_counts(), decoded_event->get_is_testsome(), decoded_event->get_flag(),
	     decoded_event->get_source(), decoded_event->get_tag(), decoded_event->get_clock(), decoded_event->msg_count);
#endif


  replaying_events.enqueue_replay(decoded_event, 0);
  delete matched_replaying_event;
  return;    
}


void rempi_encoder::fetch_local_min_id(int *min_recv_rank, size_t *min_next_clock)
{    
  REMPI_ERR("please remove this REMPI_ERR later");
  return;
}

void rempi_encoder::update_local_min_id(int min_recv_rank, size_t min_next_clock)
{    
  REMPI_ERR("please remove this REMPI_ERR later");
  return;
}

void rempi_encoder::update_fd_next_clock(
					 int is_waiting_recv,
					 int num_of_recv_msg_in_next_event,
					 size_t interim_min_clock_in_next_event,
					 size_t enqueued_count,
					 int recv_test_id,
					 int is_after_recv_event)
{ 
  REMPI_ERR("please remove this REMPI_ERR later");
  return;
}

void rempi_encoder::compute_local_min_id(rempi_encoder_input_format_test_table *test_table, int *local_min_id_rank, size_t *local_min_id_clock)
{
  REMPI_ERR("please remove this REMPI_ERR later");
  return;
}


// char* rempi_encoder::read_decoding_event_sequence(size_t *size)
// {
//   char* decoding_event_sequence;

//   decoding_event_sequence = new char[rempi_event::max_size];
//   record_fs.read(decoding_event_sequence, rempi_event::max_size);
//   /*Set size read*/
//   *size = record_fs.gcount();
//   return decoding_event_sequence;
// }



// vector<rempi_event*> rempi_encoder::decode(char *serialized_data, size_t *size)
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








