#ifndef __REMPI_EVENT_LIST_H__
#define __REMPI_EVENT_LIST_H__

#include <list>
#include <map>
#include <unordered_map>
#include <unistd.h>

#include "rempi_spsc_queue.h"
#include "rempi_mutex.h"
#include "rempi_event.h"

#define REMPI_EVENT_LIST_EMPTY     (0)
#define REMPI_EVENT_LIST_FINISH    (1)
#define REMPI_EVENT_LIST_DEQUEUED  (2)

template <class T>
class rempi_event_list
{
 private:
  rempi_spsc_queue<T> events; /* TODO: conbine events and replay_events into single unordered_map*/
  unordered_map<int, rempi_spsc_queue<T>*> replay_events;
  /*TODO: change from preivous_event to previous_recording_event*/
  rempi_event *previous_recording_event;
  unordered_map<int, rempi_event*> previous_recording_event_umap;
  unordered_map<int, rempi_event*> previous_replaying_event_umap;
  bool is_push_closed = false;
  T mpi_event;
  rempi_mutex mtx;
  size_t max_size;
  size_t spin_time;
  size_t globally_minimal_clock; /*which is used for CDC compression*/
 public:
 rempi_event_list(size_t max_size, size_t spin_time) :
  previous_recording_event(NULL), is_push_closed(false),
    max_size(max_size), spin_time(spin_time), globally_minimal_clock(0) {}
  ~rempi_event_list() {
    mtx.lock();
    while (events.rough_size()) {
      events.dequeue();
    }
    mtx.unlock();
  }
  /*TODO: push -> enqueue, pop -> dequeue */
  size_t size();
  void normal_push(T event);
  void push(rempi_event *event);
  void push_all();
  void close_push();
  T pop();
  T front();

  T     front_replay(int test_id);
  T    dequeue_replay(int test_id, int &status);
  void enqueue_replay(T event, int test_id);
		
  size_t get_globally_minimal_clock();
  void   set_globally_minimal_clock(size_t gmc);
  bool   is_push_closed_();
};

#endif
