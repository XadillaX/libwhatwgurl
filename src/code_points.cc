#include "code_points.h"

namespace whatwgurl {

// Refs: https://github.com/rmisev/url_whatwg/blob/9388886/src/url_utf.cpp#L124

// Internal bit vector for 3-byte UTF-8 validity check, for use in
// U8_IS_VALID_LEAD3_AND_T1. Each bit indicates whether one lead byte + first
// trail byte pair starts a valid sequence. Lead byte E0..EF bits 3..0 are used
// as byte index, first trail byte bits 7..5 are used as bit index into that
// byte.
uint8_t k_U8_LEAD3_T1_BITS[16] = {0x20,
                                  0x30,
                                  0x30,
                                  0x30,
                                  0x30,
                                  0x30,
                                  0x30,
                                  0x30,
                                  0x30,
                                  0x30,
                                  0x30,
                                  0x30,
                                  0x30,
                                  0x10,
                                  0x30,
                                  0x30};

// Internal bit vector for 4-byte UTF-8 validity check, for use in
// U8_IS_VALID_LEAD4_AND_T1. Each bit indicates whether one lead byte + first
// trail byte pair starts a valid sequence. First trail byte bits 7..4 are used
// as byte index, lead byte F0..F4 bits 2..0 are used as bit index into that
// byte.
uint8_t k_U8_LEAD4_T1_BITS[16] = {0x00,
                                  0x00,
                                  0x00,
                                  0x00,
                                  0x00,
                                  0x00,
                                  0x00,
                                  0x00,
                                  0x1E,
                                  0x0F,
                                  0x0F,
                                  0x0F,
                                  0x00,
                                  0x00,
                                  0x00,
                                  0x00};

}  // namespace whatwgurl
