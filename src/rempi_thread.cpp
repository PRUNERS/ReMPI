#include "rempi_thread.h"
#include "rempi_config.h"
#include "rempi_err.h"

rempi_thread::~rempi_thread()
{}

void * rempi_thread::dispatch(void * ptr)
{
  if (!ptr) {
    REMPI_ERR("thread object failed: ptr = %p", ptr);
    return 0;
  }
  static_cast<rempi_thread *>(ptr)->run();
  pthread_exit(ptr);
  return 0;
}

void rempi_thread::start()
{
  int ret;
  ret = pthread_create(&thread, 0, rempi_thread::dispatch, this);
  if (ret != 0) REMPI_ERR("pthread_create failed: ret = %d", ret);
}

void rempi_thread::join()
{
  pthread_join(thread, 0);
}
