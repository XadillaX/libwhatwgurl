#ifndef SRC_IDNA_INL_H_
#define SRC_IDNA_INL_H_

#include "idna.h"

namespace whatwgurl {

inline void SetIDNAToASCIIFunction(IDNAToASCIIFunction function) {
  kIDNAToASCII = function;
}

}  // namespace whatwgurl

#endif  // SRC_IDNA_INL_H_
