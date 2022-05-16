#ifndef INCLUDE_PORT_H_
#define INCLUDE_PORT_H_

#include <string>
#include "maybe.h"

namespace whatwgurl {

class Port : public MaybeNull<uint16_t> {
 public:
  inline Port& operator=(std::nullptr_t) {
    MaybeNull<uint16_t>::operator=(nullptr);
    return *this;
  }

  inline std::string Serialize() const {
    CHECK(!is_null());

    // To serialize an integer, represent it as the shortest possible decimal
    // number.
    char buff[6];
    int n = snprintf(buff, sizeof(buff), "%u", value());
    CHECK_GE(n, 0);

    return std::string(buff, n);
  }
};

}  // namespace whatwgurl

#endif  // INCLUDE_PORT_H_
