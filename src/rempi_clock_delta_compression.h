#ifndef __REMPI_COMPRESS_H__
#define __REMPI_COMPRESS_H__

#include <vector>
#include <set>

#include "rempi_message_manager.h"


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

class rempi_clock_delta_compression
{
 private:
  static int next_start_search_it(
   int msg_id_up_clock,
   int msg_id_left_clock,
   map<int, rempi_message_identifier*>::const_iterator &msg_ids_clocked_search_it,		 
   map<int, rempi_message_identifier*>::const_iterator &msg_ids_clocked_start_it,
   int current_column_of_search_it,
   int current_column_of_start_it);

  static int find_matched_clock_column(
   int clock_observed,				    
   map<int, rempi_message_identifier*>::const_iterator &msg_ids_clocked_search_it,
   map<int, rempi_message_identifier*>::const_iterator const  &msg_ids_clocked_search_it_end);

  static int update_start_it(
   int current_column_of_start_it,
   int current_column_of_search_it,
   vector<bool> matched_bits,
   map<int, rempi_message_identifier*>::const_iterator &msg_ids_clocked_start_it);

  static void change_to_seq_order_id(
   list<pair<int, int>*> &diff,
   vector<rempi_message_identifier*> &msg_ids_observed,
   map<int, int> &map_clock_to_order);

  static char* convert_to_diff_binary(
   list<pair<int, int>*> &diff,
   size_t &compressed_bytes);


 public:
  static char* compress(
	     map<int, rempi_message_identifier*> &msg_ids_clocked,
	     vector<rempi_message_identifier*> &msg_ids_observed,
	     size_t &compressed_bytes);
  static void decompress(
      char* compressed_data,
      size_t &compressed_bytes,
      set<rempi_message_identifier*> &rempi_ids_clock,
      vector<rempi_message_identifier*> &rempi_ids_real
);	


};


#endif
