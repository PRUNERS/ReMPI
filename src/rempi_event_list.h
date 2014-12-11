#ifndef EVENT_LIST_H
#define EVENT_LIST_H

#include <list>
#include <map>
#include <unistd.h>

#include "rempi_spsc_queue.h"
#include "rempi_mutex.h"
#include "rempi_event.h"


template <class T>
class rempi_event_list
{
	private:
		rempi_spsc_queue<T> events;
		rempi_event *previous_event;
		T mpi_event;
		rempi_mutex mtx;
		size_t max_size;
		size_t spin_time;
	public:
		rempi_event_list(size_t max_size, size_t spin_time) :
		previous_event(NULL), max_size(max_size), spin_time(spin_time) {
		      previous_event = NULL;
		}
		~rempi_event_list() {
        		mtx.lock();
			while (events.rough_size()) {
				events.dequeue();
			}
			mtx.unlock();
		}

		size_t size();
		void push(T event);
		void push_all();
		T pop();
};

#endif
