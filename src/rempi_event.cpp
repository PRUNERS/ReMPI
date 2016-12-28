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
#include <iostream>
#include <vector>

#include "rempi_event.h"
#include "rempi_err.h"
#include "rempi_util.h"


using namespace std;

int rempi_event::max_size   = sizeof(int) * REMPI_MPI_EVENT_INPUT_NUM;
int rempi_event::record_num = REMPI_MPI_EVENT_INPUT_NUM;
int rempi_event::record_element_size = sizeof(int);

void rempi_event::operator ++(int) {
  mpi_inputs[REMPI_MPI_EVENT_INPUT_INDEX_EVENT_COUNT]++;
}

bool rempi_event::operator ==(rempi_event event)
{
  

  //    if (this->mpi_inputs.size() != event.mpi_inputs.size())
  //      REMPI_ERR("size is different");
  //    }

  if (this->get_type() == REMPI_MPI_EVENT_TYPE_TEST &&
      event.get_type() == REMPI_MPI_EVENT_TYPE_TEST) {
    for (unsigned int i = 1; i < mpi_inputs.size() - 1; i++) {
      if (this->mpi_inputs[i] != event.mpi_inputs[i]) {
	return false;
      }
    }
    return true;
  }
  return false;
}

void rempi_event::push(rempi_event* event)
{
  /*TODO: Implement as a replacement of operator++*/
}

rempi_event* rempi_event::pop()
{
  rempi_event *event;
  /*TODO: Implement as a replacement of operator++*/

  /*XXX: Assuming the all recorded event is Test. 
    so return rempi_event_test, but this is resonable assumption*/
  if (mpi_inputs[REMPI_MPI_EVENT_INPUT_INDEX_EVENT_COUNT] < 0) {
    REMPI_ERR("Event count < 0 error");
  }
  if (this->get_type() == REMPI_MPI_EVENT_TYPE_RECV) {
    if (this->get_event_counts() != 1){
      REMPI_ERR("Recv event has more than one: %d", this->get_event_counts());
    }
    event = this;
  } else if (this->get_type() == REMPI_MPI_EVENT_TYPE_TEST) {
    /*TODO: if size = 1, then return this*/
    if (this->get_event_counts() == 1){
      event = this;
    } else {
      event = rempi_event::create_test_event(1, 
					     this->get_flag(),
					     this->get_rank(), 
					     this->get_with_next(), 
					     this->get_index(), 
					     this->get_msg_id(),
					     this->get_matching_group_id());
      event->clock_order = clock_order;
      event->msg_count = msg_count;
      mpi_inputs[REMPI_MPI_EVENT_INPUT_INDEX_EVENT_COUNT]--;
    }

  } else {
    REMPI_ERR("No such event type: %d", this->get_type());
  }

  return event;
}

int rempi_event::size()
{
  return mpi_inputs[REMPI_MPI_EVENT_INPUT_INDEX_EVENT_COUNT];
}

char* rempi_event::serialize(size_t &size)
{
    int *serialized_data = new int[mpi_inputs.size()];
    for (unsigned int i = 0; i < mpi_inputs.size(); i++) {
      serialized_data[i] = mpi_inputs[i];
    }
    size = mpi_inputs.size() * sizeof(int);
    return (char*)serialized_data;
}

void rempi_event::print() 
{
  fprintf(stderr, "<Event: ");
  for (int i = 0; i < mpi_inputs.size(); i++) {
    fprintf(stderr, "%d \t", (int)mpi_inputs[i]);
  }
  fprintf(stderr, ">");
  return;
}

int rempi_event::get_event_counts() 
{
  return mpi_inputs[REMPI_MPI_EVENT_INPUT_INDEX_EVENT_COUNT];
}   

int rempi_event::get_flag()
{
  return mpi_inputs[REMPI_MPI_EVENT_INPUT_INDEX_FLAG];
}

int rempi_event::get_source()
{
  return mpi_inputs[REMPI_MPI_EVENT_INPUT_INDEX_RANK];
}

int rempi_event::get_rank()
{
  return mpi_inputs[REMPI_MPI_EVENT_INPUT_INDEX_RANK];
}


int rempi_event::get_is_testsome() 
{
  return mpi_inputs[REMPI_MPI_EVENT_INPUT_INDEX_WITH_NEXT];
}

int rempi_event::get_with_next() 
{
  return mpi_inputs[REMPI_MPI_EVENT_INPUT_INDEX_WITH_NEXT];
}

void rempi_event::set_with_next(long with_next) 
{
  mpi_inputs[REMPI_MPI_EVENT_INPUT_INDEX_WITH_NEXT] = with_next;
}



// int rempi_event::get_comm_id()
// {
//   return mpi_inputs[REMPI_MPI_EVENT_INPUT_INDEX_COMM_ID];
// }

MPI_Request rempi_event::get_request()
{
  if (this->get_type() == REMPI_MPI_EVENT_TYPE_RECV) {
    return this->request;
  }
  REMPI_ERR("event_type: %d does not have request", this->get_type());
  return NULL;
}

void rempi_event::set_rank(int rank)
{
  mpi_inputs[REMPI_MPI_EVENT_INPUT_INDEX_RANK] = rank;
  return;
}


// int rempi_event::get_tag()
// {
//   return mpi_inputs[REMPI_MPI_EVENT_INPUT_INDEX_TAG];
// }

int rempi_event::get_clock()
{
  return mpi_inputs[REMPI_MPI_EVENT_INPUT_INDEX_MSG_ID];
}

int rempi_event::get_type()
{
  return mpi_inputs[REMPI_MPI_EVENT_INPUT_INDEX_TYPE];
}

int rempi_event::get_index()
{
  return mpi_inputs[REMPI_MPI_EVENT_INPUT_INDEX_INDEX];
}

void rempi_event::set_index(int index)
{
  mpi_inputs[REMPI_MPI_EVENT_INPUT_INDEX_INDEX] = index;
  return;
}

int rempi_event::get_msg_id()
{
  return mpi_inputs[REMPI_MPI_EVENT_INPUT_INDEX_MSG_ID];
}

int rempi_event::get_test_id()
{
  return mpi_inputs[REMPI_MPI_EVENT_INPUT_INDEX_MATCHING_GROUP_ID];
}

int rempi_event::get_matching_group_id()
{
  return mpi_inputs[REMPI_MPI_EVENT_INPUT_INDEX_MATCHING_GROUP_ID];
}


rempi_event* rempi_event::create_recv_event(int source, int tag, MPI_Comm comm, MPI_Request *req)
{
  rempi_event* e;
  e = new rempi_event();
  e->mpi_inputs.resize(REMPI_MPI_EVENT_INPUT_NUM);
  e->mpi_inputs[REMPI_MPI_EVENT_INPUT_INDEX_EVENT_COUNT]       = 1;
  e->mpi_inputs[REMPI_MPI_EVENT_INPUT_INDEX_TYPE]              = REMPI_MPI_EVENT_TYPE_RECV;
  e->mpi_inputs[REMPI_MPI_EVENT_INPUT_INDEX_FLAG]              = REMPI_MPI_EVENT_INPUT_IGNORE;
  e->mpi_inputs[REMPI_MPI_EVENT_INPUT_INDEX_RANK]              = source;
  e->mpi_inputs[REMPI_MPI_EVENT_INPUT_INDEX_WITH_NEXT]         = REMPI_MPI_EVENT_INPUT_IGNORE;
  e->mpi_inputs[REMPI_MPI_EVENT_INPUT_INDEX_INDEX]             = REMPI_MPI_EVENT_INPUT_IGNORE;
  e->mpi_inputs[REMPI_MPI_EVENT_INPUT_INDEX_MSG_ID]            = REMPI_MPI_EVENT_INPUT_IGNORE;
  e->mpi_inputs[REMPI_MPI_EVENT_INPUT_INDEX_MATCHING_GROUP_ID] = REMPI_MPI_EVENT_INPUT_IGNORE;
  if (req != NULL) {
    e->request = *req;
  }
  return e;
}

rempi_event* rempi_event::create_test_event(int event_count, int flag, int rank, int with_next, int index, int msg_id, int matching_id)
{
  rempi_event* e;
  e = new rempi_event();
  e->mpi_inputs.resize(REMPI_MPI_EVENT_INPUT_NUM);
  e->mpi_inputs[REMPI_MPI_EVENT_INPUT_INDEX_EVENT_COUNT]       = event_count;
  e->mpi_inputs[REMPI_MPI_EVENT_INPUT_INDEX_TYPE]              = REMPI_MPI_EVENT_TYPE_TEST;
  e->mpi_inputs[REMPI_MPI_EVENT_INPUT_INDEX_FLAG]              = flag;
  e->mpi_inputs[REMPI_MPI_EVENT_INPUT_INDEX_RANK]              = rank;
  e->mpi_inputs[REMPI_MPI_EVENT_INPUT_INDEX_WITH_NEXT]         = with_next;
  e->mpi_inputs[REMPI_MPI_EVENT_INPUT_INDEX_INDEX]             = index;
  e->mpi_inputs[REMPI_MPI_EVENT_INPUT_INDEX_MSG_ID]            = msg_id;
  e->mpi_inputs[REMPI_MPI_EVENT_INPUT_INDEX_MATCHING_GROUP_ID] = matching_id;
  //  e->ts = rempi_get_time();
  return e;
}


/*====== child class constructures ======*/

rempi_irecv_event::rempi_irecv_event(int event_counts, int count, int source, int tag, int comm, int request)
  : rempi_event()
{
    mpi_inputs.push_back(event_counts);
    mpi_inputs.push_back(count);
    mpi_inputs.push_back(source);
    mpi_inputs.push_back(tag);
    mpi_inputs.push_back(comm);
    mpi_inputs.push_back(request);
}

// [source, flag, is_testsome], <index>, clock
// rempi_test_event::rempi_test_event(int event_count, int flag, int rank, int with_next, int index, int msg_id, int mid)
// {
//   mpi_inputs.resize(REMPI_MPI_EVENT_INPUT_NUM);
//   mpi_inputs[REMPI_MPI_EVENT_INPUT_INDEX_EVENT_COUNT]       = event_count;
//   mpi_inputs[REMPI_MPI_EVENT_INPUT_INDEX_FLAG]              = flag;
//   mpi_inputs[REMPI_MPI_EVENT_INPUT_INDEX_RANK]            = rank;
//   mpi_inputs[REMPI_MPI_EVENT_INPUT_INDEX_WITH_NEXT]         = with_next;
//   mpi_inputs[REMPI_MPI_EVENT_INPUT_INDEX_INDEX]             = index;
//   mpi_inputs[REMPI_MPI_EVENT_INPUT_INDEX_MSG_ID]            = msg_id;
//   mpi_inputs[REMPI_MPI_EVENT_INPUT_INDEX_MATCHING_GROUP_ID] = mid;
//   return;
// }

// rempi_test_event::rempi_test_event(int event_counts, int is_testsome, int comm_id, int flag, int source, int tag)
//   : rempi_event()
// {
//   /*If you change this function, you also need to change: 
//      1. #define REMPI_MPI_EVENT_INPUT_NUM (6)
//      2. rempi_event::pop function
//    */
//   mpi_inputs.resize(REMPI_MPI_EVENT_INPUT_NUM);
//   mpi_inputs[REMPI_MPI_EVENT_INPUT_INDEX_EVENT_COUNTS] = event_counts;
//   mpi_inputs[REMPI_MPI_EVENT_INPUT_INDEX_IS_TESTSOME ] = is_testsome;
//   mpi_inputs[REMPI_MPI_EVENT_INPUT_INDEX_COMM_ID     ] = comm_id;
//   mpi_inputs[REMPI_MPI_EVENT_INPUT_INDEX_FLAG        ] = flag;
//   mpi_inputs[REMPI_MPI_EVENT_INPUT_INDEX_SOURCE      ] = source;
//   mpi_inputs[REMPI_MPI_EVENT_INPUT_INDEX_TAG         ] = tag;
//   mpi_inputs[REMPI_MPI_EVENT_INPUT_INDEX_CLOCK       ] = 0;
//   mpi_inputs[REMPI_MPI_EVENT_INPUT_INDEX_TEST_ID     ] = -1;
// }


// rempi_test_event::rempi_test_event(int event_counts, int is_testsome, int comm_id, int flag, int source, int tag, int clock)
//   : rempi_event()
// {
//   /*If you change this function, you also need to change: 
//      1. #define REMPI_MPI_EVENT_INPUT_NUM (6)
//      2. rempi_event::pop function
//    */
//   mpi_inputs.resize(REMPI_MPI_EVENT_INPUT_NUM);
//   mpi_inputs[REMPI_MPI_EVENT_INPUT_INDEX_EVENT_COUNTS] = event_counts;
//   mpi_inputs[REMPI_MPI_EVENT_INPUT_INDEX_IS_TESTSOME ] = is_testsome;
//   mpi_inputs[REMPI_MPI_EVENT_INPUT_INDEX_COMM_ID     ] = comm_id;
//   mpi_inputs[REMPI_MPI_EVENT_INPUT_INDEX_FLAG        ] = flag;
//   mpi_inputs[REMPI_MPI_EVENT_INPUT_INDEX_SOURCE      ] = source;
//   mpi_inputs[REMPI_MPI_EVENT_INPUT_INDEX_TAG         ] = tag;
//   mpi_inputs[REMPI_MPI_EVENT_INPUT_INDEX_CLOCK       ] = clock;
//   mpi_inputs[REMPI_MPI_EVENT_INPUT_INDEX_TEST_ID     ] = -1;
// }

// rempi_test_event::rempi_test_event(int event_counts, int is_testsome, int comm_id, int flag, int source, int tag, int clock, int test_id)
//   : rempi_event()
// {
//   /*If you change this function, you also need to change: 
//      1. #define REMPI_MPI_EVENT_INPUT_NUM (6)
//      2. rempi_event::pop function
//    */
//   mpi_inputs.resize(REMPI_MPI_EVENT_INPUT_NUM);
//   mpi_inputs[REMPI_MPI_EVENT_INPUT_INDEX_EVENT_COUNTS] = event_counts;
//   mpi_inputs[REMPI_MPI_EVENT_INPUT_INDEX_IS_TESTSOME ] = is_testsome;
//   mpi_inputs[REMPI_MPI_EVENT_INPUT_INDEX_COMM_ID     ] = comm_id;
//   mpi_inputs[REMPI_MPI_EVENT_INPUT_INDEX_FLAG        ] = flag;
//   mpi_inputs[REMPI_MPI_EVENT_INPUT_INDEX_SOURCE      ] = source;
//   mpi_inputs[REMPI_MPI_EVENT_INPUT_INDEX_TAG         ] = tag;
//   mpi_inputs[REMPI_MPI_EVENT_INPUT_INDEX_CLOCK       ] = clock;
//   mpi_inputs[REMPI_MPI_EVENT_INPUT_INDEX_TEST_ID     ] = test_id;
// }

