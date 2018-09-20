#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h> 
#include <fcntl.h>
#include <omp.h>
#include <mpi.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <linux/limits.h>

#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <atomic>
#include <list>

#include "reomp.h"
#include "reomp_config.h"
#include "reomp_gate.h"
#include "reomp_mem.h"
#include "reomp_mon.h"
#include "reomp_profile.h"
#include "mutil.h"

#define REOMP_WITH_LOCK (1)
#define REOMP_WITHOUT_LOCK (2)

static reomp_gate_t *reomp_gate = NULL;
static double start_time, end_time;

void reomp_init(int control, size_t size)
{
  start_time = reomp_util_get_time();
  reomp_config_init();
  REOMP_PROFILE(reomp_profile_init());
  reomp_gate = reomp_gate_get(reomp_config.method);
  reomp_gate->init(control, size);
  return;
}

void reomp_finalize()
{
  reomp_gate->finalize();
  REOMP_PROFILE(reomp_profile_print());
  REOMP_PROFILE(reomp_profile_finalize());
  end_time = reomp_util_get_time();
  //  MUTIL_DBG("ReOMP time: %f", end_time - start_time);
  return;
}

char* reomp_get_rr_type_str(int rr_type)
{
  switch(rr_type) {
  case REOMP_RR_TYPE_NULL:
    return (char*)"null";
  case REOMP_RR_TYPE_NONE:
    return (char*)"none";
  case REOMP_RR_TYPE_MAIN:
    return (char*)"main";
  case REOMP_RR_TYPE_LOAD:
    return (char*)"load";
  case REOMP_RR_TYPE_STORE:
    return (char*)"store";
  case REOMP_RR_TYPE_REDUCTION:
    return (char*)"reduction";
  case REOMP_RR_TYPE_CRITICAL:
    return (char*)"critical";
  case REOMP_RR_TYPE_OMP_LOCK:
    return (char*)"omp_lock";
  case REOMP_RR_TYPE_OTHER_LOCK:
    return (char*)"other lock";
  case REOMP_RR_TYPE_SINGLE:
    return (char*)"single";
  case REOMP_RR_TYPE_MASTER:
    return (char*)"master";
  case REOMP_RR_TYPE_ATOMICOP:
    return (char*)"atomicop";
  case REOMP_RR_TYPE_ATOMICLOAD:
    return (char*)"atomic_load";
  case REOMP_RR_TYPE_ATOMICSTORE:
    return (char*)"atomic_store";
  case REOMP_RR_TYPE_CPP_STL:
    return (char*)"cpp_stl";
  default:
    return (char*)"No such rr_type";
  }
  return NULL;
}

static void reomp_gate_other(int control, void* ptr, size_t size)
{
  char rw = (size_t)ptr & 0xff;
  if (rw == 0) {
    MUTIL_DBG("__kmpc_fork_call begin");
  } else if (rw == 1) {
    MUTIL_DBG("__kmpc_fork_call end");
  }
  return;
}

void REOMP_CONTROL(int control, void* ptr, size_t size)
{
  if (reomp_config.mode == REOMP_ENV_MODE_DISABLE) return;
  if (control == REOMP_GATE_IN || control == REOMP_BEF_CRITICAL_BEGIN || control == REOMP_BEF_REDUCE_BEGIN) {
    REOMP_PROFILE(reomp_profile((size_t)ptr, size));
  }
  if (control == REOMP_GATE_IN || control == REOMP_GATE_OUT) {
    if (!reomp_config.multi_clock) size = 0;
  }


  switch(control) {
  case REOMP_BEF_MAIN: // 0
    reomp_init(control, size);
    break;
  case REOMP_AFT_MAIN: // 1
    reomp_finalize();
    break;
  case REOMP_AFT_MPI_INIT: // 2
    int flag;
    int my_rank;
    if (reomp_config.method == REOMP_ENV_METHOD_TID)reomp_gate->init(control, size);
    MPI_Initialized(&flag);
    if (flag) {
      MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    } else {
      MUTIL_ERR("MPI is not initialized yet");
    }
    reomp_util_init(my_rank);    
    break;
    
  case REOMP_GATE_IN: // 10
    reomp_gate->in(control, ptr, size, REOMP_WITH_LOCK);
    break;
  case REOMP_GATE_OUT: // 11
    reomp_gate->out(control, ptr, size, REOMP_WITH_LOCK);
    break;
  case REOMP_GATE: // 12
    break;
    
  case REOMP_BEF_CRITICAL_BEGIN: // 13
    reomp_gate->in(control, ptr, size, REOMP_WITHOUT_LOCK);
    break;
  case REOMP_AFT_CRITICAL_BEGIN: // 14
    break;
  case REOMP_AFT_CRITICAL_END: // 15
    reomp_gate->out(control, ptr, size, REOMP_WITHOUT_LOCK);
    break;
    
  case REOMP_BEF_REDUCE_BEGIN: // 16
    reomp_gate->in_brb(control, ptr, size);
    break;
  case REOMP_AFT_REDUCE_BEGIN: // 17
    reomp_gate->in_arb(control, ptr, size);
    break;
  case REOMP_BEF_REDUCE_END:   // 18
    reomp_gate->out_bre(control, ptr, size);
    break;
  case REOMP_AFT_REDUCE_END:   // 19
    reomp_gate->out_are(control, ptr, size);
    break;
  case REOMP_OTHER:
    reomp_gate_other(control, ptr, size);
    break;
  default:
    MUTIL_ERR("No such control: %d", control);
  }
  return;
}



