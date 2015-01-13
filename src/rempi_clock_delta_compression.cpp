
#include <vector>
#include <map>
#include <utility>
#include <list>

#include <stdlib.h>
#include <string.h>

#include "rempi_clock_delta_compression.h"
#include "rempi_message_manager.h"
#include "rempi_err.h"
#include "rempi_mem.h"

using namespace std;

/*
Original Test (ID is just for this example);
| ID | event_count | flag | is_testsome | source | tag | comm_id |
|  0 |          92 |    0 |           0 |      0 |   0 |       0 |
|  1 |           1 |    1 |           1 |      2 |   1 |       1 |
|  2 |          34 |    0 |           0 |      0 |   0 |       0 |
|  3 |           1 |    1 |           1 |      1 |   1 |       1 |
|  4 |           1 |    1 |           1 |      3 |   1 |       1 |
|  5 |          12 |    0 |           0 |      0 |   0 |       0 |
|  6 |           1 |    1 |           1 |      1 |   1 |       1 |
|  7 |           1 |    1 |           1 |      2 |   1 |       1 |
|  8 |           1 |    1 |           1 |      3 |   1 |       1 |
|  9 |          27 |    0 |           0 |      0 |   0 |       0 |
| 10 |           1 |    1 |           1 |      1 |   1 |       1 |
| 11 |           1 |    1 |           1 |      3 |   1 |       1 |
| 12 |           5 |    0 |           0 |      0 |   0 |       0 |

| Original Test | ==> | Matched Test | Unmatched Test |

Matched Test:
| ID | source | tag | comm_id |
|  1 |      2 |   1 |       1 |
|  3 |      1 |   1 |       1 |
|  4 |      3 |   1 |       1 |
|  6 |      1 |   1 |       1 |
|  7 |      2 |   1 |       1 |
|  8 |      3 |   1 |       1 |
| 10 |      1 |   1 |       1 |
| 11 |      3 |   1 |       1 |


Unmatched Test
| ID | event_count | offset |
|  0 |          92 |      0 |
|  2 |          34 |      0 |
|  5 |          12 |      1 |
|  9 |          27 |      2 |
| 12 |           5 |      1 |

*/

class shortest_edit_distance_path_search
{
  /*
     -----------
    |   |   |   |
    |   |   |   |
     ---X--------
    |   |\  |   |
    |   |  \|   |
     -----------
    |   |   |   |
    |   |   |   |
     -----------

     (row, column) == X
   
  */
  private:
    class clock_node {
      public:
        int row;
        int column;
        int shortest_distance;
        clock_node *parent;
      clock_node(int row, int column, int shortest_distance, clock_node *parent):
	row(row), column(column), shortest_distance(shortest_distance), parent(parent){}
    };
    list<clock_node*> leaf_list;
    

    int  find_shortest_node(clock_node* new_node, clock_node* search_node, clock_node **parent) {
      while (search_node != NULL) {
	int    row_distance   = new_node->row    - (search_node->row + 1);
	int column_distance   = new_node->column - (search_node->column + 1);
	if (row_distance >= 0 && column_distance >= 0) {
	  *parent = search_node;
	  return search_node->shortest_distance + row_distance + column_distance;
	}
	search_node = search_node->parent;
      }
      return -1;
    }



  public:
    int get_shortest_distance() {
      return leaf_list.back()->shortest_distance;
    }
  
    void add_node(int row, int column) {
      clock_node *current_shortest_parent   = NULL;
      int         current_shortest_distance = row + column + 1;
      clock_node *new_node;
      list<clock_node*>::iterator leaf_list_it;
      list<clock_node*>::iterator current_shortest_leaf_it;
      bool need_erase = false;

      /*Default: Assuming the new_node conects to the root node*/
      new_node = new clock_node(row, column, row + column + 1, NULL);

      for (leaf_list_it  = leaf_list.begin();
	   leaf_list_it != leaf_list.end();
	   leaf_list_it++) {
	clock_node *search_leaf_node = *leaf_list_it;
	int         shortest_distance_from_search_node;
	clock_node *shortest_node_from_search_node;
	shortest_distance_from_search_node = find_shortest_node(new_node, search_leaf_node, &shortest_node_from_search_node);

	if (shortest_distance_from_search_node < 0) continue; 	/*There is no node to connect*/
	if (shortest_distance_from_search_node < current_shortest_distance) {
	  /*Update, if is shorter then the current shortest distance*/
	  current_shortest_distance = shortest_distance_from_search_node;
	  current_shortest_parent   = shortest_node_from_search_node;
	  if (search_leaf_node == shortest_node_from_search_node) {
	    /*If the shortest_node_from_search_node is leaf, memorize to remove this from leaf_list later*/
	    current_shortest_leaf_it = leaf_list_it;
	    need_erase = true;
	  } else {
	    need_erase = false;
	  }
	}
      }
      if (current_shortest_parent != NULL) {

	int dist  = new_node->row    - (current_shortest_parent->row + 1);
	    dist += new_node->column - (current_shortest_parent->column + 1);

	new_node->shortest_distance = current_shortest_distance + 1;
#if 1
	REMPI_DBG("   P: r: %d, c: %d /r: %d  c: %d, current: %d, dist: %d, sum dist: %d", 
		  current_shortest_parent->row,
		  current_shortest_parent->column,
		  row, column, 
		  current_shortest_distance,
		  dist,
		  new_node->shortest_distance);
#endif
	new_node->parent            = current_shortest_parent;
	if (need_erase) {
	  /*If the current_shortest_parent was leaf, remove it from leaf_list*/
	  leaf_list.erase(current_shortest_leaf_it);
	}
      }
      REMPI_DBG("row: %d, column: %d, distance: %d", row, column, new_node->shortest_distance);
      leaf_list.push_back(new_node);
    }
};


int rempi_clock_delta_compression::next_start_search_it(
 int msg_id_up_clock,
 int msg_id_left_clock,
 map<int, rempi_message_identifier*>::const_iterator &msg_ids_clocked_search_it,
 map<int, rempi_message_identifier*>::const_iterator &msg_ids_clocked_start_it,
 int current_column_of_search_it,
 int current_column_of_start_it) 
{
  /*Find start msg_id_up for msg_id_left*/
  if (msg_id_up_clock > msg_id_left_clock || current_column_of_search_it == -1) {
    /*search from the first msg id*/
    msg_ids_clocked_search_it = msg_ids_clocked_start_it;
    return current_column_of_start_it;
  } else {
    /*else serch from the right next msg ids*/
    msg_ids_clocked_search_it++;
    return current_column_of_search_it + 1;
  }
  REMPI_ERR("This must not happne");
  return -1;
}

int rempi_clock_delta_compression::find_matched_clock_column(
 int clock_left,
 map<int, rempi_message_identifier*>::const_iterator &msg_ids_clocked_search_it,
 map<int, rempi_message_identifier*>::const_iterator const &msg_ids_clocked_search_it_end)
{
  rempi_message_identifier *msg_id_up;
  int traversed_count = 0;
  while (msg_ids_clocked_search_it != msg_ids_clocked_search_it_end) {
    msg_id_up   = msg_ids_clocked_search_it->second;
    //    REMPI_DBG("%d == %d", clock_left, msg_id_up->clock);
    if (clock_left == msg_id_up->clock) {
      return traversed_count;
    }
    msg_ids_clocked_search_it++;
    traversed_count++;
  }
  REMPI_ERR("Searched to the end, which should not happen");
  return -1;
}

int rempi_clock_delta_compression::update_start_it(
   int current_column_of_start_it,
   int matched_column,
   vector<bool> matched_bits,
   map<int, rempi_message_identifier*>::const_iterator &msg_ids_clocked_start_it)
{
  matched_bits[matched_column] = true;
  for (int i = current_column_of_start_it; matched_bits[i] == true; i++) {
    msg_ids_clocked_start_it++;
    current_column_of_start_it++;
  }
  return current_column_of_start_it;
}


char* rempi_clock_delta_compression::compress(
       map<int, rempi_message_identifier*> &msg_ids_clocked,
       vector<rempi_message_identifier*> &msg_ids_observed,
       size_t &compressed_bytes)
{
  map<int, rempi_message_identifier*>::const_iterator msg_ids_clocked_search_it;
  map<int, rempi_message_identifier*>::const_iterator msg_ids_clocked_start_it;
  int current_column_of_search_it = -1;
  int current_column_of_start_it  =  0;
  vector<bool> matched_bits; /*Memorize which column of msg was matched*/
  shortest_edit_distance_path_search sed_path_seerch;

  matched_bits.resize(msg_ids_clocked.size());
  matched_bits.reserve(msg_ids_clocked.size());

  msg_ids_clocked_start_it =  msg_ids_clocked.cbegin();
  msg_ids_clocked_search_it = msg_ids_clocked_start_it;

  for (int i = 0; i < msg_ids_observed.size(); i++) {
    int matched_column, matched_row;
    rempi_message_identifier *msg_id_up, *msg_id_left;
    msg_id_up   = msg_ids_clocked_search_it->second;
    msg_id_left = msg_ids_observed[i];

    /*set "msg_ids_clocked_search_it" to start index*/
    current_column_of_search_it =
      next_start_search_it(msg_id_up->clock, msg_id_left->clock,
			   msg_ids_clocked_search_it,
			   msg_ids_clocked_start_it,
			   current_column_of_search_it,
			   current_column_of_start_it);

    /*find matched clock column*/
    current_column_of_search_it += 
      find_matched_clock_column(
				msg_id_left->clock,
				msg_ids_clocked_search_it,
				msg_ids_clocked.cend());
    
    matched_column = current_column_of_search_it;
    matched_row    = i;
    sed_path_seerch.add_node(matched_row, matched_column);
    
    /*If needed, it updates start_it*/
    current_column_of_start_it = 
      update_start_it(
			  current_column_of_start_it,
			  current_column_of_search_it,
			  matched_bits,
			  msg_ids_clocked_start_it);

  }
  sed_path_seerch.add_node(msg_ids_clocked.size(), msg_ids_clocked.size());
  REMPI_DBG("Distance: %d", sed_path_seerch.get_shortest_distance());
  

  return NULL;
}


void rempi_clock_delta_compression::decompress(
      char* compressed_data,
      size_t &compressed_bytes,
      set<rempi_message_identifier*> &rempi_ids_clock,
      vector<rempi_message_identifier*> &rempi_ids_real
)
{

  return;
}


