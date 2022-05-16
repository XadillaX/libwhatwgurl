#ifndef INCLUDE_PARSED_URL_H_
#define INCLUDE_PARSED_URL_H_

#include <string>
#include "host/host.h"
#include "maybe.h"
#include "path.h"
#include "port.h"
#include "scheme.h"

namespace whatwgurl {

struct ParsedURL {
  std::string scheme;
  std::string username;
  std::string password;
  Host host;
  Port port;
  Path path;
  MaybeNull<std::string> query;
  MaybeNull<std::string> fragment;

  inline ParsedURL()
      : scheme(""),
        username(""),
        password(""),
        host(),
        port(),
        path(),
        query(),
        fragment() {}

  inline void Clear() {
    scheme = "";
    username = "";
    password = "";
    host = nullptr;
    port = nullptr;
    path.Reset();
    query = nullptr;
    fragment = nullptr;
  }

  inline bool IsSpecial() const {
    // A URL is special if its scheme is a special scheme. A URL is not special
    // if its scheme is not a special scheme.
    return IsSpecialScheme(scheme);
  }

  inline bool IncludeCredentials() const {
    // A URL includes credentials if its username or password is not the empty
    // string.
    return !username.empty() || !password.empty();
  }

  inline bool HasOpaquePath() const {
    // A URL has an opaque path if its path is a string.
    return path.IsOpaquePath();
  }

  inline bool CannotHaveUsernamePasswordPort() const {
    // A URL cannot have a username/password/port if its host is null or the
    // empty string, or its scheme is "file".
    return host.is_null() ||
           (host->type == kOpaqueHost && host->host.opaque_host.empty()) ||
           host->type == kEmptyHost || scheme == "file";
  }

  inline void Dump() {
    printf("scheme: %s\n", scheme.c_str());
    printf("username: %s\n", username.c_str());
    printf("password: %s\n", password.c_str());

    if (host.is_null()) {
      printf("host: <null>\n");
    } else {
      switch (host->type) {
        case HostType::kEmptyHost:
          printf("host: <empty>\n");
          break;

        case HostType::kDomain:
          printf("host: <domain>: %s\n", host->domain().c_str());
          break;

        case HostType::kOpaqueHost:
          printf("host: <opaque>: %s\n", host->opaque_host().c_str());
          break;

        case HostType::kNotInitialized:
          printf("host: <not initialized>\n");
          break;

        case HostType::kIPv4Address:
          printf("host: <ipv4>: %s\n", host->ipv4_address_string().c_str());
          break;

        case HostType::kIPv6Address:
          printf("host: <ipv6>: %s\n", host->ipv6_address_string().c_str());
          break;

        default:
          printf("host: <unknown>\n");
          break;
      }
    }

    if (port.is_null()) {
      printf("port: <null>\n");
    } else {
      printf("port: %d\n", *port);
    }

    if (!path.IsOpaquePath()) {
      printf("path: %zu\n", path.size());
      for (size_t i = 0; i < path.size(); i++) {
        printf("  - (%s)\n", path[i].c_str());
      }
    } else {
      printf("path: <opaque_path> (%s)\n", path.ASCIIString().c_str());
    }

    if (query.is_null()) {
      printf("query: <null>\n");
    } else {
      printf("query: %s\n", query.value().c_str());
    }

    if (fragment.is_null()) {
      printf("fragment: <null>\n");
    } else {
      printf("fragment: %s\n", fragment.value().c_str());
    }
  }
};

}  // namespace whatwgurl

#endif  // INCLUDE_PARSED_URL_H_
