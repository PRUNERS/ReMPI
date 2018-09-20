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
#ifndef __REMPI_MSGB_H__
#define __REMPI_MSGB_H__

#include <mpi.h>

#include "rempi_type.h"
#include "rempi_event_list.h"

#define REMPI_MSGB_REQUEST_MATCHED_TYPE_NOT_MATCHED (0)
#define REMPI_MSGB_REQUEST_MATCHED_TYPE_MATCHED     (1)
#define REMPI_MSGB_REQUEST_MATCHED_TYPE_SAME        (2)

int rempi_msgb_is_matched(int requested_rank, int requested_tag, MPI_Comm requested_comm, int actual_rank, int actual_tag, MPI_Comm actual_comm);

int rempi_msgb_init(rempi_event_list<rempi_event*> *send_eq, rempi_event_list<rempi_event*> *recv_eq);
MPI_Request rempi_msgb_allocate_request(int request_type);
int rempi_msgb_register_recv(void *buf, int count, MPI_Datatype datatype, int source,
			     int tag, MPI_Comm comm, MPI_Request *request, int matching_set_id);
int rempi_msgb_progress_recv();
int rempi_msgb_cancel_request(MPI_Request *request);
int rempi_msgb_recv_msg(void* dest_buffer, int replayed_rank, int requested_tag, MPI_Comm requested_comm, size_t clock, int matcing_set_id, MPI_Status *replaying_status);
MPI_Status rempi_msgb_get_inactive_status(int source, int tag, MPI_Comm comm);
int rempi_msgb_probe_msg(int source, int tag, MPI_Comm comm, int *flag, MPI_Status *status);
int rempi_msgb_get_tag_comm(int matched_rank, size_t matched_clock, int *output_tag, MPI_Comm *output_comm);

int rempi_msgb_has_recved_msg(int source);
int rempi_msgb_has_probed_msg(int source);
size_t rempi_msgb_get_max_recved_clock(int source);


#endif

