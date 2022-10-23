#ifndef __CIRCULARBUFFER_H
#define __CIRCULARBUFFER_H

#include <boost/interprocess/managed_shared_memory.hpp>

using namespace boost::interprocess;

class CircularBuffer
{
public:
  CircularBuffer(managed_shared_memory &segment, size_t capacity);
  ~CircularBuffer();

  size_t size() const { return size_; }
  size_t capacity() const { return capacity_; }
  size_t write(managed_shared_memory &segment, const uint8_t *data, size_t bytes);
  size_t read(managed_shared_memory &segment, uint8_t *data, size_t bytes);
  void reset();
  uint8_t *start_ptr(managed_shared_memory &segment);

private:
  size_t begin_index_, end_index_, size_, capacity_;
  managed_shared_memory &segment_;
  managed_shared_memory::handle_t handle_;
};

#endif