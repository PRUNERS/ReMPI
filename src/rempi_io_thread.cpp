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
  encoder = new rempi_encoder();
  record_path = rempi_record_dir_path + "/rank_" + id + ".rempi";
};


void rempi_io_thread::complete_flush()
{
   is_complete_flush = 1;
   return;
}

void rempi_io_thread::write_record()
{
  ofstream ofs;

  ofs.open(record_path.c_str());
  
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
      ofs.write(encoded_events, size);
      /*Once writing data to a file, we do not need the events on memory*/
      delete encoded_events;
    } else {
      /*If not, wait for a while in order to reduce CPU usage.*/
      usleep(100);
    }
    //    rempi_dbgi(0, "----> events: %p, size: %lu", encoded_events, size);

    /*is_complete = 1 => event are not pushed to the event quene no longer*/
    /*if the events is empty, we can finish recoding*/
    if (is_complete_flush && events->size() == 0) {
      break;
    }
  }
  ofs.flush();
  ofs.close();
}

void rempi_io_thread::read_record()
{
  ifstream ifs;

  ifs.open(record_path.c_str());
  REMPI_ERR("open failed");
  if (!ifs.is_open()) {

  }


  while(1) {
    //    ifs.read(encoded_events, size);

  }
  ifs.close();
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


