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

  unordered_map<size_t, size_t>      epoch_umap;
  vector<size_t>                  epoch_rank_vec;
  vector<size_t>                  epoch_clock_vec;
  size_t                       epoch_size; /*each size of epoch_{rank|clock}_vec. so epoch_size x 2 is total size of epoch*/


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
  map<int, rempi_event*>       matched_events_ordered_map; /*Used in recording*/
  vector<size_t>                  matched_events_id_vec;  /*Used in replay*/
  vector<size_t>                  matched_events_delay_vec;  /*Used in replay*/
  size_t                       compressed_matched_events_size = 0;
  char*                        compressed_matched_events      = NULL;
  
  void clear();
};

class rempi_encoder_input_format
{
 public:
  size_t total_length = 0;
  
  /*Used for CDC*/  
  map<int, rempi_encoder_input_format_test_table*> test_tables_map;  

  /*vector to write to file*/
  vector<char*>  write_queue_vec;
  vector<size_t> write_size_queue_vec;
  /**/
  size_t decompressed_size;

  size_t  length();
  virtual void add(rempi_event *event);
  virtual void add(rempi_event *event, int test_id);
  virtual void format();
  virtual void debug_print();
  void clear();
};


class rempi_encoder
{
 protected:
    int mode;
    string record_path;
    fstream record_fs;
    rempi_compression_util<size_t> compression_util;
    list<rempi_event*> matched_event_pool;
    bool is_event_pooled(rempi_event* replaying_event);
    rempi_event* pop_event_pool(rempi_event* replaying_event);
 public:

    rempi_event_list<rempi_event*> *events;
    rempi_encoder(int mode);
    /*Common for record & replay*/
    virtual rempi_encoder_input_format* create_encoder_input_format();

    void open_record_file(string record_path);
    void close_record_file();

    /*For only record*/
    virtual bool extract_encoder_input_format_chunk(rempi_event_list<rempi_event*> &events, rempi_encoder_input_format &input_format);
    virtual void encode(rempi_encoder_input_format &input_format);
    virtual void write_record_file(rempi_encoder_input_format &input_format);
    /*For only replay*/
    virtual bool read_record_file(rempi_encoder_input_format &input_format);
    virtual void decode(rempi_encoder_input_format &input_format);
    virtual void insert_encoder_input_format_chunk(rempi_event_list<rempi_event*> &recording_events, rempi_event_list<rempi_event*> &replaying_events, rempi_encoder_input_format &input_format);
    
    /*Old functions for replay*/
    //    virtual char* read_decoding_event_sequence(size_t *size);
    //    virtual vector<rempi_event*> decode(char *serialized, size_t *size);
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

  rempi_clock_delta_compression *cdc;
  virtual void compress_non_matched_events(rempi_encoder_input_format &input_format, rempi_encoder_input_format_test_table  *test_table);
  virtual void compress_matched_events(rempi_encoder_input_format &input_format, rempi_encoder_input_format_test_table  *test_table);
  virtual void decompress_non_matched_events(rempi_encoder_input_format &input_format);
  virtual void decompress_matched_events(rempi_encoder_input_format &input_format);
 public:
  rempi_encoder_cdc(int mode);
  /*For common*/
  virtual rempi_encoder_input_format* create_encoder_input_format();
  void write_record_file(rempi_encoder_input_format &input_format);
  /*For only record*/
  virtual bool extract_encoder_input_format_chunk(rempi_event_list<rempi_event*> &events, rempi_encoder_input_format &input_format);
  virtual void encode(rempi_encoder_input_format &input_format);
  /*For only replay*/
  virtual bool read_record_file(rempi_encoder_input_format &input_format);
  virtual void decode(rempi_encoder_input_format &input_format);
  virtual void insert_encoder_input_format_chunk(rempi_event_list<rempi_event*> &recording_events, rempi_event_list<rempi_event*> &replaying_events, rempi_encoder_input_format &input_format);

  //  virtual vector<rempi_event*> decode(char *serialized, size_t *size);
};

class rempi_encoder_cdc_row_wise_diff : public rempi_encoder_cdc
{
 protected:
  virtual void compress_matched_events(rempi_encoder_input_format &input_format, rempi_encoder_input_format_test_table  *test_table);
 public:
  rempi_encoder_cdc_row_wise_diff(int mode);
  /*For only replay*/
  virtual vector<rempi_event*> decode(char *serialized, size_t *size);
};


class rempi_encoder_zlib : public rempi_encoder_cdc
{
 protected:
  virtual void compress_matched_events(rempi_encoder_input_format &input_format, rempi_encoder_input_format_test_table  *test_table);
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
  virtual void compress_matched_events(rempi_encoder_input_format &input_format, rempi_encoder_input_format_test_table  *test_table);
 public:
    rempi_encoder_cdc_permutation_diff(int mode);
    /*For only replay*/
    virtual vector<rempi_event*> decode(char *serialized, size_t *size);
};

#endif /* __REMPI_ENCODER_H__ */
