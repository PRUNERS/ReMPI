#include <iostream>
#include <fstream>
#include <vector>

#include "rempi_io_thread.h"
#include "rempi_config.h"
#include "rempi_err.h"
#include "rempi_util.h"

using namespace std;

rempi_mutex rempi_io_thread::mtx;

rempi_io_thread::rempi_io_thread(rempi_event_list<rempi_event*> *events, string id, int mode)
  : events(events), id(id), mode(mode)
{
  record_path = rempi_record_dir_path + "/rank_" + id + ".rempi";
  encoder = new rempi_encoder(mode);
};


void rempi_io_thread::complete_flush()
{
   is_complete_flush = 1;
   return;
}

int rempi_io_thread::is_finished_read()
{
  return read_finished;
}

void rempi_io_thread::write_record()
{

  encoder->open_record_file(record_path);
  
  while(1) {
    vector<rempi_event*> nonencoded_events;
    char *encoded_events;
    size_t size;
    //    rempi_dbgi(0, "events: %p, size: %lu", encoded_events, size);

    /*Get a sequence of events, ...  */
    encoder->get_encoding_event_sequence(*events, nonencoded_events);

    if (nonencoded_events.size() > 0) {
      /*If I get the sequence,... */
      /*... , encode(compress) the seuence*/
      encoded_events = encoder->encode(nonencoded_events, size);
      /*Then, write to file.*/
      encoder->write_record_file(encoded_events, size);
      /*Once writing data to a file, we do not need the events on memory*/
      delete encoded_events;
    } else {
      /*If not, wait for a while in order to reduce CPU usage.*/
      usleep(100);
    }

    //    REMPI_DBGI(1, "----> events: %p, size: %lu", encoded_events, size);
    //    REMPI_DBGI(1, " is_complete_flush: %d, size: %d", is_complete_flush, events->size());
    /*is_complete = 1 => event are not pushed to the event quene no longer*/
    /*if the events is empty, we can finish recoding*/
    if (is_complete_flush && events->size() == 0) {

      break;
    }
  }
  encoder->close_record_file();
}

void rempi_io_thread::read_record()
{
  encoder->open_record_file(record_path);

  while(1) {
    char *decoding_events;
    size_t size;
    vector<rempi_event*> decoded_events;
    int i;

    /*Get a sequence of events from the record file, ...  */
    decoding_events = encoder->read_decoding_event_sequence(&size);

    if (size > 0) {
      /*If I can get the sequence from the file,... */
      /*... , deencode(decompress) the seuence*/
      decoded_events = encoder->decode(decoding_events, &size); /*TODO: size is really needed ?*/
      /*Then, push the event to event list*/
      for (i = 0; i < decoded_events.size(); i++) {
	events->normal_push(decoded_events[i]);
      }
    } else {
      /*I read the all record file, so finish the decoding*/
      break;
    }
  }
  encoder->close_record_file();
  events->close_push();
  /*TODO: We still need this read_finshed variable?*/
  read_finished = 1;
  return;
}

// void rempi_io_thread::write_record()
// {
//   ofstream ofs;
//   string path;
//   int i;

//   //path = "/tmp/rank_" + id + ".rempi";
//   //path = "/p/lscratche/sato5/rempi/rank_" + id + ".rempi";
//   path = "./rank_" + id + ".rempi";
  
//   ofs.open(path);
  
//   if (!events) return;
//   while(1) {
//     int *serialized_data, size;
//     rempi_event *item = events->pop();
    
//     if (item != NULL) {
//       serialized_data = item->serialize(size);
//       ofs.write((const char*)serialized_data, sizeof(int) * size);
//     } else {
//       usleep(100);
//     }

//     if (is_complete_flush && events->size() == 0) {
//       break;
//     }
//   }
//   ofs.flush();
//   ofs.close();
// }

// void rempi_io_thread::read_record()
// {
//   ofstream ofs;
//   string path;
//   int i;

//   path = "./rank_" + id + ".rempi";
  
//   ofs.open(path);
  
//   if (!events) return;
//   while(1) {
//     int *serialized_data, size;
//     rempi_event *item = events->pop();
    
//     if (item != NULL) {
//       serialized_data = item->serialize(size);
//       ofs.write((const char*)serialized_data, sizeof(int) * size);
//     } else {
//       usleep(100);
//     }

//     if (is_complete_flush && events->size() == 0) {
//       break;
//     }
//   }
//   ofs.flush();
//   ofs.close();
// }

void rempi_io_thread::run()
{
  if (mode == 0) {
    write_record();
  } else if (mode == 1) {
    read_record();
  } else {
    fprintf(stderr, "unexpected mode in rempi_io_thread.cpp");
  }
}


