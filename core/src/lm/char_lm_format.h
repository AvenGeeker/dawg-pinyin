// SPDX-License-Identifier: Apache-2.0
// .clm binary layout (all little-endian, field-wise access only):
//   Header: magic "CLM1" | u32 version | u32 order | u32 vocab_size
//           u32 node_count
//   Vocab : vocab_size * u32 code points; index == char id.
//           id 0 = <unk>, id 1 = <s> (kBosChar below)
//   Nodes : node_count * 16 bytes
//           { u32 char_id | u8 prob_q | u8 backoff_q | u16 reserved
//             u32 first_child | u32 num_children }
//           children sorted ascending by char_id; root node index 0 holds
//           char_id 1 (<s>).
// Quantization (must match tools/lm/quantize_lm.py):
//   prob_q    = clamp(round((log10p + 10.0) * 25.5), 0, 255)   // [-10, 0]
//   backoff_q = clamp(round((backoff + 2.0) * 63.75), 0, 255)  // [-2, +2]
#ifndef DAWG_CORE_SRC_LM_CHAR_LM_FORMAT_H_
#define DAWG_CORE_SRC_LM_CHAR_LM_FORMAT_H_

#include <cstdint>

namespace dawg {

constexpr char kClmMagic[4] = {'C', 'L', 'M', '1'};
constexpr uint32_t kClmVersion = 1;
constexpr size_t kClmHeaderSize = 20;
constexpr size_t kClmNodeSize = 16;

constexpr char32_t kBosChar = 0x10FFFD;  // Private-use sentinel for <s>.
constexpr uint32_t kUnkCharId = 0;
constexpr uint32_t kBosCharId = 1;

constexpr double kProbFloorLog10 = -10.0;
constexpr double kProbCeilLog10 = 0.0;
constexpr double kProbQuantScale = 25.5;
constexpr double kBackoffMinLog10 = -2.0;
constexpr double kBackoffMaxLog10 = 2.0;
constexpr double kBackoffQuantScale = 63.75;

inline uint8_t QuantizeProb(double log10_p) {
  if (log10_p <= kProbFloorLog10) return 0;
  if (log10_p >= kProbCeilLog10) return 255;
  return static_cast<uint8_t>((log10_p - kProbFloorLog10) * kProbQuantScale +
                              0.5);
}

inline double DequantizeProb(uint8_t q) {
  return kProbFloorLog10 + q / kProbQuantScale;
}

inline uint8_t QuantizeBackoff(double log10_bo) {
  if (log10_bo <= kBackoffMinLog10) return 0;
  if (log10_bo >= kBackoffMaxLog10) return 255;
  return static_cast<uint8_t>((log10_bo - kBackoffMinLog10) *
                              kBackoffQuantScale + 0.5);
}

inline double DequantizeBackoff(uint8_t q) {
  return kBackoffMinLog10 + q / kBackoffQuantScale;
}

}  // namespace dawg

#endif  // DAWG_CORE_SRC_LM_CHAR_LM_FORMAT_H_
