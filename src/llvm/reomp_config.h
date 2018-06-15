#ifndef REOMP_CONFIG_H_
#define REOMP_CONFIG_H_

#define REOMP_MAX_THREAD (256)

#define REOMP_TIMER_MAIN       (0)
#define REOMP_TIMER_ENTIRE     (1)
#define REOMP_TIMER_GATE_TIME  (2)
#define REOMP_TIMER_IO_TIME    (3)

#define MODE "REOMP_MODE"
#define REOMP_DIR "REOMP_DIR"
#define REOMP_RECORD (0)
#define REOMP_REPLAY (1)
#define REOMP_DISABLE (2)

typedef struct {
  int   mode;
  char* record_dir;
} reomp_config_t;

extern reomp_config_t reomp_config;

void reomp_config_init();

#endif
