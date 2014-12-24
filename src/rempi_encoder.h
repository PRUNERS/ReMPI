#ifndef __REMPI_ENCODER_H__
#define __REMPI_ENCODER_H__

#include <fstream>
#include <string>

#include "rempi_event.h"
#include "rempi_event_list.h"

using namespace std;

class rempi_encoder
{
 public:
    int mode;
    string record_path;
    fstream record_fs;

    rempi_event_list<rempi_event*> *events;
    rempi_encoder(int mode);
    /*Common for record & replay*/
    virtual void open_record_file(string record_path);
    virtual void close_record_file();
    /*For only record*/
    virtual void get_encoding_event_sequence(rempi_event_list<rempi_event*> &events, vector<rempi_event*> &encoding_event_sequence);
    virtual char* encode(vector<rempi_event*> &encoding_event_sequence, size_t &size);
    virtual void write_record_file(char* encoded_events, size_t size);
    /*For only replay*/
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
