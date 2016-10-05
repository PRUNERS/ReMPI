#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <vector>

#include "rempi_err.h"
#include "rempi_event.h"
#include "rempi_encoder.h"
#include "rempi_config.h"
#include "rempi_compression_util.h"


/* ==================================== */
/*      CLASS rempi_encoder_basic             */
/* ==================================== */



rempi_encoder_basic::rempi_encoder_basic(int mode)
  : rempi_encoder(mode)
{
  return;
}



void rempi_encoder_basic::open_record_file(string record_path)
{
  if (mode == REMPI_ENV_REMPI_MODE_RECORD) {
    record_fs.open(record_path.c_str(), ios::out);
  } else if (mode == REMPI_ENV_REMPI_MODE_REPLAY) {
    record_fs.open(record_path.c_str(), ios::in);
  } else {
    REMPI_ERR("Unknown record replay mode");
  }

  if(!record_fs.is_open()) {
    REMPI_ERR("Record file open failed: %s", record_path.c_str());
  }

}

rempi_encoder_input_format* rempi_encoder_basic::create_encoder_input_format()
{ 
  return new rempi_encoder_input_format();
}


/*  ==== 3 Step compression ==== 
    1. rempi_encoder.get_encoding_event_sequence(...): [rempi_event_list => rempi_encoding_structure]
    this function translate sequence of event(rempi_event_list) 
    into a data format class(rempi_encoding_structure) whose format 
    is sutable for compression(rempi_encoder.encode(...))
   

    2. repmi_encoder.encode(...): [rempi_encoding_structure => char*]
    encode, i.e., rempi_encoding_structure -> char*
     
    3. write_record_file: [char* => "file"]
    write the data(char*)
    =============================== */

// bool rempi_encoder::extract_encoder_input_format_chunk(rempi_event_list<rempi_event*> &events, rempi_encoder_input_format &input_format)
// {
//   rempi_event *event;
//   rempi_encoder_input_format_test_table *test_table;
//   bool is_extracted = false;
//   event = events.pop();
//   /*encoding_event_sequence get only one event*/
//   if (event != NULL) {
//     //    rempi_dbgi(0, "pusu event: %p", event);                                                              
//     input_format.add(event, 0);
//     is_extracted = true;
//   }
//   return is_extracted;
// }

bool rempi_encoder_basic::extract_encoder_input_format_chunk(rempi_event_list<rempi_event*> &events, rempi_encoder_input_format &input_format)
{
  rempi_event *event_dequeued;
  bool is_ready_for_encoding = false;


  while (1) {
    /*Append events to current check as many as possible*/
    if (events.front() == NULL) break;
    event_dequeued = events.pop();
    if (event_dequeued->get_type() == REMPI_MPI_EVENT_TYPE_RECV) {
      while(event_dequeued->get_rank() == MPI_ANY_SOURCE) {
	if (events.is_push_closed_()) break;
	usleep(1);
      }
    }
    //    REMPI_DBG("event: source: %d (type: %d): %p %p", event_dequeued->get_source(), event_dequeued->get_type(), event_dequeued->request, event_dequeued);
    input_format.add(event_dequeued);
  }

  //  REMPI_DBG("input_format.length() %d", input_format.length());
  if (input_format.length() == 0) {
    return is_ready_for_encoding; /*false*/
  }



  //  REMPI_DBG("length: %d (%d)", input_format.length(), REMPI_MAX_INPUT_FORMAT_LENGTH);
  if (input_format.length() > rempi_max_event_length || events.is_push_closed_()) {
    /*If got enough chunck size, OR the end of run*/
    while (1) {
      /*Append events to current check as many as possible*/
      if (events.front() == NULL) break;
      event_dequeued = events.pop();
      //      REMPI_DBG("event: source: %d (type: %d): %p %p", event_dequeued->get_source(), event_dequeued->get_type(), event_dequeued->request, event_dequeued);
      if (event_dequeued->get_type() == REMPI_MPI_EVENT_TYPE_RECV) {
	while(event_dequeued->get_rank() == MPI_ANY_SOURCE) {
	  if (events.is_push_closed_()) break;
	  usleep(1);
	}
      }
      input_format.add(event_dequeued);
    }
    input_format.format();
    is_ready_for_encoding = true;

  }
  return is_ready_for_encoding; /*true*/
}


void rempi_encoder_basic::encode(rempi_encoder_input_format &input_format)
{
  size_t original_size, compressed_size;
  rempi_event *encoding_event;
  rempi_encoder_input_format_test_table *test_table;
  int *original_buff, original_buff_offset;
  int recoding_inputs_num = rempi_event::record_num; // count, flag, rank, with_next and clock
  

  test_table = input_format.test_tables_map[0];

  original_size = rempi_event::max_size * test_table->events_vec.size();
  original_buff = (int*)malloc(original_size);
  for (int i = 0; i < test_table->events_vec.size(); i++) {
    original_buff_offset = i * rempi_event::record_num;
    encoding_event = test_table->events_vec[i];    
    memcpy(original_buff + original_buff_offset, &encoding_event->mpi_inputs.front(), rempi_event::max_size);


#if 0
    REMPI_DBG("Encoded  : (count: %d, type: %d, flag: %d, rank: %d, with_next: %d, index: %d, msg_id: %d, gid: %d)", 
    	      encoding_event->get_event_counts(), 
    	      encoding_event->get_type(),
    	      encoding_event->get_flag(),
    	      encoding_event->get_rank(),
    	      encoding_event->get_with_next(),
    	      encoding_event->get_index(),
    	      encoding_event->get_msg_id(),
    	      encoding_event->get_matching_group_id());
#endif

    // for (int j = 0; i < rempi_event::record_num; i++) {
    //   REMPI_DBG("  %d", *(original_buff + original_buff_offset + i));
    // }


#ifdef REMPI_DBG_REPLAY
    REMPI_DBG("Encoded  : (count: %d, with_next: %d, flag: %d, source: %d, clock: %d)", 
	       encoding_event->get_event_counts(), encoding_event->get_is_testsome(), encoding_event->get_flag(), 
	       encoding_event->get_source(), encoding_event->get_clock());
#endif 

    delete encoding_event;

  }



  if (rempi_gzip) {
#if 1
    vector<char*> compressed_write_queue_vec;
    vector<size_t> compressed_write_size_queue_vec;
    size_t total_compressed_size;
    input_format.write_queue_vec.push_back((char*)original_buff);
    input_format.write_size_queue_vec.push_back(original_size);
    compression_util.compress_by_zlib_vec(input_format.write_queue_vec, input_format.write_size_queue_vec,
                                          compressed_write_queue_vec, compressed_write_size_queue_vec, total_compressed_size);
    test_table->compressed_matched_events      = compressed_write_queue_vec[0];
    test_table->compressed_matched_events_size = compressed_write_size_queue_vec[0];
    free(original_buff);
#else
    test_table->compressed_matched_events      = compression_util.compress_by_zlib((char*)original_buff, original_size, compressed_size);
    test_table->compressed_matched_events_size = compressed_size;
    free(original_buff);
#endif
  } else {
    test_table->compressed_matched_events      = (char*)original_buff;
    test_table->compressed_matched_events_size = original_size;
  }

  return;
}

void rempi_encoder_basic::write_record_file(rempi_encoder_input_format &input_format)
{
  char* whole_data;
  size_t whole_data_size;
  rempi_event *encoding_event;
  rempi_encoder_input_format_test_table *test_table;

  if (input_format.test_tables_map.size() > 1) {
    REMPI_ERR("test_table_size is %d", input_format.test_tables_map.size());
  }
  test_table = input_format.test_tables_map[0];
  whole_data      = test_table->compressed_matched_events;
  whole_data_size = test_table->compressed_matched_events_size;
  record_fs.write((char*)&whole_data_size, sizeof(size_t));
  record_fs.write(whole_data, whole_data_size);
  total_write_size += whole_data_size;
  free(whole_data);

  return;
}

void rempi_encoder_basic::close_record_file()
{
  // size_t total_write_size = 0;
  // for (int i = 0, n = write_size_vec.size(); i < n; i++) {
  //   total_write_size += write_size_vec[i];
  // }
  //  REMPI_DBG("EVAL Total I/O size: |%lu|", total_write_size);
  record_fs.close();
}

#if 1
/*File to vector */
bool rempi_encoder_basic::read_record_file(rempi_encoder_input_format &input_format)
{
  char* decoding_event_sequence;
  int* decoding_event_sequence_int;
  char* decoding_event_sequence_char;
  rempi_event* decoded_event;
  size_t size, record_size, decompressed_size;
  bool is_no_more_record = false;
  int type;
  vector<char*>   compressed_payload_vec;
  vector<size_t>  compressed_payload_size_vec;

  record_fs.read((char*)&record_size, sizeof(size_t));
  size = record_fs.gcount();
  if (size == 0) {
    is_no_more_record = true;
    return is_no_more_record;
  }
  decoding_event_sequence = (char*)malloc(record_size);
  record_fs.read(decoding_event_sequence, record_size);
  
  if (rempi_gzip) {
    compressed_payload_vec.push_back(decoding_event_sequence);
    compressed_payload_size_vec.push_back(record_size);
    compression_util.decompress_by_zlib_vec(compressed_payload_vec, compressed_payload_size_vec,
                                            input_format.write_queue_vec, input_format.write_size_queue_vec, decompressed_size);
    input_format.decompressed_size = decompressed_size;
    free(decoding_event_sequence);
  } else {
    input_format.write_queue_vec.push_back(decoding_event_sequence);
    input_format.write_size_queue_vec.push_back(record_size);
    input_format.decompressed_size = record_size;
  }

  for (int i = 0, s = input_format.write_queue_vec.size(); i < s; i++) {
    //    size_t num_vals = input_format.decompressed_size / sizeof(int);
    size_t num_vals = input_format.write_size_queue_vec[i] / sizeof(int);
    size_t decoded_num_vals = 0;
    decoding_event_sequence_int = (int*)input_format.write_queue_vec[i];
    while (decoded_num_vals < num_vals) {
      type = decoding_event_sequence_int[REMPI_MPI_EVENT_INPUT_INDEX_TYPE];
      if (type == REMPI_MPI_EVENT_TYPE_RECV) {
	decoded_event = rempi_event::create_recv_event(
						       (int)decoding_event_sequence_int[REMPI_MPI_EVENT_INPUT_INDEX_RANK],
						       REMPI_MPI_EVENT_INPUT_IGNORE,
						       REMPI_MPI_EVENT_INPUT_IGNORE,
						       REMPI_MPI_EVENT_INPUT_IGNORE);
      } else if (type == REMPI_MPI_EVENT_TYPE_TEST) {
	decoded_event = rempi_event::create_test_event(
						       (int)decoding_event_sequence_int[REMPI_MPI_EVENT_INPUT_INDEX_EVENT_COUNT],
						       (int)decoding_event_sequence_int[REMPI_MPI_EVENT_INPUT_INDEX_FLAG],
						       (int)decoding_event_sequence_int[REMPI_MPI_EVENT_INPUT_INDEX_RANK],
						       (int)decoding_event_sequence_int[REMPI_MPI_EVENT_INPUT_INDEX_WITH_NEXT],
						       (int)decoding_event_sequence_int[REMPI_MPI_EVENT_INPUT_INDEX_INDEX],
						       (int)decoding_event_sequence_int[REMPI_MPI_EVENT_INPUT_INDEX_MSG_ID],
						       (int)decoding_event_sequence_int[REMPI_MPI_EVENT_INPUT_INDEX_MATCHING_GROUP_ID]
						       );
      } else {
	REMPI_ERR("No such event type: %d", type);
      }
#if 0
      REMPI_DBGI(0, "Read   : (count: %d, type: %d, flag: %d, rank: %d, with_next: %d, index: %d, msg_id: %d, gid: %d)",
            decoded_event->get_event_counts(),
            decoded_event->get_type(),
            decoded_event->get_flag(),
            decoded_event->get_rank(),
            decoded_event->get_with_next(),
            decoded_event->get_index(),
            decoded_event->get_msg_id(),
            decoded_event->get_matching_group_id());
#endif
      input_format.add(decoded_event, 0);
      decoding_event_sequence_int += REMPI_MPI_EVENT_INPUT_NUM;
      decoded_num_vals += REMPI_MPI_EVENT_INPUT_NUM;
    }
    free(input_format.write_queue_vec[i]);
  }

  total_write_size += record_size;
  is_no_more_record = false;
  return is_no_more_record;
}

#endif


// bool rempi_encoder_basic::read_record_file(rempi_encoder_input_format &input_format)
// {
//   char* decoding_event_sequence;
//   int* decoding_event_sequence_int;
//   rempi_event* decoded_event;
//   size_t size;
//   bool is_no_more_record = false;
//   int type;
//   size_t record_size;

//   /*TODO: 
//     Currently read only one event at one this function call, 
//     but this function will read an event chunk (multiple events) in future
//    */

//   decoding_event_sequence = (char*)malloc(rempi_event::record_num * rempi_event::record_element_size);
//   record_fs.read(decoding_event_sequence, rempi_event::record_num * rempi_event::record_element_size);
//   size = record_fs.gcount();
//   if (size == 0) {
//     free(decoding_event_sequence);
//     is_no_more_record = true;
//     return is_no_more_record;
//   }
//   decoding_event_sequence_int = (int*)decoding_event_sequence;
//   type = decoding_event_sequence_int[REMPI_MPI_EVENT_INPUT_INDEX_TYPE];
//   if (type == REMPI_MPI_EVENT_TYPE_RECV) {

//     decoded_event = rempi_event::create_recv_event(
// 						   (int)decoding_event_sequence_int[REMPI_MPI_EVENT_INPUT_INDEX_RANK],
// 						   REMPI_MPI_EVENT_INPUT_IGNORE,
// 						   REMPI_MPI_EVENT_INPUT_IGNORE,
// 						   REMPI_MPI_EVENT_INPUT_IGNORE);
//   } else if (type == REMPI_MPI_EVENT_TYPE_TEST) {
//     decoded_event = rempi_event::create_test_event(
// 						   (int)decoding_event_sequence_int[REMPI_MPI_EVENT_INPUT_INDEX_EVENT_COUNT],
// 						   (int)decoding_event_sequence_int[REMPI_MPI_EVENT_INPUT_INDEX_FLAG],
// 						   (int)decoding_event_sequence_int[REMPI_MPI_EVENT_INPUT_INDEX_RANK],
// 						   (int)decoding_event_sequence_int[REMPI_MPI_EVENT_INPUT_INDEX_WITH_NEXT],
// 						   (int)decoding_event_sequence_int[REMPI_MPI_EVENT_INPUT_INDEX_INDEX],
// 						   (int)decoding_event_sequence_int[REMPI_MPI_EVENT_INPUT_INDEX_MSG_ID],
// 						   (int)decoding_event_sequence_int[REMPI_MPI_EVENT_INPUT_INDEX_MATCHING_GROUP_ID]
// 						   );
//   } else {
//     REMPI_ERR("No such event type: %d", type);
//   }
// #if 1
//   // REMPI_DBG( "Read    ; (count: %d, type: %d, flag: %d, rank: %d, with_next: %d, index: %d, msg_id: %d, gid: %d)", 
//   // 	     (int)decoding_event_sequence_int[0],
//   // 	     (int)decoding_event_sequence_int[1],
//   // 	     (int)decoding_event_sequence_int[2],
//   // 	     (int)decoding_event_sequence_int[3],
//   // 	     (int)decoding_event_sequence_int[4],
//   // 	     (int)decoding_event_sequence_int[5],
//   // 	     (int)decoding_event_sequence_int[6],
//   // 	     (int)decoding_event_sequence_int[7]
//   // 	     );

//   // REMPI_DBG("Read   : (count: %d, type: %d, flag: %d, rank: %d, with_next: %d, index: %d, msg_id: %d, gid: %d)",
//   //           decoded_event->get_event_counts(),
//   //           decoded_event->get_type(),
//   //           decoded_event->get_flag(),
//   //           decoded_event->get_rank(),
//   //           decoded_event->get_with_next(),
//   //           decoded_event->get_index(),
//   //           decoded_event->get_msg_id(),
//   //           decoded_event->get_matching_group_id());

// #endif

//   free(decoding_event_sequence);
//   input_format.add(decoded_event, 0);
//   is_no_more_record = false;
//   total_write_size += size;
//   return is_no_more_record;
// }


void rempi_encoder_basic::decode(rempi_encoder_input_format &input_format)
{
  /*Do nothing*/
  return;
}

// /*vector to event_list*/
// void rempi_encoder::insert_encoder_input_format_chunk(rempi_event_list<rempi_event*> &recording_events, rempi_event_list<rempi_event*> &replaying_events, rempi_encoder_input_format &input_format)
// {
//   size_t size;
//   rempi_event *decoded_event;
//   rempi_encoder_input_format_test_table *test_table;

//   /*Get decoded event to replay*/
//   test_table = input_format.test_tables_map[0];
//   decoded_event = test_table->events_vec[0];
//   test_table->events_vec.pop_back();


// #ifdef REMPI_DBG_REPLAY
//   REMPI_DBGI(REMPI_DBG_REPLAY, "Decoded ; (count: %d, with_next: %d, flag: %d, source: %d, tag: %d, clock: %d)",
// 	     decoded_event->get_event_counts(), decoded_event->get_is_testsome(), decoded_event->get_flag(),
// 	     decoded_event->get_source(), decoded_event->get_tag(), decoded_event->get_clock());
// #endif

//   if (decoded_event->get_flag() == 0) {
//     /*If this is flag == 0, simply enqueue it*/
//     replaying_events.enqueue_replay(decoded_event, 0);
//     return;
//   }

  
//   rempi_event *matched_replaying_event;
//   if (decoded_event->get_flag()) {
//     while ((matched_replaying_event = pop_event_pool(decoded_event)) == NULL) { /*Pop from matched_event_pool*/
//       /*If this event is matched test, wait until this message really arrive*/
//       rempi_event *matched_event;
//       int event_list_status;
//       while (recording_events.front_replay(0) == NULL) { 
// 	/*Wait until any message arraves */
// 	usleep(100);
//       }
//       matched_event = recording_events.dequeue_replay(0, event_list_status);
//       matched_event_pool.push_back(matched_event);
// #ifdef REMPI_DBG_REPLAY
//       REMPI_DBGI(REMPI_DBG_REPLAY, "RCQ->PL ; (count: %d, with_next: %d, flag: %d, source: %d, tag: %d, clock: %d, msg_count: %d %p)",
// 		 matched_event->get_event_counts(), matched_event->get_is_testsome(), matched_event->get_flag(),
// 		 matched_event->get_source(), matched_event->get_tag(), matched_event->get_clock(), matched_event->msg_count, 
// 		 matched_event);
// #endif
//     }
//   } 

//   /* matched_replaying_event == decoded_event: this two event is same*/

//   decoded_event->msg_count = matched_replaying_event->msg_count;
// #ifdef REMPI_DBG_REPLAY
//   REMPI_DBGI(REMPI_DBG_REPLAY, "PL->RPQ ; (count: %d, with_next: %d, flag: %d, source: %d, tag: %d, clock: %d, msg_count: %d)",
// 	     decoded_event->get_event_counts(), decoded_event->get_is_testsome(), decoded_event->get_flag(),
// 	     decoded_event->get_source(), decoded_event->get_tag(), decoded_event->get_clock(), decoded_event->msg_count);
// #endif


//   replaying_events.enqueue_replay(decoded_event, 0);
//   delete matched_replaying_event;
//   return;    
// }

/*vector to event_list*/
void rempi_encoder_basic::insert_encoder_input_format_chunk(rempi_event_list<rempi_event*> &recording_events, rempi_event_list<rempi_event*> &replaying_events, rempi_encoder_input_format &input_format)
{
  size_t size;
  rempi_event *decoded_event;
  rempi_encoder_input_format_test_table *test_table;

  /*Get decoded event to replay*/
  test_table = input_format.test_tables_map[0];
  for (int i = 0, s = test_table->events_vec.size(); i < s; i++) {
    decoded_event = test_table->events_vec[i];

#ifdef REMPI_DBG_REPLAY
    REMPI_DBG("Decoded  : (count: %d, flag: %d, rank: %d, with_next: %d, index: %d, msg_id: %d, gid: %d)", 
	      decoded_event->get_event_counts(), 
	      decoded_event->get_flag(),
	      decoded_event->get_rank(),
	      decoded_event->get_with_next(),
	      decoded_event->get_index(),
	      decoded_event->get_msg_id(),
	      decoded_event->get_matching_group_id());
#endif
    replaying_events.enqueue_replay(decoded_event, 0);
  }

  return;    
}


void rempi_encoder_basic::fetch_local_min_id(int *min_recv_rank, size_t *min_next_clock)
{    
  //  REMPI_ERR("please remove this REMPI_ERR later");
  return;
}


void rempi_encoder_basic::update_fd_next_clock(
					 int is_waiting_recv,
					 int num_of_recv_msg_in_next_event,
					 size_t interim_min_clock_in_next_event,
					 size_t enqueued_count,
					 int recv_test_id,
					 int is_after_recv_event)
{ 
  //  REMPI_ERR("please remove this REMPI_ERR later");
  return;
}

void rempi_encoder_basic::set_fd_clock_state(int flag)
{
  return;
}

void rempi_encoder_basic::compute_local_min_id(rempi_encoder_input_format_test_table *test_table, int *local_min_id_rank, size_t *local_min_id_clock, int recv_test_id)
{
  //  REMPI_ERR("please remove this REMPI_ERR later");
  return;
}


// char* rempi_encoder_basic::read_decoding_event_sequence(size_t *size)
// {
//   char* decoding_event_sequence;

//   decoding_event_sequence = new char[rempi_event::max_size];
//   record_fs.read(decoding_event_sequence, rempi_event::max_size);
//   /*Set size read*/
//   *size = record_fs.gcount();
//   return decoding_event_sequence;
// }



// vector<rempi_event*> rempi_encoder_basic::decode(char *serialized_data, size_t *size)
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








