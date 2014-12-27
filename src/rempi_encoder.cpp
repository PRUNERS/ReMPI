#include <vector>

#include "rempi_err.h"
#include "rempi_event.h"
#include "rempi_encoder.h"
#include "rempi_config.h"



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

void rempi_encoder::get_encoding_event_sequence(rempi_event_list<rempi_event*> &events, vector<rempi_event*> &encoding_event_sequence)
{
  rempi_event *event;
  event = events.pop();
  /*encoding_event_sequence get only one event*/
  if (event != NULL) {
    //    rempi_dbgi(0, "pusu event: %p", event);
    encoding_event_sequence.push_back(event);
  }
  return;
}

int count;

char* rempi_encoder::encode(vector<rempi_event*> &encoding_event_sequence, size_t &size)
{
  char* serialized_data;
  rempi_event *encoding_event;
  /*encoding_event_sequence has only one event*/
  encoding_event = encoding_event_sequence[0];
  //  rempi_dbgi(0, "-> encoding: %p, size: %d: count: %d", encoding_event, encoding_event_sequence.size(), count++);
  serialized_data = encoding_event->serialize(size); 
  encoding_event_sequence.pop_back();
  //  rempi_dbgi(0, "--> encoding: %p, size: %d: count: %d", encoding_event, encoding_event_sequence.size(), count++);
  encoding_event->print();
  fprintf(stderr, "\n");
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




