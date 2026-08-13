// SPDX-License-Identifier: Apache-2.0
#ifndef DAWG_CORE_SRC_LM_CHAR_LM_H_
#define DAWG_CORE_SRC_LM_CHAR_LM_H_

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include "dawg/error.h"
#include "util/mmap_file.h"

namespace dawg {

// Read-only character-level n-gram language model (charlm). Scores are
// base-10 logs; backoff follows the standard ARPA semantics over the packed
// trie.
class CharLm {
 public:
  CharLm() = default;
  CharLm(const CharLm&) = delete;
  CharLm& operator=(const CharLm&) = delete;

  ErrorCode Load(const std::string& path);
  // Caller-owned buffer; must outlive this object.
  ErrorCode LoadFromBuffer(const uint8_t* data, size_t size);

  bool valid() const { return data_ != nullptr; }
  int order() const { return order_; }

  // Maps a code point to its vocab id; unknown chars map to kUnkCharId.
  uint32_t MapChar(char32_t c) const;

  // Sum of log10 probabilities for each char of `word`, conditioned on
  // `context` (oldest first). The effective context is the trailing
  // (order-1) chars of `context`; an empty context is padded with <s> to
  // match training-time line starts.
  double ScoreWord(const std::vector<char32_t>& word,
                   const std::vector<char32_t>& context) const;

 private:
  const uint8_t* NodeAt(uint32_t node) const;
  int32_t FindChild(uint32_t node, uint32_t char_id) const;
  int32_t DescendPath(const std::vector<uint32_t>& ids, size_t begin) const;
  double ScoreRec(const std::vector<uint32_t>& ctx, size_t begin,
                  uint32_t target) const;

  util::MmapFile file_;
  const uint8_t* data_ = nullptr;
  size_t size_ = 0;
  size_t vocab_offset_ = 0;
  size_t nodes_offset_ = 0;
  uint32_t order_ = 0;
  uint32_t vocab_size_ = 0;
  uint32_t node_count_ = 0;
  std::unordered_map<char32_t, uint32_t> char_to_id_;
};

}  // namespace dawg

#endif  // DAWG_CORE_SRC_LM_CHAR_LM_H_
