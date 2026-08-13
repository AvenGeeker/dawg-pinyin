// SPDX-License-Identifier: Apache-2.0
// .dawg binary layout (all little-endian, field-wise access only):
//   Header  : magic "DWG1" | u32 version | u32 node_count | u32 child_count
//             u32 entry_count | u32 text_size
//   Nodes   : node_count * 12 bytes
//             { u32 first_child | u16 num_children | u32 first_entry
//               u16 num_entries }
//   Children: child_count * 6 bytes { u16 syllable_id | u32 node_index }
//             sorted ascending by syllable_id within each node
//   Entries : entry_count * 11 bytes
//             { u32 text_offset | u16 text_len | u8 syllable_count
//               i32 weight }
//   Text    : text_size bytes of concatenated UTF-8 entry text
// Root node index is always 0. Entry texts are guaranteed to contain one
// code point per syllable (enforced by the builder).
#ifndef DAWG_CORE_SRC_DAWG_DAWG_FORMAT_H_
#define DAWG_CORE_SRC_DAWG_DAWG_FORMAT_H_

#include <cstdint>

namespace dawg {

constexpr char kDawgMagic[4] = {'D', 'W', 'G', '1'};
constexpr uint32_t kDawgVersion = 1;
constexpr size_t kDawgHeaderSize = 24;
constexpr size_t kDawgNodeSize = 12;
constexpr size_t kDawgChildSize = 6;
constexpr size_t kDawgEntrySize = 11;

}  // namespace dawg

#endif  // DAWG_CORE_SRC_DAWG_DAWG_FORMAT_H_
