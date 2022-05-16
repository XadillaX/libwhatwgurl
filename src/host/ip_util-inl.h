#ifndef SRC_HOST_IP_UTIL_INL_H_
#define SRC_HOST_IP_UTIL_INL_H_

#include <stddef.h>
#include <stdio.h>
#include "host/ip_util.h"

namespace whatwgurl {

typedef IPv4TransformSegmentResult (*TransformIPv4SegmentAdapter)(
    const char* segment, unsigned int max_range, unsigned int* out);

inline IPv4TransformSegmentResult TransformIPv4SegmentFromHex(
    const char* segment, unsigned int max_range, unsigned int* out) {
  uint64_t ret = 0;
  const char* p = segment;

  while (*p) {
    ret *= 16;
    if (*p >= '0' && *p <= '9') {
      ret += (*p - '0');
    } else if (*p >= 'A' && *p <= 'F') {
      ret += (*p - 'A' + 10);
    } else if (*p >= 'a' && *p <= 'f') {
      ret += (*p - 'a' + 10);
    } else {
      return kIPv4TransformSegmentNotANumber;
    }

    if (ret > max_range) return kIPv4TransformSegmentOutOfRange;
    p++;
  }

  *out = ret;
  return kIPv4TransformSegmentOK;
}

inline IPv4TransformSegmentResult TransformIPv4SegmentFromOct(
    const char* seg, unsigned int max_range, unsigned int* out) {
  uint64_t ret = 0;
  const char* p = seg;

  while (*p) {
    ret *= 8;
    if (*p >= '0' && *p <= '7') {
      ret += (*p - '0');
    } else if (*p == '8' || *p == '9') {
      return kIPv4TransformSegmentOutOfRange;
    } else {
      return kIPv4TransformSegmentNotANumber;
    }

    if (ret > max_range) return kIPv4TransformSegmentOutOfRange;
    p++;
  }

  *out = ret;
  return kIPv4TransformSegmentOK;
}

inline IPv4TransformSegmentResult TransformIPv4SegmentFromDec(
    const char* seg, unsigned int max_range, unsigned int* out) {
  uint64_t ret = 0;
  const char* p = seg;

  while (*p) {
    ret *= 10;
    if (*p >= '0' && *p <= '9') {
      ret += (*p - '0');
    } else {
      return kIPv4TransformSegmentNotANumber;
    }

    if (ret > max_range) return kIPv4TransformSegmentOutOfRange;
    p++;
  }

  *out = ret;
  return kIPv4TransformSegmentOK;
}

inline IPv4TransformSegmentResult TransformIPv4SegmentToNumber(
    const char* seg, unsigned int max_range, unsigned int* out) {
  IPv4TransformSegmentResult ret;
  int offset = 0;
  const char* p = seg;
  TransformIPv4SegmentAdapter adapter = nullptr;

  if (*p != '0') {
    adapter = TransformIPv4SegmentFromDec;
  } else {
    switch (*(p + 1)) {
      case 'x':
      case 'X':
        offset = 2;
        adapter = TransformIPv4SegmentFromHex;
        break;

      default:
        offset = 1;
        adapter = TransformIPv4SegmentFromOct;
        break;
    }
  }

  ret = adapter(seg + offset, max_range, out);
  if (ret < kIPv4TransformSegmentOK) {
    *out = 0;
  }

  return ret;
}

}  // namespace whatwgurl

#endif  // SRC_HOST_IP_UTIL_INL_H_
