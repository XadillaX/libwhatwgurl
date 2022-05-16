#ifndef INCLUDE_CODE_POINTS_H_
#define INCLUDE_CODE_POINTS_H_

#include <inttypes.h>
#include "percent_encode.h"

namespace whatwgurl {

// An ASCII tab or newline is U+0009 TAB, U+000A LF, or U+000D CR.
#define IsASCIITabOrNewline(c) (c == 0x09 || c == 0x0A || c == 0x0D)

// ASCII whitespace is U+0009 TAB, U+000A LF, U+000C FF, U+000D CR, or U+0020
// SPACE.
#define IsASCIIWhitespace(c)                                                   \
  (c == 0x09 || c == 0x0A || c == 0x0C || c == 0x0D || c == 0x20)

// A C0 control is a code point in the range U+0000 NULL to U+001F INFORMATION
// SEPARATOR ONE, inclusive.
#define IsC0Control(c) (c >= 0 && c <= 0x1F)

// A C0 control or space is a C0 control or U+0020 SPACE.
#define IsC0ControlOrSpace(c) (c == 0x20 || IsC0Control(c))

// A control is a C0 control or a code point in the range U+007F DELETE to
// U+009F APPLICATION PROGRAM COMMAND, inclusive.
#define IsControl(c) (IsC0Control(c) || c == 0x7F || c == 0x9f)

// An ASCII digit is a code point in the range U+0030 (0) to U+0039 (9),
// inclusive.
#define IsASCIIDigit(c) (c >= '0' && c <= '9')

// An ASCII upper hex digit is an ASCII digit or a code point in the range
// U+0041 (A) to U+0046 (F), inclusive.
#define IsASCIIUpperHexDigit(c) (c >= 'A' && c <= 'F')

// An ASCII lower hex digit is an ASCII digit or a code point in the range
// U+0061 (a) to U+0066 (f), inclusive.
#define IsASCIILowerHexDigit(c) (c >= 'a' && c <= 'f')

// An ASCII hex digit is an ASCII upper hex digit or ASCII lower hex digit.
#define IsASCIIHexDigit(c) (IsASCIIUpperHexDigit(c) || IsASCIILowerHexDigit(c))

// An ASCII upper alpha is a code point in the range U+0041 (A) to U+005A (Z),
// inclusive.
#define IsASCIIUpperAlpha(c) (c >= 'A' && c <= 'Z')

// An ASCII lower alpha is a code point in the range U+0061 (a) to U+007A (z),
// inclusive.
#define IsASCIILowerAlpha(c) (c >= 'a' && c <= 'z')

// An ASCII alpha is an ASCII upper alpha or ASCII lower alpha.
#define IsASCIIAlpha(c) (IsASCIIUpperAlpha(c) || IsASCIILowerAlpha(c))

// An ASCII alphanumeric is an ASCII alpha or an ASCII digit.
#define IsASCIIAlphanumeric(c) (IsASCIIAlpha(c) || IsASCIIDigit(c))

// A forbidden host code point is U+0000 NULL, U+0009 TAB, U+000A LF, U+000D CR,
// U+0020 SPACE, U+0023 (#), U+002F (/), U+003A (:), U+003C (<), U+003E (>),
// U+003F (?), U+0040 (@), U+005B ([), U+005C (\), U+005D (]), U+005E (^), or
// U+007C (|).
//
// Partition is for CPPLINT indent.
#define _IsForbiddenHostCodePointPart1(c)                                      \
  c == 0x00 || c == 0x09 || c == 0x0A || c == 0x0D || c == 0x20 ||
#define _IsForbiddenHostCodePointPart2(c)                                      \
  c == 0x23 || c == 0x2F || c == 0x3A || c == 0x3C || c == 0x3E ||
#define _IsForbiddenHostCodePointPart3(c)                                      \
  c == 0x3F || c == 0x40 || c == 0x5B || c == 0x5C || c == 0x5D ||
#define _IsForbiddenHostCodePointPart4(c) c == 0x5E || c == 0x7C
#define IsForbiddenHostCodePoint(c)                                            \
  (_IsForbiddenHostCodePointPart1(c) _IsForbiddenHostCodePointPart2(c)         \
       _IsForbiddenHostCodePointPart3(c) _IsForbiddenHostCodePointPart4(c))

// A forbidden domain code point is a forbidden host code point, a C0 control,
// U+0025 (%), or U+007F DELETE.
#define IsForbiddenDomainCodePoint(c)                                          \
  (IsForbiddenHostCodePoint(c) || IsC0Control(c) || c == 0x25 || c == 0x7F)

CodePointMacroToInlineFunction(IsASCIITabOrNewline);
CodePointMacroToInlineFunction(IsASCIIWhitespace);
CodePointMacroToInlineFunction(IsC0Control);
CodePointMacroToInlineFunction(IsC0ControlOrSpace);
CodePointMacroToInlineFunction(IsControl);
CodePointMacroToInlineFunction(IsASCIIDigit);
CodePointMacroToInlineFunction(IsASCIIUpperHexDigit);
CodePointMacroToInlineFunction(IsASCIILowerHexDigit);
CodePointMacroToInlineFunction(IsASCIIHexDigit);
CodePointMacroToInlineFunction(IsASCIIUpperAlpha);
CodePointMacroToInlineFunction(IsASCIILowerAlpha);
CodePointMacroToInlineFunction(IsASCIIAlpha);
CodePointMacroToInlineFunction(IsASCIIAlphanumeric);
CodePointMacroToInlineFunction(IsForbiddenHostCodePoint);
CodePointMacroToInlineFunction(IsForbiddenDomainCodePoint);

// Is this code unit a lead surrogate (U+d800..U+dbff)?
// Based on U16_IS_LEAD in utf16.h from ICU
// Refs: https://github.com/rmisev/url_whatwg/blob/9388886/src/url_utf.h#L147
template <typename T>
inline bool U16IsLead(T c) {
  return (c & 0xfffffc00) == 0xd800;
}

// Refs: https://github.com/rmisev/url_whatwg/blob/9388886/src/url_utf.h#L95
extern uint8_t k_U8_LEAD3_T1_BITS[16];
extern uint8_t k_U8_LEAD4_T1_BITS[16];

inline bool ReadCodePoint(const char*& first,  // NOLINT(runtime/references)
                          const char* last,
                          uint32_t& c) {  // NOLINT(runtime/references)
  c = static_cast<uint8_t>(*first++);
  if (c & 0x80) {
    uint8_t __t = 0;
    if (first != last &&
        // fetch/validate/assemble all but last trail byte
        (c >= 0xE0
             ? (c < 0xF0 ?  // U+0800..U+FFFF except surrogates
                    k_U8_LEAD3_T1_BITS[c &= 0xF] &
                            (1
                             << ((__t = static_cast<uint8_t>(*first)) >> 5)) &&
                        (__t &= 0x3F, 1)
                         :  // U+10000..U+10FFFF
                    (c -= 0xF0) <= 4 &&
                        k_U8_LEAD4_T1_BITS
                                [(__t = static_cast<uint8_t>(*first)) >> 4] &
                            (1 << c) &&
                        (c = (c << 6) | (__t & 0x3F), ++first != last) &&
                        (__t = static_cast<uint8_t>(
                             static_cast<uint8_t>(*first) - 0x80)) <= 0x3F) &&
                   // valid second-to-last trail byte
                   (c = (c << 6) | __t, ++first != last)
             :  // U+0080..U+07FF
             c >= 0xC2 && (c &= 0x1F, 1)) &&
        // last trail byte
        (__t = static_cast<uint8_t>(static_cast<uint8_t>(*first) - 0x80)) <=
            0x3F &&
        (c = (c << 6) | __t, ++first, 1)) {
      // valid utf-8
    } else {
      // ill-formed
      // c = 0xfffd;
      return false;
    }
  }
  return true;
}

// Refs: https://github.com/rmisev/url_whatwg/blob/9388886/src/url_utf.h#L73
template <typename CharT>
inline bool ReadUtfChar(const CharT*& first,  // NOLINT(runtime/references)
                        const CharT* last,
                        uint32_t& code_point) {  // NOLINT(runtime/references)
  if (!ReadCodePoint(first, last, code_point)) {
    code_point = 0xFFFD;  // REPLACEMENT CHARACTER
    return false;
  }
  return true;
}

// Ref: https://github.com/rmisev/url_whatwg/blob/9388886/src/url_utf.cpp#L75
inline int CompareByCodeUnits(const std::string& lhs, const std::string& rhs) {
  const char* it1 = lhs.c_str();
  const char* it2 = rhs.c_str();
  const char* last1 = it1 + lhs.size();
  const char* last2 = it2 + rhs.size();
  while (it1 != last1 && it2 != last2) {
    if (static_cast<unsigned char>(*it1) < 0x80 ||
        static_cast<unsigned char>(*it2) < 0x80) {
      if (*it1 == *it2) {
        ++it1, ++it2;
        continue;
      }
      return static_cast<int>((static_cast<unsigned char>(*it1))) -
             static_cast<int>((static_cast<unsigned char>(*it2)));
    }

    // read code points
    uint32_t cp1, cp2;
    ReadUtfChar(it1, last1, cp1);
    ReadUtfChar(it2, last2, cp2);
    if (cp1 == cp2) continue;

    // code points not equal - compare code units
    uint32_t cu1 = cp1 <= 0xffff ? cp1 : ((cp1 >> 10) + 0xd7c0);
    uint32_t cu2 = cp2 <= 0xffff ? cp2 : ((cp2 >> 10) + 0xd7c0);
    // cu1 can be equal to cu2 if they both are lead surrogates
    if (cu1 == cu2) {
      CHECK(U16IsLead(cu1));
      // get trail surrogates
      cu1 = (cp1 & 0x3ff);  // | 0xdc00;
      cu2 = (cp2 & 0x3ff);  // | 0xdc00;
    }
    return static_cast<int>(cu1) - static_cast<int>(cu2);
  }
  if (it1 != last1) return 1;
  if (it2 != last2) return -1;
  return 0;
}

}  // namespace whatwgurl

#endif  // INCLUDE_CODE_POINTS_H_
