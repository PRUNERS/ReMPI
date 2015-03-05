#include <vector>

#include "rempi_err.h"
#include "rempi_event.h"
#include "rempi_encoder.h"
#include "rempi_config.h"


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



bool rempi_encoder::extract_encoder_input_format_chunk(rempi_event_list<rempi_event*> &events, rempi_encoder_input_format &input_format)
{
  rempi_event *event;
  rempi_encoder_input_format_test_table *test_table;
  bool is_extracted = false;
  event = events.pop();
  /*encoding_event_sequence get only one event*/
  if (event != NULL) {
    //    rempi_dbgi(0, "pusu event: %p", event);                                                              
    input_format.add(event, 0);
    is_extracted = true;
  }
  return is_extracted;
}


void rempi_encoder::encode(rempi_encoder_input_format &input_format)
{
  size_t size;
  rempi_event *encoding_event;
  rempi_encoder_input_format_test_table *test_table;
 
  test_table = input_format.test_tables_map[0];
  /*encoding_event_sequence has only one event*/
  encoding_event = test_table->events_vec[0];
  //  rempi_dbgi(0, "-> encoding: %p, size: %d: count: %d", encoding_event, encoding_event_sequence.size(), count++);                                      
  test_table->compressed_matched_events      = encoding_event->serialize(size);
  test_table->compressed_matched_events_size = size;

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
  int* decoding_event_sequence_int;
  rempi_event* decoded_event;
  size_t size;
  bool is_no_more_record = false;

  decoding_event_sequence = new char[rempi_event::max_size];
  record_fs.read(decoding_event_sequence, rempi_event::max_size);
  size = record_fs.gcount();
  if (size == 0) {
    delete decoding_event_sequence;
    is_no_more_record = true;
    return is_no_more_record;
  }
  decoding_event_sequence_int = (int*)decoding_event_sequence;

  
  // for (int i = 0; i < 8; i++) {
  //   fprintf(stderr, "int: %d   ==== ", decoding_event_sequence_int[i]);
  // }
  // REMPI_DBG("");

  decoded_event = new rempi_test_event(
					decoding_event_sequence_int[0],
					decoding_event_sequence_int[1],
					decoding_event_sequence_int[2],
					decoding_event_sequence_int[3],
					decoding_event_sequence_int[4],
					decoding_event_sequence_int[5],
					decoding_event_sequence_int[6],
					decoding_event_sequence_int[7]
					);

  input_format.add(decoded_event, 0);
  is_no_more_record = false;
  return is_no_more_record;
}

void rempi_encoder::decode(rempi_encoder_input_format &input_format)
{
  return;
}

/*vector to event_list*/
void rempi_encoder::insert_encoder_input_format_chunk(rempi_event_list<rempi_event*> &events, rempi_encoder_input_format &input_format)
{
  size_t size;
  rempi_event *decoded_event;
  rempi_encoder_input_format_test_table *test_table;
 
  test_table = input_format.test_tables_map[0];
  decoded_event = test_table->events_vec[0];
  test_table->events_vec.pop_back();
  events.enqueue_replay(decoded_event, 0);
  REMPI_DBG("Decoded  : (flag: %d, source: %d)", decoded_event->get_flag(), decoded_event->get_source());
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








