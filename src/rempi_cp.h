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
#ifndef __REMPI_CP_H__
#define __REMPI_CP_H__

struct rempi_cp_prop_clock
{
  size_t clock;
  size_t send_count;
};




void rempi_cp_init(const char* record_path);
int  rempi_cp_initialized();
void rempi_cp_finalize();


/* ===============
   rempi_cp_record_send/recv must be called whenever sending/receving messages 
   to keep track of how many messages are sent/received out

   For rempi_cp_record_send:
     - MPI_Send/Bsend/Rsend/Ssend
     - MPI_Isend/Ibsend/Irsend/Issend
     - MPI_Start/Startall for sending

   For rempi_cp_record_recv:
     - MPI_Wait/Waitany/Waitsome/Waitall
     - MPI_Test/Testany/Testsome/Testall (when messages matched)
     
   so that rempi_cp_has_in_flight_msgs function find out if in-flight messages exist or not
*/ 
void rempi_cp_record_send(int dest_rank, size_t clock);
void rempi_cp_record_recv(int rank, size_t clock);
int  rempi_cp_has_in_flight_msgs(int source_rank);
/* =============== */


/* MPI_Get to fetch look-ahead send clocks */
void rempi_cp_gather_clocks();
/* Get local look-ahead receive clock fetched by rempi_cp_gather_clocks() */
size_t rempi_cp_get_gather_clock(int source_rank);
/* Get local send count            fetched by rempi_cp_gather_clocks() */
size_t rempi_cp_get_gather_send_count(int source_rank);

/* set look-ahead send clock*/
void   rempi_cp_set_scatter_clock(size_t clock);
/* get look-ahead send clock*/
size_t rempi_cp_get_scatter_clock(void);

/* get sender_list */
int *rempi_cp_get_pred_ranks(size_t *length);

#endif



