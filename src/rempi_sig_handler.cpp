#include <signal.h>
#include <sys/types.h>
#include <unistd.h>
#include <mpi.h>

#include "rempi_event_list.h"
#include "rempi_io_thread.h"
#include "rempi_err.h"
#include "rempi_config.h"

rempi_io_thread *registered_record_thread;
rempi_event_list<rempi_event*> *registered_recording_event_list;
unsigned int *registered_validation_code;
int registered_my_rank;

void rempi_sig_handler_run(int signum)
{
  
  REMPI_DBG("Get sigmal");
  if (registered_my_rank == 0) REMPI_PRT("Dumping record file");
  registered_recording_event_list->push_all();
  if (registered_my_rank == 0) REMPI_PRT("Syncing with IO thread");
  registered_record_thread->join();  
  //  if (registered_my_rank == 0) REMPI_PRT("Calling MPI_Finalize");
  //  PMPI_Finalize();

  if (registered_my_rank == 0) REMPI_PRT(" ... done");
  sleep(2);
  exit(0);
  return;
}

void rempi_sig_handler_init(int rank, rempi_io_thread *record_thread, rempi_event_list<rempi_event*> *recording_event_list, unsigned int *validation_code)
{
  registered_my_rank = rank;
  registered_record_thread = record_thread;
  registered_recording_event_list = recording_event_list;
  registered_validation_code = validation_code;
  signal(12, rempi_sig_handler_run);
  return;
}



