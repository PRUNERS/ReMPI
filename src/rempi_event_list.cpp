
#include <unistd.h>

#include "rempi_mutex.h"
#include "rempi_spsc_queue.h"
#include "rempi_event_list.h"
#include "rempi_event.h"
#include "rempi_util.h"
#include "rempi_err.h"


using namespace std;

template class rempi_event_list<rempi_event*>; // to tell compiler

template <class T>
size_t rempi_event_list<T>::size()
{
  mtx.lock();
  size_t result = events.rough_size();
  mtx.unlock();
  return result;
}

template <class T>
void rempi_event_list<T>::normal_push(T event)
{
  mtx.lock();
  while (events.rough_size() >= max_size) {
    mtx.unlock();
    usleep(spin_time);
    mtx.lock();
  }
  events.enqueue(event);
  mtx.unlock();
}

template <class T>
void rempi_event_list<T>::push(T event)
{
  if (is_push_closed) {
    REMPI_ERR("event_list push was closed");
  }

  if (this->previous_recording_event == NULL) {
    previous_recording_event = event;
    return;
  }

  if ((*this->previous_recording_event) == *event) {
    /*If the event is exactry same as previsou one, ...*/
    /*Just increment the counter of the event
     in order to reduce the memory consumption.
     Even if an application polls with MPI_Test, 
     we only increment an counter of the previous MPI_Test event
    */
    (*this->previous_recording_event)++;
    delete event;
  } else {
    mtx.lock();
    while (events.rough_size() >= max_size) {
      mtx.unlock();
      usleep(spin_time);
      mtx.lock();
    }
    events.enqueue(previous_recording_event);
    mtx.unlock();
    previous_recording_event = event;
  }
  return;
}

template <class T>
void rempi_event_list<T>::push_all()
{
  if (is_push_closed) {
    REMPI_ERR("event_list push was closed");
  }

  if (previous_recording_event == NULL) return;
  mtx.lock();
  while (events.rough_size() >= max_size) {
    mtx.unlock();
    usleep(spin_time);
    mtx.lock();
  }
  events.enqueue(previous_recording_event);
  mtx.unlock();
}

template <class T>
void rempi_event_list<T>::close_push()
{
  is_push_closed = true;
  return;
}

/*
Pop an event until event_count becomes 0.
If event_count == 0, then dequeue next event.
If no events to be dequeued, this thread ends its work.

If thread finished reading all events, and pushed to event_lists, and the events are all pupoed, 
This function retunr NULL
*/
template <class T>
T rempi_event_list<T>::decode_pop()
{
  rempi_event *event;

  /*TODO: TODO(A) */
  while (previous_replaying_event == NULL) {
    mtx.lock();
    previous_replaying_event = events.dequeue();
    mtx.unlock();
    //    REMPI_DBG("== %p\n", previous_replaying_event);
    bool is_queue_empty = (previous_replaying_event == NULL);
    if (is_queue_empty && is_push_closed) return NULL;
  }

  /*From here, previous_replaying_event has "event instance" at least*/
  if (previous_replaying_event->size() <= 0) {
    delete previous_replaying_event;
    previous_replaying_event = NULL;

    /*TODO: Implement a function to combine to TODO(A) above*/
    while (previous_replaying_event == NULL) {
      mtx.lock();
      previous_replaying_event = events.dequeue();
      mtx.unlock();
      //      fprintf(stderr, "%p\n", previous_replaying_event, events.rough_size());
      bool is_queue_empty = (previous_replaying_event == NULL);
      if (is_queue_empty && is_push_closed) return NULL;
    }
  }
  //  fprintf(stderr, "%p (size: %lu)\n", previous_replaying_event, events.rough_size());

  /*From here, previous_replaying_event has an actual evnet at least*/

  event = previous_replaying_event->pop();
  
  if (event == NULL) {
    REMPI_ERR("previous replaying event pop failed");
  }
  //  event->print();
  return event;

  /*
    while (events.rough_size() <= 0)
    {
    count << events.rough_size() << endl;
    mtx.unlock();
    usleep(spin_time);
    mtx.lock();
    }
  */

}

template <class T>
T rempi_event_list<T>::pop()
{
  mtx.lock();
  /*
    while (events.rough_size() <= 0)
    {
    count << events.rough_size() << endl;
    mtx.unlock();
    usleep(spin_time);
    mtx.lock();
    }
  */
  T item = events.dequeue();
  mtx.unlock();
  return item;
}


