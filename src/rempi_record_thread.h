#ifndef __CONSUMER_H__
#define __CONSUMER_H__

#include <string>
#include <fstream>

#include "rempi_event.h"
#include "rempi_event_list.h"
#include "rempi_thread.h"

using namespace std;

class rempi_record_thread : public rempi_thread
{
	private:
		static rempi_mutex mtx;
		int is_complete_flush;
		rempi_event_list<rempi_event*> *events;
		string id;
	protected:
		virtual void run();
	public:
		rempi_record_thread(rempi_event_list<rempi_event*> *events, string id)
					: events(events), id(id) {};
		void complete_flush();
//		void consume(size_t);
};

#endif
