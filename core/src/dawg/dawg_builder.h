// SPDX-License-Identifier: Apache-2.0
#ifndef DAWG_CORE_SRC_DAWG_DAWG_BUILDER_H_
#define DAWG_CORE_SRC_DAWG_DAWG_BUILDER_H_

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "dawg/error.h"

namespace dawg {

// Offline builder for .dawg lexicon binaries. Host tools and tests link
// this; the runtime decoder only needs DawgReader.
class DawgBuilder {
 public:
  DawgBuilder() = default;
  DawgBuilder(const DawgBuilder&) = delete;
  DawgBuilder& operator=(const DawgBuilder&) = delete;

  // Adds one entry. `text` must be valid UTF-8 with exactly as many code
  // points as syllables in `pinyin` (space-separated spellings), which must
  // all exist in the syllable table. Duplicate (text, pinyin) pairs keep the
  // larger weight.
  ErrorCode AddEntry(std::string_view text, std::string_view pinyin,
                     int32_t weight);

  // Sorts, de-duplicates and minimizes the automaton. Invalidates nothing;
  // may be called once.
  ErrorCode Build();

  // Requires a successful Build().
  const std::vector<uint8_t>& image() const { return image_; }
  std::vector<uint8_t> Serialize() const { return image_; }
  ErrorCode WriteFile(const std::string& path) const;

  size_t entry_count() const { return entry_count_; }
  bool built() const { return built_; }

 private:
  struct RawEntry {
    std::vector<int> syllables;
    std::string text;
    int32_t weight = 0;
  };
  std::vector<RawEntry> entries_;
  std::vector<uint8_t> image_;
  size_t entry_count_ = 0;
  bool built_ = false;
};

}  // namespace dawg

#endif  // DAWG_CORE_SRC_DAWG_DAWG_BUILDER_H_
