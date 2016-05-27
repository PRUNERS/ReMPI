#include <mpi.h>
#if MPI_VERSION == 3 && !defined(REMPI_LITE)

#include <stdlib.h>
#include <string.h>

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


/* ==================================== */
/* CLASS rempi_encoder_cdc_input_format_test_table */

bool rempi_encoder_input_format_test_table::is_decoded_all()
{
  bool is_unmatched_finished = true;

#ifdef REMPI_DBG_REPLAY
  // REMPI_DBGI(REMPI_DBG_REPLAY, "unamtched size: %d, replayed count: %d, count: %d", 
  // 	     this->unmatched_events_umap.size(), this->replayed_matched_event_index, count);
#endif
  if (!unmatched_events_umap.empty()) {
    return false;
  }

  if (replayed_matched_event_index < count) {
    return false;
  }

  
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
    test_tables_map[event->get_test_id()] = new rempi_encoder_input_format_test_table();
  }
  test_table = test_tables_map[event->get_test_id()];

// #ifdef REMPI_DBG_REPLAY
//     REMPI_DBGI(REMPI_DBG_REPLAY, "  ADD   ; (count: %d, with_next: %d, flag: %d, source: %d, tag: %d, clock: %d): test_id: %d", 
//      	        event->get_event_counts(),  event->get_is_testsome(),  event->get_flag(),
// 	       event->get_source(),  event->get_tag(),  event->get_clock(), event->get_test_id());
// #endif


  if (is_matched_event) {
    test_table->events_vec.push_back(event);
    if (event->get_is_testsome()) {
      int added_event_index;
      added_event_index = test_table->events_vec.size() - 1;
      test_table->with_previous_vec.push_back(added_event_index);
    }
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
    /*format epoch line, and puth into vectork*/
    for (unordered_map<size_t, size_t>::iterator epoch_it  = test_table->epoch_umap.begin();
	 epoch_it != test_table->epoch_umap.end();
	 epoch_it++) {
      test_table->epoch_rank_vec.push_back (epoch_it->first );
      test_table->epoch_clock_vec.push_back(epoch_it->second);
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

    REMPI_DBG(  " ====== Test_id: %2d (test_table: %p)======", test_id, test_table);

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

#if 1
    for (int i = 0; i < test_table->matched_events_square_sizes_vec.size(); i++) {
      REMPI_DBG("  matched(sq):  id: %d, size: %d", i, 
		test_table->matched_events_square_sizes_vec[i]);
    }
#endif

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


rempi_encoder_cdc::rempi_encoder_cdc(int mode)
  : rempi_encoder(mode)
  , fd_clocks(NULL)
{
  /* === Create CDC object  === */
  cdc = new rempi_clock_delta_compression(1);

  /* === Load CLMPI module  === */
  {
    // int err;
    // PNMPI_modHandle_t handle_test, handle_clmpi;
    // PNMPI_Service_descriptor_t serv;
    // /*Load clock-mpi*/
    // err=PNMPI_Service_GetModuleByName(PNMPI_MODULE_CLMPI, &handle_clmpi);
    // if (err!=PNMPI_SUCCESS) {
    //   REMPI_ERR("failed to load CLMPI modules");
    // }

    // /*Get clock-mpi service: fetch_next_clocks*/
    // err=PNMPI_Service_GetServiceByName(handle_clmpi,"clmpi_get_local_clock","p",&serv);
    // if (err!=PNMPI_SUCCESS) {
    //   REMPI_ERR("failed to load CLMPI function: clmpi_get_local_clock");
    // }
    // clmpi_get_local_clock=(PNMPIMOD_get_local_clock_t) ((void*)serv.fct);
    clmpi_get_local_clock = PNMPIMOD_get_local_clock;

    // /*Get clock-mpi service: fetch_next_clocks*/
    // err=PNMPI_Service_GetServiceByName(handle_clmpi,"clmpi_get_local_sent_clock","p",&serv);
    // if (err!=PNMPI_SUCCESS) {
    //   REMPI_ERR("failed to load CLMPI function: clmpi_get_local_sent_clock");
    // }
    // clmpi_get_local_sent_clock=(PNMPIMOD_get_local_sent_clock_t) ((void*)serv.fct);
    clmpi_get_local_sent_clock = PNMPIMOD_get_local_sent_clock;
  }

  if (mode == REMPI_ENV_REMPI_MODE_REPLAY) {
    /* == Init Window for one-sided communication for frontier detection*/
    PMPI_Comm_dup(MPI_COMM_WORLD, &mpi_fd_clock_comm);
    PMPI_Win_allocate(sizeof(struct frontier_detection_clocks), sizeof(size_t), MPI_INFO_NULL, mpi_fd_clock_comm, &fd_clocks, &mpi_fd_clock_win);
    memset(fd_clocks, 0, sizeof(struct frontier_detection_clocks));
    PMPI_Win_lock_all(MPI_MODE_NOCHECK, mpi_fd_clock_win);
    //    PMPI_Win_lock_all(0, mpi_fd_clock_win);
  }
  
  global_local_min_id.rank = -1;
  global_local_min_id.clock = 0;

  return;
}

rempi_encoder_cdc::~rempi_encoder_cdc()
{
  return;
}



void rempi_encoder_cdc::fetch_local_min_id(int *min_recv_rank, size_t *min_next_clock)
{  

  /*Fetch*/
  int i;
  int flag;

  //  if (!mc_flag || !mc_length) return;
  /*When CDC finish initialization of mc_recv_ranks, mc_next_clocks .., 
    then set mc_length. If mc_length is <= 0, skip execution of MPI_Get */
  if (mc_length <= 0) {
    //TODO: For now, main thread call MPI_Get everytime
    *min_recv_rank  = -1;
    *min_next_clock =  0;
    return;
  }

  // for (int i = 0; i < mc_length; ++i) {
  //   REMPI_DBGI(1, "Before recved: rank: %d clock:%lu, my next clock: %lu", mc_recv_ranks[i], mc_next_clocks[i], fd_clocks->next_clock);
  // }


// #ifdef REMPI_DBG_REPLAY
//   for (int i = 0; i < mc_length; ++i) {
//     REMPI_DBGI(REMPI_DBG_REPLAY, "Before recved: rank: %d clock:%lu, my next clock: %lu", mc_recv_ranks[i], mc_next_clocks[i], fd_clocks->next_clock);
//   }
// #endif


  /*Only after MPI_Win_flush_local_all, the retrived values by PMPI_Get become visible*/
  /* We call PMPI_Win_flush_local_all first to sync with the previous PMPI_Get,
       then post the next PMPI_Get(), which is synced by the next fetch_and_update_local_min_id() call.
     In this way, we mimic asynchronously fetch and update. */
  //  double s = rempi_get_time();
  PMPI_Win_flush_local_all(mpi_fd_clock_win);
  //  double e = rempi_get_time();  if (e - s > 0.001) REMPI_DBGI(0, "flush time: %f", e - s);
  /* --------------------- */


  PMPI_Win_flush_local_all(mpi_fd_clock_win);

  for (i = 0; i < mc_length; ++i) {
    PMPI_Get(&mc_next_clocks[i], sizeof(size_t), MPI_BYTE, mc_recv_ranks[i], 0, sizeof(size_t), MPI_BYTE, mpi_fd_clock_win);
  }



#ifdef REMPI_DBG_REPLAY
  for (int i = 0; i < mc_length; ++i) {
    REMPI_DBGI(REMPI_DBG_REPLAY, "After recved: rank: %d clock:%lu", mc_recv_ranks[i], mc_next_clocks[i]);
  }
#endif


  /*mc_next_clocks can change later, so copy to tmp_mc_next_clocks */
  memcpy(tmp_mc_next_clocks, mc_next_clocks, sizeof(size_t) * mc_length);

  *min_recv_rank  = mc_recv_ranks[0];
  *min_next_clock = mc_next_clocks[0];
  for (int i = 1; i < mc_length; ++i) {
    if (*min_next_clock > mc_next_clocks[i]) { 
      *min_recv_rank  = mc_recv_ranks[i];
      *min_next_clock = mc_next_clocks[i];
    } else if (*min_next_clock == mc_next_clocks[i]) { /*tie-break*/
      if (*min_recv_rank > mc_recv_ranks[i]) {
	*min_recv_rank  = mc_recv_ranks[i];
	*min_next_clock = mc_next_clocks[i];
      }
    }
  }

#ifdef REMPI_DBG_REPLAY
  //  REMPI_DBGI(REMPI_DBG_REPLAY, "GLLC Clock Fetched: rank:%d clock:%lu", *min_recv_rank, *min_next_clock);
#endif

  // TODO: move to here later
  // for (i = 0; i < mc_length; ++i) {
  //   PMPI_Get(&mc_next_clocks[i], sizeof(size_t), MPI_BYTE, mc_recv_ranks[i], 0, sizeof(size_t), MPI_BYTE, mpi_fd_clock_win);
  // }
  return;
}

  
void rempi_encoder_cdc::compute_local_min_id(rempi_encoder_input_format_test_table *test_table, 
					    int *local_min_id_rank,
					    size_t *local_min_id_clock)
{
  int epoch_rank_vec_size  = test_table->epoch_rank_vec.size();
  int epoch_clock_vec_size = test_table->epoch_clock_vec.size();
  if (epoch_rank_vec_size == 0 || epoch_clock_vec_size == 0) {
    REMPI_ERR("Epoch line is not decoded");
  }
  *local_min_id_rank  = test_table->epoch_rank_vec[0];
  *local_min_id_clock = this->solid_mc_next_clocks_umap[*local_min_id_rank];
  for (int i = 0; i < epoch_rank_vec_size; i++) {
    int    tmp_rank  =  test_table->epoch_rank_vec[i];
    size_t tmp_clock =  this->solid_mc_next_clocks_umap[tmp_rank];


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

/*TODO: change func name to update_solid_mc*/
int rempi_encoder_cdc::update_local_min_id(int min_recv_rank, size_t min_next_clock)
{
  for (int i = 0; i < mc_length; i++) {
    solid_mc_next_clocks_umap[mc_recv_ranks[i]] = tmp_mc_next_clocks[i];
  }
  //  memcpy(solid_mc_next_clocks, tmp_mc_next_clocks, sizeof(size_t) * mc_length);  

  int    last_local_min_id_rank  = global_local_min_id.rank;
  size_t last_local_min_id_clock = global_local_min_id.clock;


  global_local_min_id.rank  = min_recv_rank;
  global_local_min_id.clock = min_next_clock;


  if (last_local_min_id_rank  != global_local_min_id.rank ||
      last_local_min_id_clock != global_local_min_id.clock) {
#ifdef REMPI_DBG_REPLAY
    REMPI_DBGI(REMPI_DBG_REPLAY, "GLLC Clock Update: (rank: %d, clock:%lu) -> (rank:%d, clock:%lu)",
	       last_local_min_id_rank, last_local_min_id_clock,
	       min_recv_rank, min_next_clock
	       );
#endif
    return 1;
  }
  return 0;
}



rempi_encoder_input_format* rempi_encoder_cdc::create_encoder_input_format()
{
    return new rempi_encoder_cdc_input_format();
}


void rempi_encoder_cdc::compress_non_matched_events(rempi_encoder_input_format &input_format, rempi_encoder_input_format_test_table *test_table)
{
  char *write_buff;
  size_t write_size;

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



// void rempi_encoder_cdc::compress_non_matched_events(rempi_encoder_input_format &input_format, rempi_encoder_input_format_test_table *test_table)
// {
//   //  char  *compressed_buff, *original_buff;
//   //  size_t compressed_size,  original_size;


//   /*=== message count ===*/
//   input_format.write_queue_vec.push_back((char*)&test_table->count);
//   input_format.write_size_queue_vec.push_back(sizeof(test_table->count));
//   /*=====================*/

//   /*=== Compress Epoch   ===*/
//   /* (0) size*/
//   if (test_table->epoch_rank_vec.size() != test_table->epoch_clock_vec.size()) {
//     REMPI_ERR("Epoch size is inconsistent");
//   }
//   test_table->epoch_size = test_table->epoch_rank_vec.size() * sizeof(test_table->epoch_rank_vec[0]);
//   input_format.write_queue_vec.push_back((char*)&test_table->epoch_size);
//   input_format.write_size_queue_vec.push_back(sizeof(test_table->epoch_size));
//   /* (1) rank*/
//   input_format.write_queue_vec.push_back((char*)&test_table->epoch_rank_vec[0]);
//   input_format.write_size_queue_vec.push_back(test_table->epoch_size);
//   /* (2) clock*/
//   input_format.write_queue_vec.push_back((char*)&test_table->epoch_clock_vec[0]);
//   input_format.write_size_queue_vec.push_back(test_table->epoch_size);
//   /*====================================*/

//   /*=== Compress with_previous ===*/
//   //test_table->compressed_with_previous = (char*)compression_util.compress_by_zero_one_binary(test_table->with_previous_vec, compressed_size);
//   compression_util.compress_by_linear_prediction(test_table->with_previous_vec);
//   test_table->compressed_with_previous        = (char*)&test_table->with_previous_vec[0];
//   //   test_table->compressed_with_previous_size   = compressed_size;
//   test_table->compressed_with_previous_size   = test_table->with_previous_vec.size() * sizeof(test_table->with_previous_vec[0]);
//   test_table->compressed_with_previous_length = test_table->with_previous_vec.size();

//   input_format.write_queue_vec.push_back((char*)&test_table->compressed_with_previous_length);
//   input_format.write_size_queue_vec.push_back(sizeof(test_table->compressed_with_previous_length));
//   input_format.write_queue_vec.push_back((char*)&test_table->compressed_with_previous_size);
//   input_format.write_size_queue_vec.push_back(sizeof(test_table->compressed_with_previous_size));
//   input_format.write_queue_vec.push_back((char*)test_table->compressed_with_previous);
//   input_format.write_size_queue_vec.push_back(test_table->compressed_with_previous_size);
//   // REMPI_DBG("with_previous: %lu bytes (length: %lu)", 
//   // 	      test_table->compressed_with_previous_size, 
//   // 	      test_table->compressed_with_previous_length);
//   /*====================================*/


//   /*=== Compress unmatched_events ===*/
//   /* (1) id */
//   compression_util.compress_by_linear_prediction(test_table->unmatched_events_id_vec);
//   test_table->compressed_unmatched_events_id        = (char*)&test_table->unmatched_events_id_vec[0];
//   test_table->compressed_unmatched_events_id_size   = test_table->unmatched_events_id_vec.size() * sizeof(test_table->unmatched_events_id_vec[0]);

//   input_format.write_queue_vec.push_back((char*)&test_table->compressed_unmatched_events_id_size);
//   input_format.write_size_queue_vec.push_back(sizeof(test_table->compressed_unmatched_events_id_size));
//   input_format.write_queue_vec.push_back(test_table->compressed_unmatched_events_id);
//   input_format.write_size_queue_vec.push_back(test_table->compressed_unmatched_events_id_size);
//   // compressed_buff = compression_util.compress_by_zlib(original_buff, original_size, compressed_size);
//   // /* If compressed data is bigger than oroginal data, use original data*/    
//   // test_table->compressed_unmatched_events_id           = (compressed_buff == NULL)? original_buff:compressed_buff;
//   // test_table->compressed_unmatched_events_id_size      = (compressed_buff == NULL)? original_size:compressed_size;
//   //REMPI_DBG("unmatched(id): %lu bytes (<-zlib- %lu)", test_table->compressed_unmatched_events_id_size, original_size);

//   /* (2) count */
//   /*   count will be totally random number, so this function does not do zlib*/
//   test_table->compressed_unmatched_events_count        = (char*)&test_table->unmatched_events_count_vec[0];
//   test_table->compressed_unmatched_events_count_size   = test_table->unmatched_events_count_vec.size() * sizeof(test_table->unmatched_events_count_vec[0]);
//   input_format.write_queue_vec.push_back((char*)&test_table->compressed_unmatched_events_count_size);
//   input_format.write_size_queue_vec.push_back(sizeof(test_table->compressed_unmatched_events_count));
//   input_format.write_queue_vec.push_back(test_table->compressed_unmatched_events_count);
//   input_format.write_size_queue_vec.push_back(test_table->compressed_unmatched_events_count_size);
    
//   //    compressed_buff = compression_util.compress_by_zlib(original_buff, original_size, compressed_size);
//   /* If compressed data is bigger than oroginal data, use original data*/    
//   //    test_table->compressed_unmatched_events_count      = (compressed_buff == NULL)? original_buff:compressed_buff;
//   //    test_table->compressed_unmatched_events_count_size = (compressed_buff == NULL)? original_size:compressed_size;
//   //    REMPI_DBG("unmatched(ct): %lu bytes (<-zlib- %lu)", test_table->compressed_unmatched_events_count_size, original_size);
//   /*=======================================*/
//   return;
// }

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
  rempi_event *event_dequeued;
  bool is_ready_for_encoding = false;

  while (1) {
    /*Append events to current check as many as possible*/
    if (events.front() == NULL || input_format.length() > rempi_max_event_length) break;
    event_dequeued = events.pop();
    // REMPI_DBGI(0, "EXT ; (count: %d, with_next: %d, flag: %d, source: %d, clock: %d)", 
    //  	        event_dequeued->get_event_counts(),  event_dequeued->get_is_testsome(),  event_dequeued->get_flag(),
    // 	        event_dequeued->get_source(), event_dequeued->get_clock());
    input_format.add(event_dequeued);
  }

  if (input_format.length() == 0) return is_ready_for_encoding; /*false*/

  if (input_format.length() > rempi_max_event_length || events.is_push_closed_()) {
    /*If got enough chunck size, OR the end of run*/
    input_format.format();
    is_ready_for_encoding = true;
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
  
  if(rempi_gzip) {
    vector<char*> compressed_write_queue_vec;
    vector<size_t> compressed_write_size_queue_vec;
    compression_util.compress_by_zlib_vec(input_format.write_queue_vec, input_format.write_size_queue_vec,
					  compressed_write_queue_vec, compressed_write_size_queue_vec, total_compressed_size);
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
    is_no_more_record = true;
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


  //  size_t sum = 0;
  /*
    After decoding by zlib, decoded data is chunked (size: ZLIB_CHUNK).
    input_format.write_queue_vec is an array of the chunks.    
   */
  for (int i = 0; i < input_format.write_queue_vec.size(); i++) {
    memcpy(decompressed_record, input_format.write_queue_vec[i], input_format.write_size_queue_vec[i]);
    decompressed_record += input_format.write_size_queue_vec[i];
    //    sum += input_format.write_size_queue_vec[i];
  }
  //  REMPI_DBG("total size: %lu, total size: %lu", input_format.decompressed_size, sum);


  int test_id = 0;
  while (decoding_address < decompressed_record) {
    rempi_encoder_input_format_test_table *test_table;

    test_table = new rempi_encoder_input_format_test_table();
    input_format.test_tables_map[test_id++] = test_table;

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
	test_table->epoch_rank_vec.push_back(epoch_rank[i]);
	test_table->epoch_clock_vec.push_back(epoch_clock[i]);
	/*=== Decode recv_ranks for minimal clock fetching  ===*/
	mc_recv_ranks_uset.insert((int)(epoch_rank[i]));
	//REMPI_DBGI(REMPI_DBG_REPLAY, "->> %d (%lu)", (int)(*epoch_rank), test_table->epoch_size);
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
    /*----------- event ------------*/
    test_table->compressed_matched_events          = decoding_address;
    /*   --  id  --  */
    {
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
      cdc_prepare_decode_indices(test_table->count,
				 test_table->matched_events_id_vec,
				 test_table->matched_events_delay_vec,
				 test_table->matched_events_square_sizes_vec,
				 test_table->matched_events_permutated_indices_vec);
    }

    
    /* ======== Several initialization of test_table for replay ==========*/
    for (int i = 0, n = test_table->epoch_rank_vec.size(); i < n ; i++) {
      int rank = test_table->epoch_rank_vec[i];
      test_table->pending_event_count_umap[rank] = 0;
      test_table->current_epoch_line_umap[rank] = 0;
    }
    /* ===================================================================*/


#if 0
    {
      int *a, *b;
      a  = (int*)test_table->compressed_matched_events;
      b  = (int*)test_table->compressed_matched_events;
      b +=  test_table->compressed_matched_events_size /sizeof(int)/  2;
      for (int i = 0; i < test_table->compressed_matched_events_size / sizeof(int) / 2; i++) {
	REMPI_DBG("--> %d %d", a[i], b[i]);
      }
    }
#endif

    /*   ------------ */
    decoding_address += test_table->compressed_matched_events_size;

    /*===============================*/
  } /* -- End of while */



  /*=== Decode recv_ranks for minimal clock fetching  ===*/
  {

    int index = 0;
    input_format.mc_length          = mc_recv_ranks_uset.size();
    input_format.mc_recv_ranks      =    (int*)rempi_malloc(sizeof(int) * input_format.mc_length);
    input_format.mc_next_clocks     = (size_t*)rempi_malloc(sizeof(size_t) * input_format.mc_length);
    input_format.tmp_mc_next_clocks = (size_t*)rempi_malloc(sizeof(size_t) * input_format.mc_length);

#ifdef BGQ
    for (unordered_set<int>::const_iterator cit = mc_recv_ranks_uset.cbegin(),
	   cit_end = mc_recv_ranks_uset.cend();
	 cit != cit_end;
	 cit++) {
      int rank = *cit;
#else
    for (const int &rank: mc_recv_ranks_uset) {
#endif
      input_format.mc_recv_ranks [index]      = rank;
      input_format.mc_next_clocks[index]      = 0;
      input_format.tmp_mc_next_clocks[index]  = 0;
      this->solid_mc_next_clocks_umap[rank]  = 0;
#ifdef REMPI_DBG_REPLAY
      REMPI_DBGI(REMPI_DBG_REPLAY, "index %d: %d %d ", 
		 index, input_format.mc_recv_ranks[index], input_format.mc_next_clocks[index]);
#endif 

      index++;
    }

    this->mc_recv_ranks        = input_format.mc_recv_ranks;
    this->mc_next_clocks       = input_format.mc_next_clocks;
    this->tmp_mc_next_clocks   = input_format.tmp_mc_next_clocks;
    this->mc_length      = input_format.mc_length;
    /*array for waiting receive msg count for each test_id*/
    //    int    *tmp =  new int[input_format.test_tables_map.size()];;

    int    *tmp =  (int*)rempi_malloc(sizeof(int) * input_format.test_tables_map.size());
    memset(tmp, 0, sizeof(int) * input_format.test_tables_map.size());
    this->num_of_recv_msg_in_next_event = tmp;
    //    size_t *tmp2 = new size_t[input_format.test_tables_map.size()];;
    size_t *tmp2 = (size_t*)rempi_malloc(sizeof(size_t) *input_format.test_tables_map.size());
    memset(tmp2, 0, sizeof(size_t) * input_format.test_tables_map.size());
    this->dequeued_count = tmp2;
    size_t *tmp3 = (size_t*)rempi_malloc(sizeof(size_t) *input_format.test_tables_map.size());
    memset(tmp3, 0, sizeof(size_t) * input_format.test_tables_map.size());
    this->interim_min_clock_in_next_event = tmp3;
  }

  
  /*=====================================================*/

  if (decoding_address != decompressed_record) {
    REMPI_ERR("Inconsistent size");
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
						   vector<int> &matched_events_square_sizes_vec,
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
#ifdef BGQ
  for (vector<size_t>::const_iterator cit = matched_events_id_vec.cbegin(),
	 cit_end = matched_events_id_vec.cend();
       cit != cit_end;
       cit++) {
    size_t id = *cit;
#else
  for (size_t &id: matched_events_id_vec) {
#endif
    advance(it, (int)(id) - last_id);
    permutated_indices_it_map[id] = it;
    last_id = id;
    //    REMPI_DBG(" -- id: %d (= %d) -- size:%lu", id, *it, permutated_indices_list.size());
    //    REMPI_DBGI(3, " -- id: %d (= %d) --", *it, permutated_indices_it_map.size());
  }  
  
  // for (int i = 0; i < matched_events_id_vec.size(); ++i) {
  //   REMPI_DBGI(3, "matched id: %d (= %d), delay: %d", *permutated_indices_it_map[matched_events_id_vec[i]], matched_events_id_vec[i], matched_events_delay_vec[i]);
  // }
  
  // for (auto &v: permutated_indices_it_map) {
  //   REMPI_DBGI(3, " -- id: %d -- size: %d", v.first, permutated_indices_it_map.size());
  // }

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
#ifdef BGQ
  for (list<int>::const_iterator cit = permutated_indices_list.cbegin(),
	 cit_end = permutated_indices_list.cend();
       cit != cit_end;
       cit++) {
    int v = *cit;
#else
  for (int &v: permutated_indices_list) {
#endif
    matched_events_permutated_indices_vec[v] = sequence++;
  }
  
  /* == square_sizes_indices == */
  int init_square_size = 0;
  int max = 0;
  matched_events_square_sizes_vec.push_back(init_square_size);
  for (int i = 0, n = matched_events_permutated_indices_vec.size(); i < n; i++) {
    matched_events_square_sizes_vec.back()++;
    if (max < matched_events_permutated_indices_vec[i]) {
      max = matched_events_permutated_indices_vec[i];
    }
    if (i == max) {
      matched_events_square_sizes_vec.push_back(init_square_size);
    }
  }
  if (matched_events_square_sizes_vec.back() == 0) {
    matched_events_square_sizes_vec.pop_back();
  }
  return;
    
}



bool rempi_encoder_cdc::cdc_decode_ordering(rempi_event_list<rempi_event*> &recording_events, vector<rempi_event*> &event_vec, rempi_encoder_input_format_test_table* test_table, list<rempi_event*> &replay_event_list, int test_id)
{
  int    local_min_id_rank  = -1;
  size_t local_min_id_clock =  0;
  vector<rempi_event*> replay_event_vec;
#ifdef REMPI_DBG_REPLAY
  bool is_ordered_event_list_updated = false;
  bool is_solid_ordered_event_list_updated = false;
    
#endif

  if (replay_event_list.size() != 0) {
    REMPI_ERR("replay_event_list is not empty, and the replaying events are not passed to replaying_events");
  }  

#ifdef REMPI_DBG_REPLAY
  //  REMPI_DBGI(REMPI_DBG_REPLAY, "unamtched size: %d, replayed count: %d, count %d", test_table->unmatched_events_umap.size(), test_table->replayed_matched_event_index, test_table->count);
#endif


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

    replay_event_list.push_back(unmatched_event);
    test_table->unmatched_events_umap.erase(test_table->replayed_matched_event_index);
    return test_table->is_decoded_all();
  }




  /* ===== replaying events is matched event from here =======*/
  this->compute_local_min_id(test_table, &local_min_id_rank, &local_min_id_clock);
  
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
      CDC events flow:
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
  if (!event_vec.empty()) {

    for (int i = 0, n = event_vec.size(); i < n; i++) {
      rempi_event *event;
      event = event_vec[i];
      test_table->ordered_event_list.push_back(event);
      /*2. update pending_msg_count_umap, epoch_line_umap */
      test_table->pending_event_count_umap[event->get_source()]++; /*TODO: this umap is not used*/
      test_table->current_epoch_line_umap[event->get_source()] = event->get_clock();
#ifdef REMPI_DBG_REPLAY
      REMPI_DBGI(REMPI_DBG_REPLAY, "RCQ -> PQ ; (count: %d, with_next: %d, flag: %d, source: %d, clock: %d): list size: %d",
		 event_vec[i]->get_event_counts(), event_vec[i]->get_is_testsome(), event_vec[i]->get_flag(),
		 event_vec[i]->get_source(),  event_vec[i]->get_clock(), 
		 test_table->ordered_event_list.size());
      is_ordered_event_list_updated = true;
#endif
    }
    event_vec.clear();
  }
  /* ====== End of Operation A  ========*/


  /*Sort observed list, and create totally-ordered list*/
  test_table->ordered_event_list.sort(compare);    

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
#ifdef BGQ
      for (list<rempi_event*>::const_iterator cit = test_table->ordered_event_list.cbegin(),
	     cit_end = test_table->ordered_event_list.cend();
	   cit != cit_end;
	   cit++) {
	rempi_event *event = *cit;
#else
      for (rempi_event *event:test_table->ordered_event_list) {
#endif
	bool is_reached_epoch_line = test_table->is_reached_epoch_line();
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
      main thread may enqueue events(A), and increment <local_min_id>, then solid order become wrong order 
      because moving events to solid_ordered_event_list based on <local_min_id>, 
      but event A is not included in ordered_event_list at that time.
      So making suare that  main thread did not enqueue any events*/


    if (recording_events.size_replay(test_id) == 0) {       
      /*Move solid events to solid_ordered_event_list*/
      int next_clock_order = test_table->solid_ordered_event_list.size() + test_table->replayed_matched_event_index;
      for (int i = 0; i < solid_event_count; ++i) {
	/*ordered_event_list ====> ordered_event_chunk_list */
	rempi_event *event = test_table->ordered_event_list.front();
	if (test_table->solid_ordered_event_list.size() > 0) {
	  if (!compare(test_table->solid_ordered_event_list.back(), event)) {
	    REMPI_ERR("Enqueuing an event (rank: %d, clock:%lu) which is smaller than (rank: %d, clock: %lu)",
		      event->get_source(), event->get_clock(),
		      test_table->solid_ordered_event_list.back()->get_source(),
		      test_table->solid_ordered_event_list.back()->get_clock());
	  }
	}
	test_table->      ordered_event_list.pop_front();
	test_table->solid_ordered_event_list.push_back(event);
	event->clock_order = next_clock_order++;
      }
    }



//       REMPI_DBGI(0, "LIST Queue Update: Local_min (rank: %d, clock: %lu): count: %d, test_id: %d",
// 		 local_min_id_rank, local_min_id_clock, solid_event_count, test_id);
// #ifdef BGQ
//       for (list<rempi_event*>::const_iterator it = test_table->ordered_event_list.cbegin(), 
// 	     it_end = test_table->ordered_event_list.cend();
// 	   it !=it_end;
// 	   it++) {
// 	rempi_event *e = *it;
// #else
//       for (rempi_event *e: test_table->ordered_event_list) {
// #endif     
// 	REMPI_DBGI(0, "       list (rank: %d, clock: %lu): count: %d", e->get_source(), e->get_clock(), solid_event_count);
//       }

// #ifdef BGQ
//       for (list<rempi_event*>::const_iterator it = test_table->solid_ordered_event_list.cbegin(), 
// 	     it_end = test_table->solid_ordered_event_list.cend();
// 	   it !=it_end;
// 	   it++) {
// 	rempi_event *e = *it;
// #else
//       for (rempi_event *e: test_table->solid_ordered_event_list) {
// #endif
// 	REMPI_DBGI(0, "      slist (rank: %d, clock: %lu, order: %lu): count: %d", 
// 		   e->get_source(), e->get_clock(), e->clock_order, solid_event_count);
//       }


    
#ifdef REMPI_DBG_REPLAY
    if (is_ordered_event_list_updated || is_solid_ordered_event_list_updated) {
      REMPI_DBGI(REMPI_DBG_REPLAY, "LIST Queue Update: Local_min (rank: %d, clock: %lu): count: %d, test_id: %d",
		 local_min_id_rank, local_min_id_clock, solid_event_count, test_id);
#ifdef BGQ
      for (list<rempi_event*>::const_iterator it = test_table->ordered_event_list.cbegin(), 
	     it_end = test_table->ordered_event_list.cend();
	   it !=it_end;
	   it++) {
	rempi_event *e = *it;
#else
      for (rempi_event *e: test_table->ordered_event_list) {
#endif     
	REMPI_DBGI(REMPI_DBG_REPLAY, "       list (rank: %d, clock: %lu): count: %d", e->get_source(), e->get_clock(), solid_event_count);
      }

#ifdef BGQ
      for (list<rempi_event*>::const_iterator it = test_table->solid_ordered_event_list.cbegin(), 
	     it_end = test_table->solid_ordered_event_list.cend();
	   it !=it_end;
	   it++) {
	rempi_event *e = *it;
#else
      for (rempi_event *e: test_table->solid_ordered_event_list) {
#endif
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

#if 0
//   int i = 0;
//   int added_count = 0;
//   for (rempi_event *replaying_event: test_table->solid_ordered_event_list) {
//     int permutated_index = test_table->matched_events_permutated_indices_vec[replaying_event->clock_order]
//       - test_table->replayed_matched_event_index;
// #ifdef REMPI_DBG_REPLAY
//     if (is_ordered_event_list_updated || is_solid_ordered_event_list_updated) {
//       REMPI_DBGI(REMPI_DBG_REPLAY, "  index: %d -> %d(%d) replayed_count: %d (event_clock: %d)", replaying_event->clock_order, 
// 		 test_table->matched_events_permutated_indices_vec[replaying_event->clock_order], 
// 		 permutated_index, test_table->replayed_matched_event_index, replaying_event->get_clock());
//     }
// #endif
//     if (0 <= permutated_index && permutated_index < outcount) {
//       replay_event_vec[permutated_index] = replaying_event;
//       added_count++;
//     } else if (permutated_index < 0){
//       REMPI_ERR("permutated_index:%d < 0 (record data may be truncated)", permutated_index);
//     }
//     if (i = outcount) {
//       /*
// 	I'my replaying outcount of receive events. 
// 	solid_ordered_event_list[outcount-1] <= max of clocks in the replaying receive events 
// 	at least, the clock becomes solid_ordered_event_list[outcount-1] (+1 in actual)
// 	So I set replaying_event->get_clock() (=solid_ordered_event_list[outcount-1])
//       */
//       interim_min_clock_in_next_event[test_id] = (size_t)replaying_event->get_clock();
//     }
//     i++;
//   }

#else

  int added_count = 0;
#ifdef BGQ
  for (list<rempi_event*>::const_iterator cit = test_table->solid_ordered_event_list.cbegin(),
	 cit_end = test_table->solid_ordered_event_list.cend();
       cit != cit_end;
       cit++) {
    rempi_event *replaying_event = *cit;
#else
  for (rempi_event *replaying_event: test_table->solid_ordered_event_list) {
#endif

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

  this->compute_local_min_id(test_table, &local_min_id_rank, &local_min_id_clock); /*Update again*/
  /*When calling compute_local_min_id, there may be an event in recording_events. 
    If this is the case, the event may have smaller clocl than local_min_id_clock, and 
    the tmp_interim_min_clock become bigger than correct value, and is wrong.
    So update tmp_interim_min_clock only when recording_events is empty even after 
    calling compute_local_min_id.
  */

  list<rempi_event*>::const_iterator cit     = test_table->ordered_event_list.cbegin();
  list<rempi_event*>::const_iterator cit_end = test_table->ordered_event_list.cend();
  if (recording_events.size_replay(test_id) == 0 && local_min_id_clock != PNMPI_MODULE_CLMPI_COLLECTIVE) {
    clmpi_get_local_sent_clock(&tmp_interim_min_clock);
    /*local_sent_clock is sent clock value, so the local_clock is local_sent_clock + 1*/
    //    tmp_interim_min_clock++;
#ifdef BGQ
    for (vector<rempi_event*>::const_iterator cit_replay_event = replay_event_vec.cbegin(),
	   cit_replay_event_end = replay_event_vec.cend();
	 cit_replay_event != cit_replay_event_end;
	 cit_replay_event++) {
      rempi_event *replaying_event = *cit_replay_event;
#else    
    for (rempi_event *replaying_event: replay_event_vec) {
#endif
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
    if (interim_min_clock_in_next_event[test_id] < tmp_interim_min_clock) {
      cit     = test_table->ordered_event_list.cbegin();
      cit_end = test_table->ordered_event_list.cend();
      size_t local_clock_dbg;
      clmpi_get_local_sent_clock(&local_clock_dbg);
      REMPI_DBGI(REMPI_DBG_REPLAY, "INTRM update: local_clock: %lu", local_clock_dbg);
      for (rempi_event *replaying_event: replay_event_vec) {
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
      REMPI_DBGI(REMPI_DBG_REPLAY, "INTRM update: interim: %lu => %lu (recv_test_id: %d)", interim_min_clock_in_next_event[test_id], tmp_interim_min_clock, test_id);
    }

#endif
    interim_min_clock_in_next_event[test_id] = tmp_interim_min_clock;
#endif
  }

  /*Check if all replaying events occured, 
    and are added to replay_event_vec*/
  if (added_count != outcount) {
    /*If all replaying is not present, i.e., need more messages to replay, 
      then, revoke(clear) this incomplete replay_event_vec  */
    replay_event_vec.clear();
#ifdef REMPI_DBG_REPLAY
    //    REMPI_DBGI(REMPI_DBG_REPLAY, "abort !! outcount: %d, but added_count: %d", outcount, added_count);
#endif
    int ret = test_table->is_decoded_all();
    if (ret == true) {
      REMPI_ERR("Inconsistent Replayed events and Recorded events");
    }

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

    /* =========================================================*/
    return ret;
  } else {
    /* ==== Condition for (1) ===================================*/
    /*At this point, now, the recv msg will be replayed, 
      and we do not know what the next event is now, so set 0*/
    num_of_recv_msg_in_next_event[test_id] = 0;
    /* =========================================================*/
  }




  /* Put with_next value  */
  for (int i = 0, n = replay_event_vec.size(); i < n; i++) {
// #ifdef REMPI_DBG_REPLAY
//     REMPI_DBGI(REMPI_DBG_REPLAY, "== size: %d, index: %d, added_count: %d, outcount: %d", replay_event_vec.size(), i, added_count, outcount);
// #endif
    replay_event_vec[i]->set_with_next(1);
  }
  replay_event_vec.back()->set_with_next(0);


  for (int i = 0, n = replay_event_vec.size(); i < n; i++) {
    test_table->pending_event_count_umap[replay_event_vec[i]->get_source()]--; /*TODo: this umap is not used*/
    replay_event_list.push_back(replay_event_vec[i]);
    test_table->solid_ordered_event_list.remove(replay_event_vec[i]); /*This ".remove" may be slow*/
  }

#if 1
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
		    recording_events.size_replay(test_id), test_id);
#ifdef BGQ
	  for (list<rempi_event*>::const_iterator cit = test_table->ordered_event_list.cbegin(),
		 cit_end = test_table->ordered_event_list.cend();
	       cit != cit_end;
	       cit++) {
	    rempi_event *e = *cit;
#else
	  for (rempi_event *e: test_table->ordered_event_list) {
#endif
	    //	    REMPI_DBG("       list (rank: %d, clock: %lu): count: %d", e->get_source(), e->get_clock(), solid_event_count);
	    REMPI_DBG("== Wrong       list (rank: %d, clock: %lu)", e->get_source(), e->get_clock());
	  }
#ifdef BGQ
	  for (list<rempi_event*>::const_iterator cit = test_table->solid_ordered_event_list.cbegin(),
		 cit_end = test_table->solid_ordered_event_list.cend();
	       cit != cit_end;
	       cit++) {
	    rempi_event *e = *cit;
#else
	  for (rempi_event *e: test_table->solid_ordered_event_list) {
#endif

	    //	    REMPI_DBG("      slist (rank: %d, clock: %lu): count: %d", e->get_source(), e->get_clock(), solid_event_count);
	    REMPI_DBG("== Wrong      slist (rank: %d, clock: %lu, order: %lu)", e->get_source(), e->get_clock(), e->clock_order);
	  }
	  REMPI_DBG("== Wrong replayed_matched_event_index: %d", test_table->replayed_matched_event_index);
#ifdef BGQ
	  for (vector<rempi_event*>::const_iterator cit = replay_event_vec.cbegin(),
		 cit_end = replay_event_vec.cend();
	       cit != cit_end;
	       cit++) {
	    rempi_event *replaying_event = *cit;
#else
	  for (rempi_event *replaying_event: replay_event_vec) {
#endif

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
  
#ifdef REMPI_DBG_REPLAY
  for (int i = 0, n = replay_event_vec.size(); i < n; i++) {
    REMPI_DBGI(REMPI_DBG_REPLAY, "Final   RPQv ; (count: %d, with_next: %d, flag: %d, source: %d, clock: %d)",
	       replay_event_vec[i]->get_event_counts(), replay_event_vec[i]->get_is_testsome(), replay_event_vec[i]->get_flag(),
	       replay_event_vec[i]->get_source(), replay_event_vec[i]->get_clock());
  }
#ifdef REMPI_DBG_REPLAY
  REMPI_DBGI(REMPI_DBG_REPLAY, "LIST Queue Update: Local_min (rank: %d, clock: %lu): test_id: %d",
	     local_min_id_rank, local_min_id_clock, test_id);
  for (rempi_event *e: test_table->ordered_event_list) {
    REMPI_DBGI(REMPI_DBG_REPLAY, "       list (rank: %d, clock: %lu)", e->get_source(), e->get_clock());
  }
  for (rempi_event *e: test_table->solid_ordered_event_list) {
    REMPI_DBGI(REMPI_DBG_REPLAY, "      slist (rank: %d, clock: %lu, order: %lu)", e->get_source(), e->get_clock(), e->clock_order);
  }
#endif   
#endif



  return test_table->is_decoded_all();
}


void rempi_encoder_cdc::update_fd_next_clock(
					     int is_waiting_recv, 
					     int num_of_recv_msg_in_next_event, 
					     size_t interim_min_clock_in_next_event, 
					     size_t enqueued_count,
					     int recv_test_id,
					     int is_after_reve_event)
{
  size_t local_clock;
  size_t next_clock = 0;



  bool is_waiting_msg = false;
  /*If waiting message recv to replay an event, the sending clock (next_clock) is bigger than "clock" by +1 */
  /*
    If is_waiting_recv == true, it means next events are matched recv event. 
    If is_waiting_msg == true,  it means next events are matched recv event, 
      and the events is triggered by a message which it have not received
   */
  if (is_waiting_recv) {
#if 0
    is_waiting_msg = (enqueued_count == dequeued_count[recv_test_id]);
    if (is_waiting_msg) {
      next_clock = global_local_min_id.clock;
    } else {
      /*min*/
      next_clock = (interim_min_clock_in_next_event < global_local_min_id.clock)? interim_min_clock_in_next_event:global_local_min_id.clock;
    }
#else
    next_clock = interim_min_clock_in_next_event;
#endif
    /*max*/
    //    next_clock = (clock < next_clock)? next_clock:clock;
    //    next_clock += 1; //next_clock += num_of_recv_msg_in_next_event; => this is wrong, must be +1
  } else {
    //    next_clock = sent_clock + 1;
    if (is_after_reve_event) {
      clmpi_get_local_clock(&local_clock);
    } else {
      clmpi_get_local_sent_clock(&local_clock);
      /*local_sent_clock is literary sent clock, so next clock must be over the sent clock, i.e., local_sent_clock + 1 at least*/
      local_clock++;
    }
    next_clock = local_clock;
  }

#ifdef REMPI_DBG_REPLAY  
  if (fd_clocks->next_clock < next_clock) {
    // REMPI_DBGI(REMPI_DBG_REPLAY, "NEXT Clock Update(%lu): from %d to %d:  (local_min (rank: %d, clock: %lu), local: %d num: %d): is_waiting_recv: %d, is_after_recv_event: %d", MPI_Wtime(),
    // 	       fd_clocks->next_clock, next_clock, global_local_min_id.rank, global_local_min_id.clock, 
    // 	       clock, num_of_recv_msg_in_next_event, is_waiting_recv, is_after_reve_event);
  }
#endif

  if (fd_clocks->next_clock < next_clock) {
    fd_clocks->next_clock = next_clock;
  }
  return;
}

void rempi_encoder_cdc::set_fd_clock_state(int flag)
{
  if (flag) {
    tmp_fd_next_clock = fd_clocks->next_clock;
    fd_clocks->next_clock = PNMPI_MODULE_CLMPI_COLLECTIVE;
  } else {
    fd_clocks->next_clock = tmp_fd_next_clock;
  }
  return;  
}



void rempi_encoder_cdc::insert_encoder_input_format_chunk(rempi_event_list<rempi_event*> &recording_events, rempi_event_list<rempi_event*> &replaying_events, rempi_encoder_input_format &input_format)
{
  // for (map<int, rempi_encoder_input_format_test_table*>::iterator test_table_it = input_format.test_table_map.begin();
  //      test_table_it != input_format.test_table.end(); test_table_it++) {
  //   int test_id = test_table_it->first;
  //   rempi_encoder_input_format_test_table *test_table = test_table->second;
  // }
  unordered_map<int, vector<rempi_event*>*> matched_events_vec_umap;
  list<rempi_event*> replay_event_list;
  bool is_finished = false;
  int finished_testsome_count  = 0;



  for (int i = 0, n = input_format.test_tables_map.size(); i < n; i++) {
    matched_events_vec_umap[i] = new vector<rempi_event*>();
    /*recorded events are pushed into this vector, and permutated, and outputed*/
  }

  /*all of recording_events queue (# is sleep_counter), this thread sleeps a little*/
  int sleep_counter = input_format.test_tables_map.size(); 
  for (int test_id = 0, n = input_format.test_tables_map.size(); 
       finished_testsome_count < n; 
       test_id = ++test_id % n) {

    /*TODO: For now, periodically update next_clock here. 
        this function call is not neseccory to be here, better location may exist */
    //    update_fd_next_clock(0); => Kento moved to a line just after clmpi_sync_clock in rempi_recorder_cdc
    if (matched_events_vec_umap[test_id] == NULL) {
      /*cdc_decode_ordering returns true, and Nothing to relay any more for this vectors*/
      continue;
    }

    if (recording_events.front_replay(test_id) != NULL) {      
      /*If a message(event) arrives,  */
      rempi_event *matched_event;
      int event_list_status;
      matched_event = recording_events.dequeue_replay(test_id, event_list_status);
      matched_events_vec_umap[test_id]->push_back(matched_event);

#ifdef REMPI_DBG_REPLAY
      //    REMPI_DBGI(REMPI_DBG_REPLAY, "===== checkpoint test_id: %d: %lu", test_id, matched_events_vec_umap[test_id]->size());
#endif

    } else {
      sleep_counter--;
      if (sleep_counter <= 0) {
	usleep(1);
	sleep_counter = input_format.test_tables_map.size();
      }      
    }


    /*Permutate "*matched_events_vec_umap"
      based on "input_format.test_tables_map", then
      output the replayed event into "replay_event_list"*/

    is_finished = cdc_decode_ordering(recording_events, *matched_events_vec_umap[test_id], input_format.test_tables_map[test_id], replay_event_list, test_id);

    if (is_finished) {
      finished_testsome_count++;
      is_finished = false;
      delete matched_events_vec_umap[test_id];
      matched_events_vec_umap[test_id] = NULL;
    }

#ifdef BGQ
    for (list<rempi_event*>::iterator it = replay_event_list.begin(),
	   it_end = replay_event_list.end();
	 it != it_end;
	 it++) {
      rempi_event *e = *it;
#else
    for (rempi_event *e: replay_event_list) {
#endif

      // REMPI_DBGI(0, "RPQv -> RPQ ; (count: %d, with_next: %d, flag: %d, source: %d, clock: %d) recv_test_id: %d",
      // 		 e->get_event_counts(), e->get_with_next(), e->get_flag(),
      // 		 e->get_source(), e->get_clock(), test_id);

#ifdef REMPI_DBG_REPLAY
      REMPI_DBGI(REMPI_DBG_REPLAY, "RPQv -> RPQ ; (count: %d, with_next: %d, flag: %d, source: %d, clock: %d) recv_test_id: %d",
		 e->get_event_counts(), e->get_is_testsome(), e->get_flag(),
		 e->get_source(), e->get_clock(), test_id);
#endif
      replaying_events.enqueue_replay(e, test_id);
    }
    replay_event_list.clear();
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


