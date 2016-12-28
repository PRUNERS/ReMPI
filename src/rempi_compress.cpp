/* ==========================ReMPI:LICENSE==========================================   
   Copyright (c) 2016, Lawrence Livermore National Security, LLC.                     
   Produced at the Lawrence Livermore National Laboratory.                            
                                                                                       
   Written by Kento Sato, kento@llnl.gov. LLNL-CODE-711357.                           
   All rights reserved.                                                               
                                                                                       
   This file is part of ReMPI. For details, see https://github.com/PRUNER/ReMPI       
   Please also see the LICENSE file for our notice and the LGPL.                      
                                                                                       
   This program is free software; you can redistribute it and/or modify it under the   
   terms of the GNU General Public License (as published by the Free Software         
   Foundation) version 2.1 dated February 1999.                                       
                                                                                       
   This program is distributed in the hope that it will be useful, but WITHOUT ANY    
   WARRANTY; without even the IMPLIED WARRANTY OF MERCHANTABILITY or                  
   FITNESS FOR A PARTICULAR PURPOSE. See the terms and conditions of the GNU          
   General Public License for more details.                                           
                                                                                       
   You should have received a copy of the GNU Lesser General Public License along     
   with this program; if not, write to the Free Software Foundation, Inc., 59 Temple   
   Place, Suite 330, Boston, MA 02111-1307 USA
   ============================ReMPI:LICENSE========================================= */
#include <unordered_map>
#include <vector>
#include <stdexcept>
#include <algorithm>
#include <string>

#include <stdlib.h>
#include <string.h>

#include "rempi_compress.h"
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

| Original Test | ==> | Unmatched Test | Matched Test |

Unmatched Test
| ID | event_count | offset |
|  0 |          92 |      0 |
|  2 |          34 |      0 |
|  5 |          12 |      1 |
|  9 |          27 |      2 |
| 12 |           5 |      1 |

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



Data format:
                  ------------------------
      Unmatched  |            5           |
                  ------------------------
 Unmatched event |    92      |     0     |
                  ------------------------
 Unmatched event |    34      |     0     |
                  ------------------------
 Unmatched event |    12      |     1     |
                  ------------------------
 Unmatched event |    27      |     2     |
                  ------------------------
 Unmatched event |     5      |     1     |
                  ------------------------
      source  KV |   3  |   2   1   3     |
                  ------------------------
      tag     KV |   1  |   1             |
                  ------------------------
      comm_id KV |   1  |   1             |
                  ------------------------
      meta       |            8           |
                  ------------------------
      event      |  00  |   0    |   0    |
                  ------------------------
      event      |  01  |   0    |   0    |
                  ------------------------
      event      |  11  |   0    |   0    |
                  ------------------------
      event      |  01  |   0    |   0    |
                  ------------------------
      event      |  00  |   0    |   0    |
                  ------------------------
      event      |  11  |   0    |   0    |
                  ------------------------
      event      |  01  |   0    |   0    |
                  ------------------------
      event      |  11  |   0    |   0    |
                  ------------------------



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


int rempi_compress::compute_required_bit(int size) {
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

void rempi_compress::set_new_value(unordered_map<int, int> &target_map, int key, int value) 
{
  try {
    target_map.at(key);
  } catch(out_of_range&) {
    target_map[key] = value;
  }
  return;
}

void rempi_compress::cpy_kv(char* compressed_data, unordered_map<int, int> &target_map, int target_kv_len_bytes)
{
  int val;
  val = target_map.size();
  memcpy(compressed_data, &val, target_kv_len_bytes);
  compressed_data += target_kv_len_bytes;
  for (unordered_map<int, int>::iterator map_it = target_map.begin();
       map_it != target_map.end(); map_it++) {
    int value_key = map_it->first;  /*Actual value*/
    int id_value = map_it->second; /*Encoded id begging from 0*/
    memcpy(compressed_data + id_value, &value_key, sizeof(int));
    REMPI_DBG("id: %d, val: %d", id_value, value_key);
  }
  return;
}


  /*
    Data format:
                  ----------------------------------------------------------------------------------
      Unmatched  |                         Numbor of unmatched test (32bit)                         |
                  ----------------------------------------------------------------------------------
 Unmatched event |           Event count (32bit)          |              Offset (32bit)             |
                  ----------------------------------------------------------------------------------
 Unmatched event |           Event count (32bit)          |              Offset (32bit)             |
                  ----------------------------------------------------------------------------------
                 |                                        :                                         |
                                                          :
                                                          :
                                                          :
                 |                                        :                                         |
                  ----------------------------------------------------------------------------------
 Unmatched event |           Event count (32bit)          |              Offset (32bit)             |
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
                 |                       |                :              |                          |
                                                          :
                                                          :
                                                          :
                 |                       |                :              |                          |
                  ----------------------------------------------------------------------------------
      event      |          source       |               tag             |         comm_id          |
                  ----------------------------------------------------------------------------------
   */
  /*
    1.shift computation
    2.
  */
char* rempi_compress::binary_compress(
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

  char *compressed_msg_ids;
  
  for (int i = 0; i < msg_ids.size(); i++) {
    msg_id = msg_ids[i];
    set_new_value( source_map,   msg_id->source,  source_new_id++);
    set_new_value(    tag_map,      msg_id->tag,     tag_new_id++);
    set_new_value(comm_id_map,  msg_id->comm_id, comm_id_new_id++);
    REMPI_DBG("%s", msg_ids[i]->to_string().c_str());
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

  compressed_msg_ids = (char*)rempi_malloc((size_t)compressed_bytes);

  /*  source KV */
  cpy_kv(compressed_msg_ids, source_map, source_kv_len_bytes);      
  compressed_msg_ids += source_kv_len_bytes + source_kv_bytes;
  /* tag KV */
  cpy_kv(compressed_msg_ids, tag_map, source_kv_len_bytes);
  compressed_msg_ids += tag_kv_len_bytes + tag_kv_bytes;
  /* comm_id KV */
  cpy_kv(compressed_msg_ids, comm_id_map, source_kv_len_bytes);
  compressed_msg_ids += comm_id_kv_len_bytes + comm_id_kv_bytes;
  /* meta  */
  {
    int val = msg_ids.size();
    memcpy(compressed_msg_ids, &val, meta_bytes);
    compressed_msg_ids += meta_bytes;
  }

  /*If compressed data requires over than sizeof(size_t) bytes, this function returns errors */
  {
    int one_byte_in_bits = 8;
    int max_size_bits = sizeof(size_t) * one_byte_in_bits;
    int compressed_size_in_bits = required_source_bit + required_tag_bit + required_comm_id_bit;

    /*TODO: remove this limit*/
    if (max_size_bits < compressed_size_in_bits)  {
      REMPI_ERR("Compressed message identifier (source, tag, comm_id) size must be less than or eaqual to %d bits, but the size is %d bits",
		max_size_bits, compressed_size_in_bits);
    }
  }
  /* events */
  for (int i = 0; i < msg_ids.size(); i++) {
    size_t source_in_bit = source_map[msg_ids[i]->source];
    size_t tag_in_bit = tag_map[msg_ids[i]->tag];
    size_t comm_id_in_bit = comm_id_map[msg_ids[i]->comm_id];
    size_t compressed_msg_id = 0;
    
#define compress_value(target) \
    for (int j = 0; j < required_ ##target ## _bit; j++) { \
      compressed_msg_id = compressed_msg_id << 1;				   \
      compressed_msg_id |= target ## _in_bit & 1;	   \
      target ## _in_bit = target ## _in_bit >> 1;						\
    }

    compress_value(source);
    compress_value(tag);
    compress_value(comm_id);

#undef compress_value

    REMPI_DBG("%s == compress => %lu", msg_ids[i]->to_string().c_str(), compressed_msg_id);

  }
    
  REMPI_DBG("%lu %lu %lu", source_map.size(), tag_map.size(), comm_id_map.size());
  REMPI_DBG("%d %d %d", required_source_bit, required_tag_bit, required_comm_id_bit);
  REMPI_DBG("%d", required_event_bytes);
  REMPI_DBG("%lu", compressed_bytes);

  return NULL;
}

void rempi_compress::binary_decompress(char* binary_compressed_data, vector<rempi_message_identifier*> &decompressed_msg_id)
{
  return;
}

