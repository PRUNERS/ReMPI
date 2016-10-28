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
				 int mode,
				 rempi_encoder *mc_encoder)
:recording_events(recording_events), replaying_events(replaying_events), mode(mode)
{
  encoder = mc_encoder;
  return;
}


rempi_io_thread::~rempi_io_thread()
{
  delete encoder;
}


int count = 0;
void rempi_io_thread::write_record()
{
  encoder->open_record_file();
  rempi_encoder_input_format *input_format = NULL;
  input_format = encoder->create_encoder_input_format();

  while(1) {
    bool is_extracted;
    char *encoded_events;
    size_t size;
    double s, e;

    /*Get a sequence of events, ...  */
    is_extracted = encoder->extract_encoder_input_format_chunk(*recording_events, input_format);

    if (is_extracted) {
      /*If I get the sequence, encode(compress) the seuence*/
      s = rempi_get_time();
      encoder->encode();
      //      input_format->debug_print();
      /*Then, write to file.*/
      encoder->write_record_file();
      count++;
      e = rempi_get_time();
      delete input_format; //TODO: also delete iternal data in this variable
      input_format = encoder->create_encoder_input_format();
    } else {
      /*If not, wait for a while in order to reduce CPU usage.*/
      usleep(100);
    }

    /*if the events is empty, we can finish recoding*/
    if (recording_events->is_push_closed_() && recording_events->size() == 0) {
      if (input_format != NULL) {
       	delete input_format;
      }
      break;
    }
  }

  encoder->close_record_file();
  return;
}

void rempi_io_thread::read_record()
{
  int is_all_finished = 0;

  while(!is_all_finished) {
    is_all_finished = encoder->progress_decoding(recording_events, replaying_events, -1);
  }


  return;
}


void rempi_io_thread::read_record_lite()
{
  rempi_encoder_input_format *input_format;
  bool is_no_more_record;


  encoder->open_record_file();
  input_format = encoder->create_encoder_input_format();

  while(1) {
    is_no_more_record = encoder->read_record_file(input_format);
    if (is_no_more_record) {
      /*If replayed all recorded events, ...*/
      replaying_events->close_push();
      delete input_format;
      break;
    } else {
      encoder->decode();
      encoder->insert_encoder_input_format_chunk(*recording_events, *replaying_events);
      delete input_format;
      input_format = encoder->create_encoder_input_format();
    }
  }


#ifdef REMPI_DBG_REPLAY
  REMPI_DBGI(REMPI_DBG_REPLAY, "end ====== (CDC thread)");
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
    if (rempi_encode == REMPI_ENV_REMPI_ENCODE_BASIC) {
      read_record_lite();
    } else {
      read_record();
    }

  } else {
    REMPI_ERR("Unexpected mode in rempi_io_thread: %d", mode);
  }
}


