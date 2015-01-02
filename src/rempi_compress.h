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
  static int compute_required_bit(int size) {
    int required_bit = 0;
    int bin;
    if (size == 0) {
      fprintf(stderr, "error\n");
    }
    /*
      size: 1 => 0bit
    */
    if (size == 1) return 0;
    /*
      size: 2 => 1bit (bin:2)
      size: 3 => 2bit (bin:4)
      size: 4 => 2bit (bin:4)
     */
    bin = 2;
    required_bit = 1;
    while(bin < size) {
      bin = bin << 1;
      required_bit++;
    }
    return required_bit;
  }

  static void set_new_value(unordered_map<int, int> &target_map, int key, int value) 
  {
    try {
      target_map.at(key);
    } catch(out_of_range&) {
      target_map[key] = value;
    }
    return;
  }

  static void cpy_kv(char* compressed_data, unordered_map<int, int> &target_map, int target_kv_len_bytes)
  {
    int val;
    val = target_map.size();
    memcpy(compressed_data, &val, target_kv_len_bytes);
    compressed_data += target_kv_len_bytes;
    /*TODO: cpy kv*/
    
    return;
  }

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
      size_t &compressed_bytes)
  {
    rempi_message_identifier *msg_id;
    unordered_map<int, int>  source_map; 
    int source_new_id = 0;
    int required_source_bit  = 0; /*TODO: remove requred_ or use required_*/
    int source_kv_len_bytes;
    int source_kv_bytes;
    unordered_map<int, int>     tag_map; 
    int tag_new_id = 0;
    int required_tag_bit     = 0;
    int tag_kv_len_bytes;
    int tag_kv_bytes;
    unordered_map<int, int> comm_id_map; 
    int comm_id_new_id = 0;
    int required_comm_id_bit = 0;
    int comm_id_kv_len_bytes;
    int comm_id_kv_bytes;

    int meta_bytes;
    int required_event_bits;
    int required_event_bytes;

    char* compressed_data;

    for (int i = 0; i < msg_ids.size(); i++) {
      msg_id = msg_ids[i];
      set_new_value( source_map,   msg_id->source,  source_new_id++);
      set_new_value(    tag_map,      msg_id->tag,     tag_new_id++);
      set_new_value(comm_id_map,  msg_id->comm_id, comm_id_new_id++);
      //      fprintf(stderr, "%s\n", msg_ids[i]->to_string().c_str());
    }

    /*  source KV */
    source_kv_len_bytes  = sizeof(int);
    source_kv_bytes      = sizeof(int) *  source_map.size();
    /*     tag KV */
    tag_kv_len_bytes     = sizeof(int);
    tag_kv_bytes         = sizeof(int) *     tag_map.size();
    /* comm_id KV */
    comm_id_kv_len_bytes = sizeof(int);
    comm_id_kv_bytes     = sizeof(int) * comm_id_map.size();
    /*       meta */
    meta_bytes           = sizeof(int);                                   
    /*     events */
    required_source_bit   = compute_required_bit( source_map.size());
    required_tag_bit      = compute_required_bit(    tag_map.size());
    required_comm_id_bit  = compute_required_bit(comm_id_map.size());
    required_event_bits   = (required_source_bit + required_tag_bit + required_comm_id_bit) * msg_ids.size() ;
    required_event_bytes  = required_event_bits / 8;
    required_event_bytes += (required_event_bits % 8 == 0)? 0:1;      

    compressed_bytes = 
      +  source_kv_len_bytes +  source_kv_bytes   /*  source KV */
      +     tag_kv_len_bytes +     tag_kv_bytes   /*     tag KV */
      + comm_id_kv_len_bytes + comm_id_kv_bytes   /* comm_id KV */
      + meta_bytes                                /*       meta */
      + required_event_bytes;                     /*     events */

    /*TODO: use rempi_malloc*/
    compressed_data = (char*)rempi_malloc(compressed_bytes);

    /*  source KV */
    cpy_kv(compressed_data, source_map, source_kv_len_bytes);      
    compressed_data += source_kv_len_bytes + source_kv_bytes;
    /* tag KV */
    cpy_kv(compressed_data, source_map, source_kv_len_bytes);
    compressed_data += tag_kv_len_bytes + tag_kv_bytes;
    /* comm_id KV */
    cpy_kv(compressed_data, source_map, source_kv_len_bytes);
    compressed_data += comm_id_kv_len_bytes + comm_id_kv_bytes;
    /* meta  */
    {
      int val = msg_ids.size();
      memcpy(compressed_data, &val, meta_bytes);
      compressed_data += meta_bytes;
    }
    /* events */
    for (int i = 0; i < msg_ids.size(); i++) {
      msg_id = msg_ids[i];
    }
    
    fprintf(stderr, "%lu %lu %lu \n", source_map.size(), tag_map.size(), comm_id_map.size());
    fprintf(stderr, "%d %d %d \n", required_source_bit, required_tag_bit, required_comm_id_bit);
    fprintf(stderr, "%d \n", required_event_bytes);
    fprintf(stderr, "%lu \n", compressed_bytes);

    return NULL;
  }

  static void binary_decompress(
      char* binary_compressed_data, 		
      vector<rempi_message_identifier*> &decompressed_msg_id)
  {
    return;
  }
};
