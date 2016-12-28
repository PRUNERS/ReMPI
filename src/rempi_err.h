/* ==========================ReMPI:LICENSE==========================================   
   Copyright (c) 2016, Lawrence Livermore National Security, LLC.                     
   Produced at the Lawrence Livermore National Laboratory.                            
                                                                                       
   Written by Kento Sato, kento@llnl.gov. LLNL-CODE-711357.                           
   All rights reserved.                                                               
                                                                                       
   This file is part of ReMPI. For details, see https://github.com/PRUNER/ReMPI       
   Please also see the LICENSE file for our notice and the LGPL.                      
                                                                                       
   This program is free software; you can redistribute it and/or modify it under the   
   terms of the GNU General Public License (as published by the Free Software         
   Foundation) version 2.1 dated February 1999.                                       
                                                                                       
   This program is distributed in the hope that it will be useful, but WITHOUT ANY    
   WARRANTY; without even the IMPLIED WARRANTY OF MERCHANTABILITY or                  
   FITNESS FOR A PARTICULAR PURPOSE. See the terms and conditions of the GNU          
   General Public License for more details.                                           
                                                                                       
   You should have received a copy of the GNU Lesser General Public License along     
   with this program; if not, write to the Free Software Foundation, Inc., 59 Temple   
   Place, Suite 330, Boston, MA 02111-1307 USA                                   
   ============================ReMPI:LICENSE========================================= */
#ifndef __REMPI_ERR_H__
#define __REMPI_ERR_H__

#include <string>
#include <assert.h>

extern int rempi_my_rank;

#define REMPI_ASSERT(b)  \
  do {                \
    rempi_assert(b);        \
  } while(0)          \


#define REMPI_ALT(err_fmt, ...)  \
  rempi_alert(" "              \
	    err_fmt	     \
	    " (%s:%s:%d)",   \
            ## __VA_ARGS__,     \
            __FILE__, __func__, __LINE__);


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


#define REMPI_PRTI(i, dbg_fmt, ...)		\
  rempi_printi(i, " "					\
	      dbg_fmt,					\
	      ## __VA_ARGS__);


#define REMPI_PRT(dbg_fmt, ...)			\
  rempi_print(" "					\
	      dbg_fmt,					\
	      ## __VA_ARGS__);


#define REMPI_PREPRINT \
  //  REMPI_DBG("__func__: %s Entered", __func__);

#define REMPI_POSTPRINT \
  //  REMPI_DBG("__func__: %s Left", __func__);



void rempi_assert(int b);
void rempi_err_init(int r);
void rempi_err(const char* fmt, ...);
void rempi_erri(int r, const char* fmt, ...);
void rempi_alert(const char* fmt, ...);
void rempi_dbg_log_print();
void rempi_dbg(const char* fmt, ...);
void rempi_dbgi(int r, const char* fmt, ...);
void rempi_print(const char* fmt, ...);
void rempi_printi(int r, const char* fmt, ...);
void rempi_debug(const char* fmt, ...);
std::string rempi_btrace_string();
void rempi_btrace();
size_t rempi_btrace_hash();
char* rempi_err_mpi_msg(int err);

void rempi_exit(int no);


#endif
