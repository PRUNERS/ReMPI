#ifndef __REMPI_CP_H__
#define __REMPI_CP_H__

struct rempi_cp_prop_clock
{
  size_t clock;
  size_t send_count;
};

#endif


//void rempi_cp_init(int input_length, int* input_pred_ranks);
void rempi_cp_init(const char* record_path);
int  rempi_cp_initialized();
void rempi_cp_gather_clocks();
void rempi_cp_finalize();


void rempi_cp_record_send(int dest_rank, size_t clock);
void rempi_cp_record_recv(int rank, size_t clock);
int  rempi_cp_has_in_flight_msgs(int source_rank);

size_t rempi_cp_get_gather_clock(int source_rank);
size_t rempi_cp_get_gather_send_count(int source_rank);
void   rempi_cp_set_scatter_clock(size_t clock);
size_t rempi_cp_get_scatter_clock(void);




