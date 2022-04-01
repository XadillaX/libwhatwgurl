#include <arpa/inet.h>
#include <math.h>
#include <netinet/in.h>
#include <whatwgurl_host.h>
#include "whatwgurl-inl.h"

namespace whatwgurl {

using table_data::C0_CONTROL_ENCODE_SET;

#define NS_IN6ADDRSZ (16)

// Locates the longest sequence of 0 segments in an IPv6 address
// in order to use the :: compression when serializing
template <typename T>
inline T* FindLongestZeroSequence(T* values, size_t len) {
  T* start = values;
  T* end = start + len;
  T* result = nullptr;

  T* current = nullptr;
  unsigned counter = 0, longest = 1;

  while (start < end) {
    if (*start == 0) {
      if (current == nullptr) current = start;
      counter++;
    } else {
      if (counter > longest) {
        longest = counter;
        result = current;
      }
      counter = 0;
      current = nullptr;
    }
    start++;
  }
  if (counter > longest) result = current;
  return result;
}

URLHost::~URLHost() {
  Reset();
}

void URLHost::ParseIPv6Host(const char* input, size_t length) {
  unsigned char buf[sizeof(struct in6_addr)];
  char* ipv6 = reinterpret_cast<char*>(malloc((length + 1) * sizeof(char)));
  *(ipv6 + length) = 0;
  memset(buf, 0, sizeof(buf));
  memcpy(ipv6, input, sizeof(const char) * length);

  int ret = inet_pton(AF_INET6, ipv6, buf);

  if (ret != 0) {
    free(ipv6);
    return;
  }

  // Ref:
  // https://sourceware.org/git/?p=glibc.git;a=blob;f=resolv/inet_ntop.c;h=c4d38c0f951013e51a4fc6eaa8a9b82e146abe5a;hb=HEAD#l119
  for (int i = 0; i < NS_IN6ADDRSZ; i += 2) {
    _value.ipv6[i >> 1] = (buf[i] << 8) | buf[i + 1];
  }

  _type = HostType::kIPv6;
  free(ipv6);
}

void URLHost::ParseIPv4Host(const char* input, size_t length, bool* is_ipv4) {
  *is_ipv4 = false;
  const char* pointer = input;
  const char* mark = input;
  const char* end = pointer + length;
  int parts = 0;
  uint32_t val = 0;
  uint64_t numbers[4];
  int tooBigNumbers = 0;
  if (length == 0) return;

  while (pointer <= end) {
    const char ch = pointer < end ? pointer[0] : kEOL;
    int64_t remaining = end - pointer - 1;
    if (ch == '.' || ch == kEOL) {
      if (++parts > static_cast<int>(arraysize(numbers))) return;
      if (pointer == mark) return;
      int64_t n = ParseNumber(mark, pointer);
      if (n < 0) return;

      if (n > 255) {
        tooBigNumbers++;
      }
      numbers[parts - 1] = n;
      mark = pointer + 1;
      if (ch == '.' && remaining == 0) break;
    }
    pointer++;
  }

  *is_ipv4 = true;

  // If any but the last item in numbers is greater than 255, return failure.
  // If the last item in numbers is greater than or equal to
  // 256^(5 - the number of items in numbers), return failure.
  if (tooBigNumbers > 1 || (tooBigNumbers == 1 && numbers[parts - 1] <= 255) ||
      numbers[parts - 1] >= pow(256, static_cast<double>(5 - parts))) {
    return;
  }

  _type = HostType::kIPv4;
  val = static_cast<uint32_t>(numbers[parts - 1]);
  for (int n = 0; n < parts - 1; n++) {
    double b = 3 - n;
    val +=
        static_cast<uint32_t>(numbers[n]) * static_cast<uint32_t>(pow(256, b));
  }

  _value.ipv4 = val;
}

void URLHost::ParseOpaqueHost(const char* input, size_t length) {
  std::string output;
  output.reserve(length);
  for (size_t i = 0; i < length; i++) {
    const char ch = input[i];
    if (ch != '%' && IsForbiddenHostCodePoint(ch)) {
      return;
    } else {
      AppendOrEscape(&output, ch, C0_CONTROL_ENCODE_SET);
    }
  }

  SetOpaque(std::move(output));
}

void URLHost::ParseHost(const char* input,
                        size_t length,
                        bool is_special,
                        bool unicode) {
  const char* pointer = input;

  if (length == 0) return;

  if (pointer[0] == '[') {
    if (pointer[length - 1] != ']') return;
    return ParseIPv6Host(++pointer, length - 2);
  }

  if (!is_special) return ParseOpaqueHost(input, length);

  // First, we have to percent decode
  std::string decoded = PercentDecode(input, length);

  // Then we have to punycode toASCII
  if (!ToASCII(decoded, &decoded)) return;

  // If any of the following characters are still present, we have to fail
  for (size_t n = 0; n < decoded.size(); n++) {
    const char ch = decoded[n];
    if (IsForbiddenHostCodePoint(ch)) {
      return;
    }
  }

  // Check to see if it's an IPv4 IP address
  bool is_ipv4;
  ParseIPv4Host(decoded.c_str(), decoded.length(), &is_ipv4);
  if (is_ipv4) return;

  // If the unicode flag is set, run the result through punycode ToUnicode
  if (unicode && !ToUnicode(decoded, &decoded)) return;

  // It's not an IPv4 or IPv6 address, it must be a domain
  SetDomain(std::move(decoded));
}

std::string URLHost::ToStringMove() {
  std::string return_value;
  switch (_type) {
    case HostType::kDomain:
    case HostType::kOpaque:
      return_value = std::move(_value.domain_or_opaque);
      break;
    default:
      return_value = ToString();
      break;
  }
  Reset();
  return return_value;
}

std::string URLHost::ToString() const {
  std::string dest;
  switch (_type) {
    case HostType::kDomain:
    case HostType::kOpaque:
      return _value.domain_or_opaque;
    case HostType::kIPv4: {
      dest.reserve(15);
      uint32_t value = _value.ipv4;
      for (int n = 0; n < 4; n++) {
        dest.insert(0, std::to_string(value % 256));
        if (n < 3) dest.insert(0, 1, '.');
        value /= 256;
      }
      break;
    }
    case HostType::kIPv6: {
      dest.reserve(41);
      dest += '[';
      const uint16_t* start = &_value.ipv6[0];
      const uint16_t* compress_pointer = FindLongestZeroSequence(start, 8);
      bool ignore0 = false;
      for (int n = 0; n <= 7; n++) {
        const uint16_t* piece = &_value.ipv6[n];
        if (ignore0 && *piece == 0)
          continue;
        else if (ignore0)
          ignore0 = false;
        if (compress_pointer == piece) {
          dest += n == 0 ? "::" : ":";
          ignore0 = true;
          continue;
        }
        char buf[5];
        snprintf(buf, sizeof(buf), "%x", *piece);
        dest += buf;
        if (n < 7) dest += ':';
      }
      dest += ']';
      break;
    }
    case HostType::kFailed:
      break;
  }
  return dest;
}

}  // namespace whatwgurl
