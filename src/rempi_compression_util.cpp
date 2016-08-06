#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>
#include <assert.h>

#include <vector>

#include "rempi_compression_util.h"
#include "rempi_err.h"

#define windowBits 15
#define GZIP_ENCODING 16
#ifdef REMPI_LITE
#define ZLIB_CHUNK (16 * 1014 * 1024)
#else
#define ZLIB_CHUNK (1 * 1024 * 1024)
#endif

#define CALL_ZLIB(x) {                                                  \
    int status;								\
    status = x;								\
    if (status < 0) {							\
      REMPI_ERR(" %s returned a bad status of %d", #x, status);		\
    }									\
  }

using namespace std;


template <class T>
unsigned char *rempi_compression_util<T>::compress_by_zero_one_binary(vector<T> &vec, size_t &output_size)
{
  unsigned char *binary;
  size_t length = vec.size();
  if (length == 0) {
    output_size = 0;
    return NULL;
  }
  size_t required_size_in_bytes = (length / 8) + ((length % 8 != 0)? 1:0); // TODO: compute exact required size
  binary = (unsigned char*)malloc(required_size_in_bytes);
  memset(binary, 0, required_size_in_bytes);
  int bin_index = 0;
  unsigned int i;
  for (i = 0; i < length; i++) {
    binary[bin_index] <<= 1;	
    if (vec[i] > 0) {
      binary[bin_index]  |= 1;
    }
    if (i % 8 == 7 && i != 0) {
      bin_index++;
    }
  }
  
  output_size = required_size_in_bytes;
  return binary;
}

template <class T>
void rempi_compression_util<T>::decompress_by_zero_one_binary(unsigned char* bin,  size_t length, vector<T> &vec)
{
  unsigned char b;
  size_t count = length;
  int index = 0;
  if (vec.size() != 0) {
    REMPI_ERR("vec has values");
  }

  unsigned char offset;
  while (1) {
    b = bin[index];
    index++;
    offset = 0x00000080;
    if (count < 8) {
      offset >>= 8 - count;
    }
    for (int j = 0; j < 8; j++) {
      if ((b & offset) != 0) {
	//	REMPI_DBG("1 desu-noo: %p %p %p %p", b, offset, b & offset, offset - b & offset);
	vec.push_back(1);
      } else {
	//	REMPI_DBG("0 desu-noo: %p %p %p %p", b, offset, b & offset, offset - b & offset);
	vec.push_back(0);
      }
      count--;
      offset >>= 1;
      if (count == 0) {return;}
    }
  }
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
    vec2[i] =  vec1[i] + vec1[i-1]
            = (vec [i] - vec [i-1]) + (vec [i-1] - vec [i-2])
            =  vec [i] - vec [i-2]
    vec3[i] =  vec2[i] - vec2[i-1]
            = (vec [i] - vec [i-1]) - (vec [i-1] - vec [i-2])
            =  vec [i] - 2*vec [i-1] + vec [i-2]
    1 1 0 0  -7  7 0 0
  */
  T t0 =0, t1 = 0, t2 = 0;
  for (unsigned int i = 0; i < vec.size(); i++) {
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
  T t0 =0, t1 = 0, t2 = 0;
  for (unsigned int i = 0; i < vec.size(); i++) {
    t0 = vec[i];
    vec[i] =  t0 + 2 * t1 - t2;
    t2 = t1;
    t1 = vec[i];
  }  
  return;
}



template <class T>
char* rempi_compression_util<T>::compress_by_zlib(char* input, size_t input_size, size_t &output_size)
{
  char* output;
  z_stream strm;

  if (input_size == 0) {
    output_size = 0;
    output=NULL;
    return output;
  }
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

  size_t output_buf_size = input_size * 2;
  output = (char*)malloc(output_buf_size);
  strm.next_out  = (unsigned char*)output;
  strm.avail_out = output_buf_size;

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



template <class T>
size_t rempi_compression_util<T>::compress_by_zlib_vec(vector<char*> &input_vec,  vector<size_t> &input_size_vec, 
						       vector<char*> &output_vec, vector<size_t> &output_size_vec, size_t &total_size)
{
  int ret, flush;
  unsigned have;
  //  char* output;
  z_stream strm;

  total_size = 0;

  /* allocate deflate state */
  strm.zalloc = Z_NULL;
  strm.zfree = Z_NULL;
  strm.opaque = Z_NULL;
  ret = deflateInit(&strm, Z_DEFAULT_COMPRESSION);
  if (ret != Z_OK)
    return ret;
  // CALL_ZLIB (
  // 	     deflateInit2(
  // 			  &strm, 
  // 			  Z_DEFAULT_COMPRESSION, 
  // 			  Z_DEFLATED,
  // 			  windowBits | GZIP_ENCODING, 
  // 			  8,
  // 			  Z_DEFAULT_STRATEGY)
  // 	     );

  for (unsigned int i = 0; i < input_size_vec.size(); i++) {
    strm.next_in =  (unsigned char*)input_vec[i];
    strm.avail_in = input_size_vec[i];
    flush = (i == input_size_vec.size() - 1)? Z_FINISH:Z_NO_FLUSH;
    /* run deflate() on input until output buffer not full, finish
       compression if all of source has been read in */
    do {
      unsigned char* out;
      strm.avail_out = ZLIB_CHUNK;
      out =  (unsigned char*)malloc(ZLIB_CHUNK);
      strm.next_out = out;
      ret = deflate(&strm, flush);    /* no bad return value */
      if (ret == Z_STREAM_ERROR) {
	REMPI_ERR("Z_STREAM_ERROR: next_in: %p, avail_in: %lu, next_out: %p (out: %p), avail_out: %lu, flush: %d",
		  strm.next_in, strm.avail_in, strm.next_out, out, strm.avail_out, flush)
      }
      have = ZLIB_CHUNK - strm.avail_out;
      if (have > 0) {
	output_vec.push_back((char*)out);
	output_size_vec.push_back(have);
	total_size += have;
      }
    } while (strm.avail_out == 0); /*Chunk is filled out the data (available buff == 0), and need to compress again*/
    assert(strm.avail_in == 0);
  }
  /* clean up and return */
  (void)deflateEnd(&strm);
  if (ret != Z_STREAM_END) {
    REMPI_ERR("Error in inflate");
  }
  return Z_OK;


#if 0
  do { 
    strm.avail_in = fread(in, 1, CHUNK, source);
    if (ferror(source)) {
      (void)deflateEnd(&strm);
      return Z_ERRNO;
    }
    flush = feof(source) ? Z_FINISH : Z_NO_FLUSH;
    strm.next_in = in;
    /* run deflate() on input until output buffer not full, finish
       compression if all of source has been read in */
    do {
      strm.avail_out = CHUNK;
      strm.next_out = out;
      ret = deflate(&strm, flush);    /* no bad return value */
      assert(ret != Z_STREAM_ERROR);  /* state not clobbered */
      have = CHUNK - strm.avail_out;
      if (fwrite(out, 1, have, dest) != have || ferror(dest)) {
	(void)deflateEnd(&strm);
	return Z_ERRNO;
      }
    } while (strm.avail_out == 0);
    assert(strm.avail_in == 0);     /* all input will be used */
    /* done when last data in file processed */
  } while (flush != Z_FINISH);
  assert(ret == Z_STREAM_END);        /* stream will be complete */

  /* clean up and return */
  (void)deflateEnd(&strm);
  return Z_OK;
#endif
}

template <class T>
size_t rempi_compression_util<T>::decompress_by_zlib_vec(vector<char*>  &input_vec, vector<size_t> &input_size_vec,
							 vector<char*> &output_vec, vector<size_t> &output_size_vec, size_t &total_size)
{
  int ret;
  unsigned have;
  z_stream strm;
  /* allocate inflate state */
  strm.zalloc = Z_NULL;
  strm.zfree = Z_NULL;
  strm.opaque = Z_NULL;
  strm.avail_in = 0;
  strm.next_in = Z_NULL;
  ret = inflateInit(&strm);
  if (ret != Z_OK)
    return ret;

  total_size = 0;
  /* decompress until deflate stream ends or end of file */

  for (unsigned int i = 0; i < input_vec.size(); i++) {
    strm.next_in  = (unsigned char*)input_vec[i];
    strm.avail_in = input_size_vec[i];
    /* run inflate() on input until output buffer not full */
    do {
      unsigned char* out;
      out =  (unsigned char*)malloc(ZLIB_CHUNK);
      strm.next_out = out;
      strm.avail_out = ZLIB_CHUNK;
      ret = inflate(&strm, Z_NO_FLUSH);
      //      ret = inflate(&strm, Z_FINISH);
      assert(ret != Z_STREAM_ERROR);  /* state not clobbered */
      switch (ret) {
      case Z_NEED_DICT:
	//	ret = Z_DATA_ERROR;     /* and fall through */
      case Z_DATA_ERROR:
      case Z_MEM_ERROR:
	(void)inflateEnd(&strm);
	REMPI_ERR("decompress zlib error: %d (Z_NEED_DICT:%d, Z_DATA_ERROR:%d, Z_MEM_ERROR:%d)", 
		  ret, Z_NEED_DICT, Z_DATA_ERROR, Z_MEM_ERROR);
	return ret;
      }
      have = ZLIB_CHUNK - strm.avail_out;
      if (have > 0) {
	output_vec.push_back((char*)out);
	output_size_vec.push_back(have);
	total_size += have;
      }
      //      REMPI_DBG("ret = %d, %d have: %lu", ret, Z_STREAM_END, have);
    } while (strm.avail_out == 0);
    /* done when inflate() says it's done */
  }
  /* clean up and return */
  (void)inflateEnd(&strm);
  if (ret != Z_STREAM_END) {
    REMPI_ERR("Error in inflate");
  }
  return ret == Z_STREAM_END ? Z_OK : Z_DATA_ERROR;
}

template class rempi_compression_util<int>;
template class rempi_compression_util<size_t>;
