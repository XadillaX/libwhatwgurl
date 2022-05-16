#include "temp_string_buffer.h"
#include <string.h>

namespace whatwgurl {

#define MAX_STEP (1024)

TempStringBuffer::TempStringBuffer(size_t init_max_len)
    : _buffer(init_max_len + 1) {
  _buffer.buffer()[0] = 0;
  _length = 0;
}

void TempStringBuffer::Append(char c) {
  if (_length + 1 + 1 > _buffer.length()) {
    ReleasedResizableBuffer<unsigned char> released = _buffer.Release();
    size_t new_length =
        std::min(released.length << 1, released.length + MAX_STEP);
    _buffer.Realloc(new_length);
    memcpy(_buffer.buffer(), released.buffer, released.length);
    released.Free();
  }

  _buffer.buffer()[_length++] = c;
}

void TempStringBuffer::Append(const std::string& str) {
  if (_length + str.length() + 1 > _buffer.length()) {
    ReleasedResizableBuffer<unsigned char> released = _buffer.Release();
    size_t new_length =
        std::max(std::min(released.length << 1, released.length + MAX_STEP),
                 _length + str.length() + 1);
    _buffer.Realloc(new_length);
    memcpy(_buffer.buffer(), released.buffer, released.length);
    released.Free();
  }

  memcpy(_buffer.buffer() + _length, str.c_str(), str.length());
  _length += str.length();
}

void TempStringBuffer::Append(const char* str, size_t len) {
  if (_length + len + 1 > _buffer.length()) {
    ReleasedResizableBuffer<unsigned char> released = _buffer.Release();
    size_t new_length =
        std::max(std::min(released.length << 1, released.length + MAX_STEP),
                 _length + len + 1);
    _buffer.Realloc(new_length);
    memcpy(_buffer.buffer(), released.buffer, released.length);
    released.Free();
  }

  memcpy(_buffer.buffer() + _length, str, len);
  _length += len;
}

void TempStringBuffer::Prepend(char c) {
  if (_length + 1 + 1 > _buffer.length()) {
    ReleasedResizableBuffer<unsigned char> released = _buffer.Release();
    size_t new_length =
        std::min(released.length << 1, released.length + MAX_STEP);
    _buffer.Realloc(new_length);

    unsigned char* buffer = _buffer.buffer();
    memcpy(buffer + 1, released.buffer, released.length);
    buffer[0] = c;
    released.Free();
  } else {
    unsigned char* buffer = _buffer.buffer();
    memmove(buffer + 1, buffer, _length);
    buffer[0] = c;
  }

  _length++;
}

void TempStringBuffer::Prepend(const std::string& str) {
  if (_length + str.length() + 1 > _buffer.length()) {
    ReleasedResizableBuffer<unsigned char> released = _buffer.Release();
    size_t new_length =
        std::max(std::min(released.length << 1, released.length + MAX_STEP),
                 _length + str.length() + 1);
    _buffer.Realloc(new_length);
    unsigned char* buffer = _buffer.buffer();
    memcpy(buffer + str.length(), released.buffer, released.length);
    memcpy(buffer, str.c_str(), str.length());
    released.Free();
  } else {
    unsigned char* buffer = _buffer.buffer();
    memmove(buffer + str.length(), buffer, _length);
    memcpy(buffer, str.c_str(), str.length());
  }

  _length += str.length();
}

void TempStringBuffer::Prepend(const char* str, size_t len) {
  if (_length + len + 1 > _buffer.length()) {
    ReleasedResizableBuffer<unsigned char> released = _buffer.Release();
    size_t new_length =
        std::max(std::min(released.length << 1, released.length + MAX_STEP),
                 _length + len + 1);
    _buffer.Realloc(new_length);
    unsigned char* buffer = _buffer.buffer();
    memcpy(buffer + len, released.buffer, released.length);
    memcpy(buffer, str, len);
    released.Free();
  } else {
    unsigned char* buffer = _buffer.buffer();
    memmove(buffer + len, buffer, _length);
    memcpy(buffer, str, len);
  }

  _length += len;
}

void TempStringBuffer::Replace(size_t pos, char c) {
  CHECK(pos < _length);
  _buffer.buffer()[pos] = c;
}

}  // namespace whatwgurl
