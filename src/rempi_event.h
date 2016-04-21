/*
 * event.h
 *
 *  Created on: Nov 7, 2014
 *      Author: sato5
 */

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
    vector<int> mpi_inputs; /*TODO: use array insted of vector*/

    rempi_event()
      : clock_order(-1)
      , msg_count(-1)
      , request(NULL) {}

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
