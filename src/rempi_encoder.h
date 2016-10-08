#ifndef __REMPI_ENCODER_H__
#define __REMPI_ENCODER_H__

#include <fstream>
#include <string>
#include <map>
#include <array>
#include <unordered_set>

#include "rempi_event.h"
#include "rempi_event_list.h"
#include "rempi_compression_util.h"
#include "rempi_clock_delta_compression.h"
#include "rempi_config.h"
#include "rempi_mutex.h"

#define REMPI_ENCODER_REPLAYING_TYPE_ANY (0)
#define REMPI_ENCODER_REPLAYING_TYPE_RECV (1)
#define REMPI_ENCODER_NO_MATCHING_SET_ID (-1)

#define REMPI_ENCODER_NO_UPDATE (0)
#define REMPI_ENCODER_LAST_RECV_CLOCK_UPDATE (1)
#define REMPI_ENCODER_LOOK_AHEAD_CLOCK_UPDATE (2)

/*In CDC, rempi_endoers needs to fetch and update next_clocks, 
so CLMPI and PNMPI module need to be included*/
#if !defined(REMPI_LITE)
#include "clmpi.h"
#endif

using namespace std;

bool compare(
             rempi_event* event1,
             rempi_event* event2);

bool compare2(int source, size_t clock,
              rempi_event* event2);

void rempi_copy_vec(size_t* array, size_t length, vector<size_t>&vec);

void rempi_copy_vec_int(int* array, size_t length, vector<size_t>&vec);

class rempi_encoder_input_format_test_table
{
 public:
  bool is_decoded_all();
  bool is_pending_all_rank_msg();
  bool is_reached_epoch_line();


  /*Used for any compression*/
  vector<rempi_event*> events_vec;

  /*Used for CDC*/
  int count;// = 0;
  
  static unordered_set<int> all_epoch_rank_uset; /* Union of epoch_rank_vec across different epochs */


  unordered_map<size_t, size_t>      epoch_umap;   /*To keep track of current frontier line on replay*/
  vector<size_t>                  epoch_rank_vec;  /*ranks creating this epoch line in this test_table from record file*/
  vector<size_t>                  epoch_clock_vec; /*each value of epoch line of ranks in  epoch_rank_vec from record file*/
  unordered_map<int, size_t>      epoch_line_umap;  /*Map rank to last clock in the epoch */
  size_t                       epoch_size; /*each size of epoch_{rank|clock}_vec. so epoch_size x 2 is total size of epoch*/


  vector<size_t>               with_previous_vec;
  vector<bool>                 with_previous_bool_vec;
  size_t                       compressed_with_previous_length;// = 0;
  size_t                       compressed_with_previous_size;//   = 0;
  char*                        compressed_with_previous;//        = NULL;
  vector<size_t>               matched_index_vec;
  size_t                       compressed_matched_index_length;
  size_t                       compressed_matched_index_size;
  char*                        compressed_matched_index;
  vector<size_t>               unmatched_events_id_vec;
  vector<size_t>               unmatched_events_count_vec;
  unordered_map<size_t, size_t>        unmatched_events_umap; /*Used in replay*/
  size_t                       compressed_unmatched_events_id_size;// = 0;
  char*                        compressed_unmatched_events_id;//      = NULL;
  size_t                       compressed_unmatched_events_count_size;// = 0;
  char*                        compressed_unmatched_events_count;//      = NULL;
  map<int, rempi_event*>       matched_events_ordered_map; /*Used in recording*/
  vector<size_t>               matched_events_id_vec;      /*Used in replay*/
  vector<size_t>               matched_events_delay_vec;   /*Used in replay*/
  vector<int>                  matched_events_permutated_indices_vec; /*Used in replay*/
  size_t                       replayed_matched_event_index;// = 0;
  size_t                       compressed_matched_events_size;// = 0;
  char*                        compressed_matched_events;//      = NULL;

  rempi_encoder_input_format_test_table()
    : count(0)
    , compressed_with_previous_length(0)
    , compressed_with_previous_size(0)
    , compressed_with_previous(NULL)
    , compressed_matched_index_length(0)
    , compressed_matched_index_size(0)
    , compressed_matched_index(NULL)
    , compressed_unmatched_events_id_size(0)
    , compressed_unmatched_events_id(NULL)
    , compressed_unmatched_events_count_size(0)
    , compressed_unmatched_events_count(NULL)
    , replayed_matched_event_index(0)
    , compressed_matched_events_size(0)
    , compressed_matched_events(NULL) {}

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

  int within_epoch(rempi_event *e);  
  void clear();
};

class rempi_encoder_input_format
{
 public:
  rempi_event *last_added_event;
  char* decompressed_record_char;

  size_t total_length;// = 0;
  
  /*Used for CDC*/  
  map<int, rempi_encoder_input_format_test_table*> test_tables_map;  

  /* === For CDC replay ===*/
  int    mc_length;//       = -1;
  /*All source ranks in all receive MPI functions*/
  int    *mc_recv_ranks;//  = NULL;
#ifndef CP_DBG
  /*List of all next_clocks of recv_ranks, rank recv_ranks[i]'s next_clocks is next_clocks[i]*/
  size_t *mc_next_clocks;// = NULL;
  size_t *tmp_mc_next_clocks;
#endif
  /* ======================*/  

  /*vector to write to file*/
  vector<char*>  write_queue_vec;
  vector<size_t> write_size_queue_vec;
  /**/
  size_t decompressed_size;


  rempi_encoder_input_format()
    : total_length(0)
    , decompressed_record_char(NULL)
    , mc_length(-1)
    , mc_recv_ranks(NULL)
#ifndef CP_DBG
    , mc_next_clocks(NULL)
    , tmp_mc_next_clocks(NULL) 
#endif
    {}

  ~rempi_encoder_input_format() 
    {
      this->clear();
    }
  
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
  static const size_t all_epoch_rank_separator = 0;
  struct local_minimal_id {
    int rank;
    size_t clock;
  };
    size_t total_write_size;
    int mode;
    string record_path;
    fstream record_fs;
    rempi_compression_util<size_t> compression_util;
    list<rempi_event*> matched_event_pool;
    bool is_event_pooled(rempi_event* replaying_event);
    rempi_event* pop_event_pool(rempi_event* replaying_event);

    rempi_mutex progress_decoding_mtx;

    /* === For CDC replay ===*/
    int    mc_length;//       = 0;
    /*All source ranks in all receive MPI functions*/
    int    *mc_recv_ranks;//  = NULL; 
    vector<unordered_map<int, size_t>*> solid_mc_next_clocks_umap_vec; 
#ifndef CP_DBG
    int    mc_flag;//         = 0; /*if mc_flag == 1: main thread execute frontier detection*/
    /*List of all next_clocks of recv_ranks, rank recv_ranks[i]'s next_clocks is next_clocks[i]*/
    size_t *mc_next_clocks;// = NULL;
    /* temporal array until in-flight message checking*/
    size_t *tmp_mc_next_clocks;  
    /* If no in-flight message, copyed from tmp_mc_next_clocks to solid_ordered_event_list*/
#endif

    //    unordered_map<int, size_t> solid_mc_next_clocks_umap; 
    /* ======================*/

    vector<size_t> write_size_vec;

 public:
    struct local_minimal_id global_local_min_id; /*minimal <rank,clock> in all senders across different MFs*/

    int *num_of_recv_msg_in_next_event;// = NULL; /*array[i] contain the number of test_id=i*/
    size_t *interim_min_clock_in_next_event;// = NULL;
    size_t *dequeued_count;// = NULL;
    size_t tmp_fd_next_clock; /*Temporal variable for set_fd_clock_state*/

    rempi_event_list<rempi_event*> *events;

    rempi_encoder(int mode)
      : mode(mode) 
      , mc_length(-1)
      , mc_recv_ranks(NULL)
#ifndef CP_DBG
      , mc_flag(0)
      , mc_next_clocks(NULL) 
      , tmp_mc_next_clocks(NULL) 
#endif
      , num_of_recv_msg_in_next_event(NULL)
      , interim_min_clock_in_next_event(NULL) 
      , dequeued_count(NULL)
      , tmp_fd_next_clock(0)
      , total_write_size(0)
      {}

    /*Common for record & replay*/
    virtual rempi_encoder_input_format* create_encoder_input_format();

    void open_record_file(string record_path);
    virtual void close_record_file();
    void set_record_path(string record_path);

    /*For only record*/
    virtual bool extract_encoder_input_format_chunk(rempi_event_list<rempi_event*> &events, rempi_encoder_input_format &input_format);
    virtual void encode(rempi_encoder_input_format &input_format);
    virtual void write_record_file(rempi_encoder_input_format &input_format);
    /*For only replay*/
    virtual bool read_record_file(rempi_encoder_input_format &input_format);
    virtual void decode(rempi_encoder_input_format &input_format);
    virtual int progress_decoding(rempi_event_list<rempi_event*> *recording_events, rempi_event_list<rempi_event*> *replaying_events, int recv_test_id);
    virtual void insert_encoder_input_format_chunk(rempi_event_list<rempi_event*> &recording_events, rempi_event_list<rempi_event*> &replaying_events, rempi_encoder_input_format &input_format);




    virtual void compute_local_min_id(rempi_encoder_input_format_test_table *test_table, 
				      int *local_min_id_rank, 
				      size_t *local_min_id_clock,
				      int recv_test_id);
    virtual void set_fd_clock_state(int flag); /*flag: 1 during collective, flag: 0 during not collective*/

    
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

class rempi_encoder_basic : public rempi_encoder
{

 /* protected: */
 /*    size_t total_write_size; */
 /*    int mode; */
 /*    string record_path; */
 /*    fstream record_fs; */
 /*    rempi_compression_util<size_t> compression_util; */
 /*    list<rempi_event*> matched_event_pool; */
 /*    bool is_event_pooled(rempi_event* replaying_event); */
 /*    rempi_event* pop_event_pool(rempi_event* replaying_event); */


 /*    /\* === For CDC replay ===*\/ */
 /*    int    mc_flag;//         = 0; /\*if mc_flag == 1: main thread execute frontier detection*\/ */
 /*    int    mc_length;//       = 0; */
 /*    /\*All source ranks in all receive MPI functions*\/ */
 /*    int    *mc_recv_ranks;//  = NULL;  */
 /*    /\*List of all next_clocks of recv_ranks, rank recv_ranks[i]'s next_clocks is next_clocks[i]*\/ */
 /*    size_t *mc_next_clocks;// = NULL; */
 /*    /\* temporal array until in-flight message checking*\/ */
 /*    size_t *tmp_mc_next_clocks;   */
 /*    /\* If no in-flight message, copyed from tmp_mc_next_clocks to solid_ordered_event_list*\/ */
 /*    unordered_map<int, size_t> solid_mc_next_clocks_umap;  */
 /*    /\* ======================*\/ */

 /*    vector<size_t> write_size_vec; */

 public:
    rempi_encoder_basic(int mode);

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



    virtual void compute_local_min_id(rempi_encoder_input_format_test_table *test_table, 
				      int *local_min_id_rank, 
				      size_t *local_min_id_clock,
				      int recv_test_id);
    virtual void set_fd_clock_state(int flag); /*flag: 1 during collective, flag: 0 during not collective*/

};


#if MPI_VERSION == 3 && !defined(REMPI_LITE)
class rempi_encoder_cdc : public rempi_encoder
{  
  struct frontier_detection_clocks{ /*fd = frontier detection*/
    size_t next_clock;    /*TODO: Remove    next_clock, which is not used any more */
    //    size_t trigger_clock; /*TODO: Remove trigger_clock, which is not used any more */
  };

 private:
  int howto_update_look_ahead_recv_clock(int recv_rank, int matching_set_id, 
					 unordered_set<int> *probed_message_source_set,
					 unordered_set<int> *pending_message_source_set,
					 unordered_map<int, size_t> *recv_message_source_umap,
					 unordered_map<int, size_t> *recv_clock_umap,
					 unordered_map<int, size_t> *solid_mc_next_clocks_umap,
					 int replaying_matching_set_id);

 protected:
  /* === For frontier detection === */

  MPI_Comm mpi_fd_clock_comm;
  MPI_Win mpi_fd_clock_win;
  PNMPIMOD_get_local_clock_t clmpi_get_local_clock;
  PNMPIMOD_get_local_clock_t clmpi_get_local_sent_clock;

  int decoding_state;
  int finished_testsome_count;
  unordered_map<int, list<rempi_event*>*> matched_events_list_umap;
  list<rempi_event*> replay_event_list;
  rempi_encoder_input_format *decoding_input_format;


  /* ============================== */

  void init_recv_ranks(char* path);
  void cdc_prepare_decode_indices(
				  size_t matched_event_count,
				  vector<size_t> &matched_events_id_vec,
				  vector<size_t> &matched_events_delay_vec,
				  vector<int> &matched_events_permutated_indices);
  virtual bool cdc_decode_ordering(rempi_event_list<rempi_event*> *recording_events, list<rempi_event*> *event_vec, rempi_encoder_input_format_test_table* test_table, list<rempi_event*> *replay_event_list, int test_id, int local_min_id_rank, size_t local_min_id_clock);


  rempi_clock_delta_compression *cdc;
  virtual void compress_non_matched_events(rempi_encoder_input_format &input_format, rempi_encoder_input_format_test_table  *test_table);
  virtual void compress_matched_events(rempi_encoder_input_format &input_format, rempi_encoder_input_format_test_table  *test_table);
  virtual void decompress_non_matched_events(rempi_encoder_input_format &input_format);
  virtual void decompress_matched_events(rempi_encoder_input_format &input_format);

 public:

#ifndef CP_DBG
  struct frontier_detection_clocks *fd_clocks;// = NULL;
#endif

  rempi_encoder_cdc(int mode);
  ~rempi_encoder_cdc();

  void init_cp(const char* record_path);

  /*For common*/
  virtual rempi_encoder_input_format* create_encoder_input_format();
  void write_record_file(rempi_encoder_input_format &input_format);
  /*For only record*/
  virtual bool extract_encoder_input_format_chunk(rempi_event_list<rempi_event*> &events, rempi_encoder_input_format &input_format);
  virtual void encode(rempi_encoder_input_format &input_format);
  /*For only replay*/
  virtual bool read_record_file(rempi_encoder_input_format &input_format);
  virtual void decode(rempi_encoder_input_format &input_format);

  virtual int progress_decoding(rempi_event_list<rempi_event*> *recording_events, rempi_event_list<rempi_event*> *replaying_events, int recv_test_id);
  void insert_encoder_input_format_chunk_recv_test_id(rempi_event_list<rempi_event*> *recording_events, rempi_event_list<rempi_event*> *replaying_events, rempi_encoder_input_format *input_format, bool *is_finished, int *has_new_event, int recv_test_id);
  virtual void insert_encoder_input_format_chunk(rempi_event_list<rempi_event*> &recording_events, rempi_event_list<rempi_event*> &replaying_events, rempi_encoder_input_format &input_format);

  virtual void fetch_remote_look_ahead_send_clocks();
  virtual int update_local_look_ahead_recv_clock(
						 unordered_set<int> *probed_message_source_set, 
						 unordered_set<int> *update_sources_set, 
						 unordered_map<int, size_t> *recv_message_source_umap,
						 unordered_map<int, size_t> *recv_clock_umap,
						 int recv_test_id);

  virtual void update_local_look_ahead_send_clock(
						  int replaying_event_type,
						  int matching_set_id);

  virtual void compute_local_min_id(rempi_encoder_input_format_test_table *test_table, 
				    int *local_min_id_rank, 
				    size_t *local_min_id_clock,
				    int recv_test_id);
  virtual void set_fd_clock_state(int flag); /*flag: 1 during collective, flag: 0 during not collective*/    
  void close_record_file();

  //  virtual vector<rempi_event*> decode(char *serialized, size_t *size);
};
#endif


#if MPI_VERSION == 3 && !defined(REMPI_LITE)
class rempi_encoder_rep : public rempi_encoder_cdc
{
 public:
 rempi_encoder_rep(int mode)
   : rempi_encoder_cdc(mode) {}

  virtual void compress_non_matched_events(rempi_encoder_input_format &input_format, rempi_encoder_input_format_test_table  *test_table);
  virtual void compress_matched_events(rempi_encoder_input_format &input_format, rempi_encoder_input_format_test_table  *test_table);

 protected:
  virtual bool cdc_decode_ordering(rempi_event_list<rempi_event*> *recording_events, list<rempi_event*> *event_vec, rempi_encoder_input_format_test_table* test_table, list<rempi_event*> *replay_event_list, int test_id, int local_min_id_rank, size_t local_min_id_clock);

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

#endif /* MPI_VERSION == 3*/

#endif /* __REMPI_ENCODER_H__ */
