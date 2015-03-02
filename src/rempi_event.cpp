#include <iostream>
#include <vector>

#include "rempi_event.h"
#include "rempi_err.h"


using namespace std;

int rempi_event::max_size = sizeof(int) * REMPI_MPI_EVENT_INPUT_NUM;

void rempi_event::operator ++(int) {
  mpi_inputs[0]++;
}

bool rempi_event::operator ==(rempi_event event)
{
    if (this->mpi_inputs.size() != event.mpi_inputs.size()) {
      cerr << "something wrong !" << endl;
    }
    for (unsigned int i = 1; i < mpi_inputs.size() - 1; i++) {
      if (this->mpi_inputs[i] != event.mpi_inputs[i]) {
	return false;
      }
    }
    return true;
}

void rempi_event::push(rempi_event* event)
{
  /*TODO: Implement as a replacement of operator++*/
}

rempi_event* rempi_event::pop()
{
  rempi_event *event;
  /*TODO: Implement as a replacement of operator++*/
  mpi_inputs[0]--;
  /*XXX: Assuming the all recorded event is Test. 
    so return rempi_event_test, but this is resonable assumption*/
  if (mpi_inputs[0] < 0) {
    REMPI_ERR("Event count < 0 error");
  }
  event = new rempi_test_event(1, mpi_inputs[1], mpi_inputs[2], mpi_inputs[3], mpi_inputs[4], mpi_inputs[5]);
  return event;
}

int rempi_event::size()
{
  return mpi_inputs[0];
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
  return mpi_inputs[REMPI_MPI_EVENT_INPUT_INDEX_EVENT_COUNTS];
}   

int rempi_event::get_is_testsome() 
{
  return mpi_inputs[REMPI_MPI_EVENT_INPUT_INDEX_IS_TESTSOME];
}

int rempi_event::get_comm_id()
{
  return mpi_inputs[REMPI_MPI_EVENT_INPUT_INDEX_COMM_ID];
}

int rempi_event::get_flag()
{
  return mpi_inputs[REMPI_MPI_EVENT_INPUT_INDEX_FLAG];
}

int rempi_event::get_source()
{
  return mpi_inputs[REMPI_MPI_EVENT_INPUT_INDEX_SOURCE];
}

int rempi_event::get_tag()
{
  return mpi_inputs[REMPI_MPI_EVENT_INPUT_INDEX_TAG];
}

int rempi_event::get_clock()
{
  return mpi_inputs[REMPI_MPI_EVENT_INPUT_INDEX_CLOCK];
}

int rempi_event::get_test_id()
{
  return mpi_inputs[REMPI_MPI_EVENT_INPUT_INDEX_TEST_ID];
}


/*====== child class constructures ======*/

rempi_irecv_event::rempi_irecv_event(int event_counts, int count, int source, int tag, int comm, int request) {
    mpi_inputs.push_back(event_counts);
    mpi_inputs.push_back(count);
    mpi_inputs.push_back(source);
    mpi_inputs.push_back(tag);
    mpi_inputs.push_back(comm);
    mpi_inputs.push_back(request);
}

rempi_test_event::rempi_test_event(int event_counts, int is_testsome, int comm_id, int flag, int source, int tag)
{
  /*If you change this function, you also need to change: 
     1. #define REMPI_MPI_EVENT_INPUT_NUM (6)
     2. rempi_event::pop function
   */
  mpi_inputs.resize(REMPI_MPI_EVENT_INPUT_NUM);
  mpi_inputs[REMPI_MPI_EVENT_INPUT_INDEX_EVENT_COUNTS] = event_counts;
  mpi_inputs[REMPI_MPI_EVENT_INPUT_INDEX_IS_TESTSOME ] = is_testsome;
  mpi_inputs[REMPI_MPI_EVENT_INPUT_INDEX_COMM_ID     ] = comm_id;
  mpi_inputs[REMPI_MPI_EVENT_INPUT_INDEX_FLAG        ] = flag;
  mpi_inputs[REMPI_MPI_EVENT_INPUT_INDEX_SOURCE      ] = source;
  mpi_inputs[REMPI_MPI_EVENT_INPUT_INDEX_TAG         ] = tag;
  mpi_inputs[REMPI_MPI_EVENT_INPUT_INDEX_CLOCK       ] = 0;
  mpi_inputs[REMPI_MPI_EVENT_INPUT_INDEX_TEST_ID     ] = -1;
}

rempi_test_event::rempi_test_event(int event_counts, int is_testsome, int comm_id, int flag, int source, int tag, int clock)
{
  /*If you change this function, you also need to change: 
     1. #define REMPI_MPI_EVENT_INPUT_NUM (6)
     2. rempi_event::pop function
   */
  mpi_inputs.resize(REMPI_MPI_EVENT_INPUT_NUM);
  mpi_inputs[REMPI_MPI_EVENT_INPUT_INDEX_EVENT_COUNTS] = event_counts;
  mpi_inputs[REMPI_MPI_EVENT_INPUT_INDEX_IS_TESTSOME ] = is_testsome;
  mpi_inputs[REMPI_MPI_EVENT_INPUT_INDEX_COMM_ID     ] = comm_id;
  mpi_inputs[REMPI_MPI_EVENT_INPUT_INDEX_FLAG        ] = flag;
  mpi_inputs[REMPI_MPI_EVENT_INPUT_INDEX_SOURCE      ] = source;
  mpi_inputs[REMPI_MPI_EVENT_INPUT_INDEX_TAG         ] = tag;
  mpi_inputs[REMPI_MPI_EVENT_INPUT_INDEX_CLOCK       ] = clock;
  mpi_inputs[REMPI_MPI_EVENT_INPUT_INDEX_TEST_ID     ] = -1;
}

rempi_test_event::rempi_test_event(int event_counts, int is_testsome, int comm_id, int flag, int source, int tag, int clock, int test_id)
{
  /*If you change this function, you also need to change: 
     1. #define REMPI_MPI_EVENT_INPUT_NUM (6)
     2. rempi_event::pop function
   */
  mpi_inputs.resize(REMPI_MPI_EVENT_INPUT_NUM);
  mpi_inputs[REMPI_MPI_EVENT_INPUT_INDEX_EVENT_COUNTS] = event_counts;
  mpi_inputs[REMPI_MPI_EVENT_INPUT_INDEX_IS_TESTSOME ] = is_testsome;
  mpi_inputs[REMPI_MPI_EVENT_INPUT_INDEX_COMM_ID     ] = comm_id;
  mpi_inputs[REMPI_MPI_EVENT_INPUT_INDEX_FLAG        ] = flag;
  mpi_inputs[REMPI_MPI_EVENT_INPUT_INDEX_SOURCE      ] = source;
  mpi_inputs[REMPI_MPI_EVENT_INPUT_INDEX_TAG         ] = tag;
  mpi_inputs[REMPI_MPI_EVENT_INPUT_INDEX_CLOCK       ] = clock;
  mpi_inputs[REMPI_MPI_EVENT_INPUT_INDEX_TEST_ID     ] = test_id;
}

