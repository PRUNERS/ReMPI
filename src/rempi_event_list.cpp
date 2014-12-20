
#include <unistd.h>

#include "rempi_mutex.h"
#include "rempi_spsc_queue.h"
#include "rempi_event_list.h"
#include "rempi_event.h"
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
void rempi_event_list<T>::push(T event)
{
  if (this->previous_event == NULL) {
    previous_event = event;
    return;
  }
  
  if ((*this->previous_event) == *event) {
    (*this->previous_event)++;
    delete event;
  } else {
    mtx.lock();
    while (events.rough_size() >= max_size)
      {
	mtx.unlock();
	usleep(spin_time);
	mtx.lock();
      }
    events.enqueue(previous_event);
    mtx.unlock();
    previous_event = event;
  }
  return;
}

template <class T>
void rempi_event_list<T>::push_all()
{
  if (previous_event == NULL) return;
  mtx.lock();
  while (events.rough_size() >= max_size) {
    mtx.unlock();
    usleep(spin_time);
    mtx.lock();
  }
  events.enqueue(previous_event);
  mtx.unlock();
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


