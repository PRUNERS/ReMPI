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
#ifndef __REMPI_COMPRESSION_UTIL_H__
#define __REMPI_COMPRESSION_UTIL_H__

#include <vector>

using namespace std;

template <class T> 
class rempi_compression_util
{
 public:
  unsigned char*  compress_by_zero_one_binary(vector<T> &vec, size_t &output_size);
  void decompress_by_zero_one_binary(unsigned char* bin,  size_t length, vector<T> &vec);
  void   compress_by_linear_prediction(vector<T> &vec);
  void decompress_by_linear_prediction(vector<T> &vec);
  size_t compress_by_zlib_vec(vector<char*>  &input_vec,  vector<size_t>  &input_size_vec,
			      vector<char*> &output_vec,  vector<size_t> &output_size_vec, size_t &total_size);
  size_t decompress_by_zlib_vec(vector<char*>  &input_vec, vector<size_t>  &input_size_vec,
				vector<char*> &output_vec, vector<size_t> &output_size_vec, size_t &total_size);
  char*  compress_by_zlib(char* input, size_t input_size, size_t &output_size);
  void decompress_by_zlib(char* out, size_t size);
};


#endif /* __REMPI_COMPRESSION_UTIL_H__ */
