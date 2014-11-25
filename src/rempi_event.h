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
		virtual void operator ++(int) {
		mpi_inputs[0]++;
		};

		virtual bool operator ==(rempi_event event){
		  if (this->mpi_inputs.size() != event.mpi_inputs.size()) {
		    cerr << "something wrong !" << endl;
		  }
		  for (unsigned int i = 1; i < mpi_inputs.size(); i++) {
		    if (this->mpi_inputs[i] != event.mpi_inputs[i]) {
		      return false;
		    }
		  }
		  return true;
		};

		virtual int* serialize(int &size) {
		  int *serialized_data = new int[mpi_inputs.size()];
		  for (unsigned int i = 0; i < mpi_inputs.size(); i++) {
			  serialized_data[i] = mpi_inputs[i];
		  }
		  size = mpi_inputs.size();
		   return serialized_data;
	}
};

class rempi_irecv_event : public rempi_event
{
	public:
//		int event_counts;
    	//int count;
	    //int source;
		//int tag;
		//int comm;
		//int	request;
	rempi_irecv_event(int event_counts, int count, int source, int tag, int comm, int request) {
		mpi_inputs.push_back(event_counts);
		mpi_inputs.push_back(count);
		mpi_inputs.push_back(source);
		mpi_inputs.push_back(tag);
		mpi_inputs.push_back(comm);
		mpi_inputs.push_back(request);
	};


};


class rempi_test_event : public rempi_event
{
	public:
		rempi_test_event(int event_counts, int is_testsome, int request, int flag, int source, int tag)
	{
		mpi_inputs.push_back(event_counts);
		mpi_inputs.push_back(is_testsome);
		mpi_inputs.push_back(request);
		mpi_inputs.push_back(flag);
		mpi_inputs.push_back(source);
		mpi_inputs.push_back(tag);
	};


};

// 100000(w/o thread): 38 32 34 38 41
// 100000(w   thread): 42 29 34

#endif /* REMPI_EVENT_H_ */
