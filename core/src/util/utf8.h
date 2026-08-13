// SPDX-License-Identifier: Apache-2.0
#ifndef DAWG_CORE_SRC_UTIL_UTF8_H_
#define DAWG_CORE_SRC_UTIL_UTF8_H_

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace dawg {
namespace util {

// Decodes UTF-8 into code points. Invalid sequences are skipped (consumed one
// byte at a time) rather than failing, since candidate text coming from
// trusted dictionaries is valid by construction while user-provided context
// must never hard-fail.
std::vector<char32_t> DecodeUtf8(std::string_view text);

// Appends the UTF-8 encoding of `c` to *out. Code points above U+10FFFF are
// encoded as U+FFFD.
void AppendUtf8(char32_t c, std::string* out);

inline std::string EncodeUtf8(const std::vector<char32_t>& chars) {
  std::string out;
  for (char32_t c : chars) AppendUtf8(c, &out);
  return out;
}

}  // namespace util
}  // namespace dawg

#endif  // DAWG_CORE_SRC_UTIL_UTF8_H_
