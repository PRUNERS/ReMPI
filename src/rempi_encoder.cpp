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
  return events_vec.size();
}

void rempi_encoder_input_format::add(rempi_event *event)
{
  events_vec.push_back(event);
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

void rempi_encoder::write_record_file(char* encoded_events, size_t size)
{
  record_fs.write(encoded_events, size);
  //TODO: delete encoded_event
}

void rempi_encoder::close_record_file()
{
  record_fs.close();
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
  event = events.pop();
  bool is_extracted = false;
  /*encoding_event_sequence get only one event*/
  if (event != NULL) {
    //    rempi_dbgi(0, "pusu event: %p", event);                                                                                                               input_format.events_vec.push_back(event);
    is_extracted = true;
  }
  return is_extracted;
}


char* rempi_encoder::encode(rempi_encoder_input_format &input_format, size_t &size)
{
  char* serialized_data;
  rempi_event *encoding_event;
  /*encoding_event_sequence has only one event*/
  encoding_event = input_format.events_vec[0];
  //  rempi_dbgi(0, "-> encoding: %p, size: %d: count: %d", encoding_event, encoding_event_sequence.size(), count++);                                      
  serialized_data = encoding_event->serialize(size);
  input_format.events_vec.pop_back();
  //  rempi_dbgi(0, "--> encoding: %p, size: %d: count: %d", encoding_event, encoding_event_sequence.size(), count++);                                      
  //  encoding_event->print();                                                                                                                              
  //  fprintf(stderr, "\n");                                                                                                                                
  delete encoding_event;
  return serialized_data;
}

char* rempi_encoder::read_decoding_event_sequence(size_t *size)
{
  char* decoding_event_sequence;

  decoding_event_sequence = new char[rempi_event::max_size];
  record_fs.read(decoding_event_sequence, rempi_event::max_size);
  /*Set size read*/
  *size = record_fs.gcount();
  return decoding_event_sequence;
}



vector<rempi_event*> rempi_encoder::decode(char *serialized_data, size_t *size)
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

// rempi_encoder_no_encoding::rempi_encoder_no_encoding(rempi_event_list<rempi_event*> *events)
//   : events(events){}
// vector<rempi_event*> rempi_encoder_no_encoding::get_encoding_event_sequence(){}
// char* rempi_encoder_no_encoding::encode(vector<rempi_event*> events, size_t *size){}
// char* rempi_encoder_no_encoding::read_decoding_event_sequence(size_t *size){}
// vector<rempi_event*> decode(char *serialized, size_t *size){}




