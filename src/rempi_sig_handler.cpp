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

sighandler_t rempi_sig_handler_sigsegv_default;

static void rempi_sig_handler_ignore(int signum)
{
  return;
}

void rempi_sig_handler_run(int signum)
{
  if (signal(12, rempi_sig_handler_ignore) == SIG_ERR) {
    REMPI_ERR("failed");
  }

  REMPI_DBG("Get sigmal: %d", signum);
  REMPI_PRT("Dumping record file");
  registered_recording_event_list->push_all();
  REMPI_PRT("Syncing with IO thread");
  registered_record_thread->join();  
  //  if (registered_my_rank == 0) REMPI_PRT("Calling MPI_Finalize");
  //  PMPI_Finalize();

  REMPI_PRT(" ... done");
  // if (registered_my_rank == 0) {
  //   REMPI_DBG(" Valication: %lu", *registered_validation_code);
  // }
  sleep(1);
  if (signum == SIGSEGV) {
    rempi_sig_handler_sigsegv_default(SIGSEGV);  
  } else {
    REMPI_DBG("exit ...");
    exit(1);
  }
  return;
}


void rempi_sig_handler_init(int rank, rempi_io_thread *record_thread, rempi_event_list<rempi_event*> *recording_event_list, unsigned int *validation_code)
{
  registered_my_rank = rank;
  registered_record_thread = record_thread;
  registered_recording_event_list = recording_event_list;
  registered_validation_code = validation_code;
  if (signal(12, rempi_sig_handler_run) == SIG_ERR) {
    REMPI_ERR("signal failed 12");
  }
  rempi_sig_handler_sigsegv_default = signal(SIGSEGV, rempi_sig_handler_run);
  if (rempi_sig_handler_sigsegv_default == SIG_ERR) {
    REMPI_ERR("signal failed %d", SIGSEGV);
  }
  return;
}

#if 0
void rempi_sig_handler_run_test(int signum)
{
  
  REMPI_DBG("Got sigmal: %d", signum);
  sleep(2);
  exit(1);
  return;
}

void rempi_sig_handler_init_test()
{
  if (signal(12, rempi_sig_handler_run_test) == SIG_ERR) {
    REMPI_ERR("failed");
  }
  return;
}
#endif



