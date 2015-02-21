#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>

#include <vector>

#include "rempi_compression_util.h"
#include "rempi_err.h"

#define windowBits 15
#define GZIP_ENCODING 16

#define CALL_ZLIB(x) {                                                  \
    int status;								\
    status = x;								\
    if (status < 0) {							\
      fprintf (stderr,							\
	       "%s:%d: %s returned a bad status of %d.\n",		\
	       __FILE__, __LINE__, #x, status);				\
      exit (EXIT_FAILURE);						\
    }									\
  }

using namespace std;

template class rempi_compression_util<int>;
template class rempi_compression_util<size_t>;

template <class T>
char *rempi_compression_util<T>::compress_by_zero_one_binary(vector<T> &vec, size_t &output_size)
{
  char *binary;
  size_t length = vec.size();
  size_t required_size_in_bytes = (length / 8) + 1; // TODO: compute exact required size
  binary = (char*)malloc(required_size_in_bytes);
  memset(binary, 0, required_size_in_bytes);
  int bin_index = 0;
  for (int i = 0; i < length; i++) {
    if (vec[i] == 1) {
      binary[bin_index]  += 1;
    }
    binary[bin_index] <<= 1;	

    if (i % 8 == 0 && i != 0) {
      bin_index++;
    }
  }
  output_size = required_size_in_bytes;
  return binary;
}

template <class T>
void rempi_compression_util<T>::decompress_by_zero_one_binary(char* bin,  size_t length, vector<T> &vec)
{
  return;
}


template <class T>
void rempi_compression_util<T>::compress_by_linear_prediction(vector<T> &vec)
{
  /*
         1 3 5 7   2  4 6 8
     -   0 2 5 7   9 -3 6 8
    -------------------------
         1 1 0 0  -7  7 0 0

         1 3 5 7   1  3 5 7
     -   0 2 5 7   9 -5 6 8
    -------------------------
         1 1 0 0  -8  8 0 0

	  a[i] => a[i] - (a[i-1] + (a[i-1] - a[i-2]))
               => a[i] - 2a[i-1] + a[i-2]
  */
  /*
    1 3 5 7  2 4 6 8
    vec1[i] =  vec [i] - vec [i-1]
    1 2 2 2 -5 2 2 2
    vec2[i] =  vec1[i] + vec1[i-1]
            = (vec [i] - vec [i-1]) + (vec [i-1] - vec [i-2])
            =  vec [i] - vec [i-2]
    1 3 4 4 -3 -3  4 4
    vec3[i] =  vec2[i] - vec2[i-1]
            = (vec [i] - vec [i-1]) - (vec [i-1] - vec [i-2])
            =  vec [i] - 2*vec [i-1] + vec [i-2]
    1 2 1 0 -7 -7  7 7
  */
  T t0 =0, t1 = 0, t2 = 0;
  for (int i = 0; i < vec.size(); i++) {
    t0 = vec[i];
    vec[i] =  t0 - 2 * t1 + t2;
    t2 = t1;
    t1 = t0;
  }
  return;
}


template <class T>
void rempi_compression_util<T>::decompress_by_linear_prediction(vector<T> &vec)
{

}




template <class T>
char* rempi_compression_util<T>::compress_by_zlib(char* input, size_t input_size, size_t &output_size)
{
  char* output;
  z_stream strm;

  strm.zalloc = Z_NULL;
  strm.zfree  = Z_NULL;
  strm.opaque = Z_NULL;
  CALL_ZLIB (
	     deflateInit2(
			  &strm, 
			  Z_DEFAULT_COMPRESSION, 
			  Z_DEFLATED,
			  windowBits | GZIP_ENCODING, 
			  8,
			  Z_DEFAULT_STRATEGY)
	     );

  strm.next_in   = (unsigned char*)input;
  strm.avail_in  = input_size;

  output = (char*)malloc(input_size);
  strm.next_out  = (unsigned char*)output;
  strm.avail_out = input_size;

  CALL_ZLIB (deflate (&strm, Z_FINISH));

  if (strm.avail_out == 0) {
    /*
      strm.avail_out is remaining buffer size of strm.next_out after deflate.
      The function allocates output buffer with the same size as original size,
      If deflate run out output buffer, this means the compressed size is 
      bigger than original size. Thus this function simply return NULL.
     */
    output_size = 0;
    free(output);
    output = NULL;
  } else {
    output_size = strm.total_out;
  }
  deflateEnd (&strm);

  return output;
}

template <class T>
void rempi_compression_util<T>::decompress_by_zlib(char* out, size_t size)
{
}
