#ifndef __MUTEX_H__
#define __MUTEX_H__

#include <pthread.h>

class rempi_mutex
{
	private:
		pthread_mutex_t mutex;
	public:
		rempi_mutex();
		~rempi_mutex();
		void lock();
		void unlock();
		bool trylock();
};

#endif
