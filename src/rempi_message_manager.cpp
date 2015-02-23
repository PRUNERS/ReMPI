#include <unordered_map>
#include <stdexcept>
#include <algorithm>
#include <string>

#include "mpi.h"
#include "rempi_message_manager.h"
#include "rempi_err.h"
#include "rempi_mem.h"

using namespace std;

bool rempi_message_identifier::operator==(rempi_message_identifier &msg_id) 
{
  if (this->source  == msg_id.source  &&
      this->tag     == msg_id.tag     &&
      this->comm_id == msg_id.comm_id) {
    return true;
  }
  return false;
}

bool rempi_message_identifier::operator<(rempi_message_identifier &msg_id) 
{
  return this->clock < msg_id.clock;
}

bool rempi_message_identifier::operator>(rempi_message_identifier &msg_id) 
{
  return this->clock > msg_id.clock;
}

string rempi_message_identifier::to_string()
{
  return "(source: " + std::to_string((long long)this->source) + 
           ", tag: " + std::to_string((long long)this->tag) + 
       ", comm_id: " + std::to_string((long long)this->comm_id) + ")";
}

// size_t rempi_message_identifier::compress_value(size_t val, size_t bits)
// {
//   size_t compressed_val = 0;
//   for (int i = 0; i < bits; i++) {
//     compressed_val |= val & 1;
//     compressed_val << 1;
//     val >> 1;
//   }  
//   return compressed_val;
// }


/*If compressed data requires over than sizeof(size_t) bytes, this function returns errors */
size_t rempi_message_identifier::compress(size_t source_value, size_t source_bits, size_t tag_value, size_t tag_bits, size_t comm_id_value, size_t comm_id_bits)
{
  int one_byte_in_bits = 8;
  int max_size_bits = sizeof(size_t) * one_byte_in_bits;
  int compressed_size_in_bits = source_bits + tag_bits + comm_id_bits;
  size_t compressed_msg_id = 0;

  /*TODO: remove this limit*/
  if (max_size_bits < compressed_size_in_bits)  {
    REMPI_ERR("Compressed message identifier (source, tag, comm_id) size must be less than or eaqual to %d bits, but the size is %d bits", 
	      max_size_bits, compressed_size_in_bits);
  }

#define compress_value(target) \
  for (int i = 0; i < target ## _bits; i++) { \
    compressed_msg_id |= target ## _value & 1; \
    compressed_msg_id << 1; \
    target ## _value >> 1; \
  } 

  compress_value(source);
  compress_value(tag);
  compress_value(comm_id);

#undef compress_value

  return compressed_msg_id;
}

void rempi_message_manager::add_pending_recv(MPI_Request *request, int source, int tag, int comm_id)
{
  REMPI_DBG("Request: %p source %d tag %d comm_id %d", request, source, tag, comm_id);
  if (pending_recvs.find(request) == pending_recvs.end()) {
    /*If pending_recvs does not have key:request*/
    pending_recvs[request] = new rempi_message_identifier(source, tag, comm_id);
  } else {
    /*else, the user is calling next MPI_recieve with the same request 
      as privious before the prefious reveive matches*/
    REMPI_ERR(" Caling MPI_Irecv before the previous MPI_Irecv with the same MPI_request matches");
  }

  return;
}

int  rempi_message_manager::query_pending_comm_id(MPI_Request *request)
{
  rempi_message_identifier *msg_id;

  try {
    /*Making sure that the Key exesits, so this ordered_map does not create new key*/
    pending_recvs.at(request);
  } catch(out_of_range&) {
    REMPI_ERR(" Testing/Proving before MPI_Irecv: %p", request);
  }

  msg_id = pending_recvs[request];
  return msg_id->comm_id;
}


int rempi_message_manager::add_matched_recv(MPI_Request *request, int source, int tag)
{
  rempi_message_identifier *msg_id;
  REMPI_DBG("Marched Request: %p source %d tag %d", request, source, tag);
  try {
    /*Make sure that the Key exesits, so this ordered_map does not create new key*/
    pending_recvs.at(request);
  } catch(out_of_range&) {
    REMPI_ERR(" Testing/Proving before MPI_Irecv: %p", request);
  }

  msg_id = pending_recvs[request];
  /*Source: (This checking is not needed as long as MPI is working properly) This is just for debugging*/
  bool is_any_source     = msg_id->source == MPI_ANY_SOURCE;
  bool is_source_matched = msg_id->source == source;
  if (!(is_any_source || is_source_matched)) {
    REMPI_ERR(" Inconsistent receive: Request:%p Matched Msg(source: %d, tag: %d) by Request(source: %d, tag:%d)",  request, source, tag, msg_id->source, msg_id->tag);
  }
  /*TAG: (This checking is not needed as long as MPI is working properly) This is just for debugging*/
  bool is_any_tag     = msg_id->tag == MPI_ANY_TAG;
  bool is_tag_matched = msg_id->tag == tag;
  if (!( is_any_tag || is_tag_matched)) {
    REMPI_ERR(" Inconsistent receive: Matched Msg (source: %d, tag: %d) by Request(source: %d, tag:%d)", 
	      source, tag, msg_id->source, msg_id->tag);
  }
  /*The message matched, so move from pending_recvs to matched_recv*/
  pending_recvs.erase(request);
  msg_id->source = source;
  msg_id->tag    = tag;
  matched_recvs.push_back(msg_id);

  return msg_id->comm_id;
}

bool  rempi_message_manager::is_matched_recv(int source, int tag, int comm_id)
{
  rempi_message_identifier *msg_id;

  msg_id = new rempi_message_identifier(source, tag, comm_id);
  auto result = find(matched_recvs.begin(), matched_recvs.end(), msg_id);
  delete msg_id;
  return result != matched_recvs.end();
}


void rempi_message_manager::refresh_matched_recv()
{
  unordered_map<MPI_Request*, rempi_message_identifier*>::iterator pending_msg_it;
  MPI_Request *request;
  int i = 0;
  int flag;
  MPI_Status matched_request_status;


  /*TODO: Use Testsome for more performance improvement (maybe)*/
  for (pending_msg_it = pending_recvs.begin(); pending_msg_it != pending_recvs.end(); pending_msg_it++) {
    request = pending_msg_it->first;
    PMPI_Test(request, &flag, &matched_request_status);
    if (flag) break;
  }
  add_matched_recv(
	   request,
	   matched_request_status.MPI_SOURCE,
           matched_request_status.MPI_TAG);;
  return;
}

void rempi_message_manager::remove_matched_recv(int source, int tag, int comm_id)
{
  rempi_message_identifier *msg_id;
  list<rempi_message_identifier*>::iterator msg_id_it;

  msg_id = new rempi_message_identifier(source, tag, comm_id);

  for (msg_id_it = matched_recvs.begin(); msg_id_it != matched_recvs.end(); msg_id_it++) {
    if(*msg_id_it == msg_id) {
      matched_recvs.erase(msg_id_it);
      delete *msg_id_it;
      delete msg_id;
      return;
    }
  }
  REMPI_ERR("Matched recv(source: %d, tag:%d, comm_id:%d) does not exist", source, tag, comm_id);
  return;
}

void rempi_message_manager::print_pending_recv() 
{
  unordered_map<MPI_Request*, rempi_message_identifier*>::iterator msg_id_it;
  rempi_message_identifier *msg_id;
  MPI_Request *request;
  for (msg_id_it = pending_recvs.begin(); msg_id_it != pending_recvs.end(); msg_id_it++) {
    request = msg_id_it->first;
    msg_id = msg_id_it->second;
    REMPI_DBG("<%p:%s> ", request,  msg_id->to_string().c_str());
  }
  return;
}

void rempi_message_manager::print_matched_recv() 
{
  list<rempi_message_identifier*>::iterator msg_id_it;
  rempi_message_identifier *msg_id;
  for (msg_id_it = matched_recvs.begin(); msg_id_it != matched_recvs.end(); msg_id_it++) {
    msg_id = *msg_id_it;
    REMPI_DBG("%s ",  msg_id->to_string().c_str());
  }
  return;
}




