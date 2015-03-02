#ifndef __REMPI_ENCODER_H__
#define __REMPI_ENCODER_H__

#include <fstream>
#include <string>
#include <map>

#include "rempi_event.h"
#include "rempi_event_list.h"
#include "rempi_compression_util.h"
#include "rempi_clock_delta_compression.h"

using namespace std;

class rempi_encoder_input_format_test_table
{
 public:
  /*Used for any compression*/
  vector<rempi_event*> events_vec;
  /*Used for CDC*/
  int count = 0;
  vector<size_t>               with_previous_vec;
  size_t                       compressed_with_previous_length = 0;
  size_t                       compressed_with_previous_size   = 0;
  char*                        compressed_with_previous        = NULL;
  vector<size_t>               unmatched_events_id_vec;
  vector<size_t>               unmatched_events_count_vec;
  size_t                       compressed_unmatched_events_id_size = 0;
  char*                        compressed_unmatched_events_id      = NULL;
  size_t                       compressed_unmatched_events_count_size = 0;
  char*                        compressed_unmatched_events_count      = NULL;
  map<int, rempi_event*>       matched_events_ordered_map;
  size_t                       compressed_matched_events_size = 0;
  char*                        compressed_matched_events      = NULL;
};

class rempi_encoder_input_format
{
 public:
  size_t total_length = 0;
  /*Used for CDC*/  
  map<int, rempi_encoder_input_format_test_table*> test_tables_map;  

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
    virtual rempi_encoder_input_format* create_encoder_input_format();
    void open_record_file(string record_path);
    void close_record_file();
    virtual void write_record_file(rempi_encoder_input_format &input_format);
    virtual char* read_decoding_event_sequence(size_t *size);

    /*For only record*/
    virtual bool extract_encoder_input_format_chunk(rempi_event_list<rempi_event*> &events, rempi_encoder_input_format &input_format);
    virtual void encode(rempi_encoder_input_format &input_format);

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
 protected:
  rempi_compression_util<size_t> compression_util;
  rempi_clock_delta_compression *cdc;
  virtual void compress_non_matched_events(rempi_encoder_input_format_test_table  *test_table);
  virtual void compress_matched_events(rempi_encoder_input_format_test_table  *test_table);
 public:
  rempi_encoder_cdc(int mode);
  /*For common*/
  virtual rempi_encoder_input_format* create_encoder_input_format();
  void write_record_file(rempi_encoder_input_format &input_format);
  /*For only record*/
  virtual bool extract_encoder_input_format_chunk(rempi_event_list<rempi_event*> &events, rempi_encoder_input_format &input_format);
  virtual void encode(rempi_encoder_input_format &input_format);
  /*For only replay*/
  virtual vector<rempi_event*> decode(char *serialized, size_t *size);
};

class rempi_encoder_cdc_row_wise_diff : public rempi_encoder_cdc
{
 protected:
  virtual void compress_matched_events(rempi_encoder_input_format_test_table  *test_table);
 public:
  rempi_encoder_cdc_row_wise_diff(int mode);
  /*For only replay*/
  virtual vector<rempi_event*> decode(char *serialized, size_t *size);
};


class rempi_encoder_zlib : public rempi_encoder_cdc
{
 protected:
  virtual void compress_matched_events(rempi_encoder_input_format_test_table  *test_table);
 public:
    rempi_encoder_zlib(int mode);
    /*For only replay*/
    virtual vector<rempi_event*> decode(char *serialized, size_t *size);
};

class rempi_encoder_simple_zlib : public rempi_encoder_zlib
{
 public:
  rempi_encoder_simple_zlib(int mode);
  virtual rempi_encoder_input_format* create_encoder_input_format();
};

class rempi_encoder_cdc_permutation_diff : public rempi_encoder_cdc
{
 protected:
  virtual void compress_matched_events(rempi_encoder_input_format_test_table  *test_table);
 public:
    rempi_encoder_cdc_permutation_diff(int mode);
    /*For only replay*/
    virtual vector<rempi_event*> decode(char *serialized, size_t *size);
};

#endif /* __REMPI_ENCODER_H__ */
