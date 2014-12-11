#ifndef __REMPI_CONFIG_H__
#define __REMPI_CONFIG_H__

#define REMPI_ENV_NAME_MODE "REMPI_MODE"
#define REMPI_ENV_REMPI_MODE_RECORD (0)
#define REMPI_ENV_REMPI_MODE_REPLAY (1)

extern int rempi_mode;
void rempi_set_configuration();

#endif
