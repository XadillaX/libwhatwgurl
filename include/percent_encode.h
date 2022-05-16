#ifndef INCLUDE_PERCENT_ENCODE_H_
#define INCLUDE_PERCENT_ENCODE_H_

#include <inttypes.h>
#include "temp_string_buffer.h"

namespace whatwgurl {

#define PER_PERCENT_HEX_LENGTH (3)
#define CodePointMacroToLambda(name) [](unsigned char c) { return name(c); }
#define CodePointMacroToInlineFunction(name)                                   \
  inline bool name##Function(unsigned char c) { return name(c); }

#define PERCENT_ENCODES(V)                                                     \
  V(C0ControlPercentEncodeSet)                                                 \
  V(FragmentPercentEncodeSet)                                                  \
  V(QueryPercentEncodeSet)                                                     \
  V(SpecialQueryPercentEncodeSet)                                              \
  V(PathPercentEncodeSet)                                                      \
  V(UserInfoPercentEncodeSet)                                                  \
  V(ComponentPercentEncodeSet)                                                 \
  V(ApplicationXFormUrlEncodedPercentEncodeSet)

namespace percent_encode {

extern const char hex[256 * 4];
extern const unsigned char hexval[256];

#define DECLARE_CONST_UINT8_T_ARRAY(name) extern const uint8_t k##name[32];

PERCENT_ENCODES(DECLARE_CONST_UINT8_T_ARRAY)

#undef DECLARE_CONST_UINT8_T_ARRAY

// TODO(XadillaX): The `encoding` parameter is not supported yet.
void Encode(const unsigned char* input,
            size_t input_size,
            const uint8_t* percent_encode_set,
            TempStringBuffer* output,
            bool space_as_plus = false);
void Decode(const unsigned char* input,
            size_t input_size,
            TempStringBuffer* output);

}  // namespace percent_encode

#define BitAt(set, c) (!!(set[c >> 3] & (1 << (c & 7))))
#define InSomePercentEncodeSet(c, set_name)                                    \
  (BitAt(whatwgurl::percent_encode::set_name, c))

#define InC0ControlPercentEncodeSet(c)                                         \
  InSomePercentEncodeSet(c, kC0ControlPercentEncodeSet)
#define InFragmentPercentEncodeSet(c)                                          \
  InSomePercentEncodeSet(c, kFragmentPercentEncodeSet)
#define InQueryPercentEncodeSet(c)                                             \
  InSomePercentEncodeSet(c, kQueryPercentEncodeSet)
#define InSpecialQueryPercentEncodeSet(c)                                      \
  InSomePercentEncodeSet(c, kSpecialQueryPercentEncodeSet)
#define InPathPercentEncodeSet(c)                                              \
  InSomePercentEncodeSet(c, kPathPercentEncodeSet)
#define InUserInfoPercentEncodeSet(c)                                          \
  InSomePercentEncodeSet(c, kUserInfoPercentEncodeSet)
#define InComponentPercentEncodeSet(c)                                         \
  InSomePercentEncodeSet(c, kComponentPercentEncodeSet)
#define InApplicationXFormUrlEncodedPercentEncodeSet(c)                        \
  InSomePercentEncodeSet(c, kApplicationXFormUrlEncodedPercentEncodeSet)

#define DECLARE_IN_PERCENT_ENCODE_SET_FUNCTION(name)                           \
  CodePointMacroToInlineFunction(In##name);                                    \
  namespace percent_encode {                                                   \
  inline void EncodeBy##name(const unsigned char* input,                       \
                             size_t input_size,                                \
                             TempStringBuffer* output,                         \
                             bool space_as_plus = false) {                     \
    whatwgurl::percent_encode::Encode(input,                                   \
                                      input_size,                              \
                                      whatwgurl::percent_encode::k##name,      \
                                      output,                                  \
                                      space_as_plus);                          \
  }                                                                            \
  }  // namespace percent_encode

PERCENT_ENCODES(DECLARE_IN_PERCENT_ENCODE_SET_FUNCTION)

#undef DECLARE_IN_PERCENT_ENCODE_SET_FUNCTION

}  // namespace whatwgurl

#endif  // INCLUDE_PERCENT_ENCODE_H_
