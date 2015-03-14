#include <vector>

#include "rempi_err.h"
#include "rempi_event.h"
#include "rempi_encoder.h"
#include "rempi_config.h"
#include "rempi_compression_util.h"

#define MAX_INPUT_FORMAT_LENGTH (2000000000)

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

void rempi_encoder_input_format::debug_print()
{
  return;
}


/* ==================================== */
/*      CLASS rempi_encoder             */
/* ==================================== */

rempi_encoder::rempi_encoder(int mode): mode(mode) {}

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
  list<rempi_event*>::const_iterator matched_event_pool_it;
  for (matched_event_pool_it  = matched_event_pool.cbegin();
       matched_event_pool_it != matched_event_pool.cend();
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

  if (input_format.length() > MAX_INPUT_FORMAT_LENGTH || events.is_push_closed_()) {
    /*If got enough chunck size, OR the end of run*/
    input_format.format();
    is_ready_for_encoding = true;
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
  for (int i = 0, j = 0; i < test_table->events_vec.size(); i++, j += recoding_inputs_num) {
    int encodign_buff_head_index = i * recoding_inputs_num;
    encoding_event = test_table->events_vec[i];    
    original_buff[encodign_buff_head_index + 0] = encoding_event->get_event_counts();
    original_buff[encodign_buff_head_index + 1] = encoding_event->get_is_testsome();
    original_buff[encodign_buff_head_index + 2] = encoding_event->get_flag();
    original_buff[encodign_buff_head_index + 3] = encoding_event->get_source();
    original_buff[encodign_buff_head_index + 4] = encoding_event->get_clock();
  }
  //  rempi_dbgi(0, "-> encoding: %p, size: %d: count: %d", encoding_event, encoding_event_sequence.size(), count++);                                     
#if 0
  test_table->compressed_matched_events      = compression_util.compress_by_zlib((char*)original_buff, original_size, compressed_size);
  test_table->compressed_matched_events_size = compressed_size;
#endif
  test_table->compressed_matched_events      = (char*)original_buff;
  test_table->compressed_matched_events_size = original_size;
  
  REMPI_DBG("xOriginal size: count:%5d(matched/unmatched entry) x 4 bytes x 5 = %d bytes,  Compressed size: %lu bytes , %d %lu", 
	    input_format.total_length, original_size , compressed_size, 
	    original_size, compressed_size)

  return;
}

void rempi_encoder::write_record_file(rempi_encoder_input_format &input_format)
{
  char* whole_data;
  size_t whole_data_size;
  rempi_event *encoding_event;
  rempi_encoder_input_format_test_table *test_table;

  test_table = input_format.test_tables_map[0];
  whole_data      = test_table->compressed_matched_events;
  whole_data_size = test_table->compressed_matched_events_size;
  record_fs.write(whole_data, whole_data_size);
  
  encoding_event = test_table->events_vec[0];
  test_table->events_vec.pop_back();
  delete encoding_event;
  return;
}

void rempi_encoder::close_record_file()
{
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
  REMPI_DBG("Read  : event:%d is_testsome:%d request:%d flag:%d source:%d tag:%d clock:%d test_id:%d",   
	    (int)decoding_event_sequence_int[0],
	    (int)decoding_event_sequence_int[1],
	    -1,
	    (int)decoding_event_sequence_int[2],
	    (int)decoding_event_sequence_int[3],
	    -1,
	    (int)decoding_event_sequence_int[4],
	    0);
#endif
  input_format.add(decoded_event, 0);
  is_no_more_record = false;
  return is_no_more_record;
}

void rempi_encoder::decode(rempi_encoder_input_format &input_format)
{
  return;
}

/*vector to event_list*/
void rempi_encoder::insert_encoder_input_format_chunk(rempi_event_list<rempi_event*> &recording_events, rempi_event_list<rempi_event*> &replaying_events, rempi_encoder_input_format &input_format)
{
  size_t size;
  rempi_event *decoded_event;
  rempi_encoder_input_format_test_table *test_table;
 
  test_table = input_format.test_tables_map[0];
  decoded_event = test_table->events_vec[0];
  test_table->events_vec.pop_back();
  if (decoded_event->get_flag() == 0) {
    replaying_events.enqueue_replay(decoded_event, 0);
    return;
  } 

  while (is_event_pooled(decoded_event)) {
    rempi_event *matched_event;

    while (recording_events.front() == NULL) {
      usleep(100);
    }
    matched_event = recording_events.pop();
    matched_event_pool.push_front(matched_event);
  }
  REMPI_DBG("Decoded  : (flag: %d, source: %d)", decoded_event->get_flag(), decoded_event->get_source());
  replaying_events.enqueue_replay(decoded_event, 0);
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








