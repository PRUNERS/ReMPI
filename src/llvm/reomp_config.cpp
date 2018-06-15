#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#include "reomp_config.h"
#include "mutil.h"

reomp_config_t reomp_config;

void reomp_config_init()
{
  char *env;
  struct stat st;

  if (!(env = getenv(REOMP_DIR))) {
    reomp_config.record_dir = (char*)".";
  } else {
    reomp_config.record_dir = env;
  }

  if (stat(reomp_config.record_dir, &st) != 0) {
    if (mkdir(reomp_config.record_dir, S_IRWXU) < 0) {
      MUTIL_ERR("Failded to make directory: %s", reomp_config.record_dir);
    }
  }

  if (!(env = getenv(MODE))) {
    reomp_config.mode = REOMP_DISABLE;
  } else if (!strcmp(env, "record") || atoi(env) == REOMP_RECORD) {
    reomp_config.mode = REOMP_RECORD;
  } else if (!strcmp(env, "replay") || atoi(env) == REOMP_REPLAY) {
    reomp_config.mode = REOMP_REPLAY;
  } else if (!strcmp(env, "disable") || atoi(env) == REOMP_DISABLE) {
    reomp_config.mode = REOMP_DISABLE;
  } else {
    MUTIL_DBG("Unknown REOMP_MODE: %s", env);
    MUTIL_ERR("Set REOMP_MODE=<0(record)|1(replay)\n");
  }
  return;
}
