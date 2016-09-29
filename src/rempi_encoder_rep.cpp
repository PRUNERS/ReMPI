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
#include "rempi_cp.h"



/* ==================================== */
/*      CLASS rempi_encoder_rep         */
/* ==================================== */



void rempi_encoder_rep::compress_non_matched_events(rempi_encoder_input_format &input_format, rempi_encoder_input_format_test_table *test_table)
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
  test_table->compressed_with_previous_length = 0;
  test_table->compressed_with_previous_size   = 0;
  test_table->compressed_with_previous        = NULL;
  input_format.write_queue_vec.push_back((char*)&test_table->compressed_with_previous_length);
  input_format.write_size_queue_vec.push_back(sizeof(test_table->compressed_with_previous_length));
  input_format.write_queue_vec.push_back((char*)&test_table->compressed_with_previous_size);
  input_format.write_size_queue_vec.push_back(sizeof(test_table->compressed_with_previous_size));
  /*====================================*/

  /*=== Compress unmatched_events ===*/
  /* (1) id */
  test_table->compressed_unmatched_events_id        = NULL;
  test_table->compressed_unmatched_events_id_size   = 0;
  input_format.write_queue_vec.push_back((char*)&test_table->compressed_unmatched_events_id_size);
  input_format.write_size_queue_vec.push_back(sizeof(test_table->compressed_unmatched_events_id_size));
  //  input_format.write_queue_vec.push_back(test_table->compressed_unmatched_events_id);
  //  input_format.write_size_queue_vec.push_back(test_table->compressed_unmatched_events_id_size);
  /* (2) count */
  /*   count will be totally random number, so this function does not do zlib*/
  test_table->compressed_unmatched_events_count        = NULL;
  test_table->compressed_unmatched_events_count_size   = 0;
  input_format.write_queue_vec.push_back((char*)&test_table->compressed_unmatched_events_count_size);
  input_format.write_size_queue_vec.push_back(sizeof(test_table->compressed_unmatched_events_count));
  //  input_format.write_queue_vec.push_back(test_table->compressed_unmatched_events_count);
  //  input_format.write_size_queue_vec.push_back(test_table->compressed_unmatched_events_count_size);
  /*=======================================*/
  return;
}


void rempi_encoder_rep::compress_matched_events(rempi_encoder_input_format &input_format, rempi_encoder_input_format_test_table *test_table)
{
  size_t compressed_size,  original_size;
  test_table->compressed_matched_events_size           = 0;
  input_format.write_queue_vec.push_back((char*)&test_table->compressed_matched_events_size);
  input_format.write_size_queue_vec.push_back(sizeof(test_table->compressed_matched_events_size));

  return;
}




bool rempi_encoder_rep::cdc_decode_ordering(rempi_event_list<rempi_event*> *recording_events, list<rempi_event*> *event_list, rempi_encoder_input_format_test_table* test_table, list<rempi_event*> *replay_event_list, int test_id, int local_min_id_rank, size_t local_min_id_clock)
{
  vector<rempi_event*> replay_event_vec;
#ifdef REMPI_DBG_REPLAY
  bool is_ordered_event_list_updated = false;
  bool is_solid_ordered_event_list_updated = false;
#endif

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
      //#ifdef BGQ
      for (list<rempi_event*>::const_iterator cit = test_table->ordered_event_list.cbegin(),
	     cit_end = test_table->ordered_event_list.cend();
	   cit != cit_end;
	   cit++) {
	rempi_event *event = *cit;
// #else
//       for (rempi_event *event:test_table->ordered_event_list) {
// #endif
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
      //#ifdef BGQ
      for (list<rempi_event*>::const_iterator it = test_table->ordered_event_list.cbegin(), 
	     it_end = test_table->ordered_event_list.cend();
	   it !=it_end;
	   it++) {
	rempi_event *e = *it;
// #else
//       for (rempi_event *e: test_table->ordered_event_list) {
// #endif     
	REMPI_DBGI(REMPI_DBG_REPLAY, "       list (rank: %d, clock: %lu): count: %d", e->get_source(), e->get_clock(), solid_event_count);
      }

      //#ifdef BGQ
      for (list<rempi_event*>::const_iterator it = test_table->solid_ordered_event_list.cbegin(), 
	     it_end = test_table->solid_ordered_event_list.cend();
	   it !=it_end;
	   it++) {
	rempi_event *e = *it;
// #else
//       for (rempi_event *e: test_table->solid_ordered_event_list) {
// #endif
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
  //#ifdef BGQ
  for (list<rempi_event*>::const_iterator cit = test_table->solid_ordered_event_list.cbegin(),
	 cit_end = test_table->solid_ordered_event_list.cend();
       cit != cit_end;
       cit++) {
    rempi_event *replaying_event = *cit;
// #else
//   for (rempi_event *replaying_event: test_table->solid_ordered_event_list) {
// #endif

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
    //#ifdef BGQ
    for (vector<rempi_event*>::const_iterator cit_replay_event = replay_event_vec.cbegin(),
	   cit_replay_event_end = replay_event_vec.cend();
	 cit_replay_event != cit_replay_event_end;
	 cit_replay_event++) {
      rempi_event *replaying_event = *cit_replay_event;
// #else    
//     for (rempi_event *replaying_event: replay_event_vec) {
// #endif
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

#endif //MPI_VERSION == 3


