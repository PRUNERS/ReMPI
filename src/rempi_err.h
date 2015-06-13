#ifndef __REMPI_ERR_H__
#define __REMPI_ERR_H__

#include <string>

/*if __VA_ARGS__ is empty, the previous comma can be removed by "##" statement*/
#define REMPI_ERR(err_fmt, ...)  \
  rempi_err(" "              \
	    err_fmt	     \
	    " (%s:%s:%d)",   \
            ## __VA_ARGS__,     \
            __FILE__, __func__, __LINE__);

/*if __VA_ARGS__ is empty, the previous comma can be removed by "##" statement*/
#define REMPI_DBG(dbg_fmt, ...)			\
  rempi_dbg(" "					\
	    dbg_fmt				\
	    " (%s:%d)",			\
            ## __VA_ARGS__,			\
            __FILE__, __LINE__);
  //            __FILE__, __func__, __LINE__);


/*if __VA_ARGS__ is empty, the previous comma can be removed by "##" statement*/
#define REMPI_DBGI(i, dbg_fmt, ...)		\
  rempi_dbgi(i,					\
	     " "				\
	     dbg_fmt				\
	     " (%s:%d)",			\
	     ## __VA_ARGS__,			\
            __FILE__, __LINE__);
//	     __FILE__, __func__, __LINE__);


void rempi_err_init(int r);
void rempi_err(const char* fmt, ...);
void rempi_erri(int r, const char* fmt, ...);
void rempi_alert(const char* fmt, ...);
void rempi_dbg(const char* fmt, ...);
void rempi_dbgi(int r, const char* fmt, ...);
void rempi_print(const char* fmt, ...);
void rempi_debug(const char* fmt, ...);
std::string rempi_btrace_string();
void rempi_btrace();


void rempi_exit(int no);

#endif
