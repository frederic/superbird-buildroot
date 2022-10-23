#include <algorithm> // for std::min
#include <iostream>
#include <cstring>

#include <boost/interprocess/managed_shared_memory.hpp>

#include "CircularBuffer.h"

using namespace boost::interprocess;

CircularBuffer::CircularBuffer(managed_shared_memory &segment, size_t capacity)
  : begin_index_(0)
  , end_index_(0)
  , size_(0)
  , capacity_(capacity)
  , segment_(segment)
{
  void *shptr = segment_.allocate(capacity);
  handle_ = segment_.get_handle_from_address(shptr);
}

CircularBuffer::~CircularBuffer()
{
  void *shptr = segment_.get_address_from_handle(handle_);
  segment_.deallocate(shptr);
}

size_t CircularBuffer::write(managed_shared_memory &segment, const uint8_t *data, size_t bytes)
{
  if (bytes == 0) return 0;

  uint8_t *ptr = static_cast<uint8_t *>(segment.get_address_from_handle(handle_));
  size_t capacity = capacity_;
  size_t bytes_to_write = std::min(bytes, capacity - size_);

  if (bytes_to_write <= capacity - end_index_) {
    memcpy(ptr + end_index_, data, bytes_to_write);
    end_index_ += bytes_to_write;
    if (end_index_ == capacity) end_index_ = 0;
  } else {
    size_t size_1 = capacity - end_index_;
    memcpy(ptr + end_index_, data, size_1);
    size_t size_2 = bytes_to_write - size_1;
    memcpy(ptr, data + size_1, size_2);
    end_index_ = size_2;
  }

  size_ += bytes_to_write;
  return bytes_to_write;
}

size_t CircularBuffer::read(managed_shared_memory &segment, uint8_t *data, size_t bytes)
{
  if (bytes == 0) return 0;

  uint8_t *ptr = static_cast<uint8_t *>(segment.get_address_from_handle(handle_));
  size_t capacity = capacity_;
  size_t bytes_to_read = std::min(bytes, size_);

  if (bytes_to_read <= capacity - begin_index_) {
    memcpy(data, ptr + begin_index_, bytes_to_read);
    begin_index_ += bytes_to_read;
    if (begin_index_ == capacity) begin_index_ = 0;
  } else {
    size_t size_1 = capacity - begin_index_;
    memcpy(data, ptr + begin_index_, size_1);
    size_t size_2 = bytes_to_read - size_1;
    memcpy(data + size_1, ptr, size_2);
    begin_index_ = size_2;
  }

  size_ -= bytes_to_read;
  return bytes_to_read;
}

uint8_t* CircularBuffer::start_ptr(managed_shared_memory &segment)
{
  return static_cast<uint8_t *>(segment.get_address_from_handle(handle_));
}

void CircularBuffer::reset()
{
  begin_index_ = end_index_ = size_ = 0;
}