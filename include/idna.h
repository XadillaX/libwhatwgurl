#ifndef INCLUDE_IDNA_H_
#define INCLUDE_IDNA_H_

#include <stddef.h>
#include <string>

namespace whatwgurl {

enum IDNAMode {
  // Default mode for maximum compatibility.
  kIDNADefault,
  // Ignore all errors in IDNA conversion, if possible.
  kIDNALenient,
  // Enforce STD3 rules (UseSTD3ASCIIRules) and DNS length restrictions
  // (VerifyDnsLength). Corresponds to `beStrict` flag in the "domain to ASCII"
  // algorithm.
  kIDNAStrict
};

typedef int32_t (*IDNAToASCIIFunction)(std::string* buf,
                                       const char* input,
                                       size_t length,
                                       IDNAMode mode);

extern IDNAToASCIIFunction kIDNAToASCII;

}  // namespace whatwgurl

#endif  // INCLUDE_IDNA_H_
