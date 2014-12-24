#ifndef __REMPI_IO_THREAD_H__
#define __REMPI_IO_THREAD_H__

#include <string>
#include <fstream>

#include "rempi_event.h"
#include "rempi_event_list.h"
#include "rempi_thread.h"
#include "rempi_encoder.h"

using namespace std;

class rempi_io_thread : public rempi_thread
{
	private:
		static rempi_mutex mtx;
		/*1 if this MPI process finish pushing events*/
		int is_complete_flush;
		/*1 if this thread read reacord file to the end*/
		int read_finished;
		rempi_event_list<rempi_event*> *events;
		string id;
		string record_path;
		int mode;
		rempi_encoder *encoder;
	protected:
		virtual void run();
		void write_record();
		void read_record();
	public:
                rempi_io_thread(rempi_event_list<rempi_event*> *events, string id, int mode);
		void complete_flush();
		int  is_finished_read();
};

#endif
