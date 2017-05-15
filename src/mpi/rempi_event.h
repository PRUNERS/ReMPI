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

#ifndef REMPI_EVENT_H_
#define REMPI_EVENT_H_

#include <mpi.h>
#include <iostream>
#include <vector>

#define REMPI_MPI_EVENT_TYPE_RECV (1)
#define REMPI_MPI_EVENT_TYPE_TEST (2)
#define REMPI_MPI_EVENT_TYPE_PROB (3)

#define REMPI_MPI_EVENT_RANK_CANCELED (MPI_ANY_SOURCE - 1)


// [source, is_testsome], <index>, clock 
#define REMPI_MPI_EVENT_INPUT_NUM (8)
#define REMPI_MPI_EVENT_INPUT_INDEX_EVENT_COUNT       (0)
#define REMPI_MPI_EVENT_INPUT_INDEX_TYPE              (1)
#define REMPI_MPI_EVENT_INPUT_INDEX_FLAG              (2)
#define REMPI_MPI_EVENT_INPUT_INDEX_RANK              (3)
#define REMPI_MPI_EVENT_INPUT_INDEX_WITH_NEXT         (4)
#define REMPI_MPI_EVENT_INPUT_INDEX_INDEX             (5)
#define REMPI_MPI_EVENT_INPUT_INDEX_MSG_ID            (6)
#define REMPI_MPI_EVENT_INPUT_INDEX_MATCHING_GROUP_ID (7)
/* #define REMPI_MPI_EVENT_INPUT_INDEX_COMM_ID      (2) */
/* #define REMPI_MPI_EVENT_INPUT_INDEX_TAG          (5) */

#define REMPI_MPI_EVENT_INPUT_IGNORE (0)
#define REMPI_MPI_EVENT_NOT_WITH_NEXT (0)
#define REMPI_MPI_EVENT_WITH_NEXT     (1)


using namespace std;

class rempi_event
{

  public:
    static int max_size;
    /*TODO: fix progrems below: 
      clock is size_t, but the other values are int
      for now, I use long to allocate 64bit
     */
    static int record_num;
    static int record_element_size;

    int clock_order; /*Ordered by clock when CDC compression is used */
    int msg_count; /*Actual message count from sender. This is used in copy_proxy_buf*/

    MPI_Request request;
    MPI_Datatype datatype;

    int tag;
    MPI_Comm comm;
    
    vector<int> mpi_inputs; /*TODO: use array insted of vector*/
    double ts;

    rempi_event()
      : clock_order(-1)
      , msg_count(-1)
      , request(NULL) {}

    ~rempi_event()
      {
	mpi_inputs.clear();
      }
     

    virtual void operator ++(int);
    virtual bool operator ==(rempi_event event);
    virtual void push(rempi_event* event);
    virtual rempi_event* pop();
    virtual int size();
    virtual char* serialize(size_t &size);
    
    virtual int get_event_counts();
    virtual int get_flag();
    virtual int get_source();
    virtual int get_is_testsome();
    virtual void set_with_next(long);
    virtual int get_clock();
    virtual int get_test_id();

    virtual int get_type();
    virtual int get_rank();
    virtual void set_rank(int rank);
    virtual int get_with_next();
    virtual int get_index();
    virtual void set_index(int index);
    virtual int get_msg_id();
    virtual int get_matching_group_id();

    virtual MPI_Request get_request();


    static rempi_event* create_recv_event(int source, int tag, MPI_Comm comm, MPI_Request *req);
    static rempi_event* create_test_event(int event_count, int flag, int rank, int with_next, int index, int msg_id, int matching_id);


    //    virtual int get_comm_id();


    //    virtual int get_tag();

    void print();
};


class rempi_irecv_event : public rempi_event
{
 public:
  rempi_irecv_event(int event_counts, int count, int source, int tag, int comm, int request);

};

class rempi_test_event : public rempi_event
{
 public:
  //  rempi_test_event(int event_count, int flag, int rank, int with_next, int index, int msg_id, int matching_id);
};

#endif /* REMPI_EVENT_H_ */
