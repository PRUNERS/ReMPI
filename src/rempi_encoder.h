#ifndef __REMPI_ENCODER_H__
#define __REMPI_ENCODER_H__

#include <string>

#include "rempi_event.h"
#include "rempi_event_list.h"

using namespace std;

class rempi_encoder
{
  public:
    rempi_event_list<rempi_event*> *events;

    rempi_encoder();
    virtual void get_encoding_event_sequence(rempi_event_list<rempi_event*> &events, vector<rempi_event*> &encoding_event_sequence);
      //    virtual void get_encoding_event_sequence(vector<rempi_event*> &encoding_event_sequence);
    virtual char* encode(vector<rempi_event*> &encoding_event_sequence, size_t &size);
    virtual char* read_decoding_event_sequence(size_t *size);
    virtual vector<rempi_event*> decode(char *serialized, size_t *size);
};

/* class rempi_encoder_no_encoding : public rempi_encoder */
/* { */
/*   public: */
/*     rempi_event_list<rempi_event*> *events; */
/*     rempi_encoder_no_encoding(rempi_event_list<rempi_event*> *events); */
/*     virtual vector<rempi_event*> get_encoding_event_sequence(); */
/*     virtual char* encode(vector<rempi_event*> events, size_t *size); */
/*     virtual char* read_decoding_event_sequence(size_t *size); */
/*     virtual vector<rempi_event*> decode(char *serialized, size_t *size); */
/* }; */

#endif /* __REMPI_ENCODER_H__ */
