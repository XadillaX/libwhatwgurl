#include "scheme.h"

namespace whatwgurl {

char const kSpecialSchemas[SPECIAL_SCHEMA_COUNT + 1]
                          [SPECIAL_SCHEMA_MAX_LENGTH + 1] = {
                              "ftp", "file", "http", "https", "ws", "wss", ""};

std::map<std::string, DefaultPortResult> kSchemePorts;

void InitSchemePorts() {
  kSchemePorts["http"] = {false, 80};
  kSchemePorts["https"] = {false, 443};
  kSchemePorts["ws"] = {false, 80};
  kSchemePorts["wss"] = {false, 443};
  kSchemePorts["file"] = {true, 0};
  kSchemePorts["ftp"] = {false, 21};
}

}  // namespace whatwgurl
