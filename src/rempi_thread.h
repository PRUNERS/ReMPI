#ifndef __REMPI_THREAD_H__
#define __REMPI_THREAD_H__

#include <pthread.h>

class rempi_thread
{
	private:
		pthread_t thread;
		static void * dispatch(void *);
	protected:
		virtual void run() = 0;
	public:
		virtual ~rempi_thread();
		void start();
		void join();
};

#endif
