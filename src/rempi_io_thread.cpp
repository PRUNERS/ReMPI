#include <iostream>
#include <fstream>
#include <vector>

#include "rempi_io_thread.h"
#include "rempi_config.h"
#include "rempi_err.h"
#include "rempi_util.h"

using namespace std;

rempi_mutex rempi_io_thread::mtx;

rempi_io_thread::rempi_io_thread(rempi_event_list<rempi_event*> *recording_events, 
				 rempi_event_list<rempi_event*> *replaying_events, 
				 string id, int mode)
#ifdef BGQ
{
  rempi_io_thread::rempi_io_thread(recording_events, replaying_events, id, mode, NULL);
}
#else
  :rempi_io_thread(recording_events, replaying_events, id, mode, NULL)
{}
#endif



rempi_io_thread::rempi_io_thread(rempi_event_list<rempi_event*> *recording_events, 
				 rempi_event_list<rempi_event*> *replaying_events, 
				 string id, int mode,
				 rempi_encoder **mc_encoder)
:recording_events(recording_events), replaying_events(replaying_events), id(id), mode(mode)
{
  record_path = rempi_record_dir_path + "/rank_" + id + ".rempi";
  if (rempi_encode == 0) {
    encoder = new rempi_encoder(mode);                       //  (1): Simple record (count, flag, rank with_next and clock)
  } else if (rempi_encode == 1) {
    encoder = new rempi_encoder_simple_zlib(mode);           //  (2): (1) + format change
  } else if (rempi_encode == 2) {
    encoder = new rempi_encoder_zlib(mode);                  //  (3): (2) + distingusishing different test/testsome
  } else if (rempi_encode == 3) {
    encoder = new rempi_encoder_cdc_row_wise_diff(mode);     //  (4): (3) + row_wise diff
  } else if (rempi_encode == 4) {
    encoder = new rempi_encoder_cdc(mode);                   //  (5): (3) + edit distance (two values for an only permutated message)
  } else if (rempi_encode == 5) {
    encoder = new rempi_encoder_cdc_permutation_diff(mode);  //  (6): (3) + edit distance (one value for each message)
  }  else if (rempi_encode == 6) {
    if (mode == REMPI_ENV_REMPI_MODE_REPLAY) {
      encoder = new rempi_encoder_rep(mode);                 //  (7): Reproducibile MPI
    } else {
      REMPI_ERR("No such option conbination");
    }
  } else {
    REMPI_ERR("No such encode");
  }

  if (mc_encoder != NULL) {
    *mc_encoder = encoder;
  }

}

void rempi_io_thread::write_record()
{

  encoder->open_record_file(record_path);
  rempi_encoder_input_format *nonencoded_events;
  nonencoded_events = encoder->create_encoder_input_format();

  while(1) {
    /*use "new" to be able to select compression methods depending on specified input value*/

    bool is_extracted;
    char *encoded_events;
    size_t size;
    double s, e;

    /*Get a sequence of events, ...  */
    is_extracted = encoder->extract_encoder_input_format_chunk(*recording_events, *nonencoded_events);

    if (is_extracted) {
      /*If I get the sequence,... */
      /*... , encode(compress) the seuence*/
      s = rempi_get_time();
      encoder->encode(*nonencoded_events);
      /*Then, write to file.*/
      encoder->write_record_file(*nonencoded_events);
      e = rempi_get_time();
      //REMPI_DBG(" RATE |%f|%d|%f|" , nonencoded_events->length() / (e - s), nonencoded_events->length(), e - s);

      nonencoded_events->debug_print();
      
      delete nonencoded_events; //TODO: also delete iternal data in this variable
      nonencoded_events = encoder->create_encoder_input_format();
      /*TODO: free rempi_encoded_cdc_input_format*/
    } else {
      /*If not, wait for a while in order to reduce CPU usage.*/
      usleep(100);
    }

    //    REMPI_DBGI(1, "----> events: %p, size: %lu", encoded_events, size);
    //    REMPI_DBGI(1, " is_complete_flush: %d, size: %d", is_complete_flush, events->size());
    //    REMPI_DBG(" is_cclosed: %d, size: %d", events->is_push_closed_(), events->size());
    /*is_complete = 1 => event are not pushed to the event quene no longer*/
    /*if the events is empty, we can finish recoding*/
    if (recording_events->is_push_closed_() && recording_events->size() == 0) {
      break;
    }
  }
  encoder->close_record_file();
  return;
}

void rempi_io_thread::read_record()
{
  encoder->open_record_file(record_path);
  rempi_encoder_input_format *input_format;
  input_format = encoder->create_encoder_input_format();

  while(1) {
    bool is_no_more_record;

    is_no_more_record = encoder->read_record_file(*input_format);
    if (is_no_more_record) {
      /*If replayed all recorded events, ...*/
      replaying_events->close_push();
      break;
    } else {
      encoder->decode(*input_format);
      encoder->insert_encoder_input_format_chunk(*recording_events, *replaying_events, *input_format);
      delete input_format;
      input_format = encoder->create_encoder_input_format();
    }
  }

#ifdef REMPI_DBG_REPLAY
  //  REMPI_DBGI(REMPI_DBG_REPLAY,"end ======");
  REMPI_DBG("end ====== (CDC thread)");
#endif

  encoder->close_record_file();
  return;
}

// void rempi_io_thread::read_record()
// {
//   encoder->open_record_file(record_path);

//   while(1) {
//     char *decoding_events;
//     size_t size;
//     vector<rempi_event*> decoded_events;
//     int i;

//     /*Get a sequence of events from the record file, ...  */
//     decoding_events = encoder->read_decoding_event_sequence(&size);

//     if (size > 0) {
//       /*If I can get the sequence from the file,... */
//       /*... , deencode(decompress) the seuence*/
//       decoded_events = encoder->decode(decoding_events, &size); /*TODO: size is really needed ?*/
//       /*Then, push the event to event list*/
//       for (i = 0; i < decoded_events.size(); i++) {
// 	replaying_events->normal_push(decoded_events[i]);
//       }
//     } else {
//       /*I read the all record file, so finish the decoding*/
//       break;
//     }
//   }
//   encoder->close_record_file();
//   replaying_events->close_push();
//   /*TODO: We still need this read_finshed variable?*/
//   read_finished = 1;
//   return;
// }


void rempi_io_thread::run()
{
  if (mode == 0) {
    write_record();
  } else if (mode == 1) {
    read_record();
  } else {
    REMPI_DBG("Unexpected mode in rempi_io_thread");
  }
}


