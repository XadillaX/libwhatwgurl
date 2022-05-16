#ifndef INCLUDE_HOST_HOST_ITEM_H_
#define INCLUDE_HOST_HOST_ITEM_H_

#include <inttypes.h>
#include <string.h>
#include <string>
#include "ip_util.h"
#include "utils/assert.h"

namespace whatwgurl {

enum HostType {
  kDomain = 0,
  kIPv4Address,
  kIPv6Address,
  kOpaqueHost,
  kEmptyHost,

  kNotInitialized = -1,
};

union HostUnion {
  std::string domain;
  std::string opaque_host;
  std::string empty_host;
  uint32_t ipv4_address;
  uint16_t ipv6_address[8];

  HostUnion() : ipv4_address(0) {}
  ~HostUnion() {}
};

struct HostItem {
  HostType type;
  HostUnion host;

  inline HostItem() : type(kNotInitialized), host() {}
  inline HostItem(const HostItem& other) : type(kNotInitialized), host() {
    Set(other);
  }
  inline HostItem& operator=(const HostItem& other) {
    Set(other);
    return *this;
  }
  inline ~HostItem() { Destruct(); }

  inline void SetOpaqueHost(const std::string& opaque_host) {
    Destruct();
    type = kOpaqueHost;
    new (&host.opaque_host) std::string(opaque_host);
  }

  inline void SetDomain(const std::string& domain) {
    Destruct();
    type = kDomain;
    new (&host.domain) std::string(domain);
  }

  inline void SetIPv4Address(uint32_t ipv4_address) {
    Destruct();
    type = kIPv4Address;
    host.ipv4_address = ipv4_address;
  }

  inline void SetIPv6Address(const uint16_t* ipv6_address) {
    Destruct();
    type = kIPv6Address;
    memcpy(host.ipv6_address, ipv6_address, sizeof(host.ipv6_address));
  }

  inline void SetEmptyHost() {
    Destruct();
    type = kEmptyHost;
    new (&host.empty_host) std::string();
  }

  inline std::string domain() const {
    CHECK(type == kDomain);
    return host.domain;
  }

  inline std::string opaque_host() const {
    CHECK(type == kOpaqueHost);
    return host.opaque_host;
  }

  inline std::string empty_host() const {
    CHECK(type == kEmptyHost);
    return host.empty_host;
  }

  inline uint32_t ipv4_address() const {
    CHECK(type == kIPv4Address);
    return host.ipv4_address;
  }

  inline std::string ipv4_address_string() const {
    CHECK(type == kIPv4Address);
    char ip[kMaxFormattedHostIPv4Len + 1];
    unsigned char bytes[4];
    bytes[0] = (host.ipv4_address >> 24) & 0xff;
    bytes[1] = (host.ipv4_address >> 16) & 0xff;
    bytes[2] = (host.ipv4_address >> 8) & 0xff;
    bytes[3] = host.ipv4_address & 0xff;
    snprintf(
        ip, sizeof(ip), "%d.%d.%d.%d", bytes[0], bytes[1], bytes[2], bytes[3]);
    return ip;
  }

  inline const uint16_t* ipv6_address() const {
    CHECK(type == kIPv6Address);
    return host.ipv6_address;
  }

  inline std::string ipv6_address_string() const {
    CHECK(type == kIPv6Address);

    std::string ipv6_str = "";
    const uint16_t* start = host.ipv6_address;
    const uint16_t* compress_pointer = FindLongestZeroSequence(start, 8);
    bool ignore0 = false;
    for (int n = 0; n <= 7; n++) {
      const uint16_t* piece = host.ipv6_address + n;
      if (ignore0 && *piece == 0)
        continue;
      else if (ignore0)
        ignore0 = false;
      if (compress_pointer == piece) {
        ipv6_str += n == 0 ? "::" : ":";
        ignore0 = true;
        continue;
      }
      char buf[5];
      snprintf(buf, sizeof(buf), "%x", *piece);
      ipv6_str += buf;
      if (n < 7) ipv6_str += ':';
    }

    return ipv6_str;
  }

  inline std::string Serialize() const {
    switch (type) {
      // If host is an IPv4 address, return the result of running the IPv4
      // serializer on host.
      case kIPv4Address:
        return ipv4_address_string();

      // Otherwise, if host is an IPv6 address, return U+005B ([), followed by
      // the result of running the IPv6 serializer on host, followed by U+005D
      // (]).
      case kIPv6Address:
        return '[' + ipv6_address_string() + ']';

      // Otherwise, host is a domain, opaque host, or empty host, return host.
      case kDomain:
        return domain();

      case kOpaqueHost:
        return opaque_host();

      case kEmptyHost:
        return "";

      default:
        UNREACHABLE();
    }
  }

 private:
  inline void Set(const HostItem& other) {
    Destruct();
    type = other.type;
    switch (type) {
      case kDomain:
        new (&host.domain) std::string(other.host.domain);
        break;
      case kIPv4Address:
        host.ipv4_address = other.host.ipv4_address;
        break;
      case kIPv6Address:
        memcpy(host.ipv6_address,
               other.host.ipv6_address,
               sizeof(host.ipv6_address));
        break;
      case kOpaqueHost:
        new (&host.opaque_host) std::string(other.host.opaque_host);
        break;
      case kEmptyHost:
        new (&host.empty_host) std::string();
        break;
      case kNotInitialized:
        HostItem();
        break;
      default:
        UNREACHABLE();
    }
  }

  inline void Destruct() {
    using string = std::string;
    switch (type) {
      case kDomain:
        host.domain.~string();
        break;
      case kEmptyHost:
        host.empty_host.~string();
        break;
      case kOpaqueHost:
        host.opaque_host.~string();
        break;
      default:
        break;
    }
    type = kNotInitialized;
  }
};

}  // namespace whatwgurl

#endif  // INCLUDE_HOST_HOST_ITEM_H_
