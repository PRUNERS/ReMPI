#include <stdlib.h>
#include <stdio.h>

#include "reomp_config.h"
#include "reomp_gate.h"
#include "mutil.h"

reomp_gate_t* reomp_gate_get(int method)
{
  switch(method) {
  case REOMP_ENV_METHOD_CLOCK_CONCURRENT:
    return &reomp_gate_cclock;
  case REOMP_ENV_METHOD_CLOCK:
    return &reomp_gate_clock;
  case REOMP_ENV_METHOD_SCLOCK:
    return &reomp_gate_sclock;
  case REOMP_ENV_METHOD_TID:
    return &reomp_gate_tid;
  default:
    MUTIL_DBG("No such method: %s=%d", REOMP_ENV_NAME_METHOD, method);
  }
  return NULL;
}
