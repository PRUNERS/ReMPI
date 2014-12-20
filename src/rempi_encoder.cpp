#include <vector>

#include "rempi_err.h"
#include "rempi_event.h"
#include "rempi_encoder.h"

rempi_encoder::rempi_encoder() {}
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
  delete encoding_event;  
  return serialized_data;
}
char* rempi_encoder::read_decoding_event_sequence(size_t *size)
{return NULL;}

vector<rempi_event*> rempi_encoder::decode(char *serialized, size_t *size)
{
  vector<rempi_event*> vec;
  return vec;
}

// rempi_encoder_no_encoding::rempi_encoder_no_encoding(rempi_event_list<rempi_event*> *events)
//   : events(events){}
// vector<rempi_event*> rempi_encoder_no_encoding::get_encoding_event_sequence(){}
// char* rempi_encoder_no_encoding::encode(vector<rempi_event*> events, size_t *size){}
// char* rempi_encoder_no_encoding::read_decoding_event_sequence(size_t *size){}
// vector<rempi_event*> decode(char *serialized, size_t *size){}




