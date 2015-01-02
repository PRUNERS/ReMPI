#include <vector>

#include "../src/rempi_message_manager.h"
#include "../src/rempi_compress.h"

int main(int argc, char* argv[]) {
  vector<rempi_message_identifier*> msg_ids;
  char* compressed_data;
  size_t compressed_size;
  msg_ids.push_back(new rempi_message_identifier(2, 1, 1));
  msg_ids.push_back(new rempi_message_identifier(1, 1, 1));
  msg_ids.push_back(new rempi_message_identifier(3, 1, 1));
  msg_ids.push_back(new rempi_message_identifier(1, 1, 1));
  msg_ids.push_back(new rempi_message_identifier(2, 1, 1));
  msg_ids.push_back(new rempi_message_identifier(3, 1, 1));
  msg_ids.push_back(new rempi_message_identifier(1, 1, 1));
  msg_ids.push_back(new rempi_message_identifier(3, 1, 1));
  compressed_data = rempi_compress::binary_compress(msg_ids, compressed_size);
  return 0;
}
