#ifndef REOMP_CONFIG_H_
#define REOMP_CONFIG_H_

#define REOMP_MAX_THREAD (256)

#define REOMP_TIMER_MAIN       (0)
#define REOMP_TIMER_ENTIRE     (1)
#define REOMP_TIMER_GATE_TIME  (2)
#define REOMP_TIMER_IO_TIME    (3)

#define REOMP_ENV_NAME_MODE   "REOMP_MODE"
#define REOMP_ENV_MODE_RECORD (0)
#define REOMP_ENV_MODE_REPLAY (1)
#define REOMP_ENV_MODE_DISABLE (2)
#define REOMP_ENV_NAME_METHOD "REOMP_METHOD"
#define REOMP_ENV_METHOD_CLOCK (0)
#define REOMP_ENV_METHOD_SCLOCK (1)
#define REOMP_ENV_METHOD_TID   (2)  
#define REOMP_ENV_NAME_DIR "REOMP_DIR"


typedef struct {
  int   mode;
  int method;
  char* record_dir;
} reomp_config_t;

extern reomp_config_t reomp_config;

void reomp_config_init();

#endif
