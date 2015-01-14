
#include <vector>
#include <map>
#include <unordered_map>
#include <utility>
#include <list>
#include <algorithm>
#include <utility>

#include <stdlib.h>
#include <string.h>

#include "rempi_clock_delta_compression.h"
#include "rempi_message_manager.h"
#include "rempi_err.h"
#include "rempi_mem.h"

#define EDIT_IGNORE (0)
#define EDIT_ADD    (1)
#define EDIT_REMOVE (-1)

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
      int row_distance;
      int column_distance;
      int shortest_distance;
      clock_node *parent;
      clock_node(int row, int column, int row_distance, int column_distance, int shortest_distance, clock_node *parent):
	row(row), column(column), row_distance(row_distance), column_distance(column_distance), shortest_distance(shortest_distance), parent(parent){}
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

  /*pare = (first: {left|up} index second: " " => 0, ">" => 1, "<" => -1  )*/
  void convert_to_diff_reversed(list<pair<int, int>*> &diff) {
      clock_node* node;
      node = leaf_list.back();
      while (node != NULL) {
	diff.push_back(new pair<int, int>(node->column, EDIT_IGNORE));
	for (int column = 0; column < node->column_distance; column++) {
	  int up_index = (node->column - 1) - column;
	  diff.push_back(new pair<int, int>(up_index, EDIT_REMOVE));
	}	
	for (int row = 0;       row < node->row_distance; row++) {
	  int left_index = (node->row - 1) - row;
	  diff.push_back(new pair<int, int>(left_index, EDIT_ADD));
	}
	node = node->parent;
      }
      /*We do not need the most bottom-righ node*/
      delete diff.front();
      diff.pop_front();
      return;
    }

    int get_shortest_distance() {
      /*We do not include the distance of the last leaf itself, so -1*/
      return leaf_list.back()->shortest_distance - 1;
    }
  
  void add_node(int row, int column) {
      clock_node *current_shortest_parent   = NULL;
      int         current_shortest_distance = row + column + 1;
      clock_node *new_node;
      list<clock_node*>::iterator leaf_list_it;
      list<clock_node*>::iterator current_shortest_leaf_it;
      bool need_erase = false;

      /*Default: Assuming the new_node conects to the root node*/
      new_node = new clock_node(row, column, row, column, row + column + 1, NULL);

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

	new_node->row_distance   =  new_node->row    - (current_shortest_parent->row + 1);
	new_node->column_distance = new_node->column - (current_shortest_parent->column + 1);

	new_node->shortest_distance = current_shortest_distance + 1;

#if 0
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
#if 0
      REMPI_DBG("row: %d, column: %d, distance: %d", row, column, new_node->shortest_distance);
#endif
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

void rempi_clock_delta_compression::change_to_seq_order_id(
    list<pair<int, int>*> &diff,
    vector<rempi_message_identifier*> &msg_ids_observed,
    map<int, int> &map_clock_to_order)
{
  list<pair<int, int>*>::iterator diff_it;
  for (diff_it = diff.begin(); diff_it != diff.end(); diff_it++) {
    int diff_type = (*diff_it)->second;
    if (diff_type == EDIT_ADD) {
      int row = (*diff_it)->first;
      int clock = msg_ids_observed[row]->clock;
      int seq_order_id = map_clock_to_order[clock];
      (*diff_it)->first = seq_order_id;
    }
  }
  return;
}

char* rempi_clock_delta_compression::convert_to_diff_binary(
    list<pair<int, int>*> &diff,
    size_t compressed_bytes)
{
  list<pair<int, int>*>::iterator diff_it;
  vector<int> clock_delta_data;
  unordered_map<int, pair<int, int>*> map_id_to_type_and_index;
  unordered_map<int, pair<int, int>*>::iterator  map_id_to_type_and_index_it;
  char* clock_delta_data_char;

  int fixed_msg_count = 0;
  for (diff_it = diff.begin(); diff_it != diff.end(); diff_it++) {
    int id;
    int type;
    pair<int, int>* current_count_and_type;

    type = (*diff_it)->second;
    current_count_and_type = new pair<int, int>(fixed_msg_count, type);    

    id   = (*diff_it)->first;
    if(!map_id_to_type_and_index.insert({id, current_count_and_type}).second) {
      /*If the id have already been in the map */
      int data;
      int delta;

      pair<int, int> *last_count_and_type;
      int last_fixed_msg_count, last_type;

      /*TODO: Do not call insert twice for performance*/
      map_id_to_type_and_index_it = map_id_to_type_and_index.insert({id, current_count_and_type}).first;
      last_count_and_type = (*map_id_to_type_and_index_it).second;
      last_fixed_msg_count = last_count_and_type->first;
      last_type            = last_count_and_type->second;

      REMPI_DBG("id:%d , type:%d, loop_count: %d", id, type, fixed_msg_count);
      REMPI_DBG("  ==> id:%d , type:%d, loop_count: %d", id, last_type, last_fixed_msg_count);
      
      
      
      clock_delta_data.push_back(data);
      clock_delta_data.push_back(delta);

      delete last_count_and_type;
      delete current_count_and_type;
      if (map_id_to_type_and_index.erase(id) == 0) {
	REMPI_ERR("This should not happens");
      }
    }
    fixed_msg_count++;
  }
  

  clock_delta_data_char = (char*)&clock_delta_data[0];
  return clock_delta_data_char;
}
  

char* rempi_clock_delta_compression::compress(
       map<int, rempi_message_identifier*> &msg_ids_clocked,
       vector<rempi_message_identifier*> &msg_ids_observed,
       size_t &compressed_bytes)
{
  map<int, rempi_message_identifier*>::const_iterator msg_ids_clocked_search_it;
  map<int, rempi_message_identifier*>::const_iterator msg_ids_clocked_start_it;
  /*To memorize order of clock: 
    clock: 0 12 34 45 98
    map_clock_to_order[0] = 0
    map_clock_to_order[12] = 1
    map_clock_to_order[34] = 2
    map_clock_to_order[45] = 3
    map_clock_to_order[98] = 4
   */
  map<int, int> map_clock_to_order; 
  int current_column_of_search_it = -1;
  int current_column_of_start_it  =  0;
  vector<bool> matched_bits; /*Memorize which column of msg was matched*/
  shortest_edit_distance_path_search sed_path_seerch;
  char *compressed_data;

  matched_bits.resize(msg_ids_clocked.size());
  matched_bits.reserve(msg_ids_clocked.size());

  msg_ids_clocked_start_it =  msg_ids_clocked.cbegin();
  msg_ids_clocked_search_it = msg_ids_clocked_start_it;

  for (int i = 0; i < msg_ids_observed.size(); i++) {
    int matched_column, matched_row;
    rempi_message_identifier *msg_id_up, *msg_id_left, *matched_msg_id_up;
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
    matched_msg_id_up = msg_ids_clocked_search_it->second;
    map_clock_to_order[matched_msg_id_up->clock] = matched_column;
    
    /*If needed, it updates start_it*/
    current_column_of_start_it = 
      update_start_it(
			  current_column_of_start_it,
			  current_column_of_search_it,
			  matched_bits,
			  msg_ids_clocked_start_it);

  }
  /*Finally, add morst bottom-right node, because path from this bottom-right node to root node
   becomes shortest path. And sed_path_seerch return results based on this bottom-right node*/
  sed_path_seerch.add_node(msg_ids_clocked.size(), msg_ids_clocked.size());
  /*Distance from the bottom-right node*/
  
#if 0
  REMPI_DBG("Distance: %d", sed_path_seerch.get_shortest_distance());
#endif

#if 0
  {
    map<int, int>::iterator map_it;
    // for_each(map_clock_to_order.begin(), map_clock_to_order.end(), [](pair<int, int> n){ 
    // 	REMPI_DBG("Map: %d -> %d", n.first, n.second);	
    //   });
    for (map_it  = map_clock_to_order.begin(); 
	 map_it != map_clock_to_order.end(); map_it++) {
	REMPI_DBG("Map: %d -> %d", map_it->first, map_it->second);	
    }
  }
#endif


  {
    list<pair<int, int>*> diff;
    sed_path_seerch.convert_to_diff_reversed(diff);

#if 0
    list<pair<int, int>*>::iterator diff_it;
    for (diff_it  = diff.begin(); 
	 diff_it != diff.end(); diff_it++) {
      REMPI_DBG("Map: %d -> %d", (*diff_it)->first, (*diff_it)->second);	
    }
#endif

    change_to_seq_order_id(diff, msg_ids_observed, map_clock_to_order);

#if 1
    list<pair<int, int>*>::iterator diff_it_1;
    REMPI_DBG(" ==== ");
    for (diff_it_1  = diff.begin(); 
	 diff_it_1 != diff.end(); diff_it_1++) {
      REMPI_DBG("Map: %d -> %d", (*diff_it_1)->first, (*diff_it_1)->second);	
    }
#endif
    
    compressed_data = convert_to_diff_binary(diff, compressed_bytes);
 }
  

  return compressed_data;
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


