// SPDX-License-Identifier: Apache-2.0
// Little-endian field access over untrusted buffers. All binary formats in
// this project are little-endian; never reinterpret_cast packed structs.
#ifndef DAWG_CORE_SRC_UTIL_READ_INT_H_
#define DAWG_CORE_SRC_UTIL_READ_INT_H_

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <vector>

namespace dawg {
namespace util {

inline uint16_t ReadU16(const uint8_t* p) {
  return static_cast<uint16_t>(p[0] | (static_cast<uint16_t>(p[1]) << 8));
}

inline uint32_t ReadU32(const uint8_t* p) {
  return static_cast<uint32_t>(p[0]) |
         (static_cast<uint32_t>(p[1]) << 8) |
         (static_cast<uint32_t>(p[2]) << 16) |
         (static_cast<uint32_t>(p[3]) << 24);
}

inline int32_t ReadI32(const uint8_t* p) {
  uint32_t u = ReadU32(p);
  int32_t v;
  std::memcpy(&v, &u, sizeof(v));
  return v;
}

inline void PutU16(std::vector<uint8_t>* out, uint16_t v) {
  out->push_back(static_cast<uint8_t>(v & 0xFF));
  out->push_back(static_cast<uint8_t>((v >> 8) & 0xFF));
}

inline void PutU32(std::vector<uint8_t>* out, uint32_t v) {
  out->push_back(static_cast<uint8_t>(v & 0xFF));
  out->push_back(static_cast<uint8_t>((v >> 8) & 0xFF));
  out->push_back(static_cast<uint8_t>((v >> 16) & 0xFF));
  out->push_back(static_cast<uint8_t>((v >> 24) & 0xFF));
}

inline void PutI32(std::vector<uint8_t>* out, int32_t v) {
  uint32_t u;
  std::memcpy(&u, &v, sizeof(u));
  PutU32(out, u);
}

inline void PutU8(std::vector<uint8_t>* out, uint8_t v) { out->push_back(v); }

}  // namespace util
}  // namespace dawg

#endif  // DAWG_CORE_SRC_UTIL_READ_INT_H_
