
#include <unistd.h>
#include <limits>

#include "rempi_mutex.h"
#include "rempi_spsc_queue.h"
#include "rempi_event_list.h"
#include "rempi_event.h"
#include "rempi_util.h"
#include "rempi_err.h"
#include "rempi_config.h"


using namespace std;



template <class T>
size_t rempi_event_list<T>::size()
{
  //  mtx.lock();
  size_t result = events.rough_size();
  //  mtx.unlock();
  return result;
}

template <class T>
void rempi_event_list<T>::normal_push(T event)
{
  //  mtx.lock();
  while (events.rough_size() >= max_size) {
    //    mtx.unlock();
    REMPI_DBG("rempi_event_list exceeded max_size");
    usleep(spin_time);
    //    mtx.lock();
  }
  events.enqueue(event);
  //  mtx.unlock();
}


//previous_recording_event_umap
template <class T>
void rempi_event_list<T>::push(rempi_event *event)
{
  if (is_push_closed) {
    REMPI_ERR("event_list push was closed");
  }

  if (this->previous_recording_event == NULL) {
    previous_recording_event = event;
    return;
  }

  //TODO: use previous_recording_event_umap
  if ((*this->previous_recording_event) == *event 
      && this->previous_recording_event->get_test_id() == event->get_test_id()) {
    /*If the event is exactry same as previsou one, ...*/
    /*Just increment the counter of the event
     in order to reduce the memory consumption.
     Even if an application polls with MPI_Test, 
     we only increment an counter of the previous MPI_Test event
    */
    (*this->previous_recording_event)++;
    delete event;
  } else {
    //    mtx.lock();
    while (events.rough_size() >= max_size) {
      //      mtx.unlock();
      REMPI_DBG("rempi_event_list exceeded max_size");
      usleep(spin_time);
      //      mtx.lock();
    }
    events.enqueue(previous_recording_event);
    //    mtx.unlock();
    previous_recording_event = event;
  }
  return;
}

template <class T>
void rempi_event_list<T>::push_all()
{
  if (is_push_closed) {
    REMPI_ERR("event_list push was closed");
  }

  if (previous_recording_event != NULL) {
    /*If previous is not empty, push the event to the event_list*/
    //    mtx.lock();
    while (events.rough_size() >= max_size) {
      //      mtx.unlock();
      REMPI_DBG("rempi_event_list exceeded max_size");
      usleep(spin_time);
      //      mtx.lock();
    }
    events.enqueue(previous_recording_event);
    //    mtx.unlock();
  }

  set_globally_minimal_clock(numeric_limits<size_t>::max());
  close_push();

}

template <class T>
void rempi_event_list<T>::close_push()
{
  is_push_closed = true;
  return;
}

template <class T>
bool rempi_event_list<T>::is_push_closed_()
{
  return is_push_closed;
}

/*
Pop an event until event_count becomes 0.
If event_count == 0, then dequeue next event.
If no events to be dequeued, this thread ends its work.

If thread finished reading all events, and pushed to event_lists, and the events are all pupoed, 
This function retunr NULL
*/
template <class T>
T rempi_event_list<T>::dequeue_replay(int test_id, int &status)
{
  rempi_event *event;
  rempi_spsc_queue<rempi_event*> *spsc_queue;
  bool is_queue_empty;
  
  //  REMPI_DBGI(0, "called dequeue_replay: %d", test_id);
  
  if (previous_replaying_event_umap.find(test_id) == previous_replaying_event_umap.end()) {
    previous_replaying_event_umap[test_id] = NULL;
  } else {
    if (previous_replaying_event_umap[test_id] != NULL) {
      if (previous_replaying_event_umap[test_id]->size() <= 0) {
	delete previous_replaying_event_umap[test_id];
	previous_replaying_event_umap[test_id] = NULL;
      }
    }
  }
  
  /*Did io thread create the replay_events(test_id) spsc_queue instance for decoding ?*/
  mtx.lock();
  if (replay_events.find(test_id) == replay_events.end()) {
    status = REMPI_EVENT_LIST_EMPTY;
    mtx.unlock();
    return NULL;
  }
  spsc_queue = replay_events[test_id];
  mtx.unlock();


  if (previous_replaying_event_umap[test_id] == NULL) {
    if (is_push_closed) {
      /* Note: 
         is_push_closed is need to be tested before calling spsc_queue->dequeu()
         because io thread calls spsc_queue->enqueue() first, then set 
	 is_push_closed to 1;	 
      */
      previous_replaying_event_umap[test_id] = spsc_queue->dequeue(); // 
      is_queue_empty = (previous_replaying_event_umap[test_id] == NULL);
      if (is_queue_empty) {
	status = REMPI_EVENT_LIST_FINISH;
	return NULL;
      }
    } else {
      //      REMPI_DBGI(0, "c called dequeue_replay: %d", test_id);
      previous_replaying_event_umap[test_id] = spsc_queue->dequeue(); // 
      is_queue_empty = (previous_replaying_event_umap[test_id] == NULL);
      if (is_queue_empty) {
	// REMPI_DBG("push closed %p", previous_replaying_event_umap[test_id]);
	status = REMPI_EVENT_LIST_EMPTY;
	return NULL;
      }
    }
  }

  /*From here, previous_replaying_event has "event instance" at least*/

  event = previous_replaying_event_umap[test_id]->pop();

// #ifdef REMPI_DBG_REPLAY
//   REMPI_DBGI(REMPI_DBG_REPLAY, "DQN : (count: %d, with_next: %d, flag: %d, source: %d, tag: %d, clock: %d, msg_count: %d %p)",
// 	     event->get_event_counts(), event->get_is_testsome(), event->get_flag(),
// 	     event->get_source(), event->get_tag(), event->get_clock(), event->msg_count,  event);
// #endif
  

  if (event == NULL) {
    REMPI_ERR("Poped replaying event is null: this should not be occured");
  }
  status = REMPI_EVENT_LIST_DEQUEUED;
  return event;
}

// template <class T>
// T rempi_event_list<T>::dequeue_replay(int test_id)
// {
//   rempi_event *event;
//   rempi_spsc_queue<rempi_event*> *spsc_queue;
  
//   if (previous_replaying_event_umap.find(test_id) == previous_replaying_event_umap.end()) {
//     previous_replaying_event_umap[test_id] = NULL;
//   }
  
//   //  previous_event = previous_replaying_event[test_id];
//   while(replay_events.find(test_id) == replay_events.end()) {
//     usleep(spin_time);
//   }
//   spsc_queue = replay_events[test_id];

//   /*TODO: TODO(A) */
//   while (previous_replaying_event_umap[test_id] == NULL) {
//     if (is_push_closed) {
//       previous_replaying_event_umap[test_id] = spsc_queue->dequeue();
//       bool is_queue_empty = (previous_replaying_event_umap[test_id] == NULL);
//       if (is_queue_empty) {
// 	REMPI_DBG("push closed %p", previous_replaying_event_umap[test_id]);
// 	return NULL;
//       }
//     } else {
//       previous_replaying_event_umap[test_id] = spsc_queue->dequeue();
//     }
//     // previous_replaying_event_umap[test_id] = spsc_queue->dequeue();
//     // bool is_queue_empty = (previous_replaying_event_umap[test_id] == NULL);
//     // if (is_queue_empty && is_push_closed) {
//     //   REMPI_DBG("push closed %p", previous_replaying_event_umap[test_id]);
//     //   return NULL;
//     // }
//   }

//   /*From here, previous_replaying_event has "event instance" at least*/
//   if (previous_replaying_event_umap[test_id]->size() <= 0) {
//     delete previous_replaying_event_umap[test_id];
//     previous_replaying_event_umap[test_id] = NULL;

//     /*TODO: Implement a function to combine to TODO(A) above*/
//     while (previous_replaying_event_umap[test_id] == NULL) {
//       if (is_push_closed) {
// 	previous_replaying_event_umap[test_id] = spsc_queue->dequeue();
// 	bool is_queue_empty = (previous_replaying_event_umap[test_id] == NULL);
// 	if (is_queue_empty) {
// 	  REMPI_DBG("push closed %p", previous_replaying_event_umap[test_id]);
// 	  return NULL;
// 	}
//       } else {
// 	previous_replaying_event_umap[test_id] = spsc_queue->dequeue();
//       }
//       // previous_replaying_event_umap[test_id] = spsc_queue->dequeue();
//       // bool is_queue_empty = (previous_replaying_event_umap[test_id] == NULL);
//       // if (is_queue_empty && is_push_closed) {
//       // 	REMPI_DBG("push closed2 %p", previous_replaying_event_umap[test_id]);
//       // 	return NULL;
//       // }
//     }
//   }

//   event = previous_replaying_event_umap[test_id]->pop();
  
//   if (event == NULL) {
//     REMPI_ERR("previous replaying event pop failed");
//   }
//   //  event->print();
//   return event;

// }



// template <class T>
// T rempi_event_list<T>::dequeue_replay(int test_id)
// {
//   rempi_event *event;
//   rempi_spsc_queue<rempi_event*> *spsc_queue;
  
//   if (previous_replaying_event_umap.find(test_id) == previous_replaying_event_umap.end()) {
//     previous_replaying_event_umap[test_id] = NULL;
//   }
  
//   //  previous_event = previous_replaying_event[test_id];
//   while(replay_events.find(test_id) == replay_events.end()) {
//     usleep(spin_time);
//   }
//   spsc_queue = replay_events[test_id];

//   /*TODO: TODO(A) */
//   while (previous_replaying_event_umap[test_id] == NULL) {
//     previous_replaying_event_umap[test_id] = spsc_queue->dequeue();
//     bool is_queue_empty = (previous_replaying_event_umap[test_id] == NULL);
//     if (is_queue_empty && is_push_closed) {
//       REMPI_DBG("push closed %p", previous_replaying_event_umap[test_id]);
//       return NULL;
//     }
//   }

//   /*From here, previous_replaying_event has "event instance" at least*/
//   if (previous_replaying_event_umap[test_id]->size() <= 0) {
//     delete previous_replaying_event_umap[test_id];
//     previous_replaying_event_umap[test_id] = NULL;

//     /*TODO: Implement a function to combine to TODO(A) above*/
//     while (previous_replaying_event_umap[test_id] == NULL) {
//       previous_replaying_event_umap[test_id] = spsc_queue->dequeue();
//       bool is_queue_empty = (previous_replaying_event_umap[test_id] == NULL);
//       if (is_queue_empty && is_push_closed) {
// 	REMPI_DBG("push closed2 %p", previous_replaying_event_umap[test_id]);
// 	return NULL;
//       }
//     }
//   }

//   event = previous_replaying_event_umap[test_id]->pop();
  
//   if (event == NULL) {
//     REMPI_ERR("previous replaying event pop failed");
//   }
//   //  event->print();
//   return event;

// }

template <class T>
T rempi_event_list<T>::front_replay(int test_id)
{
  rempi_spsc_queue<rempi_event*> *spsc_queue;
  mtx.lock();
  if (replay_events.find(test_id) == replay_events.end()) {
    mtx.unlock();
    return NULL;
  } 
  // else if (replay_events[test_id]->rough_size() == 0) {
  //   return NULL;
  // }
  T e = replay_events[test_id]->front();
  mtx.unlock();
  return e; 
}

template <class T>
void rempi_event_list<T>::enqueue_replay(T event, int test_id)
{

  rempi_spsc_queue<rempi_event*> *spsc_queue;
  mtx.lock();
  if (replay_events.find(test_id) == replay_events.end()) {
    replay_events[test_id] = new rempi_spsc_queue<rempi_event*>();
  }

  spsc_queue = replay_events[test_id];
  mtx.unlock();

  while (spsc_queue->rough_size() >= max_size) {
    REMPI_DBG("rempi_event_list exceeded max_size");
    usleep(spin_time);
  }
  spsc_queue->enqueue(event);
  return;
}


template <class T>
size_t rempi_event_list<T>::size_replay(int test_id)
{
  rempi_spsc_queue<rempi_event*> *spsc_queue;
  mtx.lock();
  if (replay_events.find(test_id) == replay_events.end()) {
    mtx.unlock();
    return 0;
  }
  spsc_queue = replay_events[test_id];
  mtx.unlock();
  return spsc_queue->rough_size();
}

template <class T>
size_t rempi_event_list<T>::get_enqueue_count(int test_id)
{
  rempi_spsc_queue<rempi_event*> *spsc_queue;
  mtx.lock();
  if (replay_events.find(test_id) == replay_events.end()) {
    mtx.unlock();
    return 0;
  }
  spsc_queue = replay_events[test_id];
  mtx.unlock();
  return spsc_queue->get_enqueue_count();
}

template <class T>
size_t rempi_event_list<T>::get_dequeue_count(int test_id)
{
  rempi_spsc_queue<rempi_event*> *spsc_queue;
  mtx.lock();
  if (replay_events.find(test_id) == replay_events.end()) {
    mtx.unlock();
    return 0;
  }
  spsc_queue = replay_events[test_id];
  mtx.unlock();
  return spsc_queue->get_dequeue_count();
}

template <class T>
T rempi_event_list<T>::pop()
{
  //  mtx.lock();
  /*
    while (events.rough_size() <= 0)
    {
    count << events.rough_size() << endl;
    mtx.unlock();
    usleep(spin_time);
    mtx.lock();
    }
  */
  T item = events.dequeue();
  //  mtx.unlock();
  return item;
}


template <class T>
T rempi_event_list<T>::front()
{
  //  mtx.lock();
  T item = events.front();
  //  mtx.unlock();
  return item;
}

template <class T>
size_t rempi_event_list<T>::get_globally_minimal_clock()
{
  return globally_minimal_clock;
}

template <class T>
void rempi_event_list<T>::set_globally_minimal_clock(size_t gmc)
{
  globally_minimal_clock = gmc;
}

template class rempi_event_list<rempi_event*>; // to tell compiler
