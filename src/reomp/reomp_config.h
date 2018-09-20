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
#define REOMP_ENV_METHOD_CLOCK_CONCURRENT (0)
#define REOMP_ENV_METHOD_CLOCK (1)
#define REOMP_ENV_METHOD_TID   (2)

#define REOMP_ENV_METHOD_SCLOCK (3)
#define REOMP_ENV_NAME_MULTI_CLOCK "REOMP_MULTI_CLOCK"
#define REOMP_ENV_NAME_PROFILE "REOMP_PROFILE"
#define REOMP_ENV_NAME_DIR "REOMP_DIR"

#define REOMP_PATH_FORMAT ("%s/rank_%d-tid_%d.reomp")


typedef struct {
  int   mode;
  int method;
  int profile_level = 0;
  char* record_dir;
  int multi_clock;
} reomp_config_t;

extern reomp_config_t reomp_config;

void reomp_config_init();

#endif
