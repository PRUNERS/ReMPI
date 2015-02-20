#ifndef __REMPI_ENCODER_H__
#define __REMPI_ENCODER_H__

#include <fstream>
#include <string>

#include "rempi_event.h"
#include "rempi_event_list.h"

using namespace std;

class rempi_encoder_input_format
{
 public:
  /*Used for any compression*/
  vector<rempi_event*> events_vec;

  /*Used for CDC*/
  vector<size_t>               with_previous_vec;
  size_t                       compressed_with_previous_size;
  char*                        compressed_with_previous;
  vector<pair<size_t, size_t>> unmatched_events_vec;
  size_t                       compressed_unmatched_events_size;
  char*                        compressed_unmatched_events;
  map<int, rempi_event*>       matched_events_ordered_map;
  size_t                       compressed_matched_events_size;
  char*                        compressed_matched_events;


  size_t  length();
  virtual void add(rempi_event *event);
  virtual void format();
  virtual void debug_print();
};


class rempi_encoder
{
 protected:
    int mode;
    string record_path;
    fstream record_fs;
 public:

    rempi_event_list<rempi_event*> *events;
    rempi_encoder(int mode);
    /*Common for record & replay*/
    void open_record_file(string record_path);
    void close_record_file();
    void write_record_file(char* encoded_events, size_t size);
    char* read_decoding_event_sequence(size_t *size);

    /*For only record*/
    virtual bool extract_encoder_input_format_chunk(rempi_event_list<rempi_event*> &events, rempi_encoder_input_format &input_format);
    virtual char* encode(rempi_encoder_input_format &input_format, size_t &size);

    /*For only replay*/
    virtual vector<rempi_event*> decode(char *serialized, size_t *size);
};



class rempi_encoder_cdc_input_format: public rempi_encoder_input_format
{
 private:
  //  bool compare(rempi_event *event1, rempi_event *event2);
 public:
 

  virtual void add(rempi_event *event);
  virtual void format();
  virtual void debug_print();
};


class rempi_encoder_cdc : public rempi_encoder
{
 public:
 
    rempi_encoder_cdc(int mode);
    /*For only record*/
    virtual bool extract_encoder_input_format_chunk(rempi_event_list<rempi_event*> &events, rempi_encoder_input_format &input_format);
    virtual char* encode(rempi_encoder_input_format &input_format, size_t &size);
    /*For only replay*/
    virtual vector<rempi_event*> decode(char *serialized, size_t *size);
};

#endif /* __REMPI_ENCODER_H__ */
