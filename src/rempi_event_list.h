#ifndef __REMPI_EVENT_LIST_H__
#define __REMPI_EVENT_LIST_H__

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
		/*TODO: change from preivous_event to previous_recording_event*/
		rempi_event *previous_recording_event;
		rempi_event *previous_replaying_event;
		bool is_push_closed;
		T mpi_event;
		rempi_mutex mtx;
		size_t max_size;
		size_t spin_time;
		size_t globally_minimal_clock; /*which is used for CDC compression*/
	public:
                rempi_event_list(size_t max_size, size_t spin_time) :
		  previous_recording_event(NULL), previous_replaying_event(NULL), is_push_closed(false),
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
		void push(T event);
		void push_all();
		void close_push();
		T decode_pop();
		T pop();
		T front();
		
		size_t get_globally_minimal_clock();
		void   set_globally_minimal_clock(size_t gmc);
		bool   is_push_closed_();
};

#endif
