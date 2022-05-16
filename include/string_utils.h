#ifndef INCLUDE_STRING_UTILS_H_
#define INCLUDE_STRING_UTILS_H_

#include <algorithm>
#include <string>
#include "temp_string_buffer.h"

namespace whatwgurl {

typedef bool (*IsSomeCharFunction)(unsigned char);

namespace string_utils {

inline bool HasLeading(const std::string& str,
                       IsSomeCharFunction is_some_char) {
  return !str.empty() && is_some_char(str[0]);
}

inline bool HasTrailing(const std::string& str,
                        IsSomeCharFunction is_some_char) {
  return !str.empty() && is_some_char(str[str.size() - 1]);
}

inline bool TrimLeft(std::string* str, IsSomeCharFunction is_some_char) {
  if (HasLeading(*str, is_some_char)) {
    str->erase(str->begin(),
               std::find_if(str->begin(), str->end(), [&](unsigned char c) {
                 return !is_some_char(c);
               }));
  }
  return false;
}

inline bool TrimRight(std::string* str, IsSomeCharFunction is_some_char) {
  if (HasTrailing(*str, is_some_char)) {
    str->erase(std::find_if(str->rbegin(),
                            str->rend(),
                            [&](unsigned char c) { return !is_some_char(c); })
                   .base(),
               str->end());
  }
  return false;
}

inline bool Trim(std::string* str, IsSomeCharFunction is_some_char) {
  return TrimLeft(str, is_some_char) || TrimRight(str, is_some_char);
}

inline bool Remove(std::string* str, IsSomeCharFunction is_some_char) {
  size_t init_len = str->size();
  str->erase(std::remove_if(str->begin(), str->end(), is_some_char),
             str->end());
  return init_len != str->size();
}

inline char ToLower(char c) {
  return c >= 'A' && c <= 'Z' ? c + ('a' - 'A') : c;
}

inline bool Contains(const std::string& str, IsSomeCharFunction is_some_char) {
  return std::find_if(str.begin(), str.end(), is_some_char) != str.end();
}

inline bool Contains(const TempStringBuffer& str,
                     IsSomeCharFunction is_some_char) {
  const unsigned char* ptr = str.unsigned_string();
  const unsigned char* end = ptr + str.length();
  while (ptr != end) {
    if (is_some_char(*ptr)) {
      return true;
    }
    ++ptr;
  }
  return false;
}

}  // namespace string_utils
}  // namespace whatwgurl

#endif  // INCLUDE_STRING_UTILS_H_
