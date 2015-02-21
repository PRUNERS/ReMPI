#ifndef __REMPI_COMPRESSION_UTIL_H__
#define __REMPI_COMPRESSION_UTIL_H__

#include <vector>

using namespace std;

template <class T> 
class rempi_compression_util
{
 public:
  char*  compress_by_zero_one_binary(vector<T> &vec, size_t &output_size);
  void decompress_by_zero_one_binary(char* bin,  size_t length, vector<T> &vec);
  void   compress_by_linear_prediction(vector<T> &vec);
  void decompress_by_linear_prediction(vector<T> &vec);
  char*  compress_by_zlib(char* input, size_t input_size, size_t &output_size);
  void decompress_by_zlib(char* out, size_t size);
};


#endif /* __REMPI_COMPRESSION_UTIL_H__ */
