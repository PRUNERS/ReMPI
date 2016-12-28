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
#ifndef __REMPI_MESSAGE_MANAGER_H__
#define __REMPI_MESSAGE_MANAGER_H__

#include <unordered_map>
#include <list>

#include "mpi.h"


using namespace std;

class rempi_message_identifier
{

  
 public:
  int source;
  int tag;
  int comm_id; 
  int test_id; 

  /* int clock;  */
  /* rempi_message_identifier(int source, int tag, int comm_id, int clock, int test_id):  */
  /*  source(source), tag(tag), comm_id(comm_id), clock(clock), test_id(test_id){} */

 rempi_message_identifier(int source, int tag, int comm_id, int test_id):
  source(source), tag(tag), comm_id(comm_id), test_id(test_id){}
  
  size_t compress(size_t source_value, size_t source_bits, size_t tag_value, size_t tag_bits, size_t comm_id_value, size_t comm_id_bits);
  string to_string();
  bool operator==(rempi_message_identifier &msg_id);
  bool operator<(rempi_message_identifier &msg_id);
  bool operator>(rempi_message_identifier &msg_id);
};

class rempi_message_manager
{
 private:
  /*Manages MPI_Requests being used by an MPI_Irecv, but not matche yet*/
  unordered_map<MPI_Request*, rempi_message_identifier*> pending_recvs;
  /*Manages matched massages*/
  list<rempi_message_identifier*> matched_recvs;

 public:
  void add_pending_recv(MPI_Request *request, int source, int tag, int comm_id);
  int  query_pending_comm_id(MPI_Request *request);
  int  add_matched_recv(MPI_Request *request, int source, int tag, int test_id);
  bool is_matched_recv(int source, int tag, int comm_id, int test_id);
  void refresh_matched_recv(int test_id);
  void remove_matched_recv(int source, int tag, int comm_id, int test_id);
  void print_pending_recv();
  void print_matched_recv();
};

#endif
