#ifndef __REMPI_ERR_H__
#define __REMPI_ERR_H__

void rempi_err_init(int r);
void rempi_err(const char* fmt, ...);
void rempi_erri(int r, const char* fmt, ...);
void rempi_alert(const char* fmt, ...);
void rempi_dbg(const char* fmt, ...);
void rempi_dbgi(int r, const char* fmt, ...);
void rempi_print(const char* fmt, ...);
void rempi_debug(const char* fmt, ...);
void rempi_btrace();

void rempi_exit(int no);

#endif
