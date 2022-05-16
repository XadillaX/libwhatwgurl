#include "host/host.h"
#include "code_points.h"
#include "idna.h"
#include "ip_util-inl.h"
#include "percent_encode.h"
#include "string_utils.h"

namespace whatwgurl {

using percent_encode::Decode;
using percent_encode::hex;
using percent_encode::kC0ControlPercentEncodeSet;
using std::string;

enum ParseIPv4Result : int8_t {
  kParseIPv4OK = 0,
  kParseIPv4NotIPv4 = -1,
  kParseIPv4Invalid = -2,
};

static inline bool ParseIPv6(const char* input,
                             size_t input_size,
                             Host* host,
                             bool* validation_error) {
  uint16_t ip[8];
  IPv6FormatResult ipv6_format_ret = FormatIPv6(input, input_size, ip);
  switch (ipv6_format_ret) {
    case IPv6FormatResult::kIPv6FormatOK:
      host->SetIPv6Address(ip);
      return true;
    case IPv6FormatResult::kIPv6FormatFail:
      *validation_error = true;
      return false;
    default:
      UNREACHABLE();
  }
}

static inline ParseIPv4Result ParseIPv4(const char* input,
                                        size_t input_size,
                                        Host* host,
                                        bool* validation_error) {
  uint32_t ip;
  IPv4FormatResult ipv4_format_ret = FormatIPv4(input, input_size, &ip);
  switch (ipv4_format_ret) {
    case kIPv4FormatFail:
      *validation_error = true;
      return kParseIPv4Invalid;

    case kIPv4FormatOK:
      // TODO(XadillaX): validation_error like 0x?? segments.
      host->SetIPv4Address(ip);
      return kParseIPv4OK;

    case kIPv4FormatNotAnIPv4Address:
      return kParseIPv4NotIPv4;

    default:
      UNREACHABLE();
  }
}

static inline bool ParseOpaqueHost(const TempStringBuffer& input,
                                   Host* host,
                                   bool* validation_error) {
  // The max length should be `input_size * 3`. But `output`'s length is
  // dynamic. So we use a smaller length to reduce the initialization cost.
  TempStringBuffer output(input.length() * 2);

  const unsigned char* ptr = input.unsigned_string();
  const unsigned char* end = ptr + input.length();
  for (; ptr < end; ++ptr) {
    // If input contains a forbidden host code point, validation error, return
    // failure.
    if (IsForbiddenHostCodePoint(*ptr)) {
      *validation_error = true;
      return false;
    }

    // If input contains a code point that is not a URL code point and not
    // U+0025 (%), validation error.
    // TODO(XadillaX): Validation error of not-a-URL-code-point and not U+0025.

    // If input contains a U+0025 (%) and the two code points following it are
    // not ASCII hex digits, validation error.
    unsigned char c = *ptr;
    if (c == '%') {
      if (ptr + 2 >= end || !IsASCIIHexDigit(*(ptr + 1)) ||
          !IsASCIIHexDigit(*(ptr + 2))) {
        *validation_error = true;
      }
    }

    if (!BitAt(kC0ControlPercentEncodeSet, c)) {
      output.Append(c);
    } else {
      output.Append(hex + (c * 4), PER_PERCENT_HEX_LENGTH);
    }
  }

  host->SetOpaqueHost(string(output.string(), output.length()));
  return true;
}

bool Host::Parse(const TempStringBuffer& input,
                 Host* host,
                 bool* validation_error,
                 bool is_not_special) {
  const unsigned char* data = input.unsigned_string();

  // If input starts with U+005B ([), then:
  if (*data == '[') {
    // If input does not end with U+005D (]), validation error, return failure.
    if (*(data + input.length() - 1) != ']') {
      *validation_error = true;
      return false;
    }

    // Return the result of IPv6 parsing input with its leading U+005B ([) and
    // trailing U+005D (]) removed.
    return ParseIPv6(reinterpret_cast<const char*>(data + 1),
                     input.length() - 2,
                     host,
                     validation_error);
  }

  // If isNotSpecial is true, then return the result of opaque-host parsing
  // input.
  if (is_not_special) {
    return ParseOpaqueHost(input, host, validation_error);
  }

  // Assert: input is not the empty string.
  CHECK_GT(input.length(), 0);

  // Let domain be the result of running UTF-8 decode without BOM on the
  // percent-decoding of input.
  TempStringBuffer domain(input.length());
  Decode(input.unsigned_string(), input.length(), &domain);

  // TODO(XadillaX): Alternatively UTF-8 decode without BOM or fail can be used,
  // coupled with an early return for failure, as domain to ASCII fails on
  // U+FFFD REPLACEMENT CHARACTER.

  // Let asciiDomain be the result of running domain to ASCII on domain.
  string ascii_domain;
  CHECK_NOT_NULL(kIDNAToASCII);
  int32_t ascii_domain_len = kIDNAToASCII(
      &ascii_domain, domain.string(), domain.length(), kIDNADefault);

  // If asciiDomain is failure, validation error, return failure:

  // If result is a failure value, validation error, return failure. If result
  // is the empty string, validation error, return failure.
  if (ascii_domain_len == -1 || ascii_domain_len == 0) {
    *validation_error = true;
    return false;
  }

  // If asciiDomain contains a forbidden domain code point, validation error,
  // return failure.
  if (string_utils::Contains(
          ascii_domain, CodePointMacroToLambda(IsForbiddenDomainCodePoint))) {
    *validation_error = true;
    return false;
  }

  // If asciiDomain ends in a number, then return the result of IPv4 parsing
  // asciiDomain.
  ParseIPv4Result parse_ipv4_result = ParseIPv4(
      ascii_domain.c_str(), ascii_domain.length(), host, validation_error);
  switch (parse_ipv4_result) {
    case kParseIPv4OK:
      return true;

    case kParseIPv4Invalid:
      return false;

    case kParseIPv4NotIPv4:
    default:
      break;
  }

  host->SetDomain(ascii_domain);
  return true;
}

void Host::SetOpaqueHost(const string& opaque_host) {
  this->_is_null = false;
  this->_value.SetOpaqueHost(opaque_host);
}

void Host::SetDomain(const string& domain) {
  this->_is_null = false;
  this->_value.SetDomain(domain);
}

void Host::SetEmptyHost() {
  this->_is_null = false;
  this->_value.SetEmptyHost();
}

void Host::SetIPv4Address(uint32_t ipv4_address) {
  this->_is_null = false;
  this->_value.SetIPv4Address(ipv4_address);
}

void Host::SetIPv6Address(const uint16_t* ipv6_address) {
  this->_is_null = false;
  this->_value.SetIPv6Address(ipv6_address);
}

void Host::SetNull() {
  this->SetValue(nullptr);
}

}  // namespace whatwgurl
