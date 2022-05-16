#include "host/ip_util.h"
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include "ip_util-inl.h"
#include "utils/resizable_buffer.h"

namespace whatwgurl {

using std::string;

#define NS_IN6ADDRSZ 16

IPv6FormatResult FormatIPv6(const char* in, int in_len, uint16_t* out) {
  // The last character of `in` may not be a '\0'.
  ResizableBuffer<char> maybe_ipv6(in_len + 1);
  memcpy(maybe_ipv6.buffer(), in, in_len * sizeof(char));
  maybe_ipv6.buffer()[in_len] = '\0';

  unsigned char buf[sizeof(struct in6_addr)];
  int ret = inet_pton(AF_INET6, maybe_ipv6.buffer(), buf);
  if (ret <= 0) {
    return kIPv6FormatFail;
  }

  // Ref:
  // https://sourceware.org/git/?p=glibc.git;a=blob;f=resolv/inet_ntop.c;h=c4d38c0f951013e51a4fc6eaa8a9b82e146abe5a;hb=HEAD#l119
  for (int i = 0; i < NS_IN6ADDRSZ; i += 2) {
    out[i >> 1] = (buf[i] << 8) | buf[i + 1];
  }

  return kIPv6FormatOK;
}

IPv4FormatResult FormatIPv4(const char* in, int in_len, uint32_t* out) {
  // `inet_aton` only transform IPv4 without validation. The input may be an
  // invalid IP. So here we transform IPv4 via logic that implements by myself.
  if (*(in + in_len - 1) == '.') {
    in_len--;
  }

  if (!in_len || *in == '.' || *(in + in_len - 1) == '.') {
    return kIPv4FormatNotAnIPv4Address;
  }

  ResizableBuffer<char> in_copy(in_len + 1);
  char* in_signed_buffer = static_cast<char*>(in_copy.buffer());
  memcpy(in_signed_buffer, in, sizeof(char) * (in_len));
  in_signed_buffer[in_len] = 0;

  char* p = in_signed_buffer + 1;
  char* in_segs[4] = {in_signed_buffer, nullptr, nullptr, nullptr};
  unsigned int ip_segments[4] = {0, 0, 0, 0};
  int segs_count = 1;
  char* last_seg = nullptr;

  while (*p != 0) {
    if (*(p - 1) == '.') {
      *(p - 1) = 0;
      if (segs_count < 4) {
        in_segs[segs_count++] = p;
      } else {
        segs_count++;
      }

      last_seg = p;
    }

    p++;
  }

  if (segs_count > 4) {
    if (last_seg && *last_seg >= '0' && *last_seg <= '9') {
      return kIPv4FormatFail;
    }

    return kIPv4FormatNotAnIPv4Address;
  }

  IPv4FormatResult maybe_result = kIPv4FormatOK;
  for (int i = 0; i < segs_count; i++) {
    p = in_segs[i];
    unsigned int max_range = 255;
    IPv4TransformSegmentResult ret;
    if (i == segs_count - 1) {
      switch (segs_count) {
        case 1:
          max_range = 0xffffffff;
          break;
        case 2:
          max_range = 0xffffff;
          break;
        case 3:
          max_range = 0xffff;
          break;
        default:
          break;
      }
    }

    if (*p == 0) {
      ret = kIPv4TransformSegmentOutOfRange;
    } else {
      ret =
          TransformIPv4SegmentToNumber(in_segs[i], max_range, ip_segments + i);
    }
    switch (ret) {
      case kIPv4TransformSegmentNotANumber:
        maybe_result = kIPv4FormatNotAnIPv4Address;
        break;

      case kIPv4TransformSegmentOutOfRange:
        if (maybe_result != kIPv4FormatNotAnIPv4Address) {
          maybe_result = kIPv4FormatFail;
        }

        if (i == segs_count - 1) {
          return kIPv4FormatFail;
        }
        break;

      default:
        if (i == segs_count - 1 && maybe_result != kIPv4FormatOK) {
          return kIPv4FormatFail;
        }
        break;
    }
  }

  if (maybe_result != kIPv4FormatOK) {
    return maybe_result;
  }

  unsigned char bytes[4];
  switch (segs_count) {
    case 1:
      bytes[3] = ip_segments[0] & 0xff;
      bytes[2] = (ip_segments[0] >> 8) & 0xff;
      bytes[1] = (ip_segments[0] >> 16) & 0xff;
      bytes[0] = (ip_segments[0] >> 24) & 0xff;
      break;

    case 2:
      bytes[0] = ip_segments[0];
      bytes[3] = ip_segments[1] & 0xff;
      bytes[2] = (ip_segments[1] >> 8) & 0xff;
      bytes[1] = (ip_segments[1] >> 16) & 0xff;
      break;

    case 3:
      bytes[0] = ip_segments[0];
      bytes[1] = ip_segments[1];
      bytes[3] = ip_segments[2] & 0xff;
      bytes[2] = (ip_segments[2] >> 8) & 0xff;
      break;

    case 4:
      bytes[0] = ip_segments[0];
      bytes[1] = ip_segments[1];
      bytes[2] = ip_segments[2];
      bytes[3] = ip_segments[3];
      break;

    default:
      UNREACHABLE();
  }

  *out = (bytes[0] << 24) | (bytes[1] << 16) | (bytes[2] << 8) | bytes[3];
  return kIPv4FormatOK;
}

}  // namespace whatwgurl
