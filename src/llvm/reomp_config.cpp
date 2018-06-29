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

  if (!(env = getenv(REOMP_ENV_NAME_MODE))) {
    reomp_config.mode = REOMP_ENV_MODE_DISABLE;
  } else if (!strcmp(env, "record") || atoi(env) == REOMP_ENV_MODE_RECORD) {
    reomp_config.mode = REOMP_ENV_MODE_RECORD;
  } else if (!strcmp(env, "replay") || atoi(env) == REOMP_ENV_MODE_REPLAY) {
    reomp_config.mode = REOMP_ENV_MODE_REPLAY;
  } else if (!strcmp(env, "disable") || atoi(env) == REOMP_ENV_MODE_DISABLE) {
    reomp_config.mode = REOMP_ENV_MODE_DISABLE;
  } else {
    MUTIL_DBG("Unknown REOMP_MODE: %s", env);
    MUTIL_ERR("Set REOMP_MODE=<0(record)|1(replay)\n");
  }

  if (!(env = getenv(REOMP_ENV_NAME_METHOD))) {
    reomp_config.method = REOMP_ENV_METHOD_CLOCK;
  } else {
    reomp_config.method = atoi(env);
  }

  if (!(env = getenv(REOMP_ENV_NAME_PROFILE))) {
    reomp_config.profile_level = 0;
  } else {
    reomp_config.profile_level = atoi(env);
  }    

  if (!(env = getenv(REOMP_ENV_NAME_DIR))) {
    reomp_config.record_dir = (char*)".";
  } else {
    reomp_config.record_dir = env;
  }

  if (stat(reomp_config.record_dir, &st) != 0) {
    if (mkdir(reomp_config.record_dir, S_IRWXU) < 0) {
      if (errno != EEXIST) {
	MUTIL_ERR("Failded to make directory: path=%s: %s", reomp_config.record_dir, strerror(errno));
      }
    }
  }

  return;
}
