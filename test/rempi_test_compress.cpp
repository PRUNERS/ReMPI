#include <vector>
#include <set>
#include <map>
#include <fstream>
#include <algorithm>
#include <utility>

#include <zlib.h>

#include "../src/rempi_message_manager.h"
#include "../src/rempi_clock_delta_compression.h"
#include "../src/rempi_compress.h"
#include "../src/rempi_compression_util.h"
#include "../src/rempi_err.h"
#include "../src/rempi_util.h"

using namespace std;

string file;

void test_simple() {
  vector<rempi_message_identifier*> msg_ids;
  char* compressed_data;
  size_t compressed_size;
  // msg_ids.push_back(new rempi_message_identifier(2, 1, 1));
  // msg_ids.push_back(new rempi_message_identifier(1, 1, 1));
  // msg_ids.push_back(new rempi_message_identifier(3, 1, 1));
  // msg_ids.push_back(new rempi_message_identifier(1, 1, 1));
  // msg_ids.push_back(new rempi_message_identifier(2, 1, 1));
  // msg_ids.push_back(new rempi_message_identifier(3, 1, 1));
  // msg_ids.push_back(new rempi_message_identifier(1, 1, 1));
  // msg_ids.push_back(new rempi_message_identifier(3, 1, 1));
  //  compressed_data = rempi_compress::binary_compress(msg_ids, compressed_size);
}

bool compare(rempi_event* event1,
             rempi_event* event2)
{
  if (event1->get_clock() < event2->get_clock()) {
    return true;
  } else if (event1->get_clock() == event2->get_clock()) {
    return event1->get_source() < event2->get_source();
  }
  return false;
}


void test_clock_delta() {
  map<int, rempi_event*> msg_ids_clocked;
  vector<rempi_event*> msg_ids_observed;
  vector<rempi_event*> *sorted_msg_ids_observed;
  rempi_event *e;
  size_t compressed_size;
  char* compressed_data;

  int id = 0;
  double s_add, e_add, s_comp, e_comp;


#define ADD_EVENT(rank, clock)			\
  e = new rempi_test_event(1, 0, 0, 1, rank, 0, clock, 0);	\
  msg_ids_observed.push_back(e);

  s_add = rempi_get_time();

  /*test 1*/
  // ADD_EVENT(3, 10);
  // ADD_EVENT(2, 11);
  // ADD_EVENT(1, 12);
  // ADD_EVENT(3, 16);
  // ADD_EVENT(2, 29);
  // ADD_EVENT(1, 18);
  // ADD_EVENT(3, 31);
  // ADD_EVENT(2, 31);
  // ADD_EVENT(3, 36);
  // ADD_EVENT(1, 22);
  // ADD_EVENT(2, 36);
  // ADD_EVENT(1, 36);

  /*test 2*/
  ADD_EVENT(1, 32); /*8*/
  ADD_EVENT(1, 11); /*1*/
  ADD_EVENT(1, 17); /*4*/
  ADD_EVENT(1, 36); /*9*/
  ADD_EVENT(1, 31); /*7*/
  ADD_EVENT(1, 12); /*2*/
  ADD_EVENT(1, 22); /*6*/
  ADD_EVENT(1, 10); /*0*/
  ADD_EVENT(1, 16); /*3*/
  ADD_EVENT(1, 18); /*5*/  

#undef ADD_EVENT
  
  sorted_msg_ids_observed = new vector<rempi_event*>(msg_ids_observed);
  sort(sorted_msg_ids_observed->begin(), sorted_msg_ids_observed->end(), compare);
  for (int i = 0; i < sorted_msg_ids_observed->size(); i++) {
    msg_ids_clocked[i] = sorted_msg_ids_observed->at(i);
    sorted_msg_ids_observed->at(i)->clock_order = i;
  }

  
  for (rempi_event *e: msg_ids_observed) {
    REMPI_DBG(" rank: %d , clock: %d", e->get_source(), e->get_clock());
  }

  for (auto &p: msg_ids_clocked) {
    int id = p.first;
    rempi_event *e = p.second;
    REMPI_DBG(" order: %d , rank: %d clock: %d", id, e->get_source(), e->get_clock());
  }
  

  e_add = rempi_get_time();
  REMPI_DBG("ADD: %f", e_add - s_add);

  rempi_clock_delta_compression *cdc = new rempi_clock_delta_compression(1);
  
  s_comp = rempi_get_time();
  compressed_data = cdc->compress(msg_ids_clocked, msg_ids_observed, compressed_size);
  e_comp = rempi_get_time();
  REMPI_DBG("COM: %f", e_comp - s_comp);

#if 1
  {
    int i;
    int *compressed_data_int = (int*)compressed_data;
    for (i = 0; i < compressed_size/4; i++) {
      REMPI_DBG("  %d", compressed_data_int[i]);
    }
  }
#endif
}


void split(std::vector<std::string> &v, const std::string &input_string, const std::string &delimiter)
{
  std::string::size_type index = input_string.find_first_of(delimiter);

  if (index != std::string::npos) {
    v.push_back(input_string.substr(0, index));
    split(v, input_string.substr(index + 1), delimiter);
  } else {
    v.push_back(input_string);
  }
}




bool test_clock_compare(const pair<int, int> &id1,
			const pair<int, int> &id2)
{
  if (id1.first == id2.first) {
    return id1.second < id2.second;
  }
  return id1.first < id2.first;
}


void test_zlib(size_t length)
{
  vector<char*> input_vec, output_vec, output_vec2;
  vector<size_t>  input_size_vec, output_size_vec, output_size_vec2;
  size_t num_array = 8;
  size_t total_size = 0;

  rempi_compression_util<int> *cutil = new rempi_compression_util<int>();

  for (int i = 0; i < num_array; i++) {
    size_t size = sizeof(int) * length;
    int *int_array =  (int*)malloc(sizeof(int) * length);
    for (int j = 0; j < length; j++) {
      int_array[j] = rand() / 1000;
    }
    input_vec.push_back((char*)int_array);
    input_size_vec.push_back(size);
    total_size += size;
  }
  int *array = (int*)input_vec[0];
  size_t sum = 0;
  for (int i = 0; i < input_size_vec.size(); i++) {
    sum += input_size_vec[i];
  }
  //  fprintf(stderr, "Original size  : %10lu %d %lu\n", total_size, array[0], sum);
  cutil->compress_by_zlib_vec(input_vec, input_size_vec, output_vec, output_size_vec, total_size);
  sum = 0;
  for (int i = 0; i < output_size_vec.size(); i++) {
    sum += output_size_vec[i];
  }
  //  fprintf(stderr, "compressed size: %10lu length: %lu %lu\n", total_size, output_vec.size(), sum);
  // for (int i = 0; i < output_size_vec.size(); i++) {
  //   fprintf(stderr, "%lu\n", output_size_vec[i]);
  // }
  //  fprintf(stderr, "               : %p %lu\n", output_vec.front(). output_size_vec.front());
  // char*  a = output_vec.front();
  // size_t s = output_size_vec.front();
  // fprintf(stderr, "               : %p %lu\n", a, s);
  cutil->decompress_by_zlib_vec(output_vec, output_size_vec, output_vec2, output_size_vec2, total_size);
  array = (int*)output_vec[0];
  sum = 0;
  for (int i = 0; i < output_size_vec2.size(); i++) {
    sum += output_size_vec2[i];
  }
  //  fprintf(stderr, "Original size  : %10lu %d %lu %lu\n", total_size, array[0], sum, output_vec2.size());
  // char *final_char = (char*)malloc(total_size);
  // for (int i = 0; i < output_vec2.size(); i++) {
  //   memcpy(final_int, output_vec2[i], output_size_vec2[i]);
  //   final_int += output_size_vec2[i];
  // }

  // vector<>
  


  return;
}

int test_lp(int length)
{
  vector<int> test_vec;
  vector<int> test_vec2;
  // test_vec.push_back(1);
  // test_vec.push_back(3);
  // test_vec.push_back(2);
  // test_vec.push_back(5);
  for (int i = 0; i < length; i++) {
    test_vec.push_back(rand()/1000);
  }
  test_vec2 = test_vec;
  rempi_compression_util<int> *cutil = new rempi_compression_util<int>();  
  cutil->compress_by_linear_prediction(test_vec);
  cutil->decompress_by_linear_prediction(test_vec);
  for (int i = 0; i < test_vec.size(); i++) {
    if(test_vec2[i] != test_vec[i]) {
      fprintf(stderr, "Error in index: %d\n: should be %d but %d\n", i, test_vec2[i], test_vec[i]);
    }
  }
  return 0;
}

int test_bin(int length)
{
  vector<int> test_vec;
  vector<int> test_vec2;
  unsigned char* bin;
  size_t size;
  rempi_compression_util<int> *cutil = new rempi_compression_util<int>();  

   for (int i = 0; i < length; i++) {
        test_vec.push_back(rand() % 2);
   }
   test_vec2 = test_vec;
   bin = cutil->compress_by_zero_one_binary(test_vec, size);
   test_vec.clear();
   cutil->decompress_by_zero_one_binary(bin, test_vec2.size(), test_vec);

  for (int i = 0; i < test_vec2.size(); i++) {
    if(test_vec2[i] != test_vec[i]) {
      fprintf(stderr, "Error in index: %d: should be %d but %d\n", i, test_vec2[i], test_vec[i]);
    }
  }
  return 0;
}


int main(int argc, char* argv[]) {

  test_clock_delta();

#if 0
 if (argc != 2) {
   REMPI_ERR("a.out <file>");
 }
 file = string(argv[1]);
 // test_clock_delta2();
 // test_clock_delta3();

 for (int i = 1; i < 1024 * 1024; i = i*4) {
   for (int j = -3; j <= 3; j++) {
     //     REMPI_DBG("i: %d", i);
     int s = i + j;
     if (s < 1) continue;
     test_zlib(s);
   }
 }
 REMPI_DBG("Zib OK");
 for (int i = 1; i < 1000; i++) {
   test_lp(i);
 }
 REMPI_DBG("LP OK");
 for (int i = 1; i < 1000; i++) {
   test_bin(i);
 }
 REMPI_DBG("BIN OK");

 return 0;
#endif
}


