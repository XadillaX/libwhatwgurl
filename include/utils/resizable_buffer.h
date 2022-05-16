#ifndef INCLUDE_UTILS_RESIZABLE_BUFFER_H_
#define INCLUDE_UTILS_RESIZABLE_BUFFER_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "utils/assert.h"

// Refer: https://github.com/XadillaX/node-sfml/blob/main/src/resizable_buffer.h
namespace whatwgurl {

template <typename T>
struct ReleasedResizableBuffer {
  T* buffer;
  size_t length;
  size_t byte_length;

  inline void Free() {
    if (buffer) {
      free(buffer);
      buffer = nullptr;
    }
  }
};

template <typename T>
class ResizableBuffer {
 public:
  inline ResizableBuffer()
      : _buffer(nullptr),
        _length(0),
        _real_length(0),
        _byte_length(0),
        _page_size(getpagesize()) {}

  explicit ResizableBuffer(size_t initial_length) : ResizableBuffer() {
    Realloc(initial_length);
  }

  inline ~ResizableBuffer() {
    if (_buffer) {
      free(_buffer);
      _buffer = nullptr;
    }
  }

  inline void Realloc(size_t new_length) {
    if (new_length <= _real_length) {
      _length = new_length;
      _byte_length = _length * sizeof(T);
      return;
    }

    if (_buffer != nullptr) {
      free(_buffer);
      _buffer = nullptr;
    }

    _length = new_length;
    _real_length = _length;
    _byte_length = _length * sizeof(T);

    if (_byte_length >= _page_size) {
      CHECK_EQ(
          posix_memalign(
              reinterpret_cast<void**>(&_buffer), _page_size, _byte_length),
          0);
    } else {
      _buffer = reinterpret_cast<T*>(malloc(_byte_length));
    }

    memset(byte_buffer(), 0, _byte_length);
  }

  inline ReleasedResizableBuffer<T> Release() {
    ReleasedResizableBuffer<T> ret;
    ret.buffer = _length ? _buffer : nullptr;
    ret.length = _length;
    ret.byte_length = _byte_length;

    _buffer = _length ? nullptr : _buffer;
    _real_length = _length ? 0 : _real_length;

    _length = 0;
    _byte_length = 0;

    return ret;
  }

  inline T* operator*() { return _buffer; }
  inline const T* operator*() const { return _buffer; }

  inline T* buffer() { return _buffer; }
  inline const T* buffer() const { return _buffer; }

  inline size_t length() const { return _length; }
  inline size_t byte_length() const { return _byte_length; }

  inline char* byte_buffer() { return reinterpret_cast<char*>(_buffer); }
  inline const char* byte_buffer() const {
    return reinterpret_cast<const char*>(_buffer);
  }

  inline unsigned char* unsigned_byte_buffer() {
    return reinterpret_cast<unsigned char*>(_buffer);
  }
  inline const unsigned char* unsigned_byte_buffer() const {
    return reinterpret_cast<const unsigned char*>(_buffer);
  }

 private:
  T* _buffer;
  size_t _length;
  size_t _real_length;
  size_t _byte_length;
  size_t _page_size;
};

}  // namespace whatwgurl

#endif  // INCLUDE_UTILS_RESIZABLE_BUFFER_H_
