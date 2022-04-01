#ifndef SRC_WHATWGURL_INL_H_
#define SRC_WHATWGURL_INL_H_

#include <stddef.h>
#include <stdlib.h>
#include <whatwgurl.h>
#include <cstdint>
#include <string>

namespace whatwgurl {

using table_data::hex;

// https://url.spec.whatwg.org/#eof-code-point
constexpr char kEOL = -1;

template <typename T, size_t N>
inline constexpr size_t arraysize(const T (&)[N]) {
  return N;
}

#define CHAR_TEST(bits, name, expr)                                            \
  template <typename T>                                                        \
  inline bool name(const T ch) {                                               \
    static_assert(sizeof(ch) >= (bits) / 8,                                    \
                  "Character must be wider than " #bits " bits");              \
    return (expr);                                                             \
  }

#define ARGS(XX)                                                               \
  XX(kArgFlags)                                                                \
  XX(kArgProtocol)                                                             \
  XX(kArgUsername)                                                             \
  XX(kArgPassword)                                                             \
  XX(kArgHost)                                                                 \
  XX(kArgPort)                                                                 \
  XX(kArgPath)                                                                 \
  XX(kArgQuery)                                                                \
  XX(kArgFragment)                                                             \
  XX(kArgCount)  // This one has to be last.

#define ERR_ARGS(XX)                                                           \
  XX(ErrArgFlags)                                                              \
  XX(ErrArgInput)

#define SPECIALS(XX)                                                           \
  XX(ftp, 21, "ftp:")                                                          \
  XX(file, -1, "file:")                                                        \
  XX(http, 80, "http:")                                                        \
  XX(https, 443, "https:")                                                     \
  XX(ws, 80, "ws:")                                                            \
  XX(wss, 443, "wss:")

enum UrlCbArgs {
#define XX(name) name,
  ARGS(XX)
#undef XX
};

enum url_error_cb_args {
#define XX(name) name,
  ERR_ARGS(XX)
#undef XX
};

#define TWO_CHAR_STRING_TEST(bits, name, expr)                                 \
  template <typename T>                                                        \
  inline bool name(const T ch1, const T ch2) {                                 \
    static_assert(sizeof(ch1) >= (bits) / 8,                                   \
                  "Character must be wider than " #bits " bits");              \
    return (expr);                                                             \
  }                                                                            \
  template <typename T>                                                        \
  inline bool name(const std::basic_string<T>& str) {                          \
    static_assert(sizeof(str[0]) >= (bits) / 8,                                \
                  "Character must be wider than " #bits " bits");              \
    return str.length() >= 2 && name(str[0], str[1]);                          \
  }

// https://infra.spec.whatwg.org/#ascii-tab-or-newline
CHAR_TEST(8, IsASCIITabOrNewline, (ch == '\t' || ch == '\n' || ch == '\r'))

// https://infra.spec.whatwg.org/#c0-control-or-space
CHAR_TEST(8, IsC0ControlOrSpace, (ch >= '\0' && ch <= ' '))

// https://infra.spec.whatwg.org/#ascii-digit
CHAR_TEST(8, IsASCIIDigit, (ch >= '0' && ch <= '9'))

// https://infra.spec.whatwg.org/#ascii-hex-digit
CHAR_TEST(8,
          IsASCIIHexDigit,
          (IsASCIIDigit(ch) || (ch >= 'A' && ch <= 'F') ||
           (ch >= 'a' && ch <= 'f')))

// https://infra.spec.whatwg.org/#ascii-alpha
CHAR_TEST(8,
          IsASCIIAlpha,
          ((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z')))

// https://infra.spec.whatwg.org/#ascii-alphanumeric
CHAR_TEST(8, IsASCIIAlphanumeric, (IsASCIIDigit(ch) || IsASCIIAlpha(ch)))

// https://infra.spec.whatwg.org/#ascii-lowercase
template <typename T>
inline T ASCIILowercase(T ch) {
  return IsASCIIAlpha(ch) ? (ch | 0x20) : ch;
}

// https://url.spec.whatwg.org/#forbidden-host-code-point
CHAR_TEST(8,
          IsForbiddenHostCodePoint,
          ch == '\0' || ch == '\t' || ch == '\n' || ch == '\r' || ch == ' ' ||
              ch == '#' || ch == '%' || ch == '/' || ch == ':' || ch == '?' ||
              ch == '@' || ch == '[' || ch == '<' || ch == '>' || ch == '\\' ||
              ch == ']' || ch == '^' || ch == '|')

// https://url.spec.whatwg.org/#windows-drive-letter
TWO_CHAR_STRING_TEST(8,
                     IsWindowsDriveLetter,
                     (IsASCIIAlpha(ch1) && (ch2 == ':' || ch2 == '|')))

// https://url.spec.whatwg.org/#normalized-windows-drive-letter
TWO_CHAR_STRING_TEST(8,
                     IsNormalizedWindowsDriveLetter,
                     (IsASCIIAlpha(ch1) && ch2 == ':'))

#undef TWO_CHAR_STRING_TEST

inline bool BitAt(const uint8_t a[], const uint8_t i) {
  return !!(a[i >> 3] & (1 << (i & 7)));
}

// Appends ch to str. If ch position in encode_set is set, the ch will
// be percent-encoded then appended.
inline void AppendOrEscape(std::string* str,
                           const unsigned char ch,
                           const uint8_t encode_set[]) {
  if (BitAt(encode_set, ch))
    *str += hex + ch * 4;  // "%XX\0" has a length of 4
  else
    *str += ch;
}

inline int64_t ParseNumber(const char* start, const char* end) {
  unsigned R = 10;
  if (end - start >= 2 && start[0] == '0' && (start[1] | 0x20) == 'x') {
    start += 2;
    R = 16;
  }
  if (end - start == 0) {
    return 0;
  } else if (R == 10 && end - start > 1 && start[0] == '0') {
    start++;
    R = 8;
  }
  const char* p = start;

  while (p < end) {
    const char ch = p[0];
    switch (R) {
      case 8:
        if (ch < '0' || ch > '7') return -1;
        break;
      case 10:
        if (!IsASCIIDigit(ch)) return -1;
        break;
      case 16:
        if (!IsASCIIHexDigit(ch)) return -1;
        break;
    }
    p++;
  }
  return strtoll(start, nullptr, R);
}

template <typename T>
inline unsigned hex2bin(const T ch) {
  if (ch >= '0' && ch <= '9') return ch - '0';
  if (ch >= 'A' && ch <= 'F') return 10 + (ch - 'A');
  if (ch >= 'a' && ch <= 'f') return 10 + (ch - 'a');
  return static_cast<unsigned>(-1);
}

inline std::string PercentDecode(const char* input, size_t len) {
  std::string dest;
  if (len == 0) return dest;
  dest.reserve(len);
  const char* pointer = input;
  const char* end = input + len;

  while (pointer < end) {
    const char ch = pointer[0];
    size_t remaining = end - pointer - 1;
    if (ch != '%' || remaining < 2 ||
        (ch == '%' &&
         (!IsASCIIHexDigit(pointer[1]) || !IsASCIIHexDigit(pointer[2])))) {
      dest += ch;
      pointer++;
      continue;
    } else {
      unsigned a = hex2bin(pointer[1]);
      unsigned b = hex2bin(pointer[2]);
      char c = static_cast<char>(a * 16 + b);
      dest += c;
      pointer += 3;
    }
  }
  return dest;
}

inline bool IsSpecial(const std::string& scheme) {
#define V(_, __, name)                                                         \
  if (scheme == name) return true;
  SPECIALS(V);
#undef V
  return false;
}

inline int NormalizePort(const std::string& scheme, int p) {
#define V(_, port, name)                                                       \
  if (scheme == name && p == port) return -1;
  SPECIALS(V);
#undef V
  return p;
}

// https://url.spec.whatwg.org/#start-with-a-windows-drive-letter
inline bool StartsWithWindowsDriveLetter(const char* p, const char* end) {
  size_t length = end - p;
  return length >= 2 && IsWindowsDriveLetter(p[0], p[1]) &&
         (length == 2 || p[2] == '/' || p[2] == '\\' || p[2] == '?' ||
          p[2] == '#');
}

inline void ShortenUrlPath(UrlData* url) {
  if (url->path.empty()) return;
  if (url->path.size() == 1 && url->scheme == "file:" &&
      IsNormalizedWindowsDriveLetter(url->path[0]))
    return;
  url->path.pop_back();
}

// Single dot segment can be ".", "%2e", or "%2E"
inline bool IsSingleDotSegment(const std::string& str) {
  switch (str.size()) {
    case 1:
      return str == ".";
    case 3:
      return str[0] == '%' && str[1] == '2' && ASCIILowercase(str[2]) == 'e';
    default:
      return false;
  }
}

// Double dot segment can be:
//   "..", ".%2e", ".%2E", "%2e.", "%2E.",
//   "%2e%2e", "%2E%2E", "%2e%2E", or "%2E%2e"
inline bool IsDoubleDotSegment(const std::string& str) {
  switch (str.size()) {
    case 2:
      return str == "..";
    case 4:
      if (str[0] != '.' && str[0] != '%') return false;
      return ((str[0] == '.' && str[1] == '%' && str[2] == '2' &&
               ASCIILowercase(str[3]) == 'e') ||
              (str[0] == '%' && str[1] == '2' &&
               ASCIILowercase(str[2]) == 'e' && str[3] == '.'));
    case 6:
      return (str[0] == '%' && str[1] == '2' && ASCIILowercase(str[2]) == 'e' &&
              str[3] == '%' && str[4] == '2' && ASCIILowercase(str[5]) == 'e');
    default:
      return false;
  }
}

inline bool DefaultToUnicode(const std::string& input, std::string* output) {
  *output = input;
  return true;
}

inline bool DefaultToASCII(const std::string& input, std::string* output) {
  *output = input;
  return true;
}

extern UnicodeASCIITransformer ToUnicode;
extern UnicodeASCIITransformer ToASCII;

}  // namespace whatwgurl

#endif  // SRC_WHATWGURL_INL_H_
