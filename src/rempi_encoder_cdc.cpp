#include <mpi.h>
#if MPI_VERSION == 3 && !defined(REMPI_LITE)

#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <vector>
#include <algorithm>
#include <unordered_set>

#include "rempi_err.h"
#include "rempi_mem.h"
#include "rempi_util.h"
#include "rempi_event.h"
#include "rempi_encoder.h"
#include "rempi_config.h"
#include "rempi_clock_delta_compression.h"
#include "rempi_compression_util.h"
#include "rempi_cp.h"
#include "rempi_request_mg.h"
#include "rempi_msg_buffer.h"


#define REMPI_DECODE_STATUS_OPEN         (0)
#define REMPI_DECODE_STATUS_CREATE_READ  (1)
#define REMPI_DECODE_STATUS_DECODE  (2)
#define REMPI_DECODE_STATUS_INSERT  (3)
#define REMPI_DECODE_STATUS_CLOSE   (4)
#define REMPI_DECODE_STATUS_COMPLETE   (5)


unordered_set<int> rempi_encoder_input_format_test_table::all_epoch_rank_uset;


/* ==================================== */
/* CLASS rempi_encoder_cdc_input_format_test_table */

bool rempi_encoder_input_format_test_table::is_decoded_all()
{

  if (!unmatched_events_umap.empty()) {
    return false;
  }

  if ((int)replayed_matched_event_index < count && count != 0) {
    return false;
  }

  // REMPI_DBGI(0, "true");
  // REMPI_DBGI(0,"unamtched size: %d, replayed count: %d, count: %d", 
      	     // this->unmatched_events_umap.size(), this->replayed_matched_event_index, count);
  
  return true;
}

/*TODO: This function is not used*/
bool rempi_encoder_input_format_test_table::is_pending_all_rank_msg()
{

// #ifdef REMPI_DBG_REPLAY
//     REMPI_DBGI(REMPI_DBG_REPLAY, "epoch_vec size : %d", epoch_rank_vec.size());
// #endif

  for (int i = 0, n = epoch_rank_vec.size(); i < n ; i++) {
    int rank = epoch_rank_vec[i];
    int count = pending_event_count_umap[rank];
    int last_clock = epoch_clock_vec[i];
    int current_lock = current_epoch_line_umap[rank];
#ifdef REMPI_DBG_REPLAY
    //    REMPI_DBGI(REMPI_DBG_REPLAY, "rank: %d count: %d", rank, count);
#endif

    if (count == 0 && last_clock != current_lock) {
      return false;
    } else if (count < 0) {
      REMPI_ERR("count is < 0");
    }
  }
  return true;
}

int rempi_encoder_input_format_test_table::within_epoch(rempi_event *e)
{
  int within_epoch = 0;
  int rank = e->get_source();
  if (e->get_clock() <= epoch_line_umap[rank]) {
    within_epoch = 1;
  } else {
    within_epoch = 0;
  }
  return within_epoch;
}


bool rempi_encoder_input_format_test_table::is_reached_epoch_line()
{

  // #ifdef REMPI_DBG_REPLAY
  //     REMPI_DBGI(REMPI_DBG_REPLAY, "epoch_vec size : %d", epoch_rank_vec.size());
  // #endif

  for (int i = 0, n = epoch_rank_vec.size(); i < n ; i++) {
    int rank = epoch_rank_vec[i];
    int last_clock = epoch_clock_vec[i];
    int current_lock = current_epoch_line_umap[rank];
#ifdef REMPI_DBG_REPLAY
    //    REMPI_DBGI(REMPI_DBG_REPLAY, "rank: %d count: %d", rank, count);
#endif
    if (last_clock != current_lock) {
      return false;
    }
  }
  return true;
}


/* ==================================== */


/* ==================================== */
/* CLASS rempi_encoder_cdc_input_format */
/* ==================================== */

bool compare(
	     rempi_event* event1,
	     rempi_event* event2)
{
  if (event1->get_clock() < event2->get_clock()) {
    return true;
  } else if (event1->get_clock() == event2->get_clock()) {
    return event1->get_source() < event2->get_source();
  }
  return false;
}

bool compare2(int source, size_t clock,
	      rempi_event* event2)
{
  if (clock < event2->get_clock()) {
    return true;
  } else if (clock == event2->get_clock()) {
    return source < event2->get_source();
  }
  return false;
}




void rempi_encoder_cdc_input_format::add(rempi_event *event)
{
  bool is_matched_event;
  rempi_encoder_input_format_test_table *test_table;
  is_matched_event = (event->get_flag() == 1);
  if (test_tables_map.find(event->get_test_id()) == test_tables_map.end()) {
    test_tables_map[event->get_test_id()] = new rempi_encoder_input_format_test_table(event->get_test_id());
  }
  test_table = test_tables_map[event->get_test_id()];


  if (is_matched_event) {
    test_table->events_vec.push_back(event);
    if (event->get_is_testsome()) {
      int added_event_index;
      added_event_index = test_table->events_vec.size() - 1;
      test_table->with_previous_vec.push_back(added_event_index);
    }
    test_table->matched_index_vec.push_back(event->get_index());
    test_table->count++;
    total_length++;
  } else {
    int next_index_added_event_index;
    bool not_empty;
    next_index_added_event_index = test_table->events_vec.size();
    not_empty = test_table->unmatched_events_id_vec.size() > 0;

    if (not_empty && test_table->unmatched_events_id_vec.back() == next_index_added_event_index) {
      test_table->unmatched_events_count_vec.back() += 1;
    } else {
      test_table->unmatched_events_id_vec.push_back(next_index_added_event_index);
      test_table->unmatched_events_count_vec.push_back(event->get_event_counts());

      total_length++;
    }
  }
  last_added_event = event;
  
  return;
}

void rempi_encoder_cdc_input_format::format()
{
  map<int, rempi_encoder_input_format_test_table*>::iterator test_tables_map_it;
  for (test_tables_map_it  = test_tables_map.begin();
       test_tables_map_it != test_tables_map.end();
       test_tables_map_it++) {  
    rempi_encoder_input_format_test_table *test_table;
    test_table = test_tables_map_it->second;

    if (test_table->events_vec.size() > 0) {
      if (test_table->events_vec.back()->get_with_next() != REMPI_MPI_EVENT_NOT_WITH_NEXT) {
	REMPI_ERR("The events in this epoch are truncated");
      }
    }

    vector<rempi_event*> sorted_events_vec(test_table->events_vec);
    sort(sorted_events_vec.begin(), sorted_events_vec.end(), compare);
    for (int i = 0; i < sorted_events_vec.size(); i++) {
      sorted_events_vec[i]->clock_order = i;
      test_table->matched_events_ordered_map.insert(make_pair(i, sorted_events_vec[i]));

#ifdef REMPI_DBG_REPLAY
      REMPI_DBGI(REMPI_DBG_REPLAY, "order: %d source: %d , clock: %d (test_id: %d)", i, sorted_events_vec[i]->get_source(), sorted_events_vec[i]->get_clock(), test_tables_map_it->first);
#endif
      /*Update epoch line by using unordered_map*/
      /*TODO: if & else are doing the same thing except error check, REMPI_ERR("Later ... */
      if (test_table->epoch_umap.find(sorted_events_vec[i]->get_source()) == test_table->epoch_umap.end()) {
	test_table->epoch_umap[sorted_events_vec[i]->get_source()] = sorted_events_vec[i]->get_clock();
      } else {
	if (test_table->epoch_umap[sorted_events_vec[i]->get_source()] >= sorted_events_vec[i]->get_clock()) {
	  REMPI_ERR("Later message has smaller clock than earlier message: epoch line clock of rank:%d clock:%d but clock:%d", 
		    sorted_events_vec[i]->get_source(), 
		    test_table->epoch_umap[sorted_events_vec[i]->get_source()],sorted_events_vec[i]->get_clock()
		    );
	}
	test_table->epoch_umap[sorted_events_vec[i]->get_source()] = sorted_events_vec[i]->get_clock();
      }
    }
    /* format epoch line, and push into vector */
    for (unordered_map<size_t, size_t>::iterator epoch_it  = test_table->epoch_umap.begin();
	 epoch_it != test_table->epoch_umap.end();
	 epoch_it++) {
      test_table->epoch_rank_vec.push_back (epoch_it->first );
      test_table->epoch_clock_vec.push_back(epoch_it->second);
      rempi_encoder_input_format_test_table::all_epoch_rank_uset.insert(epoch_it->first);      
    }

    
  }
  return;
}

void rempi_encoder_cdc_input_format::debug_print()
{
  map<int, rempi_encoder_input_format_test_table*>::iterator test_tables_map_it;
  for (test_tables_map_it  = test_tables_map.begin();
       test_tables_map_it != test_tables_map.end();
       test_tables_map_it++) {  
    int test_id;
    rempi_encoder_input_format_test_table *test_table;
    test_id    = test_tables_map_it->first;

    test_table = test_tables_map_it->second;

    REMPI_DBG(  " ====== Test_id: %2d (test_table: %p)======", test_table->matching_set_id, test_table);

    REMPI_DBG(  "        count: %d", test_table->count);
    for (unordered_map<size_t, size_t>::iterator epoch_it = test_table->epoch_umap.begin();
	 epoch_it != test_table->epoch_umap.end();
	 epoch_it++) {
      REMPI_DBG("        epoch: rk:%d clock:%d", 
		epoch_it->first,
		epoch_it->second);
    }

    for (int i = 0; i < test_table->epoch_rank_vec.size(); i++) {
      REMPI_DBG("        epoch: rk:%d clock:%d", 
		test_table->epoch_rank_vec[i],
		test_table->epoch_clock_vec[i]);
    }

#if 0
    for (int i = 0; i < test_table->with_previous_vec.size(); i++) {
      REMPI_DBG("with_previous:  id:%d", test_table->with_previous_vec[i]);
    }
#endif

    for (int i = 0; i < test_table->with_previous_bool_vec.size(); i++) {
      int bl = 0;
      if (test_table->with_previous_bool_vec[i] == true) { 
	bl = 1;
      }
      REMPI_DBG("with_next: id:%d bol:%d" , i, bl);
    }

    for (int i = 0; i < test_table->unmatched_events_id_vec.size(); i++) {
      REMPI_DBG("    unmatched:  id: %d count: %d", test_table->unmatched_events_id_vec[i], test_table->unmatched_events_count_vec[i]);
    }

    for (unordered_map<size_t, size_t>::iterator unmatched_it = test_table->unmatched_events_umap.begin(),
	   unmatched_it_end = test_table->unmatched_events_umap.end();
	 unmatched_it != unmatched_it_end;
	 unmatched_it++) {
      REMPI_DBG("    unmatched:  id: %d count: %d", unmatched_it->first, unmatched_it->second);
    }

    for (int i = 0; i < test_table->events_vec.size(); i++) {
      REMPI_DBG("      matched: (id: %d) source: %d , clock %d", i, test_table->events_vec[i]->get_source(), test_table->events_vec[i]->get_clock());
    }

    for (int i = 0; i < test_table->matched_events_id_vec.size(); i++) {
      REMPI_DBG("      matched:  id: %d  msg_id: %d, delay: %d", i, 
		test_table->matched_events_id_vec[i], test_table->matched_events_delay_vec[i]);
    }

    // #if 0
    //     for (int i = 0; i < test_table->matched_events_square_sizes_vec.size(); i++) {
    //       REMPI_DBG("  matched(sq):  id: %d, size: %d", i, 
    // 		test_table->matched_events_square_sizes_vec[i]);
    //     }
    // #endif

    for (int i = 0; i < test_table->matched_events_permutated_indices_vec.size(); i++) {
      REMPI_DBG("  matched(pr):  id: %d  index: %d", i, 
		test_table->matched_events_permutated_indices_vec[i]);
    }

  }
  return;
}


/* ==================================== */
/*      CLASS rempi_encoder_cdc         */
/* ==================================== */



rempi_encoder_cdc::rempi_encoder_cdc(int mode, string record_path)
  : rempi_encoder(mode, record_path)
  , decoding_input_format(NULL)
  , decoding_state(REMPI_DECODE_STATUS_OPEN)
  , finished_testsome_count(0)
{

  /* === Create CDC object  === */
  cdc = new rempi_clock_delta_compression(1);

  /* === Load CLMPI module  === */
  clmpi_get_local_clock = PNMPIMOD_get_local_clock;
  clmpi_get_local_sent_clock = PNMPIMOD_get_local_sent_clock;

  if (rempi_mode == REMPI_ENV_REMPI_MODE_REPLAY) {
    this->read_footer();
  }

  
  global_local_min_id.rank = -1;
  global_local_min_id.clock = 0;

  return;
}

rempi_encoder_cdc::~rempi_encoder_cdc()
{
  for (unordered_map<int, unordered_map<int, size_t>*>::iterator it = solid_mc_next_clocks_umap_umap.begin(),
       it_end = solid_mc_next_clocks_umap_umap.end();
       it != it_end; it++) {
    free(it->second);
  }
  return;
}

void rempi_encoder_cdc::init_recv_ranks(char* record_path)
{
  return;
}

void rempi_encoder_cdc::init_cp(const char *record_path)
{
#ifdef CP_DBG  
  rempi_cp_init(record_path);
#endif   
  return;
}


int inited = 0;

void rempi_encoder_cdc::fetch_remote_look_ahead_send_clocks()
{  
  rempi_cp_gather_clocks();
  return;
}

  
void rempi_encoder_cdc::compute_local_min_id(rempi_encoder_input_format_test_table *test_table, 
					     int *local_min_id_rank,
					     size_t *local_min_id_clock, int recv_test_id)
{
  unordered_map<int, size_t> *solid_mc_next_clocks_umap;
  int epoch_rank_vec_size  = test_table->epoch_rank_vec.size();
  int epoch_clock_vec_size = test_table->epoch_clock_vec.size();
  if (epoch_rank_vec_size == 0 || epoch_clock_vec_size == 0) {
    // If this MF observed no matched events, epoch_line does not exist.
    //  REMPI_ERR("Epoch line is not decoded: %d %d", epoch_rank_vec_size, epoch_clock_vec_size);
    *local_min_id_rank = 0;
    *local_min_id_clock = 0;
    return;
  }
  solid_mc_next_clocks_umap = this->solid_mc_next_clocks_umap_umap[recv_test_id];

  *local_min_id_rank  = test_table->epoch_rank_vec[0];
  if (solid_mc_next_clocks_umap->find(*local_min_id_rank) == 
      solid_mc_next_clocks_umap->end()) {
    REMPI_ERR("No such rank: %d", *local_min_id_rank);
  }
  *local_min_id_clock = solid_mc_next_clocks_umap->at(*local_min_id_rank);
  for (int i = 0; i < epoch_rank_vec_size; i++) {
    int    tmp_rank  =  test_table->epoch_rank_vec[i];
    size_t tmp_clock =  solid_mc_next_clocks_umap->at(tmp_rank);

    /* =========== added for set_fd_clock_state ============ */
    if (tmp_clock == PNMPI_MODULE_CLMPI_COLLECTIVE) { 
      /*If this rank is in MPI_Collective, I skip this*/
      continue; 
    }
    if (*local_min_id_clock == PNMPI_MODULE_CLMPI_COLLECTIVE) {
      *local_min_id_rank   = tmp_rank;
      *local_min_id_clock  = tmp_clock;
    }
    /* =========== added for set_fd_clock_state ============ */

    if (*local_min_id_clock > tmp_clock) { 
      *local_min_id_rank   = tmp_rank;
      *local_min_id_clock  = tmp_clock;
    } else if (*local_min_id_clock == tmp_clock) { /*tie-break*/
      if (*local_min_id_rank > tmp_rank) {
	*local_min_id_rank   = tmp_rank;
	*local_min_id_clock  = tmp_clock;
      }
    }
  }


  return;
}


#define DEV_MSGB
#ifdef DEV_MSGB
int rempi_encoder_cdc::howto_update_look_ahead_recv_clock(int recv_rank, int matching_set_id,
							  unordered_set<int> *probed_message_source_set,
							  unordered_set<int> *pending_message_source_set,
							  unordered_map<int, size_t> *recv_message_source_umap,
							  unordered_map<int, size_t> *recv_clock_umap,
							  unordered_map<int, size_t> *solid_mc_next_clocks_umap,
							  int replaying_matching_set_id)
{
  int howto_update;
  size_t max_recved_clock;

  if (rempi_msgb_has_probed_msg(recv_rank)) {
    /* Because very small clock may arrive after calling MPI_recv/irecv
       we cannnot update anything.
       TODO: only update ranks with which MPI_probe did not probe
    */
    howto_update = REMPI_ENCODER_NO_UPDATE;
  } else if (replaying_matching_set_id != matching_set_id) {
    howto_update = REMPI_ENCODER_LAST_RECV_CLOCK_UPDATE;
  } else if (rempi_msgb_has_recved_msg(recv_rank)) {
    howto_update = REMPI_ENCODER_LAST_RECV_CLOCK_UPDATE;
  } else if (rempi_cp_has_in_flight_msgs(recv_rank)) {
    howto_update = REMPI_ENCODER_LAST_RECV_CLOCK_UPDATE;
  } else {
    howto_update = REMPI_ENCODER_LOOK_AHEAD_CLOCK_UPDATE;
  }
  
  if (howto_update == REMPI_ENCODER_LAST_RECV_CLOCK_UPDATE) {
    if ((max_recved_clock = rempi_msgb_get_max_recved_clock(recv_rank)) == 0) {
      howto_update = REMPI_ENCODER_NO_UPDATE;
    } else {
    /* 
       I already received "max_recved_clock", so next clock must be bigger than max_recvd_clock. 
       So compare wit "max_recved_clock + 1"
    */
      if (max_recved_clock + 1 < solid_mc_next_clocks_umap->at(recv_rank)) {
	howto_update = REMPI_ENCODER_NO_UPDATE;
      }
    }
  }
  REMPI_DBG("howto: %d", howto_update);
  return howto_update;
}
#else
int rempi_encoder_cdc::howto_update_look_ahead_recv_clock(int recv_rank, int matching_set_id,
							  unordered_set<int> *probed_message_source_set,
							  unordered_set<int> *pending_message_source_set,
							  unordered_map<int, size_t> *recv_message_source_umap,
							  unordered_map<int, size_t> *recv_clock_umap,
							  unordered_map<int, size_t> *solid_mc_next_clocks_umap,
							  int replaying_matching_set_id)
{
  int howto_update;
  size_t last_clock;

  if (probed_message_source_set->find(recv_rank) != probed_message_source_set->end()) {
    /* Because very small clock may arrive after calling MPI_recv/irecv
       we cannnot update anything.
       TODO: only update ranks with which MPI_probe did not probe
    */
    howto_update = REMPI_ENCODER_NO_UPDATE;
  } else if (pending_message_source_set->find(recv_rank) != pending_message_source_set->end() && replaying_matching_set_id != matching_set_id) {
    howto_update = REMPI_ENCODER_LAST_RECV_CLOCK_UPDATE;
  } else if (recv_message_source_umap->find(recv_rank) != recv_message_source_umap->end()) {
    howto_update = REMPI_ENCODER_LAST_RECV_CLOCK_UPDATE;
  } else if (rempi_cp_has_in_flight_msgs(recv_rank)) {
    howto_update = REMPI_ENCODER_LAST_RECV_CLOCK_UPDATE;
  } else {
    howto_update = REMPI_ENCODER_LOOK_AHEAD_CLOCK_UPDATE;
  }
  
  if (howto_update == REMPI_ENCODER_LAST_RECV_CLOCK_UPDATE) {
    if (recv_clock_umap->find(recv_rank) == recv_clock_umap->end()) {
      howto_update = REMPI_ENCODER_NO_UPDATE;
    } else {
      last_clock = recv_clock_umap->at(recv_rank);
      if (last_clock < solid_mc_next_clocks_umap->at(recv_rank)) {
	howto_update = REMPI_ENCODER_NO_UPDATE;
      }
    }
  }
  return howto_update;
}
#endif


/*
  input:
  has_probed_message: flag if there any probed messages by MPI_Iprobe (but not received)
  recv_message_source_umap: Source rank set. The received messages may or may not belong to message_set_id.
  pending_message_source_set: Source rank set. But the recieved messages from the source rank do not belong to any matching_set_id yet
  pending_message_source_set is a subset of recv_message_source_umap
  recv_clock_umap: 

*/

int rempi_encoder_cdc::update_local_look_ahead_recv_clock(unordered_set<int> *probed_message_source_set, 
							  unordered_set<int> *pending_message_source_set, 
							  unordered_map<int, size_t> *recv_message_source_umap, 
							  unordered_map<int, size_t> *recv_clock_umap,
							  int replaying_matching_set_id)
{
  unordered_map<int, size_t> *solid_mc_next_clocks_umap;
  int is_updated = 0;
  int howto_update;
  int matching_set_id;

  for (unordered_map<int, unordered_map<int, size_t>*>::iterator it = solid_mc_next_clocks_umap_umap.begin(), 
	 it_end = solid_mc_next_clocks_umap_umap.end(); 
       it != it_end; it++) {
    //  for (int matching_set_id = 0, size = solid_mc_next_clocks_umap_vec.size(); matching_set_id < size; matching_set_id++) {
    matching_set_id = it->first;
    //    solid_mc_next_clocks_umap = solid_mc_next_clocks_umap_vec[matching_set_id];
    solid_mc_next_clocks_umap = it->second;
    for (int index = 0; index < mc_length; index++) {
      howto_update = howto_update_look_ahead_recv_clock(mc_recv_ranks[index], matching_set_id,
							probed_message_source_set,
							pending_message_source_set,
							recv_message_source_umap,
							recv_clock_umap,
							solid_mc_next_clocks_umap,
							replaying_matching_set_id);
      switch(howto_update) {
      case REMPI_ENCODER_NO_UPDATE:
	break;
      case REMPI_ENCODER_LAST_RECV_CLOCK_UPDATE:
#ifdef DEV_MSGB
	solid_mc_next_clocks_umap->at(mc_recv_ranks[index]) = rempi_msgb_get_max_recved_clock(mc_recv_ranks[index]);
#else
	solid_mc_next_clocks_umap->at(mc_recv_ranks[index]) = recv_clock_umap->at((mc_recv_ranks[index]));
#endif
	break;
      case REMPI_ENCODER_LOOK_AHEAD_CLOCK_UPDATE:
	solid_mc_next_clocks_umap->at(mc_recv_ranks[index]) = rempi_cp_get_gather_clock(mc_recv_ranks[index]);
	break;
      }
    }
  }
  return is_updated;
}



rempi_encoder_input_format* rempi_encoder_cdc::create_encoder_input_format()
{
  return new rempi_encoder_cdc_input_format();
}


void rempi_encoder_cdc::compress_non_matched_events(rempi_encoder_input_format &input_format, rempi_encoder_input_format_test_table *test_table)
{
  char *write_buff;
  size_t write_size;

  /*=== matching set id === */
  input_format.write_queue_vec.push_back((char*)&test_table->matching_set_id);
  input_format.write_size_queue_vec.push_back(sizeof(test_table->matching_set_id));  
  /*=======================*/


  /*=== message count ===*/
  input_format.write_queue_vec.push_back((char*)&test_table->count);
  input_format.write_size_queue_vec.push_back(sizeof(test_table->count));
  /*=====================*/

  /*=== Compress Epoch   ===*/
  /* (0) size*/
  if (test_table->epoch_rank_vec.size() != test_table->epoch_clock_vec.size()) {
    REMPI_ERR("Epoch size is inconsistent");
  }
  test_table->epoch_size = test_table->epoch_rank_vec.size() * sizeof(test_table->epoch_rank_vec[0]);
  input_format.write_queue_vec.push_back((char*)&test_table->epoch_size);
  input_format.write_size_queue_vec.push_back(sizeof(test_table->epoch_size));
  /* (1) rank*/
  input_format.write_queue_vec.push_back((char*)&test_table->epoch_rank_vec[0]);
  input_format.write_size_queue_vec.push_back(test_table->epoch_size);
  /* (2) clock*/
  input_format.write_queue_vec.push_back((char*)&test_table->epoch_clock_vec[0]);
  input_format.write_size_queue_vec.push_back(test_table->epoch_size);
  /*====================================*/

  /*=== Compress with_next ===*/
  compression_util.compress_by_linear_prediction(test_table->with_previous_vec);
  test_table->compressed_with_previous_length = test_table->with_previous_vec.size();
  test_table->compressed_with_previous_size   = test_table->with_previous_vec.size() * sizeof(test_table->with_previous_vec[0]);
  test_table->compressed_with_previous        = (char*)&test_table->with_previous_vec[0];
  input_format.write_queue_vec.push_back((char*)&test_table->compressed_with_previous_length);
  input_format.write_size_queue_vec.push_back(sizeof(test_table->compressed_with_previous_length));
  input_format.write_queue_vec.push_back((char*)&test_table->compressed_with_previous_size);
  input_format.write_size_queue_vec.push_back(sizeof(test_table->compressed_with_previous_size));
  input_format.write_queue_vec.push_back((char*)test_table->compressed_with_previous);
  input_format.write_size_queue_vec.push_back(test_table->compressed_with_previous_size);
  /*====================================*/

  /*=== Compress index ===*/
  //  compression_util.compress_by_linear_prediction(test_table->matched_index_vec);
  test_table->compressed_matched_index_length = test_table->matched_index_vec.size();
  test_table->compressed_matched_index_size   = test_table->matched_index_vec.size() * sizeof(test_table->matched_index_vec[0]);
  test_table->compressed_matched_index        = (char*)&test_table->matched_index_vec[0];
  input_format.write_queue_vec.push_back((char*)&test_table->compressed_matched_index_length);
  input_format.write_size_queue_vec.push_back(sizeof(test_table->compressed_matched_index_length));
  input_format.write_queue_vec.push_back((char*)&test_table->compressed_matched_index_size);
  input_format.write_size_queue_vec.push_back(sizeof(test_table->compressed_matched_index_size));
  input_format.write_queue_vec.push_back((char*)test_table->compressed_matched_index);
  input_format.write_size_queue_vec.push_back(test_table->compressed_matched_index_size);
  /*====================================*/

  /*=== Compress unmatched_events ===*/
  /* (1) id */
  compression_util.compress_by_linear_prediction(test_table->unmatched_events_id_vec);
  test_table->compressed_unmatched_events_id        = (char*)&test_table->unmatched_events_id_vec[0];
  test_table->compressed_unmatched_events_id_size   = test_table->unmatched_events_id_vec.size() * sizeof(test_table->unmatched_events_id_vec[0]);
  input_format.write_queue_vec.push_back((char*)&test_table->compressed_unmatched_events_id_size);
  input_format.write_size_queue_vec.push_back(sizeof(test_table->compressed_unmatched_events_id_size));
  input_format.write_queue_vec.push_back(test_table->compressed_unmatched_events_id);
  input_format.write_size_queue_vec.push_back(test_table->compressed_unmatched_events_id_size);
  /* (2) count */
  /*   count will be totally random number, so this function does not do zlib*/
  test_table->compressed_unmatched_events_count        = (char*)&test_table->unmatched_events_count_vec[0];
  test_table->compressed_unmatched_events_count_size   = test_table->unmatched_events_count_vec.size() * sizeof(test_table->unmatched_events_count_vec[0]);
  input_format.write_queue_vec.push_back((char*)&test_table->compressed_unmatched_events_count_size);
  input_format.write_size_queue_vec.push_back(sizeof(test_table->compressed_unmatched_events_count));
  input_format.write_queue_vec.push_back(test_table->compressed_unmatched_events_count);
  input_format.write_size_queue_vec.push_back(test_table->compressed_unmatched_events_count_size);
  /*=======================================*/
  return;
}


void rempi_encoder_cdc::compress_matched_events(rempi_encoder_input_format &input_format, rempi_encoder_input_format_test_table *test_table)
{
  size_t compressed_size,  original_size;
  /*Call clock delta compression*/
  test_table->compressed_matched_events = cdc->compress(
							test_table->matched_events_ordered_map, 
							test_table->events_vec, 
							original_size);
  test_table->compressed_matched_events_size           = original_size;

  input_format.write_queue_vec.push_back((char*)&test_table->compressed_matched_events_size);
  input_format.write_size_queue_vec.push_back(sizeof(test_table->compressed_matched_events_size));
  input_format.write_queue_vec.push_back(test_table->compressed_matched_events);
  input_format.write_size_queue_vec.push_back(test_table->compressed_matched_events_size);
  return;
}


/*  ==== 3 Step compression ==== 
    1. rempi_encoder_cdc.get_encoding_event_sequence(...): [rempi_event_list => rempi_encoding_structure]
    this function translate sequence of event(rempi_event_list) 
    into a data format class(rempi_encoding_structure) whose format 
    is sutable for compression(rempi_encoder_cdc.encode(...))
   

    2. repmi_encoder.encode(...): [rempi_encoding_structure => char*]
    encode, i.e., rempi_encoding_structure -> char*
     
    3. write_record_file: [char* => "file"]
    write the data(char*)
    =============================== */



bool rempi_encoder_cdc::extract_encoder_input_format_chunk(rempi_event_list<rempi_event*> &events, rempi_encoder_input_format &input_format)
{
  rempi_event *event_dequeued = NULL;
  bool is_ready_for_encoding = false;

  while (1) {
    /*Append events to current check as many as possible*/
    if (events.front() == NULL) {
      break;
    } else if (input_format.length() > rempi_max_event_length) {
      if (input_format.last_added_event->get_with_next() == REMPI_MPI_EVENT_NOT_WITH_NEXT) break;
    } 

    event_dequeued = events.pop();
    // REMPI_DBGI(0, "EXT ; (count: %d, with_next: %d, flag: %d, source: %d, clock: %d)", 
    //  	        event_dequeued->get_event_counts(),  event_dequeued->get_is_testsome(),  event_dequeued->get_flag(),
    // 	        event_dequeued->get_source(), event_dequeued->get_clock());
    input_format.add(event_dequeued);
  }

  if (input_format.length() == 0) return is_ready_for_encoding; /*false*/

  if (events.is_push_closed_())  {
    /*If the end of run, */
    input_format.format();
    is_ready_for_encoding = true;
  } else if (input_format.length() > rempi_max_event_length) {
    /* "event_dequeued->get_with_next() == REMPI_MPI_EVENT_NOT_WITH_NEXT" is to make sure that 
       with_next events (e.g. Multiple matches in Testsome) do not cross-over across different epoches
    */
    if (input_format.last_added_event->get_with_next() == REMPI_MPI_EVENT_NOT_WITH_NEXT) {
      input_format.format();
      is_ready_for_encoding = true;
    }
  }
  return is_ready_for_encoding; /*true*/
}


void rempi_encoder_cdc::encode(rempi_encoder_input_format &input_format)
{
  map<int, rempi_encoder_input_format_test_table*>::iterator test_tables_map_it;
  for (test_tables_map_it  = input_format.test_tables_map.begin();
       test_tables_map_it != input_format.test_tables_map.end();
       test_tables_map_it++) {  
    rempi_encoder_input_format_test_table *test_table;
    test_table = test_tables_map_it->second;
    /*#################################################*/
    /*     Encoding Section                            */
    /*#################################################*/
    /*=== message count ===*/    /*=== Compress with_next  ===*/  /*=== Compress unmatched_events ===*/
    compress_non_matched_events(input_format, test_table);
    /*=======================================*/
    /*=== Compress matched_events ===*/
    compress_matched_events(input_format, test_table);
    //    REMPI_DBG("# of permutated messages: %d, # of messages: %lu (%d)", test_table->compressed_matched_events_size/sizeof(int)/2, test_table->count, test_tables_map_it->first);
    /*=======================================*/
  }
  return;
}

// void rempi_encoder_cdc::write_record_file(rempi_encoder_input_format &input_format)
// {
//   size_t total_compressed_size = 0;
//   size_t compressed_size = 0;
//   map<int, rempi_encoder_input_format_test_table*>::iterator test_tables_map_it;
//   for (test_tables_map_it  = input_format.test_tables_map.begin();
//        test_tables_map_it != input_format.test_tables_map.end();
//        test_tables_map_it++) {
//     rempi_encoder_input_format_test_table *test_table;
//     test_table = test_tables_map_it->second;

//     compressed_size = 0;
//     /* Count */
//     record_fs.write((char*)&test_table->count, sizeof(test_table->count));
//     compressed_size += sizeof(test_table->count);

//     /* with_previous */
//     record_fs.write(   (char*)&test_table->compressed_with_previous_size, 
// 		        sizeof(test_table->compressed_with_previous_size));
//     compressed_size +=  sizeof(test_table->compressed_with_previous_size);
//     record_fs.write(           test_table->compressed_with_previous, 
// 			       test_table->compressed_with_previous_size);
//     compressed_size +=         test_table->compressed_with_previous_size;


//     /* unmatched */
//     record_fs.write(  (char*)&test_table->compressed_unmatched_events_id_size, 
// 		       sizeof(test_table->compressed_unmatched_events_id_size));
//     compressed_size += sizeof(test_table->compressed_unmatched_events_id_size);  
//     record_fs.write(          test_table->compressed_unmatched_events_id,    
// 			      test_table->compressed_unmatched_events_id_size);
//     compressed_size +=        test_table->compressed_unmatched_events_id_size;

//     record_fs.write(  (char*)&test_table->compressed_unmatched_events_count_size, 
// 		       sizeof(test_table->compressed_unmatched_events_count_size));
//     compressed_size += sizeof(test_table->compressed_unmatched_events_count_size);  
//     record_fs.write(          test_table->compressed_unmatched_events_count, 
// 			      test_table->compressed_unmatched_events_count_size);
//     compressed_size +=        test_table->compressed_unmatched_events_count_size;


//     /* matched */
//     record_fs.write(  (char*)&test_table->compressed_matched_events_size, 
// 		       sizeof(test_table->compressed_matched_events_size));
//     compressed_size += sizeof(test_table->compressed_matched_events_size);  
//     record_fs.write(          test_table->compressed_matched_events, 
// 			      test_table->compressed_matched_events_size);
//     compressed_size +=        test_table->compressed_matched_events_size;
//     //TODO: delete encoded_event

//     // REMPI_DBG("Original size: count:%5d x 8 bytes x 2(matched/unmatched)= %d bytes,  Compressed size: %lu bytes", 
//     // 	      test_table->count, test_table->count * 8 * 2, compressed_size);
//     total_compressed_size += compressed_size;
//     REMPI_DBG("compressed_size: %lu", compressed_size);
//   }
//   REMPI_DBG("xOriginal size: count:%5d(matched/unmatched entry) x 8 bytes= %d bytes,  Compressed size: %lu bytes , %d %lu", 
// 	    input_format.total_length, input_format.total_length * 8 , total_compressed_size, 
// 	    input_format.total_length*8, total_compressed_size);
//   //  REMPI_DBG("TOTAL Original: %d ", total_countb);
// }   


void rempi_encoder_cdc::write_record_file(rempi_encoder_input_format &input_format)
{
  
  size_t total_compressed_size = 0;
  size_t total_original_size   = 0;


  if (input_format.write_queue_vec.size() != input_format.write_size_queue_vec.size()) {
    REMPI_ERR("Size is different in write_queue_vec & write_size_queue_vec.");
  }

  for (int i = 0; i < input_format.write_size_queue_vec.size(); i++) {
    total_original_size += input_format.write_size_queue_vec[i];
  }
  //  REMPI_DBG("total_size: %lu", total_original_size);

  
  if(rempi_gzip) {
    vector<char*> compressed_write_queue_vec;
    vector<size_t> compressed_write_size_queue_vec;
    compression_util.compress_by_zlib_vec(input_format.write_queue_vec, input_format.write_size_queue_vec,
					  compressed_write_queue_vec, compressed_write_size_queue_vec, total_compressed_size);
    if (total_compressed_size == all_epoch_rank_separator) {
      REMPI_ERR("total_compressed_size is same as separator: %d", all_epoch_rank_separator);
    }
    record_fs.write((char*)&total_compressed_size, sizeof(total_compressed_size));
    write_size_vec.push_back(sizeof(total_compressed_size));
    for (int i = 0; i < compressed_write_queue_vec.size(); i++) {
      record_fs.write(compressed_write_queue_vec[i], compressed_write_size_queue_vec[i]);
      free(compressed_write_queue_vec[i]);
      write_size_vec.push_back(compressed_write_size_queue_vec[i]);
    }
  } else {
    record_fs.write((char*)&total_original_size, sizeof(total_original_size));
    write_size_vec.push_back(sizeof(total_original_size));
    for (int i = 0; i < input_format.write_queue_vec.size(); i++) {
      record_fs.write(input_format.write_queue_vec[i], input_format.write_size_queue_vec[i]);
      write_size_vec.push_back(input_format.write_size_queue_vec[i]);
    }
  }

  return;
}   



#if 0
int rempi_encoder_cdc::progress_decoding(rempi_event_list<rempi_event*> *recording_events, rempi_event_list<rempi_event*> *replaying_events, int recv_test_id) 
{
  bool is_all_finished = 0;
  bool is_epoch_finished = 0;
  int suspend_progress = 0;
  int has_new_event;
  bool is_no_more_record;

  while(!suspend_progress) {
    switch (decoding_state) {
    case REMPI_DECODE_STATUS_OPEN: //0
      this->open_record_file(record_path);
      decoding_state = REMPI_DECODE_STATUS_CREATE_READ;
      break;
    case REMPI_DECODE_STATUS_CREATE_READ: //1
      decoding_input_format = this->create_encoder_input_format();
      is_no_more_record = this->read_record_file(*decoding_input_format);


      decoding_state = (is_no_more_record)? REMPI_DECODE_STATUS_CLOSE:REMPI_DECODE_STATUS_DECODE;
      break;
    case REMPI_DECODE_STATUS_DECODE: //2
      this->decode(*decoding_input_format);
      decoding_state = REMPI_DECODE_STATUS_INSERT;
      break;
    case REMPI_DECODE_STATUS_INSERT: //3
      if (recording_events != NULL && replaying_events != NULL && recv_test_id >= 0)  {
	insert_encoder_input_format_chunk_recv_test_id(recording_events, replaying_events, decoding_input_format, 
						       &is_epoch_finished, &has_new_event, recv_test_id);
	
	if (is_epoch_finished) {
	  delete decoding_input_format;
	  decoding_input_format = NULL;
	  decoding_state = REMPI_DECODE_STATUS_CREATE_READ;
	} else {
	  suspend_progress = 1;
	}
      } else {
	suspend_progress = 1;
      }
      break;
    case REMPI_DECODE_STATUS_CLOSE: //4
      replaying_events->close_push();
      delete decoding_input_format;
      decoding_input_format = NULL;
      this->close_record_file();
      decoding_state = REMPI_DECODE_STATUS_COMPLETE;
      is_all_finished = 1;
      suspend_progress = 1;
      break;
    case REMPI_DECODE_STATUS_COMPLETE: //5
      //      REMPI_ERR("No more events to replay");
      is_all_finished = 1;
      suspend_progress = 1;
      break;
    }
  }
  //  progress_decoding_mtx.unlock();

  //  if (has_new_event == 0 && recv_test_id == -1) usleep(1);

  return is_all_finished;

}


#else
int decoding_state_old = -1;
int rempi_encoder_cdc::progress_decoding(rempi_event_list<rempi_event*> *recording_events, rempi_event_list<rempi_event*> *replaying_events, int recv_test_id) 
{
  bool is_all_finished = 0;
  bool is_epoch_finished = 0;
  int suspend_progress = 0;
  int has_new_event = 0;
  bool is_no_more_record;


  progress_decoding_mtx.lock();
  while(!suspend_progress) {
    // if (decoding_state != decoding_state_old) {
    //   REMPI_DBG("state: %d => %d (recv_test_id: %d)", decoding_state_old, decoding_state, recv_test_id);
    // }
    //    decoding_state_old = decoding_state;
    switch (decoding_state) {
    case REMPI_DECODE_STATUS_OPEN: //0
      this->open_record_file(record_path);
      decoding_state = REMPI_DECODE_STATUS_CREATE_READ;
      break;
    case REMPI_DECODE_STATUS_CREATE_READ: //1
      decoding_input_format = this->create_encoder_input_format();
      is_no_more_record = this->read_record_file(*decoding_input_format);
      decoding_state = (is_no_more_record)? REMPI_DECODE_STATUS_CLOSE:REMPI_DECODE_STATUS_DECODE;
      //      suspend_progress = 1; /*TODO: find out why out_of_range crash happens if I remove "suspend_progress=1 here "*/
      break;
    case REMPI_DECODE_STATUS_DECODE: //2
      this->decode(*decoding_input_format);
      decoding_state = REMPI_DECODE_STATUS_INSERT;
      //      suspend_progress = 1;
      //      REMPI_DBG("decoded !!");
      break;
    case REMPI_DECODE_STATUS_INSERT: //3
      if (recording_events != NULL && replaying_events != NULL)  {
	if (recv_test_id >= 0) {
	  insert_encoder_input_format_chunk_recv_test_id(recording_events, replaying_events, decoding_input_format, 
							 &is_epoch_finished, &has_new_event, recv_test_id);
	} else {
	  for (map<int, rempi_encoder_input_format_test_table*>::iterator it = decoding_input_format->test_tables_map.begin(),
		 it_end = decoding_input_format->test_tables_map.end(); it != it_end; it++) {
	    int rtid = it->first;
	    insert_encoder_input_format_chunk_recv_test_id(recording_events, replaying_events, decoding_input_format, 
							   &is_epoch_finished, &has_new_event, rtid);
	    if (is_epoch_finished) break; /*If this epoch is finished, immediatly break to avoid hang */
	  }
	}	
	if (is_epoch_finished) {
	  mc_length=0;
	  delete decoding_input_format;
	  decoding_input_format = NULL;
	  decoding_state = REMPI_DECODE_STATUS_CREATE_READ;
	} else {
	  suspend_progress = 1;
	}
      } else {
	suspend_progress = 1;
      }
      break;
    case REMPI_DECODE_STATUS_CLOSE: //4
      replaying_events->close_push();
      delete decoding_input_format;
      decoding_input_format = NULL;
      this->close_record_file();
      decoding_state = REMPI_DECODE_STATUS_COMPLETE;
      is_all_finished = 1;
      suspend_progress = 1;
      //      REMPI_DBG("No more event");
      break;
    case REMPI_DECODE_STATUS_COMPLETE: //5
      //      REMPI_DBG("Again, No more event");
      is_all_finished = 1;
      suspend_progress = 1;
      break;
    }
  }
  progress_decoding_mtx.unlock();

  if (has_new_event == 0 && recv_test_id == -1) usleep(1);

  return is_all_finished;

}
#endif

bool rempi_encoder_cdc::read_record_file(rempi_encoder_input_format &input_format)
{
  size_t chunk_size;
  char* zlib_payload;
  vector<char*>   compressed_payload_vec;
  vector<size_t>  compressed_payload_size_vec;
  vector<char*>   decompressed_payload_vec;
  vector<size_t>  decompressed_payload_size_vec;
  size_t decompressed_size, read_size;
  bool is_no_more_record; 



  record_fs.read((char*)&chunk_size, sizeof(chunk_size));
  read_size = record_fs.gcount();
  if (read_size == 0) {
    /* When chunk_size == 0 (= all_epoch_rank_separator), ReMPI stop reading files before read file to the end. 
       So read_size == 0 won't happen */
    REMPI_ERR("Record data is broken");
  }  else if (chunk_size == all_epoch_rank_separator)  {
    is_no_more_record = true;
    this->mc_length = 0; 
    return is_no_more_record;
  }

  zlib_payload = (char*)rempi_malloc(chunk_size);
  record_fs.read(zlib_payload, chunk_size);


  if (rempi_gzip) {  
    compressed_payload_vec.push_back(zlib_payload);
    compressed_payload_size_vec.push_back(chunk_size);
    compression_util.decompress_by_zlib_vec(compressed_payload_vec, compressed_payload_size_vec,
					    input_format.write_queue_vec, input_format.write_size_queue_vec, decompressed_size);
    input_format.decompressed_size = decompressed_size;
    free(zlib_payload);
  } else {
    input_format.write_queue_vec.push_back(zlib_payload);
    input_format.write_size_queue_vec.push_back(chunk_size);
    input_format.decompressed_size = chunk_size;
  }
  
  is_no_more_record = false;
  return is_no_more_record;
}


void rempi_encoder_cdc::decompress_non_matched_events(rempi_encoder_input_format &input_format)
{

}

void rempi_encoder_cdc::decompress_matched_events(rempi_encoder_input_format &input_format)
{

}


void rempi_copy_vec(size_t* array, size_t length, vector<size_t>&vec)
{
  if (!vec.empty()) {
    REMPI_ERR("vec is not empty");
  }
  vec.resize(length);
  for (int i = 0; i < length; i++) {
    vec[i] = array[i];
  }
  return;
}

void rempi_copy_vec_int(int* array, size_t length, vector<size_t>&vec)
{
  if (!vec.empty()) {
    REMPI_ERR("vec is not empty");
  }
  vec.resize(length);
  for (int i = 0; i < length; i++) {
    vec[i] = (size_t)array[i];
  }
  return;
}

void rempi_encoder_cdc::decode(rempi_encoder_input_format &input_format)
{
  char* decompressed_record;
  char* decoding_address;
  /*Used to collect all ranks, which this rank receives messages from, 
    for next_clocks updates */
  unordered_set<int> mc_recv_ranks_uset; 
  //TODO: this is redundant memcopy => remove this overhead
  decompressed_record = (char*)rempi_malloc(input_format.decompressed_size);
  decoding_address = decompressed_record;
  input_format.decompressed_record_char = decoding_address;

  //  size_t sum = 0;
  /*
    After decoding by zlib, decoded data is chunked (size: ZLIB_CHUNK).
    input_format.write_queue_vec is an array of the chunks.    
  */
  for (int i = 0; i < input_format.write_queue_vec.size(); i++) {
    memcpy(decompressed_record, input_format.write_queue_vec[i], input_format.write_size_queue_vec[i]);
    free(input_format.write_queue_vec[i]);
    decompressed_record += input_format.write_size_queue_vec[i];
    //    sum += input_format.write_size_queue_vec[i];
  }
  //  REMPI_DBG("decording_address: %lu, decompressed_record %lu", decoding_address, decompressed_record);
  //  REMPI_DBG("total size: %lu, total size: %lu", input_format.decompressed_size, sum);


  int test_id = -1;
  while (decoding_address < decompressed_record) {
    rempi_encoder_input_format_test_table *test_table;

    test_id = *(int*)decoding_address;
    decoding_address += sizeof(test_id);

    test_table = new rempi_encoder_input_format_test_table(test_id);
    input_format.test_tables_map[test_id] = test_table;

    /*=== Decode count  ===*/
    test_table->count = *(int*)decoding_address;
    decoding_address += sizeof(test_table->count);
    /*==========================*/


    /*=== Decode epoch  ===*/
    {
      size_t *epoch_rank, *epoch_clock;
      /*----------- size ------------*/
      test_table->epoch_size = *(size_t*)decoding_address;
      decoding_address += sizeof(test_table->epoch_size);
      /*----------- rank ------------*/
      epoch_rank             = (size_t*)decoding_address;
      decoding_address += test_table->epoch_size;
      /*----------- clock ------------*/
      epoch_clock             = (size_t*)decoding_address;
      decoding_address += test_table->epoch_size;
      for (int i = 0; i < test_table->epoch_size / sizeof(size_t); i++) {
	test_table->epoch_umap[epoch_rank[i]] = epoch_clock[i];
	test_table->epoch_line_umap[epoch_rank[i]] = epoch_clock[i];
	test_table->epoch_rank_vec.push_back(epoch_rank[i]);
	test_table->epoch_clock_vec.push_back(epoch_clock[i]);
	/*=== Decode recv_ranks for minimal clock fetching  ===*/
	mc_recv_ranks_uset.insert((int)(epoch_rank[i]));
	//	REMPI_DBG("->> %d (%lu)", (int)(epoch_rank[i]), test_table->epoch_size);
	/*=====================================================*/
      }


    }
    /*==========================*/



    /*=== Decode with_next ===*/
    /*----------- length ------------*/
    test_table->compressed_with_previous_length = *(size_t*)decoding_address;
    decoding_address += sizeof(test_table->compressed_with_previous_length);
    /*----------- size   ------------*/
    test_table->compressed_with_previous_size = *(size_t*)decoding_address;
    decoding_address += sizeof(test_table->compressed_with_previous_size);
    /*----------- with_next   ------------*/
    if (test_table->compressed_with_previous_size > 0) {
      test_table->compressed_with_previous = decoding_address;
      rempi_copy_vec((size_t*)test_table->compressed_with_previous, test_table->compressed_with_previous_size / sizeof(size_t), 
		     test_table->with_previous_vec);
      compression_util.decompress_by_linear_prediction(test_table->with_previous_vec);
      decoding_address += test_table->compressed_with_previous_size;
      {
	test_table->with_previous_bool_vec.resize(test_table->count, false);
	for (int i = 0, n = test_table->with_previous_vec.size(); i < n; i++) {
	  int with_next_index = test_table->with_previous_vec[i];
	  test_table->with_previous_bool_vec[with_next_index] = true;
	}
      }
    }

    /*==========================*/


    /*=== Decode matched index ===*/
    /*----------- length ------------*/
    test_table->compressed_matched_index_length = *(size_t*)decoding_address;
    decoding_address += sizeof(test_table->compressed_matched_index_length);
    /*----------- size   ------------*/
    test_table->compressed_matched_index_size = *(size_t*)decoding_address;
    decoding_address += sizeof(test_table->compressed_matched_index_size);
    /*----------- with_next   ------------*/
    if (test_table->compressed_matched_index_size > 0) {
      test_table->compressed_matched_index = decoding_address;
      rempi_copy_vec((size_t*)test_table->compressed_matched_index, test_table->compressed_matched_index_size / sizeof(size_t), 
		     test_table->matched_index_vec);
      decoding_address += test_table->compressed_matched_index_size;
    }

    /*==========================*/
    

    /*=== Decode unmatched_events ===*/
    /*===  (1) id */
    /*----------- size ------------*/
    test_table->compressed_unmatched_events_id_size = *(size_t*)decoding_address;
    decoding_address += sizeof(test_table->compressed_unmatched_events_id_size);
    /*----------- id   ------------*/
    test_table->compressed_unmatched_events_id      = decoding_address;
    rempi_copy_vec((size_t*)test_table->compressed_unmatched_events_id, test_table->compressed_unmatched_events_id_size / sizeof(size_t),
		   test_table->unmatched_events_id_vec);
    compression_util.decompress_by_linear_prediction(test_table->unmatched_events_id_vec);
    decoding_address += test_table->compressed_unmatched_events_id_size;
    /*===  (2) count */
    /*----------- size ------------*/
    test_table->compressed_unmatched_events_count_size = *(size_t*)decoding_address;
    decoding_address += sizeof(test_table->compressed_unmatched_events_count_size);
    /*----------- count   ------------*/
    test_table->compressed_unmatched_events_count      = decoding_address;
    rempi_copy_vec((size_t*)test_table->compressed_unmatched_events_count, test_table->compressed_unmatched_events_count_size / sizeof(size_t),
		   test_table->unmatched_events_count_vec);
    decoding_address += test_table->compressed_unmatched_events_count_size;
    {
      for (int i = 0, n = test_table->unmatched_events_id_vec.size(); i < n; i++) {
	size_t id    = test_table->unmatched_events_id_vec[i];
	size_t count = test_table->unmatched_events_count_vec[i];
	test_table->unmatched_events_umap[id] = count;
      }
    }
    test_table->unmatched_events_id_vec.clear();
    test_table->unmatched_events_count_vec.clear();
    /*===============================*/
    


    /*=== Decode matched_events ===*/
    /*----------- size ------------*/
    test_table->compressed_matched_events_size      = *(size_t*)decoding_address;

    //    REMPI_DBGI(3, "matched size: %lu", test_table->compressed_matched_events_size);
    decoding_address += sizeof(test_table->compressed_matched_events_size);

    if (test_table->compressed_matched_events_size > 0) {
      /*----------- event ------------*/
      test_table->compressed_matched_events          = decoding_address;
      /*   --  id  --  */
      int* copy_start;
      copy_start = (int*)test_table->compressed_matched_events;
      rempi_copy_vec_int(copy_start,
			 test_table->compressed_matched_events_size / sizeof(int) / 2,
			 test_table->matched_events_id_vec);
      compression_util.decompress_by_linear_prediction(test_table->matched_events_id_vec);
      //    REMPI_DBGI(3, "matched length: %lu", test_table->matched_events_id_vec.size());
      /*   -- delay --  */
      copy_start += 	 test_table->compressed_matched_events_size / sizeof(int) / 2;
      rempi_copy_vec_int(copy_start,
			 test_table->compressed_matched_events_size / sizeof(int) / 2,
			 test_table->matched_events_delay_vec);
      compression_util.decompress_by_linear_prediction(test_table->matched_events_delay_vec);
      
      if (test_table->matched_events_id_vec.size() != test_table->matched_events_delay_vec.size()) {
	REMPI_ERR("matched_id_vec.size != matched_delay_vec.size()");
      }
    } else {
      test_table->compressed_matched_events = NULL;
    }
    cdc_prepare_decode_indices(test_table->count,
			       test_table->matched_events_id_vec,
			       test_table->matched_events_delay_vec,
			       test_table->matched_events_permutated_indices_vec);

    

    /* ======== Several initialization of test_table for replay ==========*/
    for (int i = 0, n = test_table->epoch_rank_vec.size(); i < n ; i++) {
      int rank = test_table->epoch_rank_vec[i];
      test_table->pending_event_count_umap[rank] = 0;
      test_table->current_epoch_line_umap[rank] = 0;
    }
    /* ===================================================================*/



    /*   ------------ */
    decoding_address += test_table->compressed_matched_events_size;

    /*===============================*/
  } /* -- End of while */



  /*=== Decode recv_ranks for minimal clock fetching  ===*/
  {

    int index = 0;
#ifndef CP_DBG
    input_format.mc_next_clocks     = (size_t*)rempi_malloc(sizeof(size_t) * input_format.mc_length);
    input_format.tmp_mc_next_clocks = (size_t*)rempi_malloc(sizeof(size_t) * input_format.mc_length);
#endif

    input_format.mc_recv_ranks      =    (int*)rempi_malloc(sizeof(int) * mc_recv_ranks_uset.size());
    input_format.mc_length          = mc_recv_ranks_uset.size();


    for (unordered_set<int>::const_iterator cit = mc_recv_ranks_uset.cbegin(),
	   cit_end = mc_recv_ranks_uset.cend();
	 cit != cit_end;
	 cit++) {
      int rank = *cit;
      input_format.mc_recv_ranks [index]      = rank;

#ifndef CP_DBG
      input_format.mc_next_clocks[index]      = 0;
      input_format.tmp_mc_next_clocks[index]  = 0;
#endif
      index++;
    }

#if 1
    if (this->solid_mc_next_clocks_umap_umap.size() != 0 && this->solid_mc_next_clocks_umap_umap.size() < input_format.test_tables_map.size()) {
      REMPI_ERR("test_table_map size increased: Ues larger number for REMPI_MAX variable (default: %d)", REMPI_DEFAULT_MAX_EVENT_LENGTH);
    }
    //    this->solid_mc_next_clocks_umap_umap.resize(input_format.test_tables_map.size(), NULL);
    for (map<int, rempi_encoder_input_format_test_table*>::iterator it = input_format.test_tables_map.begin(),
	   it_end = input_format.test_tables_map.end(); it != it_end; it++) {
      int matching_set_id;
      matching_set_id = it->first;
      unordered_map<int, size_t> *umap = new unordered_map<int, size_t>();
      for (unordered_set<int>::const_iterator cit = mc_recv_ranks_uset.cbegin(),
    	     cit_end = mc_recv_ranks_uset.cend();
    	   cit != cit_end;
    	   cit++) {
    	int rank = *cit;
    	umap->insert(make_pair(rank, 0));
      }
      if (this->solid_mc_next_clocks_umap_umap.find(matching_set_id) != 
	  this->solid_mc_next_clocks_umap_umap.end()) {
	if (this->solid_mc_next_clocks_umap_umap.size() != umap->size()) {
	  REMPI_ERR("solid_mc_next_clocks_umap_umap.size() changes from %lu to %lu", 
		    this->solid_mc_next_clocks_umap_umap.size(),
		    umap->size());
	}
      } else {
	this->solid_mc_next_clocks_umap_umap[matching_set_id] = umap;
      }
    }
#else
    if (this->solid_mc_next_clocks_umap_vec.size() != 0 && this->solid_mc_next_clocks_umap_vec.size() < input_format.test_tables_map.size()) {
      REMPI_ERR("test_table_map size descreased: Ues larger number for REMPI_MAX variable (default: %d)", REMPI_DEFAULT_MAX_EVENT_LENGTH);
    }
    this->solid_mc_next_clocks_umap_vec.resize(input_format.test_tables_map.size(), NULL);
    for (int i = 0, size = this->solid_mc_next_clocks_umap_vec.size();
    	 i < size;
    	 i++){
      if (this->solid_mc_next_clocks_umap_vec[i] != NULL) continue;
      unordered_map<int, size_t> *umap = new unordered_map<int, size_t>();
      for (unordered_set<int>::const_iterator cit = mc_recv_ranks_uset.cbegin(),
    	     cit_end = mc_recv_ranks_uset.cend();
    	   cit != cit_end;
    	   cit++) {
    	int rank = *cit;
    	umap->insert(make_pair(rank, 0));
      }
      this->solid_mc_next_clocks_umap_vec[i] = umap;
    }
#endif

    this->mc_recv_ranks        = input_format.mc_recv_ranks;
    this->mc_length      = input_format.mc_length;
    // for (int i = 0; i < this->mc_length; i++) {
    //   REMPI_DBG("mc_recv_ranks[%d] = %d", i, this->mc_recv_ranks[i]);
    // }
#ifndef CP_DBG
    this->mc_next_clocks       = input_format.mc_next_clocks;
    this->tmp_mc_next_clocks   = input_format.tmp_mc_next_clocks;
#endif
    /*array for waiting receive msg count for each test_id*/
    //    int    *tmp =  new int[input_format.test_tables_map.size()];;

#if 0    
    int    *tmp =  (int*)rempi_malloc(sizeof(int) * input_format.test_tables_map.size());
    memset(tmp, 0, sizeof(int) * input_format.test_tables_map.size());
    if (this->num_of_recv_msg_in_next_event != NULL) free(this->num_of_recv_msg_in_next_event);
    this->num_of_recv_msg_in_next_event = tmp;
    size_t *tmp2 = (size_t*)rempi_malloc(sizeof(size_t) *input_format.test_tables_map.size());
    memset(tmp2, 0, sizeof(size_t) * input_format.test_tables_map.size());
    if (this->dequeued_count != NULL) free(this->dequeued_count);
    this->dequeued_count = tmp2;
    size_t *tmp3 = (size_t*)rempi_malloc(sizeof(size_t) *input_format.test_tables_map.size());
    memset(tmp3, 0, sizeof(size_t) * input_format.test_tables_map.size());
    if (this->interim_min_clock_in_next_event != NULL) free(this->interim_min_clock_in_next_event);
    this->interim_min_clock_in_next_event = tmp3;
#else
    // this->interim_min_clock_in_next_event_umap[0] = 1;
    //this->interim_min_clock_in_next_event_umap.clear();
    // this->matched_events_list_umap[0] = NULL;
    // this->matched_events_list_umap.clear();

#endif

  }

  
  /*=====================================================*/

  if (decoding_address != decompressed_record) {
    REMPI_ERR("Inconsistent size: decoded size: %lu, decompressed size: %lu", decoding_address, decompressed_record);
  }

  if (test_id > REMPI_MAX_RECV_TEST_ID) {
    REMPI_ERR("The number of test_id exceeded the limit");
  }

  
  //  input_format.debug_print();
  //  exit(0);

  return;
}

void rempi_encoder_cdc::cdc_prepare_decode_indices(
						   size_t matched_event_count,
						   vector<size_t> &matched_events_id_vec,
						   vector<size_t> &matched_events_delay_vec,
						   vector<int> &matched_events_permutated_indices_vec)
{
  list<int> permutated_indices_list;
  /*
    0:list = 0
    1:list = 1
    2:list = 2
    3:list = 3
  */
  unordered_map<int, list<int>::iterator> permutated_indices_it_map;
  /*
    0:list = 0
    map[1] --> 1:list = 1
    map[2] --> 2:list = 2
    3:list = 3
  */
  int it_index = 0;

  /*First create list initialized by sequencial numbers, 
    whose values are permutated later */
  for (int i = 0; i < matched_event_count; ++i) {
    permutated_indices_list.push_back(i);
  }

  /*memorize values to be permutated*/
  list<int>::iterator it = permutated_indices_list.begin();
  int last_id = 0;
  for (vector<size_t>::const_iterator cit = matched_events_id_vec.cbegin(),
	 cit_end = matched_events_id_vec.cend();
       cit != cit_end;
       cit++) {
    size_t id = *cit;
    advance(it, (int)(id) - last_id);
    permutated_indices_it_map[id] = it;
    last_id = id;
  }  
  
  /* ==== permutated_indices ===== */  
  for (int i = 0, n = matched_events_id_vec.size(); i < n ; i++) {
    int id    = (int)matched_events_id_vec[i];
    int delay = (int)matched_events_delay_vec[i];
    list<int>::iterator it = permutated_indices_it_map[id];
    //    REMPI_ERR("id:%d == delay:%d *it: %d", id, delay,*it);
    it = permutated_indices_list.erase(it);
    advance(it, delay);
    permutated_indices_list.insert(it, id);
  }

#ifdef REMPI_DBG_REPLAY
  // for (int &v: permutated_indices_list) {
  //   REMPI_DBGI(REMPI_DBG_REPLAY, "index: %d", v);
  // }
#endif
  matched_events_permutated_indices_vec.resize(matched_event_count, 0);
  int sequence = 0;
  for (list<int>::const_iterator cit = permutated_indices_list.cbegin(),
	 cit_end = permutated_indices_list.cend();
       cit != cit_end;
       cit++) {
    int v = *cit;
    matched_events_permutated_indices_vec[v] = sequence++;
  }
  
  /* == square_sizes_indices == */
  // int init_square_size = 0;
  // int max = 0;
  // matched_events_square_sizes_vec.push_back(init_square_size);
  // for (int i = 0, n = matched_events_permutated_indices_vec.size(); i < n; i++) {
  //   matched_events_square_sizes_vec.back()++;
  //   if (max < matched_events_permutated_indices_vec[i]) {
  //     max = matched_events_permutated_indices_vec[i];
  //   }
  //   if (i == max) {
  //     matched_events_square_sizes_vec.push_back(init_square_size);
  //   }
  // }
  // if (matched_events_square_sizes_vec.back() == 0) {
  //   matched_events_square_sizes_vec.pop_back();
  // }
  return;
    
}


static int first = 1, first2=1;

bool rempi_encoder_cdc::cdc_decode_ordering(rempi_event_list<rempi_event*> *recording_events, list<rempi_event*> *event_list, rempi_encoder_input_format_test_table* test_table, list<rempi_event*> *replay_event_list, int test_id, int local_min_id_rank, size_t local_min_id_clock)
{
  vector<rempi_event*> replay_event_vec;
  bool is_reached_epoch_line;
#ifdef REMPI_DBG_REPLAY
  bool is_ordered_event_list_updated = false;
  bool is_solid_ordered_event_list_updated = false;
    
#endif



  // if(test_id == 1) {
  //   if (test_table->ordered_event_list.size() > 0 || test_table->solid_ordered_event_list.size() > 0) {
  //     REMPI_DBGI(3, "ordered: %lu, solide: %lu",    test_table->ordered_event_list.size() ,     test_table->solid_ordered_event_list.size() );
  //   }
  // }


  
  if (test_table->is_decoded_all()) {
    /* When a function caling cdc_decode_ordering receive "true", the function increment counter by 1. 
       And the counter become same as the number of test_table, then read the next epoch. 
       If this test_table return true again, ReMPI read the next epoch while other test_tables may not be finised.
       So When the function tried to decode for this test_table even after this test_table has been already finished, 
       cdc_decode_ordering return false.
     */
    return false;
  }

  if (replay_event_list->size() != 0) {
    REMPI_ERR("replay_event_list is not empty, and the replaying events are not passed to replaying_events");
  }

#ifdef REMPI_DBG_REPLAY
  //  REMPI_DBGI(REMPI_DBG_REPLAY, "unamtched size: %d, replayed count: %d, count %d", test_table->unmatched_events_umap.size(), test_table->replayed_matched_event_index, test_table->count);
#endif

  // REMPI_DBGI(0, "--- index: %d: size: %lu (test_id: %d)", 
  // 	       test_table->replayed_matched_event_index, 
  // 	     test_table->unmatched_events_umap.size(), test_id);
  /*First, check if unmatched replay or not*/
  if (test_table->unmatched_events_umap.find(test_table->replayed_matched_event_index)
      != test_table->unmatched_events_umap.end()) {
    rempi_event *unmatched_event;
    int event_count = test_table->unmatched_events_umap[test_table->replayed_matched_event_index];
    unmatched_event = rempi_event::create_test_event(event_count, 0, 
					   REMPI_MPI_EVENT_INPUT_IGNORE,
					   REMPI_MPI_EVENT_INPUT_IGNORE,
					   REMPI_MPI_EVENT_INPUT_IGNORE,
					   REMPI_MPI_EVENT_INPUT_IGNORE,
					   REMPI_MPI_EVENT_INPUT_IGNORE);


#ifdef REMPI_DBG_REPLAY
    REMPI_DBGI(REMPI_DBG_REPLAY, "PQ -> RPQv ; (count: %d, with_next: %d, flag: %d, source: %d, clock: %d)", 
     	        unmatched_event->get_event_counts(),  unmatched_event->get_is_testsome(),  unmatched_event->get_flag(),
 	        unmatched_event->get_source(), unmatched_event->get_clock());
#endif

    replay_event_list->push_back(unmatched_event);
    test_table->unmatched_events_umap.erase(test_table->replayed_matched_event_index);
    return test_table->is_decoded_all();
  }





  /* ===== replaying events is matched event from here =======*/
  //  this->compute_local_min_id(test_table, &local_min_id_rank, &local_min_id_clock, test_id);
  // REMPI_DBGI(0, "<%d, %lu> size: %lu (recv_test_id: %d)", 
  // 	     local_min_id_rank, local_min_id_clock, test_table->ordered_event_list.size(), test_id);
  
 /* ====== Note: condition for relay =========================*/
  /*
    Before main thread increases next_clock via udpate_fd_next_clock(),
    with is_waiting_recv=1, main thread
    (1) checks if next events are recv events, so that main
    thread can make sure it
    (2) then, checks if replay queue is empty.
    If condition (1) & (2) both hold, the next event is recve event.


    (1): -- CDC thread --
      So firstly, CDC needs to tell condition for (1)
N      CDC events flow:
        CDC thread (Decode event, and put  => replay_list => 
           => replay_queue) => Main thread get events
      
      At this point, all replayed events are visible to Main thread.
      So, if CDC's "is_waiting_msg is TREUE", and 
      "handling_msg_count == adding_msg_count", then 
      Condition (1) is held.

    (2): -- Main thread --
      Main thread check 
         first, "Condition (1)" 
	 then, check if replay queue is empty.
      In this case, main thread know 
        1. Next events is receive, 
	2. And, a clock of the receive event at least more than global_local_min_id
	3. Thus, next_clock = max(global_local_min, local_clock) + 1

   */
  /* =========================================================*/
  
  /* ====== Operation A  ========*/
  /* 1. Put events to list */
  if (!event_list->empty()) {

    for (list<rempi_event*>::iterator it = event_list->begin(),
	   it_end = event_list->end();
	 it != it_end; ) {
      rempi_event *event = *it;

      if (!test_table->within_epoch(event)) {
	/* This event should be decoded in the next epoch */
	it++;
	continue;
      }
    

      test_table->ordered_event_list.push_back(event);
      /*2. update pending_msg_count_umap, epoch_line_umap */
      test_table->pending_event_count_umap[event->get_source()]++; /*TODO: this umap is not used*/
      test_table->current_epoch_line_umap[event->get_source()] = event->get_clock();

      // if (first2) {
      // 	REMPI_DBGI(0, "RCQ -> OEL ; (count: %d, with_next: %d, flag: %d, source: %d, clock: %d): list size: %d (test_id: %d)",
      // 		   event_vec[i]->get_event_counts(), event_vec[i]->get_is_testsome(), event_vec[i]->get_flag(),
      // 		   event_vec[i]->get_source(),  event_vec[i]->get_clock(), 
      // 		   test_table->ordered_event_list.size(), test_id);
      // 	first2=0;
      // }

      // REMPI_DBGI(0, "RCQ -> OEL ; (count: %d, with_next: %d, flag: %d, source: %d, clock: %d): list size: %d (test_id: %d)",
      // 		 event->get_event_counts(), event->get_is_testsome(), event->get_flag(),
      // 		 event->get_source(),  event->get_clock(), 
      // 		 test_table->ordered_event_list.size(), test_id);

#ifdef REMPI_DBG_REPLAY
      REMPI_DBGI(REMPI_DBG_REPLAY, "Local_min: <%d, %lu> resv_test_id: %d", 
	     local_min_id_rank, local_min_id_clock, test_id);
      REMPI_DBGI(REMPI_DBG_REPLAY, "RCQ -> OEL ; (count: %d, with_next: %d, flag: %d, source: %d, clock: %d): list size: %d (test_id: %d)",
		 event->get_event_counts(), event->get_is_testsome(), event->get_flag(),
		 event->get_source(),  event->get_clock(), 
		 test_table->ordered_event_list.size(), test_id);
      is_ordered_event_list_updated = true;
#endif
      
      it = event_list->erase(it);
    }
  }
  /* ====== End of Operation A  ========*/


  /*Sort observed list, and create totally-ordered list*/
  test_table->ordered_event_list.sort(compare);    
  //  REMPI_DBG("size: %lu, min_clock: %lu", test_table->ordered_event_list.size(), local_min_id_clock);

  //  usleep(50000);

  { 
    /* ====== Operation B  ========*/
    /* Count how many events move from ordered_event_list to solid_ordered_event_list */
    int solid_event_count = 0;
    if (local_min_id_clock == PNMPI_MODULE_CLMPI_COLLECTIVE) {
      /*Other ranks (except me) are in collective*/
      solid_event_count = test_table->ordered_event_list.size();
#ifdef REMPI_DBG_REPLAY
      is_solid_ordered_event_list_updated = true;
#endif
    } else {
      /*Count solid event count 
	Sorted "solid events" order does not change by the rest of events
      */
      for (list<rempi_event*>::const_iterator cit = test_table->ordered_event_list.cbegin(),
	     cit_end = test_table->ordered_event_list.cend();
	   cit != cit_end;
	   cit++) {
	rempi_event *event = *cit;
	is_reached_epoch_line = test_table->is_reached_epoch_line();
	if (!compare2(local_min_id_rank, local_min_id_clock, event) || is_reached_epoch_line) {
	  solid_event_count++;
	  /*If event < local_min_id.{rank, clock} ... */
#ifdef REMPI_DBG_REPLAY
	  is_solid_ordered_event_list_updated = true;
#endif
	} else {
	/*Because ordered_event_list is sorted, so we do not need to check the rest of events*/
	  break;
	}   
      }
    }

    /* ====== End of Operation B  ========*/

#ifdef REMPI_DBG_REPLAY
    //    REMPI_DBGI(REMPI_DBG_REPLAY, "local_min (rank: %d, clock: %lu): count: %d",
    //	       local_min_id_rank, local_min_id_clock, solid_event_count);
    /* TODO: caling recording_events.size_replay(test_id) here causes seg fault, so I removed 
       I'll find out the reason later
     */
    // REMPI_DBGI(REMPI_DBG_REPLAY, "local_min (rank: %d, clock: %lu): count: %d, RCQ:%d(test:%d)", 
    // 	       local_min_id_rank, local_min_id_clock, solid_event_count, 
    // 	       recording_events.size_replay(test_id), test_id);
#endif 
    /*Between Operation A and B, 
      main thread may enqueue events(X), and increment <local_min_id>, then solid order become wrong order 
      because moving events to solid_ordered_event_list based on <local_min_id>, 
      but event X is not included in ordered_event_list at that time.
      So making suare that  main thread did not enqueue any events*/
      //    if (recording_events.size_replay(test_id) == 0) {       
    {
      /*Move solid events to solid_ordered_event_list*/
      int next_clock_order = test_table->solid_ordered_event_list.size() + test_table->replayed_matched_event_index;
      for (int i = 0; i < solid_event_count; ++i) {
	/*ordered_event_list ====> ordered_event_chunk_list */
	rempi_event *event = test_table->ordered_event_list.front();
	if (test_table->solid_ordered_event_list.size() > 0) {
	  if (!compare(test_table->solid_ordered_event_list.back(), event)) {
	    REMPI_ERR("Enqueuing an event (rank: %d, clock:%lu) which is smaller than (rank: %d, clock: %lu) (test_id: %d)",
		      event->get_source(), event->get_clock(),
		      test_table->solid_ordered_event_list.back()->get_source(),
		      test_table->solid_ordered_event_list.back()->get_clock(),
		      test_id);
	  }
	}
	test_table->      ordered_event_list.pop_front();
	test_table->solid_ordered_event_list.push_back(event);

	// if (first) {
	//   REMPI_DBGI(0, "Put msg: source: %d, clock: %d (time: %f) %d %lu", 
	// 	     event->get_source(), event->get_clock(), PMPI_Wtime(),
	// 	     local_min_id_rank, local_min_id_clock);
	//   first = 0;
	// }
	event->clock_order = next_clock_order++;	
      }
    }


    
#ifdef REMPI_DBG_REPLAY
    if (is_ordered_event_list_updated || is_solid_ordered_event_list_updated) {
      REMPI_DBGI(REMPI_DBG_REPLAY, "LIST Queue Update: Local_min (rank: %d, clock: %lu, is_reached: %d): count: %d, test_id: %d",
		 local_min_id_rank, local_min_id_clock, is_reached_epoch_line, solid_event_count, test_id);
      for (list<rempi_event*>::const_iterator it = test_table->ordered_event_list.cbegin(), 
	     it_end = test_table->ordered_event_list.cend();
	   it !=it_end;
	   it++) {
	rempi_event *e = *it;
	REMPI_DBGI(REMPI_DBG_REPLAY, "       list (rank: %d, clock: %lu): count: %d", e->get_source(), e->get_clock(), solid_event_count);
      }

      for (list<rempi_event*>::const_iterator it = test_table->solid_ordered_event_list.cbegin(), 
	     it_end = test_table->solid_ordered_event_list.cend();
	   it !=it_end;
	   it++) {
	rempi_event *e = *it;
	REMPI_DBGI(REMPI_DBG_REPLAY, "      slist (rank: %d, clock: %lu, order: %lu): count: %d", 
		   e->get_source(), e->get_clock(), e->clock_order, solid_event_count);
      }
      is_ordered_event_list_updated = false;
      is_solid_ordered_event_list_updated = false;
    }
#endif 

  }



  /* 
     Compute outcount, 
     and initialization vector with the outcount size 
  */
  int outcount = 1;
  if (test_table->with_previous_bool_vec.size() > 0) {
    for (int i = test_table->replayed_matched_event_index; test_table->with_previous_bool_vec[i] == true; i++) {
      outcount++;
    }
  }
  replay_event_vec.resize(outcount, NULL);

  /* ==== Condition for (1) ===================================*/
  /*At this point, CDC replay receve event, so increment the value*/
  // num_of_recv_msg_in_next_event[test_id] = outcount;
  // if (test_table->solid_ordered_event_list.size() < outcount) {
  //   if (test_table->solid_ordered_event_list.size() > 0) {
  //     interim_min_clock_in_next_event[test_id] = (size_t)test_table->solid_ordered_event_list.back()->get_clock();
  //   }
  // } 
  /* =========================================================*/


  int added_count = 0;

  for (list<rempi_event*>::const_iterator cit = test_table->solid_ordered_event_list.cbegin(),
	 cit_end = test_table->solid_ordered_event_list.cend();
       cit != cit_end;
       cit++) {
    rempi_event *replaying_event = *cit;
    int permutated_index = test_table->matched_events_permutated_indices_vec[replaying_event->clock_order]
      - test_table->replayed_matched_event_index;
#ifdef REMPI_DBG_REPLAY
    if (is_ordered_event_list_updated || is_solid_ordered_event_list_updated) {
      REMPI_DBGI(REMPI_DBG_REPLAY, "  index: %d -> %d(%d) replayed_count: %d (event_clock: %d)", replaying_event->clock_order, 
		 test_table->matched_events_permutated_indices_vec[replaying_event->clock_order], 
		 permutated_index, test_table->replayed_matched_event_index, replaying_event->get_clock());
    }
#endif
    if (0 <= permutated_index && permutated_index < outcount) {
      replay_event_vec[permutated_index] = replaying_event;
      added_count++;
      if (replay_event_vec.size() == added_count) break;
    } else if (permutated_index < 0){
      REMPI_ERR("permutated_index:%d < 0 (record data may be truncated): src_idx: %d -> dest_idx: %d, replayed_index: %d", 
		permutated_index,
		replaying_event->clock_order,
		test_table->matched_events_permutated_indices_vec[replaying_event->clock_order],
		test_table->replayed_matched_event_index);
    }
  }






  /* Update interim min */
  /*   From the replay_event_vec, we estimate the next clock after this replay*/
  size_t tmp_interim_min_clock = 0;
  size_t replaying_clock = 0;

  //  this->compute_local_min_id(test_table, &local_min_id_rank, &local_min_id_clock, test_id); /*Update again*/

  list<rempi_event*>::const_iterator cit     = test_table->ordered_event_list.cbegin();
  list<rempi_event*>::const_iterator cit_end = test_table->ordered_event_list.cend();
  /*When calling compute_local_min_id, there may be an event in recording_events. 
    If this is the case, the event may have smaller clocl than local_min_id_clock, and 
    the tmp_interim_min_clock become bigger than correct value, and is wrong.
    So update tmp_interim_min_clock only when recording_events is empty even after 
    calling compute_local_min_id.
  */
  //  if (recording_events.size_replay(test_id) == 0 && local_min_id_clock != PNMPI_MODULE_CLMPI_COLLECTIVE) {
  if (local_min_id_clock != PNMPI_MODULE_CLMPI_COLLECTIVE) {
#ifdef RS_DBG
    clmpi_get_local_clock(&tmp_interim_min_clock);
#else
    clmpi_get_local_sent_clock(&tmp_interim_min_clock);
#endif

    /*local_sent_clock is sent clock value, so the local_clock is local_sent_clock + 1*/
    //    tmp_interim_min_clock++;
    for (vector<rempi_event*>::const_iterator cit_replay_event = replay_event_vec.cbegin(),
	   cit_replay_event_end = replay_event_vec.cend();
	 cit_replay_event != cit_replay_event_end;
	 cit_replay_event++) {
      rempi_event *replaying_event = *cit_replay_event;
      if (replaying_event == NULL) {
	/*Use min{local_min_clock, clock in ordered_event_list})*/
	if (cit == cit_end) {
	  replaying_clock = local_min_id_clock;
	} else {
	  if ((*cit)->get_clock() < local_min_id_clock) {
	    replaying_clock = (*cit)->get_clock();
	    cit++;
	  } else {
	    replaying_clock = local_min_id_clock;
	  }
	}
      } else {
	replaying_clock = replaying_event->get_clock();
      }
      if (tmp_interim_min_clock < replaying_clock) {
	tmp_interim_min_clock = replaying_clock;
      }
      tmp_interim_min_clock++;
    }

#ifdef REMPI_DBG_REPLAY
    if (interim_min_clock_in_next_event_umap[test_id] < tmp_interim_min_clock) {
      cit     = test_table->ordered_event_list.cbegin();
      cit_end = test_table->ordered_event_list.cend();
      size_t local_clock_dbg;
#ifdef RS_DBG
      clmpi_get_local_clock(&local_clock_dbg);
#else
      clmpi_get_local_sent_clock(&local_clock_dbg);
#endif


      REMPI_DBGI(REMPI_DBG_REPLAY, "INTRM update: local_clock: %lu", local_clock_dbg);
      for(vector<rempi_event*>::iterator it = replay_event_vec.begin(), it_end = replay_event_vec.end();
	  it != it_end; it++) {
	rempi_event *replaying_event = *it;
	//      for (rempi_event *replaying_event: replay_event_vec) {
	if (replaying_event == NULL) {
	  if (cit == cit_end) {
	    replaying_clock = local_min_id_clock;
	    REMPI_DBGI(REMPI_DBG_REPLAY, "  INTRM update: (rank: %d, clock: %lu) min", local_min_id_rank, local_min_id_clock);
	  } else {
	    if ((*cit)->get_clock() < local_min_id_clock) {
	      replaying_clock = (*cit)->get_clock();
	      REMPI_DBGI(REMPI_DBG_REPLAY, "  INTRM update: (rank: %d, clock: %lu) Q  ", (*cit)->get_source(), (*cit)->get_clock());
	      cit++;
	    } else {
	      replaying_clock = local_min_id_clock;
	      REMPI_DBGI(REMPI_DBG_REPLAY, "  INTRM update: (rank: %d, clock: %lu) min", local_min_id_rank, local_min_id_clock);
	    }
	  }
	} else {
	  replaying_clock = replaying_event->get_clock();
	  REMPI_DBGI(REMPI_DBG_REPLAY, "  INTRM update: (rank: %d, clock: %lu)", replaying_event->get_source(), replaying_event->get_clock());
	}
      }
      REMPI_DBGI(REMPI_DBG_REPLAY, "INTRM update: interim: %lu => %lu (recv_test_id: %d)", interim_min_clock_in_next_event_umap[test_id], tmp_interim_min_clock, test_id);
    }

#endif
    //    REMPI_DBG("test_id: %d, clock: %lu", test_id, tmp_interim_min_clock);
    this->interim_min_clock_in_next_event_umap[test_id] = tmp_interim_min_clock;

  }



  /*Check if all replaying events occured, 
    and are added to replay_event_vec*/
  if (added_count != outcount) {
    /*If all replaying is not present, i.e., need more messages to replay, 
      then, revoke(clear~) this incomplete replay_event_vec  */
    replay_event_vec.clear();
#ifdef REMPI_DBG_REPLAY
    //    REMPI_DBGI(REMPI_DBG_REPLAY, "abort !! outcount: %d, but added_count: %d", outcount, added_count);
#endif
    int ret = test_table->is_decoded_all();
    if (ret == true) {
      REMPI_DBG("Added count: %d, outcount: %d", added_count, outcount);
      REMPI_ERR("Inconsistent events: # replayed event: %lu, count: %lu (recv_test_id: %d)", 
    		test_table->replayed_matched_event_index, test_table->count, test_id);
    }

#if 0
    /* ==== Condition for (1) ===================================*/
    /*At this point, now, CDC need more msgs to replay.*/
    num_of_recv_msg_in_next_event[test_id] = outcount;
    size_t previous_count = dequeued_count[test_id];
    dequeued_count[test_id] = test_table->replayed_matched_event_index 
                                    + test_table->ordered_event_list.size() 
                                    + test_table->solid_ordered_event_list.size();
    if (previous_count > dequeued_count[test_id]) {
      REMPI_ERR("dequeued_count does not monotonously increase; "
		"(previous: %lu, new: %lu). "
		"This is usually caused in the multiple chunk mode \n",
		previous_count, dequeued_count[test_id]); 
      /* ------------------------------------------*/
      /*the below is a code to work around this problem in the multiple chunk mode,
        but I do not use this code for now*/
      dequeued_count[test_id] = test_table->replayed_matched_event_index 
	                                + test_table->ordered_event_list.size() 
	                                + test_table->solid_ordered_event_list.size()
	                                + previous_count;
      /* ------------------------------------------*/
    }
#endif

    /* =========================================================*/
    return ret;
  } else {
    /* ==== Condition for (1) ===================================*/
    /*At this point, now, the recv msg will be replayed, 
      and we do not know what the next event is now, so set 0*/
#if 0    
    num_of_recv_msg_in_next_event[test_id] = 0;
#endif
    /* =========================================================*/
  }



  /* Put with_next value  */
  for (int i = 0, n = replay_event_vec.size(); i < n; i++) {
    replay_event_vec[i]->set_with_next(1);
  }
  replay_event_vec.back()->set_with_next(0);


  if (test_table->matched_index_vec.size() > 0) {
    for (int i = 0; i < outcount; i++) {
      int matched_index = test_table->matched_index_vec[test_table->replayed_matched_event_index + i];
      replay_event_vec[i]->set_index(matched_index);
    }
  }


  for (int i = 0, n = replay_event_vec.size(); i < n; i++) {
    test_table->pending_event_count_umap[replay_event_vec[i]->get_source()]--; /*TODo: this umap is not used*/
    replay_event_list->push_back(replay_event_vec[i]);
    test_table->solid_ordered_event_list.remove(replay_event_vec[i]); /*This ".remove" may be slow*/
  }


  //#define REMPI_MCB_CHECK
#ifdef REMPI_MCB_CHECK
    for (int i = 0, n = replay_event_vec.size(); i < n; i++) {
      for (int j = 0; j < n; j++) { 
	if (j == i) continue;

	if (replay_event_vec[i]->get_source() == replay_event_vec[j]->get_source()) {
	  REMPI_DBG("==== Special Alart for MCB ===")
	  for (int k = 0; k < n; k++) {
	    REMPI_DBG("== Wrong RPQv ; (count: %d, with_next: %d, flag: %d, source: %d, clock: %d): order: %d",
		       replay_event_vec[k]->get_event_counts(), replay_event_vec[k]->get_is_testsome(), replay_event_vec[k]->get_flag(),
		      replay_event_vec[k]->get_source(), replay_event_vec[k]->get_clock(),
		      replay_event_vec[k]->clock_order);
	  }
	  REMPI_DBG("== Wrong local_min (rank: %d, clock: %lu): count: X, RCQ:%d(test:%d)", 
		    local_min_id_rank, local_min_id_clock,
		    recording_events->size_replay(test_id), test_id);
	  for (list<rempi_event*>::const_iterator cit = test_table->ordered_event_list.cbegin(),
		 cit_end = test_table->ordered_event_list.cend();
	       cit != cit_end;
	       cit++) {
	    rempi_event *e = *cit;
	    //	    REMPI_DBG("       list (rank: %d, clock: %lu): count: %d", e->get_source(), e->get_clock(), solid_event_count);
	    REMPI_DBG("== Wrong       list (rank: %d, clock: %lu)", e->get_source(), e->get_clock());
	  }
	  for (list<rempi_event*>::const_iterator cit = test_table->solid_ordered_event_list.cbegin(),
		 cit_end = test_table->solid_ordered_event_list.cend();
	       cit != cit_end;
	       cit++) {
	    rempi_event *e = *cit;

	    //	    REMPI_DBG("      slist (rank: %d, clock: %lu): count: %d", e->get_source(), e->get_clock(), solid_event_count);
	    REMPI_DBG("== Wrong      slist (rank: %d, clock: %lu, order: %lu)", e->get_source(), e->get_clock(), e->clock_order);
	  }
	  REMPI_DBG("== Wrong replayed_matched_event_index: %d", test_table->replayed_matched_event_index);
	  for (vector<rempi_event*>::const_iterator cit = replay_event_vec.cbegin(),
		 cit_end = replay_event_vec.cend();
	       cit != cit_end;
	       cit++) {
	    rempi_event *replaying_event = *cit;

	    int permutated_index = test_table->matched_events_permutated_indices_vec[replaying_event->clock_order]
	      - test_table->replayed_matched_event_index;
	    REMPI_DBG("== Wrong      index: %d -> %d(%d) replayed_count: %d (source: %d, event_clock: %d)", replaying_event->clock_order, 
		       test_table->matched_events_permutated_indices_vec[replaying_event->clock_order], 
		       permutated_index, test_table->replayed_matched_event_index, 
		      replaying_event->get_source(), replaying_event->get_clock());
	  }
	  //	  sleep(1);
	  //	  exit(1);
	}
      }      
    }
#endif

  test_table->replayed_matched_event_index += replay_event_vec.size();
  // REMPI_DBGI(0,"unamtched size: %d, replayed count: %d, count: %d (test_id: %d)", 
  //     	     test_table->unmatched_events_umap.size(), test_table->replayed_matched_event_index, test_table->count, test_id);


  

#ifdef REMPI_DBG_REPLAY
  for (int i = 0, n = replay_event_vec.size(); i < n; i++) {
    REMPI_DBGI(REMPI_DBG_REPLAY, "Final   RPQv ; (count: %d, with_next: %d, flag: %d, source: %d, clock: %d)",
	       replay_event_vec[i]->get_event_counts(), replay_event_vec[i]->get_is_testsome(), replay_event_vec[i]->get_flag(),
	       replay_event_vec[i]->get_source(), replay_event_vec[i]->get_clock());
  }
#ifdef REMPI_DBG_REPLAY
  REMPI_DBGI(REMPI_DBG_REPLAY, "LIST Queue Update: Local_min (rank: %d, clock: %lu): test_id: %d",
	     local_min_id_rank, local_min_id_clock, test_id);
  for (list<rempi_event*>::iterator it = test_table->ordered_event_list.begin(), 
	 it_end = test_table->ordered_event_list.end();
       it != it_end; it++) {
    //  for (rempi_event *e: test_table->ordered_event_list) {
    rempi_event *e = *it;
    REMPI_DBGI(REMPI_DBG_REPLAY, "       list (rank: %d, clock: %lu)", e->get_source(), e->get_clock());
  }
  for (list<rempi_event*>::iterator it = test_table->solid_ordered_event_list.begin(), 
	 it_end = test_table->solid_ordered_event_list.end();
       it != it_end; it++) {
    //  for (rempi_event *e: test_table->solid_ordered_event_list) {
    rempi_event *e = *it;

    REMPI_DBGI(REMPI_DBG_REPLAY, "      slist (rank: %d, clock: %lu, order: %lu)", e->get_source(), e->get_clock(), e->clock_order);
  }
#endif   
#endif


  return test_table->is_decoded_all();
}



void rempi_encoder_cdc::update_local_look_ahead_send_clock(
							   int replaying_event_type,
							   int matching_set_id)
{
  size_t next_clock;

  if (replaying_event_type == REMPI_ENCODER_REPLAYING_TYPE_RECV) {
    if (this->interim_min_clock_in_next_event_umap.find(matching_set_id) != 
	this->interim_min_clock_in_next_event_umap.end()) {
      next_clock = this->interim_min_clock_in_next_event_umap.at(matching_set_id);
    } else {
      next_clock = 0;
    }
  } else if (replaying_event_type == REMPI_ENCODER_REPLAYING_TYPE_ANY){
    clmpi_get_local_clock(&next_clock);
  } else {
    REMPI_ERR("Unknow replaying_event_type: %d", replaying_event_type);
  }

  if (rempi_cp_get_scatter_clock() < next_clock) {
    rempi_cp_set_scatter_clock(next_clock);
  }  

  return;
}

void rempi_encoder_cdc::set_fd_clock_state(int flag)
{
  if (flag) {
    tmp_fd_next_clock = rempi_cp_get_scatter_clock();
    rempi_cp_set_scatter_clock(PNMPI_MODULE_CLMPI_COLLECTIVE);
  } else {
    rempi_cp_set_scatter_clock(tmp_fd_next_clock);
  }  

  return;  
}

void rempi_encoder_cdc::read_footer()
{
  int fd;
  size_t chunk_size = 1;
  size_t read_count = 1;

  size_t rempi_cp_pred_rank_count;
  int *rempi_cp_pred_ranks;

  int rempi_reqmg_matching_set_id_length;
  int *rempi_reqmg_mpi_call_ids;
  int *rempi_reqmg_matching_set_ids;


  fd = open(record_path.c_str(), O_RDONLY);
  while(chunk_size != 0 && read_count != 0) {
    read_count = read(fd, &chunk_size, sizeof(size_t));
    lseek(fd, chunk_size, SEEK_CUR);
  }

  //  REMPI_DBGI(0, "size: %lu", chunk_size);
  if (chunk_size == 0) {
    /* rempi_cp_pred_ranks */
    read_count = read(fd, &rempi_cp_pred_rank_count, sizeof(size_t));
    //    REMPI_DBGI(0, "count: %lu", rempi_cp_pred_rank_count);
    if (rempi_cp_pred_rank_count > 0) {
      rempi_cp_pred_ranks = (int*)rempi_malloc(rempi_cp_pred_rank_count * sizeof(int));
      read_count = read(fd, rempi_cp_pred_ranks, rempi_cp_pred_rank_count * sizeof(int));
      if (read_count != rempi_cp_pred_rank_count * sizeof(int)) {
        REMPI_ERR("Incocnsistent size to actual data size");
      }
      rempi_free(rempi_cp_pred_ranks);
      /* rempi_cp_set_pred_ranks(...): This is just a placeholder for futuer refactoring */
    }


    /* rempi_reqmg_matching_set_ids */
    read_count = read(fd, &rempi_reqmg_matching_set_id_length, sizeof(int));
    //    REMPI_DBGI(0, "read_count: %d, length: %d", read_count, rempi_reqmg_matching_set_id_length);
    if (rempi_reqmg_matching_set_id_length > 0) {
      rempi_reqmg_mpi_call_ids     = (int*)rempi_malloc(rempi_reqmg_matching_set_id_length * sizeof(int));
      read_count = read(fd, rempi_reqmg_mpi_call_ids, rempi_reqmg_matching_set_id_length * sizeof(int));
      if (read_count != rempi_reqmg_matching_set_id_length * sizeof(int)) {
        REMPI_ERR("Incocnsistent size to actual data size");
      }
      rempi_reqmg_matching_set_ids = (int*)rempi_malloc(rempi_reqmg_matching_set_id_length * sizeof(int));
      read_count = read(fd, rempi_reqmg_matching_set_ids, rempi_reqmg_matching_set_id_length * sizeof(int));
      if (read_count != rempi_reqmg_matching_set_id_length * sizeof(int)) {
        REMPI_ERR("Incocnsistent size to actual data size");
      }
      rempi_reqmg_set_matching_set_id_map(rempi_reqmg_mpi_call_ids, rempi_reqmg_matching_set_ids, rempi_reqmg_matching_set_id_length);
      rempi_free(rempi_reqmg_mpi_call_ids);
      rempi_free(rempi_reqmg_matching_set_ids);
    } else {
      //      REMPI_DBG("No mathcing_set_id map");
    }

  } else {
    if (read_count == 0) REMPI_ERR("No footer in ReMPI record");
  }
  close(fd);
  
}

void rempi_encoder_cdc::write_footer()
{
    size_t val;
    /* ======= Write separator =========== */
    val = all_epoch_rank_separator;
    record_fs.write((char*)&val, sizeof(size_t));
    /* =================================== */

    /* ======= Global predecessor rank =========== */
    val = rempi_encoder_input_format_test_table::all_epoch_rank_uset.size();
    //    REMPI_DBGI(0, "length: %lu", val);
    record_fs.write((char*)&val, sizeof(size_t));
    for (unordered_set<int>::iterator it = rempi_encoder_input_format_test_table::all_epoch_rank_uset.begin(),
	   it_end = rempi_encoder_input_format_test_table::all_epoch_rank_uset.end();
	 it != it_end; it++) {
      int rank = *it;
      //      REMPI_DBGI(0, "rank: %d", rank);
      record_fs.write((char*)&rank, sizeof(int));
    }
    /* =================================== */
    
    /* ======= Global matching set ids =========== */
    int *mpi_call_ids;
    int *matching_set_ids;
    int length;
    rempi_reqmg_get_matching_set_id_map(&mpi_call_ids, &matching_set_ids, &length);
    val = length;
    //    REMPI_DBGI(0, "length: %d", length);
    record_fs.write((char*)&length, sizeof(int));
    for (int i = 0; i < length; i++) {
      record_fs.write((char*)&mpi_call_ids[i], sizeof(int));
    }
    for (int i = 0; i < length; i++) {
      //      REMPI_DBGI(0, "%d -> %d (%d/%d)", mpi_call_ids[i], matching_set_ids[i], i, length);
      record_fs.write((char*)&matching_set_ids[i], sizeof(int));
    }
    rempi_free(mpi_call_ids);
    rempi_free(matching_set_ids);

    /* =================================== */
    
    return;
}

void rempi_encoder_cdc::close_record_file()
{
  if (rempi_mode == REMPI_ENV_REMPI_MODE_RECORD) {
    write_footer();
  }

  rempi_encoder::close_record_file();
}


 void rempi_encoder_cdc::insert_encoder_input_format_chunk_recv_test_id(rempi_event_list<rempi_event*> *recording_events, rempi_event_list<rempi_event*> *replaying_events, rempi_encoder_input_format *input_format, bool *is_epoch_finished, int *has_new_event, int recv_test_id)
{
  bool is_finished;
  int local_min_id_rank = -1;
  size_t local_min_id_clock = 0;
  int sleep_counter = input_format->test_tables_map.size(); 
  *has_new_event = 0;
  *is_epoch_finished = false;



  if (matched_events_list_umap.find(recv_test_id) == matched_events_list_umap.end()) {
    matched_events_list_umap[recv_test_id]  = new list<rempi_event*>();
  }

  this->compute_local_min_id(input_format->test_tables_map.at(recv_test_id), &local_min_id_rank, &local_min_id_clock, recv_test_id);
  while (recording_events->front_replay(recv_test_id) != NULL) {      
    /*If a message(event) arrives,  */
    rempi_event *matched_event;
    int event_list_status;
    matched_event = recording_events->dequeue_replay(recv_test_id, event_list_status);
    matched_events_list_umap[recv_test_id]->push_back(matched_event);
    //    REMPI_DBGI(0, "A -> list: source: %d, clock: %d", matched_event->get_source(), matched_event->get_clock());



#ifdef REMPI_DBG_REPLAY
    //    REMPI_DBGI(REMPI_DBG_REPLAY, "===== checkpoint recv_test_id: %d: %lu", recv_test_id, matched_events_list_umap[recv_test_id]->size());
#endif

  } 

  if (matched_events_list_umap[recv_test_id]->size() != 0) *has_new_event = 1;

  //  REMPI_DBGI(0, "size: %lu", matched_events_list_umap[recv_test_id]->size());
  /*Permutate "*matched_events_list_umap"
    based on "input_format->test_tables_map", then
    output the replayed event into "replay_event_list"*/
  is_finished = cdc_decode_ordering(recording_events, matched_events_list_umap[recv_test_id], input_format->test_tables_map[recv_test_id], &replay_event_list, recv_test_id, local_min_id_rank, local_min_id_clock);
  
  if (is_finished) {
    //    delete matched_events_list_umap[recv_test_id];
    //    matched_events_list_umap[recv_test_id] = NULL;
    finished_testsome_count++;
    //    REMPI_DBGI(3, "finished: %d (count: %d / %lu)", recv_test_id, finished_testsome_count, input_format->test_tables_map.size());
    if (finished_testsome_count >= input_format->test_tables_map.size()) {

      finished_testsome_count = 0;
      *is_epoch_finished = true;
    }
  }

  for (list<rempi_event*>::iterator it = replay_event_list.begin(),
	 it_end = replay_event_list.end();
       it != it_end;
       it++) {
    rempi_event *e = *it;


#ifdef REMPI_DBG_REPLAY
    REMPI_DBGI(REMPI_DBG_REPLAY, "RPQv -> RPQ ; (count: %d, with_next: %d, flag: %d, source: %d, clock: %d) recv_recv_test_id: %d",
	       e->get_event_counts(), e->get_is_testsome(), e->get_flag(),
	       e->get_source(), e->get_clock(), recv_test_id);
#endif
    replaying_events->enqueue_replay(e, recv_test_id);
  }
  replay_event_list.clear();

  return;
}

void rempi_encoder_cdc::insert_encoder_input_format_chunk(rempi_event_list<rempi_event*> &recording_events, rempi_event_list<rempi_event*> &replaying_events, rempi_encoder_input_format &input_format)
{
  bool is_finished = false;
  int finished_testsome_count  = 0;
  int has_new_event;
  int sleep_counter = input_format.test_tables_map.size(); 


  /*all of recording_events queue (# is sleep_counter), this thread sleeps a little*/

  for (int recv_test_id = 0, n = input_format.test_tables_map.size(); 
       finished_testsome_count < n; 
       recv_test_id = ++recv_test_id % n) {
    
    insert_encoder_input_format_chunk_recv_test_id(&recording_events, &replaying_events, &input_format, &is_finished, &has_new_event, recv_test_id);
    if (is_finished) {
      finished_testsome_count++;
      is_finished = false;
    }

    if (!has_new_event) {
      sleep_counter--;
      if (sleep_counter <= 0) {
	usleep(1);
	sleep_counter = input_format.test_tables_map.size();
      }      
    }

  }

#ifdef REMPI_DBG_REPLAY  
  REMPI_DBGI(REMPI_DBG_REPLAY, "finish count: %d", finished_testsome_count);
#endif
  return;
}

// vector<rempi_event*> rempi_encoder_cdc::decode(char *serialized_data, size_t *size)
// {
//   vector<rempi_event*> vec;
//   int *mpi_inputs;
//   int i;

//   /*In rempi_envoder, the serialized sequence identicals to one Test event */
//   mpi_inputs = (int*)serialized_data;
//   vec.push_back(
//     new rempi_test_event(mpi_inputs[0], mpi_inputs[1], mpi_inputs[2], 
// 			 mpi_inputs[3], mpi_inputs[4], mpi_inputs[5])
//   );
//   delete mpi_inputs;
//   return vec;
// }


#endif


