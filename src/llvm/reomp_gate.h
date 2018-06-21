#ifndef REOMP_GATE_H_
#define REOMP_GATE_H_

//#include "reomp_gate_clock.h"

typedef void (*reomp_gate_init_t)(int control, size_t num_locks);
typedef void (*reomp_gate_finalize_t)(void);
typedef void (*reomp_gate_in_t)(int control, void* ptr, size_t lock_id, int lock);
typedef void (*reomp_gate_out_t)(int control, void* ptr, size_t lock_id, int lock);
typedef void (*reomp_gate_in_bef_reduce_begin_t)(int control, void* ptr, size_t null);
typedef void (*reomp_gate_in_aft_reduce_begin_t)(int control, void* ptr, size_t reduction_method);
typedef void (*reomp_gate_out_bef_reduce_end_t)(int control, void* ptr, size_t reduction_method);
typedef void (*reomp_gate_out_aft_reduce_end_t)(int control, void* ptr, size_t reduction_method);
  
typedef struct {
  reomp_gate_init_t         init;
  reomp_gate_finalize_t finalize;
  reomp_gate_in_t  in;
  reomp_gate_out_t out;
  reomp_gate_in_bef_reduce_begin_t in_brb;
  reomp_gate_in_aft_reduce_begin_t in_arb;
  reomp_gate_out_bef_reduce_end_t out_bre;
  reomp_gate_out_aft_reduce_end_t out_are;
} reomp_gate_t;

extern reomp_gate_t reomp_gate_clock;
extern reomp_gate_t reomp_gate_sclock;
extern reomp_gate_t reomp_gate_tid;


reomp_gate_t* reomp_gate_get(int method);

#endif

