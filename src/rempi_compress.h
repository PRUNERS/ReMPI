#ifndef __REMPI_COMPRESS_H__
#define __REMPI_COMPRESS_H__

#include <unordered_map>
#include <vector>
#include <stdexcept>
#include <algorithm>
#include <string>

#include <stdlib.h>
#include <string.h>

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


/* class rempi_message_identifier */
/* { */
/*  public: */
/*   int source; */
/*   int tag; */
/*   int comm_id;  */

/*   rempi_message_identifier(int source, int tag, int comm_id): source(source), tag(tag), comm_id(comm_id){} */
/*   string to_string(); */
/*   bool operator==(rempi_message_identifier msg_id); */
/* }; */


class rempi_compress
{
 private:
  static int test();
  static int compute_required_bit(int size);
  static void set_new_value(unordered_map<int, int> &target_map, int key, int value);
  static void cpy_kv(char* compressed_data, unordered_map<int, int> &target_map, int target_kv_len_bytes);

 public:
  /*
    Data format:
                  ----------------------------------------------------------------------------------
      source  KV | Length(32bit) | value<0>(32bit) - value<1>(32bit) - ... - value<Length-1>(32bit) |
                  ----------------------------------------------------------------------------------
      tag     KV | Length(32bit) | value<0>(32bit) - value<1>(32bit) - ... - value<Length-1>(32bit) |
                  ----------------------------------------------------------------------------------
      comm_id KV | Length(32bit) | value<0>(32bit) - value<1>(32bit) - ... - value<Length-1>(32bit) |
                  ----------------------------------------------------------------------------------
      meta       |                            Numbor of events (32bit)	                            |
                  ----------------------------------------------------------------------------------
      event      |          source       |               tag             |         comm_id          |
                  ----------------------------------------------------------------------------------
      event      |          source       |               tag             |         comm_id          |
                  ----------------------------------------------------------------------------------
                 |                                        :                                         |
                                                          :
                                                          :
                                                          :
                 |                                        :                                         |
                  ----------------------------------------------------------------------------------
      event      |          source       |               tag             |         comm_id          |
                  ----------------------------------------------------------------------------------
   */
  /*
    1.shift computation
    2.
  */
  static char* binary_compress(
      vector<rempi_message_identifier*> &msg_ids,
      size_t &compressed_bytes);
  static void binary_decompress(
      char* binary_compressed_data, 		
      vector<rempi_message_identifier*> &decompressed_msg_id);

};


#endif
