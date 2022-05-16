#ifndef INCLUDE_HOST_HOST_H_
#define INCLUDE_HOST_HOST_H_

#include "host_item.h"
#include "maybe.h"
#include "temp_string_buffer.h"

namespace whatwgurl {

class Host : public MaybeNull<HostItem> {
 public:
  static bool Parse(const TempStringBuffer& input,
                    Host* host,
                    bool* validation_error,
                    bool is_not_special = false);

  inline virtual Host& operator=(const Host& other) {
    if (this == &other) {
      return *this;
    }

    MaybeNull<HostItem>::operator=(other);
    return *this;
  }

  inline virtual Host& operator=(std::nullptr_t) {
    MaybeNull<HostItem>::operator=(nullptr);
    return *this;
  }

 public:
  void SetOpaqueHost(const std::string& opaque_host);
  void SetDomain(const std::string& domain);
  void SetEmptyHost();
  void SetIPv4Address(uint32_t ipv4_address);
  void SetIPv6Address(const uint16_t* ipv6_address);
  void SetNull();
};

}  // namespace whatwgurl

#endif  // INCLUDE_HOST_HOST_H_
