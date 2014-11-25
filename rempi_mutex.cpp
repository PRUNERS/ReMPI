#include "rempi_mutex.h"

rempi_mutex::rempi_mutex()
{
	pthread_mutex_init(&mutex, 0);
}

rempi_mutex::~rempi_mutex()
{
	pthread_mutex_destroy(&mutex);
}

void rempi_mutex::lock()
{
	//pthread_mutex_lock(&mutex);
}

void rempi_mutex::unlock()
{
	//pthread_mutex_unlock(&mutex);
}

bool rempi_mutex::trylock()
{
	return (pthread_mutex_trylock(&mutex) == 0);
}
