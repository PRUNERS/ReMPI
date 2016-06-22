#ifndef __REMPI_CP_H__
#define __REMPI_CP_H__

struct rempi_cp_prop_clock
{
  size_t next_clock;
  size_t send_count;
};

#endif


void rempi_cp_init(int input_length, int* input_pred_ranks);
int  rempi_cp_initialized();
void rempi_cp_gather_clocks();
void rempi_cp_finalize();


void rempi_cp_record_send(int dest_rank, size_t clock);
void rempi_cp_record_recv(int rank, size_t clock);
int  rempi_cp_has_in_flight_msgs(int source_rank);
void rempi_cp_set_next_clock(int clock);

