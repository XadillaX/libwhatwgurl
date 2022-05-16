#include "utils/assert.h"
#include <string>

namespace whatwgurl {

void Assert(const AssertionInfo& info) {
  char location[2048];
  char message[4096];
  snprintf(location,
           sizeof(location),
           "%s[%d]: %s:%s%s",
           "libwhatwgurl",
           getpid(),
           info.file_line,
           info.function,
           *info.function ? ":" : "");
  snprintf(message, sizeof(message), "Assertion `%s` failed.", info.message);

  fprintf(stderr, "%s %s\n", location, message);
  exit(1);
}

}  // namespace whatwgurl
