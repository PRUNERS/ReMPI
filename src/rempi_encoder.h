#ifndef __REMPI_ENCODER_H__
#define __REMPI_ENCODER_H__

#include <fstream>
#include <string>
#include <map>
#include <array>

#include "rempi_event.h"
#include "rempi_event_list.h"
#include "rempi_compression_util.h"
#include "rempi_clock_delta_compression.h"

/*In CDC, rempi_endoers needs to fetch and update next_clocks, 
so CLMPI and PNMPI module need to be included*/
#include "clmpi.h"
#include "pnmpimod.h"

using namespace std;

class rempi_encoder_input_format_test_table
{
 public:
  bool is_decoded_all();
  bool is_pending_all_rank_msg();


  /*Used for any compression*/
  vector<rempi_event*> events_vec;

  /*Used for CDC*/
  int count = 0;

  unordered_map<size_t, size_t>      epoch_umap;
  vector<size_t>                  epoch_rank_vec;  /*ranks creating this epoch line in this test_table*/
  vector<size_t>                  epoch_clock_vec; /*each value of epoch line of ranks in  epoch_rank_vec*/
  size_t                       epoch_size; /*each size of epoch_{rank|clock}_vec. so epoch_size x 2 is total size of epoch*/


  vector<size_t>               with_previous_vec;
  vector<bool>                 with_previous_bool_vec;
  size_t                       compressed_with_previous_length = 0;
  size_t                       compressed_with_previous_size   = 0;
  char*                        compressed_with_previous        = NULL;
  vector<size_t>               unmatched_events_id_vec;
  vector<size_t>               unmatched_events_count_vec;
  unordered_map<size_t, size_t>        unmatched_events_umap; /*Used in replay*/
  size_t                       compressed_unmatched_events_id_size = 0;
  char*                        compressed_unmatched_events_id      = NULL;
  size_t                       compressed_unmatched_events_count_size = 0;
  char*                        compressed_unmatched_events_count      = NULL;
  map<int, rempi_event*>       matched_events_ordered_map; /*Used in recording*/
  vector<size_t>               matched_events_id_vec;      /*Used in replay*/
  vector<size_t>               matched_events_delay_vec;   /*Used in replay*/
  vector<int>                  matched_events_square_sizes_vec;/*Used in replay*/
  int                          matched_events_square_sizes_vec_index = 0;/*Used in replay*/
  vector<int>                  matched_events_permutated_indices_vec; /*Used in replay*/
  size_t                       replayed_matched_event_index = 0;
  size_t                       compressed_matched_events_size = 0;
  char*                        compressed_matched_events      = NULL;

  /*== Used in replay decode ordering == */
  /*All events go to this list, then sorted*/
  list<rempi_event*>                 ordered_event_list; 
  /*Events (< minimul clock) in ordered_event_list go to this list.
    It is gueranteed this order is solid, and does not change by future events
  */
  list<rempi_event*>                 solid_ordered_event_list; 
  //  unordered_map<int, list<rempi_event*>*>  pending_events_umap;
  unordered_map<int, int> pending_event_count_umap; /*rank->pending_counts*/
  unordered_map<int, int> current_epoch_line_umap;  /* rank->max clock of this rank*/
  //  rempi_event*                       min_clock_event_in_current_epoch = NULL;
  
  void clear();
};

class rempi_encoder_input_format
{
 public:
  size_t total_length = 0;
  
  /*Used for CDC*/  
  map<int, rempi_encoder_input_format_test_table*> test_tables_map;  

  /* === For CDC replay ===*/
  int    mc_length       = -1;
  /*All source ranks in all receive MPI functions*/
  int    *mc_recv_ranks  = NULL; 
  /*List of all next_clocks of recv_ranks, rank recv_ranks[i]'s next_clocks is next_clocks[i]*/
  size_t *mc_next_clocks = NULL;
  /* ======================*/
  

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
  /*For CDC replay*/

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


    /* === For CDC replay ===*/
    int    mc_flag         = 0; /*if mc_flag == 1: main thread execute frontier detection*/
    int    mc_length       = 0;
    /*All source ranks in all receive MPI functions*/
    int    *mc_recv_ranks  = NULL; 
    /*List of all next_clocks of recv_ranks, rank recv_ranks[i]'s next_clocks is next_clocks[i]*/
    size_t *mc_next_clocks = NULL;
    /* ======================*/

    vector<size_t> write_size_vec;

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

    /*TODO: Due to multi-threaded issues in MPI/PNMPI, we define this function.
      But we would like to remove this function in future*/
    virtual void fetch_local_min_id (int *min_recv_rank, size_t *min_next_clock);
    virtual void update_local_min_id(int min_recv_rank, size_t min_next_clock);
    virtual void update_fd_next_clock(int is_waiting_recv, int num_of_recv_msg_in_next_event);
    int *num_of_recv_msg_in_next_event = NULL; /*array[i] contain the number of test_id=i*/
    
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
  struct local_minimal_id {
    int rank;
    size_t clock;
  };
  

  struct frontier_detection_clocks{ /*fd = frontier detection*/
    size_t next_clock;    /*TODO: Remove    next_clock, which is not used any more */
    size_t trigger_clock; /*TODO: Remove trigger_clock, which is not used any more */
  };

 private:
  /* === For frontier detection === */
  MPI_Comm mpi_fd_clock_comm;
  MPI_Win mpi_fd_clock_win;
  PNMPIMOD_get_local_clock_t clmpi_get_local_clock;
  struct local_minimal_id local_min_id= {.rank=-1, .clock=0};
  struct frontier_detection_clocks *fd_clocks = NULL;

  /* ============================== */

  void cdc_prepare_decode_indices(
				  size_t matched_event_count,
				  vector<size_t> &matched_events_id_vec,
				  vector<size_t> &matched_events_delay_vec,
				  vector<int> &matched_events_square_sizes,
				  vector<int> &matched_events_permutated_indices);
  bool cdc_decode_ordering(rempi_event_list<rempi_event*> &recording_events, vector<rempi_event*> &event_vec, rempi_encoder_input_format_test_table* test_table, list<rempi_event*> &replay_event_list, int test_id);


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

  virtual void fetch_local_min_id (int *min_recv_rank, size_t *min_next_clock);
  virtual void update_local_min_id(int min_recv_rank, size_t min_next_clock);
  virtual void update_fd_next_clock(int is_waiting_recv, int num_of_recv_msg_in_next_event);

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
