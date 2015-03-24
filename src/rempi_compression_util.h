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
