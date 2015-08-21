#include "rempi_mutex.h"
#include "rempi_err.h"
#include "rempi_config.h"

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
#ifdef REMPI_DBG_REPLAY
  //  REMPI_DBGI(REMPI_DBG_REPLAY, ">>>>> try lock");
#endif
  pthread_mutex_lock(&mutex);
#ifdef REMPI_DBG_REPLAY
  //  REMPI_DBGI(REMPI_DBG_REPLAY, ">>>>> lock");
#endif
}

void rempi_mutex::unlock()
{
  pthread_mutex_unlock(&mutex);
#ifdef REMPI_DBG_REPLAY
  //  REMPI_DBGI(REMPI_DBG_REPLAY, "<<<<<< unlock");
#endif
}

bool rempi_mutex::trylock()
{
  return (pthread_mutex_trylock(&mutex) == 0);
}
