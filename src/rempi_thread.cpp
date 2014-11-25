#include "rempi_thread.h"

rempi_thread::~rempi_thread()
{}

void * rempi_thread::dispatch(void * ptr)
{
	if (!ptr) return 0;
	static_cast<rempi_thread *>(ptr)->run();
	pthread_exit(ptr);
	return 0;
}

void rempi_thread::start()
{
	pthread_create(&thread, 0, rempi_thread::dispatch, this);
}

void rempi_thread::join()
{
	pthread_join(thread, 0);
}
