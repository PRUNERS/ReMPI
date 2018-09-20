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
#ifndef __REMPI_COMPRESS_H__
#define __REMPI_COMPRESS_H__

#include <vector>
#include <set>

//#include "rempi_message_manager.h"
#include "rempi_event.h"


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
 protected:
  /* linear_prediction:1 it          apply linear prediction
     linear_prediction:0 it does not apply linear prediction
     */
  int linear_prediction;
  int next_start_search_it(
			   int msg_id_up_clock,
			   int msg_id_left_clock,
			   map<int, rempi_event*>::const_iterator &msg_ids_clocked_search_it,		 
			   map<int, rempi_event*>::const_iterator &msg_ids_clocked_start_it,
			   int current_column_of_search_it,
			   int current_column_of_start_it);

  int find_matched_clock_column(
				rempi_event *event_left,				    
				map<int, rempi_event*>::const_iterator &msg_ids_clocked_search_it,
				map<int, rempi_event*>::const_iterator const  &msg_ids_clocked_search_it_end);

  int update_start_it(
		      int current_column_of_start_it,
		      int current_column_of_search_it,
		      vector<bool> &matched_bits,
		      map<int, rempi_event*>::const_iterator &msg_ids_clocked_start_it);
  
  void change_to_seq_order_id(
			      list<pair<int, int>*> &diff,
			      vector<rempi_event*> &msg_ids_observed,
			      map<int, int> &map_clock_to_order);
  
  char* convert_to_diff_binary(
			       list<pair<int, int>*> &diff,
			       size_t &compressed_bytes);
  

  virtual bool is_matching(rempi_event *left, rempi_event *up);

 public:
   rempi_clock_delta_compression(int linear_prediction): linear_prediction(linear_prediction) {};
   virtual char* compress(
			  map<int, rempi_event*> &msg_ids_clocked,
			  vector<rempi_event*> &msg_ids_observed,
			  size_t &compressed_bytes);
   virtual void decompress(
			   char* compressed_data,
			   size_t &compressed_bytes,
			   set<rempi_event*> &rempi_ids_clock,
			   vector<rempi_event*> &rempi_ids_real);	

};

class rempi_clock_delta_compression_2 : public rempi_clock_delta_compression
{
 protected:
  virtual bool is_matching(rempi_event *left, rempi_event *up);
 public:
 rempi_clock_delta_compression_2(int linear_prediction): rempi_clock_delta_compression(0){};
  virtual char* compress(
	     map<int, rempi_event*> &msg_ids_clocked,
	     vector<rempi_event*> &msg_ids_observed,
	     size_t &compressed_bytes);
};


#endif
