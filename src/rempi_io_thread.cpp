#include "rempi_io_thread.h"
#include "rempi_config.h"
#include <iostream>
#include <fstream>

#define PATH_DIR "/p/lscratche/sato5/rempi"

using namespace std;

rempi_mutex rempi_io_thread::mtx;

/*
void Consumer::consume(size_t num_items)
{
	if (!event_list) return;
	if (num_items <= 0) return;
	this->num_items = num_items;
	start();
}
*/

void rempi_io_thread::complete_flush()
{
   is_complete_flush = 1;
   return;
}

void rempi_io_thread::write_record()
{
  ofstream ofs;
  string path;
  int i;

  //path = "/tmp/rank_" + id + ".rempi";
  //path = "/p/lscratche/sato5/rempi/rank_" + id + ".rempi";
  path = "./rank_" + id + ".rempi";
  
  ofs.open(path);
  
  if (!events) return;
  while(1) {
    int *serialized_data, size;
    rempi_event *item = events->pop();
    
    if (item != NULL) {
      serialized_data = item->serialize(size);
      ofs.write((const char*)serialized_data, sizeof(int) * size);
    } else {
      usleep(100);
    }

    if (is_complete_flush && events->size() == 0) {
      break;
    }
  }
  ofs.flush();
  ofs.close();
}

void rempi_io_thread::read_record()
{
  ofstream ofs;
  string path;
  int i;

  path = "./rank_" + id + ".rempi";
  
  ofs.open(path);
  
  if (!events) return;
  while(1) {
    int *serialized_data, size;
    rempi_event *item = events->pop();
    
    if (item != NULL) {
      serialized_data = item->serialize(size);
      ofs.write((const char*)serialized_data, sizeof(int) * size);
    } else {
      usleep(100);
    }

    if (is_complete_flush && events->size() == 0) {
      break;
    }
  }
  ofs.flush();
  ofs.close();
}

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


