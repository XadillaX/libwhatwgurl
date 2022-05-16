#include "percent_encode.h"

namespace whatwgurl {
namespace percent_encode {

void Encode(const unsigned char* input,
            size_t input_size,
            const uint8_t* percent_encode_set,
            TempStringBuffer* output,
            bool space_as_plus) {
  // The max length should be `input_size * 3`. But `output`'s length is
  // dynamic. So we use a smaller length to reduce the initialization cost.
  output->Reset(input_size * 2);

  // Dump `percent_encode_set`.
  // for (size_t i = 0; i < 32; i++) {
  //   for (size_t j = 0; j < 8; j++) {
  //     printf("%.2X: %d ", (i * 8) + j, (percent_encode_set[i] & (1 << j)) ? 1
  //     : 0);
  //   }
  //   printf("\n");
  // }

  unsigned char isomorph;
  const unsigned char* ptr = input;
  const unsigned char* end = input + input_size;
  while (ptr < end) {
    // Let isomorph be a code point whose value is byte’s value.
    isomorph = *ptr;

    // If spaceAsPlus is true and byte is 0x20 (SP), then append U+002B (+) to
    // output and continue.
    if (space_as_plus && isomorph == 0x20) {
      output->Append('+');
      ptr++;
      continue;
    }

    // If isomorph is not in percentEncodeSet, then append isomorph to output.
    if (!BitAt(percent_encode_set, isomorph)) {
      output->Append(isomorph);

      // Otherwise, percent-encode byte and append the result to output.
    } else {
      output->Append(hex + (isomorph * 4), PER_PERCENT_HEX_LENGTH);
    }

    ptr++;
  }
}

void Decode(const unsigned char* input,
            size_t input_size,
            TempStringBuffer* output) {
  // Let output be an empty byte sequence.
  output->Reset(input_size);

  // For each byte byte in input:
  const unsigned char* end = input + input_size;
  unsigned char c;
  for (const unsigned char* ptr = input; ptr < end; ptr++) {
    c = *ptr;

    // If byte is not 0x25 (%), then append byte to output.
    if (c != 0x25) {
      output->Append(c);

      // Otherwise if byte is 0x25 (%) and the next two bytes after byte in
      // input are not in the ranges 0x30 (0) to 0x39 (9), 0x41 (A) to 0x46 (F),
      // and 0x61 (a) to 0x66 (f), all inclusive, append byte to output.
    } else if (ptr + 2 >= end) {
      output->Append(c);
    } else {
      const unsigned char c1 = hexval[*(ptr + 1)];
      const unsigned char c2 = hexval[*(ptr + 2)];
      if (c1 == 0xff || c2 == 0xff) {
        output->Append(c);

        // Otherwise:
      } else {
        // Let bytePoint be the two bytes after byte in input, decoded, and then
        // interpreted as hexadecimal number. Append a byte whose value is
        // bytePoint to output.
        output->Append((c1 << 4) | c2);

        // Skip the next two bytes in input.
        ptr += 2;
      }
    }
  }
}

}  // namespace percent_encode
}  // namespace whatwgurl
