#ifndef INCLUDE_WHATWGURL_HOST_H_
#define INCLUDE_WHATWGURL_HOST_H_

#include <stddef.h>
#include <string>

namespace whatwgurl {

enum class HostType {
  kFailed,
  kDomain,
  kIPv4,
  kIPv6,
  kOpaque,
};

// https://url.spec.whatwg.org/#concept-host
class URLHost {
 public:
  ~URLHost();

  void ParseIPv4Host(const char* input, size_t length, bool* is_ipv4);
  void ParseIPv6Host(const char* input, size_t length);
  void ParseOpaqueHost(const char* input, size_t length);
  void ParseHost(const char* input,
                 size_t length,
                 bool is_special,
                 bool unicode = false);

  bool ParsingFailed() const { return _type == HostType::kFailed; }
  std::string ToString() const;
  // Like ToString(), but avoids a copy in exchange for invalidating `*this`.
  std::string ToStringMove();

 private:
  union Value {
    std::string domain_or_opaque;
    uint32_t ipv4;
    uint16_t ipv6[8];

    ~Value() {}
    Value() : ipv4(0) {}
  };

  Value _value;
  HostType _type = HostType::kFailed;

  inline void Reset() {
    using string = std::string;
    switch (_type) {
      case HostType::kDomain:
      case HostType::kOpaque:
        _value.domain_or_opaque.~string();
        break;
      default:
        break;
    }
    _type = HostType::kFailed;
  }

  // Setting the string members of the union with = is brittle because
  // it relies on them being initialized to a state that requires no
  // destruction of old data.
  // For a long time, that worked well enough because ParseIPv6Host() happens
  // to zero-fill `_value`, but that really is relying on standard library
  // internals too much.
  // These helpers are the easiest solution but we might want to consider
  // just not forcing strings into an union.
  inline void SetOpaque(std::string&& string) {
    Reset();
    _type = HostType::kOpaque;
    new (&_value.domain_or_opaque) std::string(std::move(string));
  }

  inline void SetDomain(std::string&& string) {
    Reset();
    _type = HostType::kDomain;
    new (&_value.domain_or_opaque) std::string(std::move(string));
  }
};

}  // namespace whatwgurl

#endif  // INCLUDE_WHATWGURL_HOST_H_
