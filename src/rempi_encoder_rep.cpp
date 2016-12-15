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
#include "rempi_clock.h"



/* ==================================== */
/*      CLASS rempi_encoder_rep         */
/* ==================================== */
void rempi_encoder_rep::compress_non_matched_events(rempi_encoder_input_format_test_table *test_table)
{
  char *write_buff;
  size_t write_size;
  

  /*=== matching set id === */
  input_format->write_queue_vec.push_back((char*)&test_table->matching_set_id);
  input_format->write_size_queue_vec.push_back(sizeof(test_table->matching_set_id));
  /*=======================*/

  /*=== message count ===*/
  input_format->write_queue_vec.push_back((char*)&test_table->count);
  input_format->write_size_queue_vec.push_back(sizeof(test_table->count));
  /*=====================*/

  /*=== Compress Epoch   ===*/
  /* (0) size*/
  if (test_table->epoch_rank_vec.size() != test_table->epoch_clock_vec.size()) {
    REMPI_ERR("Epoch size is inconsistent");
  }
#if 0
  test_table->epoch_size = 0;
  input_format->write_queue_vec.push_back((char*)&test_table->epoch_size);
  input_format->write_size_queue_vec.push_back(sizeof(test_table->epoch_size));
#else
  test_table->epoch_size = test_table->epoch_rank_vec.size() * sizeof(test_table->epoch_rank_vec[0]);
  input_format->write_queue_vec.push_back((char*)&test_table->epoch_size);
  input_format->write_size_queue_vec.push_back(sizeof(test_table->epoch_size));
  /* (1) rank*/
  input_format->write_queue_vec.push_back((char*)&test_table->epoch_rank_vec[0]);
  input_format->write_size_queue_vec.push_back(test_table->epoch_size);
  /* (2) clock*/
  input_format->write_queue_vec.push_back((char*)&test_table->epoch_clock_vec[0]);
  input_format->write_size_queue_vec.push_back(test_table->epoch_size);
#endif
  /*====================================*/

  /*=== Compress with_next ===*/
  test_table->compressed_with_previous_length = 0;
  test_table->compressed_with_previous_size   = 0;
  test_table->compressed_with_previous        = NULL;
  input_format->write_queue_vec.push_back((char*)&test_table->compressed_with_previous_length);
  input_format->write_size_queue_vec.push_back(sizeof(test_table->compressed_with_previous_length));
  input_format->write_queue_vec.push_back((char*)&test_table->compressed_with_previous_size);
  input_format->write_size_queue_vec.push_back(sizeof(test_table->compressed_with_previous_size));
  /*====================================*/

  /*=== Compress index ===*/
  test_table->compressed_matched_index_length = 0;
  test_table->compressed_matched_index_size   = 0;
  test_table->compressed_matched_index        = NULL;
  input_format->write_queue_vec.push_back((char*)&test_table->compressed_matched_index_length);
  input_format->write_size_queue_vec.push_back(sizeof(test_table->compressed_matched_index_length));
  input_format->write_queue_vec.push_back((char*)&test_table->compressed_matched_index_size);
  input_format->write_size_queue_vec.push_back(sizeof(test_table->compressed_matched_index_size));
  // input_format->write_queue_vec.push_back((char*)test_table->compressed_matched_index);
  // input_format->write_size_queue_vec.push_back(test_table->compressed_matched_index_size);


  /*=== Compress unmatched_events ===*/
  /* (1) id */
  test_table->compressed_unmatched_events_id        = NULL;
  test_table->compressed_unmatched_events_id_size   = 0;
  input_format->write_queue_vec.push_back((char*)&test_table->compressed_unmatched_events_id_size);
  input_format->write_size_queue_vec.push_back(sizeof(test_table->compressed_unmatched_events_id_size));
  //  input_format->write_queue_vec.push_back(test_table->compressed_unmatched_events_id);
  //  input_format->write_size_queue_vec.push_back(test_table->compressed_unmatched_events_id_size);
  /* (2) count */
  /*   count will be totally random number, so this function does not do zlib*/
  test_table->compressed_unmatched_events_count        = NULL;
  test_table->compressed_unmatched_events_count_size   = 0;
  input_format->write_queue_vec.push_back((char*)&test_table->compressed_unmatched_events_count_size);
  input_format->write_size_queue_vec.push_back(sizeof(test_table->compressed_unmatched_events_count));
  //  input_format->write_queue_vec.push_back(test_table->compressed_unmatched_events_count);
  //  input_format->write_size_queue_vec.push_back(test_table->compressed_unmatched_events_count_size);
  /*=======================================*/
  return;
}


void rempi_encoder_rep::compress_matched_events(rempi_encoder_input_format_test_table *test_table)
{
  size_t compressed_size,  original_size;
  test_table->compressed_matched_events_size           = 0;
  input_format->write_queue_vec.push_back((char*)&test_table->compressed_matched_events_size);
  input_format->write_size_queue_vec.push_back(sizeof(test_table->compressed_matched_events_size));

  return;
}

// bool rempi_encoder_rep::extract_encoder_input_format_chunk(rempi_event_list<rempi_event*> &events, rempi_encoder_input_format *input_format)
// {
//   rempi_event *event_dequeued = NULL;
//   bool is_ready_for_encoding = false;
//   rempi_test

//   this->input_format = input_format;

//   while (1) {
//     /*Append events to current check as many as possible*/
//     if (events.front() == NULL) {
//       break;
//     } else if (input_format->length() > rempi_max_event_length) {
//       if (input_format->last_added_event->get_with_next() == REMPI_MPI_EVENT_NOT_WITH_NEXT) break;
//     }
//     event_dequeued = events.pop();

//     if (event_dequeued->get_msg_id() !=  REMPI_MPI_EVENT_INPUT_IGNORE) {
//       rempi_encoder_input_format_test_table *test_table;
//       input_format->test_tables_map.at(recv_test_id);
//       test_table = input_format->event_dequeued->get_matching_group_id());
      
//       if (test_table->epoch_umap.find(sorted_events_vec[i]->get_source()) == test_table->epoch_umap.end()) {
//         test_table->epoch_umap[sorted_events_vec[i]->get_source()] = sorted_events_vec[i]->get_clock();
//       } else {
//         if (test_table->epoch_umap[sorted_events_vec[i]->get_source()] >= sorted_events_vec[i]->get_clock()) {
//           REMPI_ERR("Later message has smaller clock than earlier message: epoch line clock of rank:%d clock:%d but clock:%d",
//                     sorted_events_vec[i]->get_source(),
//                     test_table->epoch_umap[sorted_events_vec[i]->get_source()],sorted_events_vec[i]->get_clock()
//                     );
//         }
//         test_table->epoch_umap[sorted_events_vec[i]->get_source()] = sorted_events_vec[i]->get_clock();
//       }

//       rempi_encoder_input_format_test_table::all_epoch_rank_uset.insert(event_dequeued->get_source());
//     }
//     delete event_dequeued;
//   }

//   if (events.is_push_closed_())  {
//     is_ready_for_encoding = true;
//   } else if (input_format->length() > rempi_max_event_length) {
//     if (input_format->last_added_event->get_with_next() == REMPI_MPI_EVENT_NOT_WITH_NEXT) {
//       is_ready_for_encoding = true;
//     }
//   }
//   return is_ready_for_encoding; /*true*/
// }



void rempi_encoder_rep::update_local_look_ahead_send_clock(
                                                           int replaying_event_type,
                                                           int matching_set_id)
{
  size_t next_clock;
  if (replaying_event_type == REMPI_ENCODER_REPLAYING_TYPE_RECV) {
    rempi_clock_get_local_clock(&next_clock);
  } else if (replaying_event_type == REMPI_ENCODER_REPLAYING_TYPE_ANY){
    rempi_clock_get_local_clock(&next_clock);
  } else {
    REMPI_ERR("Unknow replaying_event_type: %d", replaying_event_type);
  }
  if (rempi_cp_get_scatter_clock() < next_clock) {
    rempi_cp_set_scatter_clock(next_clock);
  }
  
  return;
}




void rempi_encoder_rep::add_to_ordered_list(list<rempi_event*> *event_list)
{

}

bool rempi_encoder_rep::cdc_decode_ordering(rempi_event_list<rempi_event*> *recording_events, list<rempi_event*> *event_list, rempi_encoder_input_format_test_table* test_table, list<rempi_event*> *replay_event_list, int test_id, int local_min_id_rank, size_t local_min_id_clock)
{
  bool is_reached_epoch_line;
#ifdef REMPI_DBG_REPLAY
  bool is_ordered_event_list_updated = false;
  bool is_solid_ordered_event_list_updated = false;
    
#endif

  if (replay_event_list->size() != 0) {
    REMPI_ERR("replay_event_list is not empty, and the replaying events are not passed to replaying_events");
  }


  //  REMPI_DBG("or size: %lu", event_list->size());
  if (!event_list->empty()) {
    //    REMPI_DBG("or asize: %lu", event_list->size());
    for (list<rempi_event*>::iterator it = event_list->begin(),
	   it_end = event_list->end();
	 it != it_end; ) {
      rempi_event *event = *it;
      test_table->ordered_event_list.push_back(event);
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
  /*Sort observed list, and create totally-ordered list*/
  test_table->ordered_event_list.sort(compare);    



  { 
    /* ====== Operation B  ========*/
    /* Count how many events move from ordered_event_list to solid_ordered_event_list */
    int solid_event_count = 0;
    if (local_min_id_clock == REMPI_CLOCK_COLLECTIVE_CLOCK) {
      /*Other ranks (except me) are in collective*/
      solid_event_count = test_table->ordered_event_list.size();
    } else {
      /*Count solid event count 
	Sorted "solid events" order does not change by the rest of events
      */
      for (list<rempi_event*>::const_iterator cit = test_table->ordered_event_list.cbegin(),
	     cit_end = test_table->ordered_event_list.cend();
	   cit != cit_end;
	   cit++) {
	rempi_event *event = *cit;
	if (!compare2(local_min_id_rank, local_min_id_clock, event)) {
	  solid_event_count++;
	} else {
	/*Because ordered_event_list is sorted, so we do not need to check the rest of events*/
	  break;
	}   
      }
    }

#ifdef REMPI_DBG_REPLAY
    if (solid_event_count > 0) {
      is_solid_ordered_event_list_updated = true;
    }
#endif




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


  int added_count = 0;
  for (list<rempi_event*>::const_iterator cit = test_table->solid_ordered_event_list.cbegin(),
	 cit_end = test_table->solid_ordered_event_list.cend();
       cit != cit_end;
       cit++) {
    rempi_event *replaying_event = *cit;
    replay_event_list->push_back(replaying_event);
#ifdef REMPI_DBG_REPLAY    
    REMPI_DBGI(REMPI_DBG_REPLAY, "Final   RPQv ; (count: %d, with_next: %d, flag: %d, source: %d, clock: %d)",
	       replaying_event->get_event_counts(), replaying_event->get_is_testsome(), replaying_event->get_flag(),
	       replaying_event->get_source(), replaying_event->get_clock());
#endif
  }
  test_table->solid_ordered_event_list.clear();
  

// #ifdef REMPI_DBG_REPLAY
//   if (test_table->ordered_event_list.size() > 0 || test_table->solid_ordered_event_list.size() > 0) {
//     REMPI_DBGI(REMPI_DBG_REPLAY, "LIST Queue Update: Local_min (rank: %d, clock: %lu): test_id: %d",
// 	       local_min_id_rank, local_min_id_clock, test_id);
//   }
//   for (list<rempi_event*>::iterator it = test_table->ordered_event_list.begin(), 
// 	 it_end = test_table->ordered_event_list.end();
//        it != it_end; it++) {
//     //  for (rempi_event *e: test_table->ordered_event_list) {
//     rempi_event *e = *it;
//     REMPI_DBGI(REMPI_DBG_REPLAY, "       list (rank: %d, clock: %lu)", e->get_source(), e->get_clock());
//   }
//   for (list<rempi_event*>::iterator it = test_table->solid_ordered_event_list.begin(), 
// 	 it_end = test_table->solid_ordered_event_list.end();
//        it != it_end; it++) {
//     //  for (rempi_event *e: test_table->solid_ordered_event_list) {
//     rempi_event *e = *it;

//     REMPI_DBGI(REMPI_DBG_REPLAY, "      slist (rank: %d, clock: %lu, order: %lu)", e->get_source(), e->get_clock(), e->clock_order);
//   }
// #endif
  return 0;
}

#endif //MPI_VERSION == 3
