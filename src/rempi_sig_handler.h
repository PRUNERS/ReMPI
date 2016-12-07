#ifndef __REMPI_SIG_HANDLER_H__
#define __REMPI_SIG_HANDLER_H__

#include "rempi_event_list.h"
#include "rempi_io_thread.h"

void rempi_sig_handler_init(int rank, rempi_io_thread *record_thread, rempi_event_list<rempi_event*> *recording_event_list, size_t *validation_code);
void rempi_sig_handler_run(int signum);

void rempi_sig_handler_run_test(int signum);
void rempi_sig_handler_init_test();


#endif

