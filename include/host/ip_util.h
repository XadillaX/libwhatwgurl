#ifndef INCLUDE_HOST_IP_UTIL_H_
#define INCLUDE_HOST_IP_UTIL_H_

#include <stdint.h>
#include <unistd.h>

namespace whatwgurl {

#define kMaxFormattedHostIPv4Len (15)

enum IPv6FormatResult : int8_t {
  kIPv6FormatOK = 0,
  kIPv6FormatFail = -1,
};

enum IPv4FormatResult : int8_t {
  kIPv4FormatOK = 0,
  kIPv4FormatFail = -1,
  kIPv4FormatNotAnIPv4Address = -2,
};

enum IPv4TransformSegmentResult : int8_t {
  kIPv4TransformSegmentOK = 0,
  kIPv4TransformSegmentNotANumber = -1,
  kIPv4TransformSegmentOutOfRange = -2,
};

IPv6FormatResult FormatIPv6(const char* in, int in_len, uint16_t* out);
IPv4FormatResult FormatIPv4(const char* in, int in_len, uint32_t* out);

inline IPv4TransformSegmentResult TransformIPv4SegmentFromHex(
    const char* segment, unsigned int max_range, unsigned int* out);
inline IPv4TransformSegmentResult TransformIPv4SegmentFromOct(
    const char* seg, unsigned int max_range, unsigned int* out);
inline IPv4TransformSegmentResult TransformIPv4SegmentFromDec(
    const char* seg, unsigned int max_range, unsigned int* out);
inline IPv4TransformSegmentResult TransformIPv4SegmentToNumber(
    const char* seg, unsigned int max_range, unsigned int* out);

template <typename T>
static inline T* FindLongestZeroSequence(T* values, size_t len) {
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

}  // namespace whatwgurl

#endif  // INCLUDE_HOST_IP_UTIL_H_
