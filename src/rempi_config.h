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
#ifndef __REMPI_CONFIG_H__
#define __REMPI_CONFIG_H__

#include <string>

using namespace std;
/* #ifdef __cplusplus  */
/* extern "C" { */
/* #endif */

extern int rempi_lite;

#define REMPI_ENV_NAME_MODE "REMPI_MODE"
#define REMPI_ENV_REMPI_MODE_RECORD (0)
#define REMPI_ENV_REMPI_MODE_REPLAY (1)
extern int rempi_mode;

#define REMPI_ENV_NAME_DIR "REMPI_DIR"
#define REMPI_MAX_LENGTH_RECORD_DIR_PATH (4096)
extern string rempi_record_dir_path;

#define REMPI_ENV_NAME_ENCODE "REMPI_ENCODE"
#define REMPI_ENV_REMPI_ENCODE_BASIC (0)
#define REMPI_ENV_REMPI_ENCODE_CDC   (4)
#define REMPI_ENV_REMPI_ENCODE_REP   (7)
extern int rempi_encode;

#define REMPI_ENV_NAME_GZIP   "REMPI_GZIP"
extern int rempi_gzip;

#define REMPI_ENV_NAME_TEST_ID "REMPI_TEST_ID"
extern int rempi_is_test_id;

#define REMPI_ENV_NAME_MAX_EVENT_LENGTH "REMPI_MAX"
#define REMPI_DEFAULT_MAX_EVENT_LENGTH (1024 * 128)
extern int rempi_max_event_length;


//#define REMPI_DBG_REPLAY (-1)
#define REMPI_ENABLE_CORE_DUMP_ON_ERR
//#define REMPI_HYPRE_TEST


//#define REMPI_DBG_LOG
#define REMPI_DBG_LOG_BUF_SIZE (512)
#define REMPI_DBG_LOG_BUF_LENGTH (1024 * 1)


//#define REMPI_DBG_ASSERT (-1)


/*TODO: With REMPI_MULTI_THREAD + Multi epoch,
  If rempi_thread is in the middle of reading next epoch 
    while main_thraed accesses to variables of the epoch (input_table),
    main_thraed may read freeed variables.
*/
//#define REMPI_MULTI_THREAD
#define REMPI_MAIN_THREAD_PROGRESS


#define REMPI_MAX_RECV_TEST_ID (128)
#define REMPI_COMM_ID_LENGTH (128) /*TODO: If this size is 16, memory overwrite related weird bug happens*/

#define REMPI_MPI_TEST      (0)
#define REMPI_MPI_TESTANY   (1)
#define REMPI_MPI_TESTSOME  (2)
#define REMPI_MPI_TESTALL   (3)
#define REMPI_MPI_WAIT      (4)
#define REMPI_MPI_WAITANY   (5)
#define REMPI_MPI_WAITSOME  (6)
#define REMPI_MPI_WAITALL   (7)
#define REMPI_MPI_PROBE     (8)
#define REMPI_MPI_IPROBE    (9)
#define REMPI_MPI_ISEND      (10)
#define REMPI_MPI_IBSEND     (11)
#define REMPI_MPI_IRSEND     (12)
#define REMPI_MPI_ISSEND     (13)

#define REMPI_MATCHING_SET_ID_UNKNOWN (-1)
#define REMPI_MATCHING_SET_ID_IGNORE (-2)

/* #define REMPI_MPI_MF_WAIT      (1 << 0) /\*0x00000001*\/ */
/* #define REMPI_MPI_MF_TEST      (1 << 1) /\*0x00000010*\/ */
/* #define REMPI_MPI_MF_SINGLE    (1 << 2) */
/* #define REMPI_MPI_MF_ANY       (1 << 3) */
/* #define REMPI_MPI_MF_SOME      (1 << 4) */
/* #define REMPI_MPI_MF_ALL       (1 << 5) */


void rempi_set_configuration(int *argc, char ***argv);

/* #ifdef __cplusplus */
/* } */
/* #endif */

/*Tentative define*/
#define REMPI_TEST_ID_PROBE (2475283)

#endif
