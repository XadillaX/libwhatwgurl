#ifndef INCLUDE_ORIGIN_H_
#define INCLUDE_ORIGIN_H_

#include <string>
#include "host/host_item.h"
#include "maybe.h"
#include "port.h"

namespace whatwgurl {

struct TupleOriginItem {
  std::string scheme;
  HostItem host;
  Port port;
  MaybeNull<std::string> domain;
};

class TupleOrigin : public MaybeNull<TupleOriginItem> {
 public:
  inline TupleOrigin& operator=(std::nullptr_t) {
    MaybeNull<TupleOriginItem>::operator=(nullptr);
    return *this;
  }

  // The serialization of an origin is the string obtained by applying the
  // following algorithm to the given origin origin:
  inline std::string Serialize() const {
    // If origin is an opaque origin, then return "null".
    if (is_null()) {
      return "null";
    }

    // Otherwise, let result be origin's scheme.
    std::string result = (*this)->scheme;

    // Append "://" to result.
    result += "://";

    // Append origin's host, serialized, to result.
    result += (*this)->host.Serialize();

    // If origin's port is non-null, append a U+003A COLON character (:), and
    // origin's port, serialized, to result.
    if (!(*this)->port.is_null()) {
      result += ":";
      result += (*this)->port.Serialize();
    }

    // Return result.
    return result;
  }

  void Dump() {
    if (is_null()) {
      printf("<null>\n");
      return;
    }

    printf("scheme: %s\n", (*this)->scheme.c_str());
    printf("host: %s\n", (*this)->host.Serialize().c_str());
    if (!(*this)->port.is_null()) {
      printf("port: %s\n", (*this)->port.Serialize().c_str());
    } else {
      printf("port: <null>\n");
    }
    printf(
        "domain: %s\n",
        (*this)->domain.is_null() ? "<null>" : (*this)->domain.value().c_str());
  }
};

}  // namespace whatwgurl

#endif  // INCLUDE_ORIGIN_H_
