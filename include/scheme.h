#ifndef INCLUDE_SCHEME_H_
#define INCLUDE_SCHEME_H_

#include <string.h>
#include <map>
#include <string>

namespace whatwgurl {

#define SPECIAL_SCHEMA_COUNT (6)
#define SPECIAL_SCHEMA_MAX_LENGTH (5)

struct DefaultPortResult {
  bool is_null;
  uint16_t port;
};

extern std::map<std::string, DefaultPortResult> kSchemePorts;
extern char const kSpecialSchemas[SPECIAL_SCHEMA_COUNT + 1]
                                 [SPECIAL_SCHEMA_MAX_LENGTH + 1];

void InitSchemePorts();

inline bool IsSpecialScheme(std::string scheme) {
  for (int i = 0; i < SPECIAL_SCHEMA_COUNT; i++) {
    if (strcmp(scheme.c_str(), kSpecialSchemas[i]) == 0) {
      return true;
    }
  }
  return false;
}

// The default port for a special scheme is listed in the second column on the
// same row. The default port for any other ASCII string is null.
//
// | Scheme | Default Port |
// |--------|--------------|
// | http   | 80           |
// | https  | 443          |
// | ws     | 80           |
// | wss    | 443          |
// | ftp    | 21           |
// | file   | null         |
inline DefaultPortResult GetDefaultPort(std::string scheme) {
  auto it = kSchemePorts.find(scheme);
  if (it == kSchemePorts.end()) {
    return {true, 0};
  }

  return it->second;
}

}  // namespace whatwgurl

#endif  // INCLUDE_SCHEME_H_
