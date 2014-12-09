/*
 * event.h
 *
 *  Created on: Nov 7, 2014
 *      Author: sato5
 */

#ifndef REMPI_EVENT_H_
#define REMPI_EVENT_H_

#include <iostream>
#include <vector>
using namespace std;

class rempi_event
{
  public:
    vector<int> mpi_inputs;
    virtual void operator ++(int);
    virtual bool operator ==(rempi_event event);
    virtual int* serialize(int &size);
};

class rempi_irecv_event : public rempi_event
{
  public:
    rempi_irecv_event(int event_counts, int count, int source, int tag, int comm, int request);
};

class rempi_test_event : public rempi_event
{
  public:
    rempi_test_event(int event_counts, int is_testsome, int request, int flag, int source, int tag);
};

#endif /* REMPI_EVENT_H_ */
