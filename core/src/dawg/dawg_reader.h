// SPDX-License-Identifier: Apache-2.0
#ifndef DAWG_CORE_SRC_DAWG_DAWG_READER_H_
#define DAWG_CORE_SRC_DAWG_DAWG_READER_H_

#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <string_view>
#include <vector>

#include "dawg/error.h"
#include "pinyin/syllabifier.h"
#include "pinyin/syllable_table.h"
#include "util/mmap_file.h"

namespace dawg {

// Read-only view over a minimized syllable-sequence FSA. Zero-copy: all
// accessors read fields straight out of the mapped image.
class DawgReader {
 public:
  DawgReader() = default;
  DawgReader(const DawgReader&) = delete;
  DawgReader& operator=(const DawgReader&) = delete;

  // Maps the file read-only. The mapping is owned by this reader.
  ErrorCode Load(const std::string& path);
  // Wraps a caller-owned buffer; data must outlive this reader.
  ErrorCode LoadFromBuffer(const uint8_t* data, size_t size);

  bool valid() const { return data_ != nullptr; }
  uint32_t entry_count() const { return entry_count_; }
  int32_t max_weight() const { return max_weight_; }

  std::string_view EntryText(uint32_t entry) const;
  int32_t EntryWeight(uint32_t entry) const;
  uint8_t EntrySyllableCount(uint32_t entry) const;

  // Trie navigation. Node indices are int32_t with -1 as "missing".
  int32_t RootNode() const { return valid() ? 0 : -1; }
  int32_t StepExact(int32_t node, int syllable_id) const;
  // Invokes cb(syllable_id, child_node) for every child whose syllable
  // starts with `initial`.
  void StepByInitial(int32_t node, char initial,
                     const std::function<void(int, int32_t)>& cb) const;
  uint16_t NumEntries(int32_t node) const;
  void ForEachNodeEntry(int32_t node,
                        const std::function<void(uint32_t)>& cb) const;

  // Convenience: enumerates all entries whose syllable sequence matches the
  // constraint path (full edges match exactly, abbreviated edges match any
  // syllable sharing the initial).
  ErrorCode ForEachMatch(const std::vector<SyllableEdge>& path,
                         const std::function<void(uint32_t)>& cb) const;

 private:
  ErrorCode Parse(const uint8_t* data, size_t size);
  const uint8_t* nodes() const { return data_ + nodes_offset_; }
  const uint8_t* children() const { return data_ + children_offset_; }
  const uint8_t* entries() const { return data_ + entries_offset_; }
  const uint8_t* text() const { return data_ + text_offset_; }
  const SyllableTable* table_ = nullptr;

  util::MmapFile file_;
  const uint8_t* data_ = nullptr;
  size_t size_ = 0;
  size_t nodes_offset_ = 0;
  size_t children_offset_ = 0;
  size_t entries_offset_ = 0;
  size_t text_offset_ = 0;
  uint32_t node_count_ = 0;
  uint32_t child_count_ = 0;
  uint32_t entry_count_ = 0;
  int32_t max_weight_ = 0;
};

}  // namespace dawg

#endif  // DAWG_CORE_SRC_DAWG_DAWG_READER_H_
