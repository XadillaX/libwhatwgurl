#ifndef INCLUDE_TEMP_STRING_BUFFER_H_
#define INCLUDE_TEMP_STRING_BUFFER_H_

#include <string>
#include "utils/resizable_buffer.h"

namespace whatwgurl {

class TempStringBuffer {
 public:
  explicit TempStringBuffer(size_t init_max_len = 0);

  inline size_t length() const { return _length; }

  inline void SetEmpty() {
    _length = 0;
    _buffer.buffer()[0] = '\0';
  }

  void Append(char c);
  void Append(const std::string& str);
  void Append(const char* str, size_t len);

  void Prepend(char c);
  void Prepend(const std::string& str);
  void Prepend(const char* str, size_t len);

  void Replace(size_t pos, char c);

  inline const unsigned char* unsigned_string() const {
    const unsigned char* buffer = _buffer.buffer();

    // Dummy const function, lazy append 0
    const_cast<unsigned char*>(buffer)[_length] = 0;

    return buffer;
  }

  inline const char* string() const {
    const unsigned char* buffer = unsigned_string();
    return reinterpret_cast<const char*>(buffer);
  }

  inline const char* operator*() const { return string(); }

  inline uint32_t ToInteger() const {
    // TODO(XadillaX): Performance should be improved.
    uint32_t result = 0;
    const char* buffer = this->string();
    for (size_t i = 0; i < _length; i++) {
      if (buffer[i] >= '0' && buffer[i] <= '9') {
        if (result > (UINT32_MAX - (buffer[i] - '0')) / 10) {
          return UINT32_MAX;
        }

        result = result * 10 + buffer[i] - '0';
      } else {
        return 0;
      }
    }

    return result;
  }

  void Reset(size_t max_len) {
    _length = 0;
    _buffer.Realloc(max_len);
  }

 private:
  ResizableBuffer<unsigned char> _buffer;
  size_t _length;
};

}  // namespace whatwgurl

#endif  // INCLUDE_TEMP_STRING_BUFFER_H_
