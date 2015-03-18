#ifndef __REMPI_CONFIG_H__
#define __REMPI_CONFIG_H__

#include <string>

using namespace std;
/* #ifdef __cplusplus  */
/* extern "C" { */
/* #endif */

#define REMPI_ENV_NAME_MODE "REMPI_MODE"
#define REMPI_ENV_REMPI_MODE_RECORD (0)
#define REMPI_ENV_REMPI_MODE_REPLAY (1)
extern int rempi_mode;

#define REMPI_ENV_NAME_DIR "REMPI_DIR"
#define REMPI_MAX_LENGTH_RECORD_DIR_PATH (256)
extern string rempi_record_dir_path;

//#define REMPI_DBG_REPLAY (2)

void rempi_set_configuration(int *argc, char ***argv);

/* #ifdef __cplusplus */
/* } */
/* #endif */

#endif
