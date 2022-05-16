#ifndef INCLUDE_PARSE_H_
#define INCLUDE_PARSE_H_

#include <memory>
#include <string>
#include "parsed_url.h"

namespace whatwgurl {

enum ParseState {
  kSchemeStartState = 0,
  kSchemeState = 1,
  kNoSchemeState = 2,
  kSpecialRelativeOrAuthorityState = 3,
  kPathOrAuthorityState = 4,
  kRelativeState = 5,
  kRelativeSlashState = 6,
  kSpecialAuthoritySlashesState = 7,
  kSpecialAuthorityIgnoreSlashesState = 8,
  kAuthorityState = 9,
  kHostState = 10,
  kHostNameState = 11,
  kPortState = 12,
  kFileState = 13,
  kFileSlashState = 14,
  kFileHostState = 15,
  kPathStartState = 16,
  kPathState = 17,
  kOpaquePathState = 18,
  kQueryState = 19,
  kFragmentState = 20,

  kNotGiven = -1,
  kParseErrorState = -2,
};

bool Parse(const std::string& input,
           const ParsedURL* base,
           std::shared_ptr<ParsedURL>* url,
           bool* validation_error,
           ParseState state_override = kNotGiven);

}  // namespace whatwgurl

#endif  // INCLUDE_PARSE_H_
