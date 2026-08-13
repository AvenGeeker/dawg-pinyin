// SPDX-License-Identifier: Apache-2.0
#include "util/utf8.h"

namespace dawg {
namespace util {

namespace {

// Decodes one code point starting at *pos and advances *pos. Invalid input
// consumes a single byte and yields U+FFFD.
char32_t DecodeOne(std::string_view text, size_t* pos) {
  const auto* s = reinterpret_cast<const uint8_t*>(text.data());
  size_t i = *pos;
  uint8_t b0 = s[i];
  if (b0 < 0x80) {
    *pos = i + 1;
    return b0;
  }
  size_t extra = 0;
  uint32_t value = 0;
  if ((b0 & 0xE0) == 0xC0) {
    extra = 1;
    value = b0 & 0x1Fu;
  } else if ((b0 & 0xF0) == 0xE0) {
    extra = 2;
    value = b0 & 0x0Fu;
  } else if ((b0 & 0xF8) == 0xF0) {
    extra = 3;
    value = b0 & 0x07u;
  } else {
    *pos = i + 1;
    return 0xFFFD;
  }
  if (i + extra >= text.size()) {
    *pos = text.size();
    return 0xFFFD;
  }
  for (size_t k = 1; k <= extra; ++k) {
    uint8_t b = s[i + k];
    if ((b & 0xC0) != 0x80) {
      *pos = i + 1;
      return 0xFFFD;
    }
    value = (value << 6) | (b & 0x3Fu);
  }
  *pos = i + extra + 1;
  if ((extra == 1 && value < 0x80) || (extra == 2 && value < 0x800) ||
      (extra == 3 && value < 0x10000) ||
      (value >= 0xD800 && value <= 0xDFFF) || value > 0x10FFFF) {
    return 0xFFFD;
  }
  return value;
}

}  // namespace

std::vector<char32_t> DecodeUtf8(std::string_view text) {
  std::vector<char32_t> out;
  size_t pos = 0;
  while (pos < text.size()) {
    out.push_back(DecodeOne(text, &pos));
  }
  return out;
}

void AppendUtf8(char32_t c, std::string* out) {
  if (c > 0x10FFFF || (c >= 0xD800 && c <= 0xDFFF)) c = 0xFFFD;
  if (c < 0x80) {
    out->push_back(static_cast<char>(c));
  } else if (c < 0x800) {
    out->push_back(static_cast<char>(0xC0 | (c >> 6)));
    out->push_back(static_cast<char>(0x80 | (c & 0x3F)));
  } else if (c < 0x10000) {
    out->push_back(static_cast<char>(0xE0 | (c >> 12)));
    out->push_back(static_cast<char>(0x80 | ((c >> 6) & 0x3F)));
    out->push_back(static_cast<char>(0x80 | (c & 0x3F)));
  } else {
    out->push_back(static_cast<char>(0xF0 | (c >> 18)));
    out->push_back(static_cast<char>(0x80 | ((c >> 12) & 0x3F)));
    out->push_back(static_cast<char>(0x80 | ((c >> 6) & 0x3F)));
    out->push_back(static_cast<char>(0x80 | (c & 0x3F)));
  }
}

}  // namespace util
}  // namespace dawg
