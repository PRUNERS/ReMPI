#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <iostream>

#include "rempi_err.h"
#include "rempi_config.h"

using namespace std;

int rempi_mode = 0;
string rempi_record_dir_path = ".";
int rempi_encode;
int rempi_gzip;
int rempi_is_test_id;

void rempi_set_configuration(int *argc, char ***argv)
{
  char* env;
  int env_int;

  if (NULL == (env = getenv(REMPI_ENV_NAME_MODE))) {
    rempi_dbgi(0, "getenv failed: Please specify REMPI_MODE (%s:%s:%d)", __FILE__, __func__, __LINE__);
    exit(0);
  }
  env_int = atoi(env);
  if (env_int == 0) {
    rempi_mode = 0;
  } else if (env_int == 1) {
    rempi_mode = 1;
  } else {
    rempi_dbgi(0, "Invalid value for REMPI_MODE: %s (%s:%s:%d)", env, __FILE__, __func__, __LINE__);
    exit(0);
  }

  if (NULL == (env = getenv(REMPI_ENV_NAME_DIR))) {
    // defoult: current dir
    rempi_record_dir_path =  ".";
  } else {
    rempi_record_dir_path = env;
  }

  if (NULL == (env = getenv(REMPI_ENV_NAME_ENCODE))) {
    rempi_dbgi(0, "getenv failed: Please specify REMPI_ENCODE (%s:%s:%d)", __FILE__, __func__, __LINE__);
    exit(0);
  } else {
    env_int = atoi(env);
    rempi_encode = env_int;
   }

  if (NULL == (env = getenv(REMPI_ENV_NAME_GZIP))) {
    rempi_dbgi(0, "getenv failed: Please specify REMPI_GZIP (%s:%s:%d)", __FILE__, __func__, __LINE__);
    exit(0);
  } else {
    env_int = atoi(env);
    rempi_gzip = env_int;
  }

  if (NULL == (env = getenv(REMPI_ENV_NAME_TEST_ID))) {
    rempi_dbgi(0, "getenv failed: Please specify REMPI_TEST_ID (%s:%s:%d)", __FILE__, __func__, __LINE__);
    exit(0);
  } else {
    env_int = atoi(env);
    rempi_is_test_id = env_int;
  }

  if (rempi_mode == 1 && rempi_is_test_id == 1) {
    REMPI_DBGI(0, "REMPI_MODE=1 & REMPI_IS_TEST_IS=1 is not supported yet");
  }


  REMPI_DBGI(0, "mode  :  %d", rempi_mode);
  REMPI_DBGI(0, "dir   :  %s", rempi_record_dir_path.c_str());
  REMPI_DBGI(0, "encode:  %d", rempi_encode);
  REMPI_DBGI(0, "gzip  :  %d", rempi_gzip);
  REMPI_DBGI(0, "testid:  %d", rempi_is_test_id);

  return;
}


