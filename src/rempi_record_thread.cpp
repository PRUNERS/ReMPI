#include "rempi_record_thread.h"
#include <iostream>
#include <fstream>

#define PATH_DIR "/p/lscratche/sato5/rempi"

using namespace std;

rempi_mutex rempi_record_thread::mtx;

/*
void Consumer::consume(size_t num_items)
{
	if (!event_list) return;
	if (num_items <= 0) return;
	this->num_items = num_items;
	start();
}
*/

void rempi_record_thread::complete_flush()
{
	is_complete_flush = 1;
}

void rempi_record_thread::run()
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
