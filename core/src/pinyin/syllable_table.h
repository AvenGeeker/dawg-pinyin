// SPDX-License-Identifier: Apache-2.0
#ifndef DAWG_CORE_SRC_PINYIN_SYLLABLE_TABLE_H_
#define DAWG_CORE_SRC_PINYIN_SYLLABLE_TABLE_H_

#include <string_view>
#include <vector>

namespace dawg {

// Static table of standard toneless Mandarin pinyin syllables (~360). The
// syllable id is the index into the table and is stable across runs; binary
// assets (.dawg) depend on it, so the table is append-only.
class SyllableTable {
 public:
  static const SyllableTable& Instance();

  int size() const { return static_cast<int>(syllables_.size()); }
  std::string_view text(int id) const { return syllables_[id]; }
  // Returns the syllable id or -1 when unknown.
  int Find(std::string_view spelling) const;
  // All syllable ids whose spelling starts with `initial`; empty for letters
  // that never begin a syllable.
  const std::vector<int>& ByInitial(char initial) const;
  bool IsValidInitial(char initial) const;

 private:
  SyllableTable();
  std::vector<std::string_view> syllables_;
  // 26 buckets; bucket index = letter - 'a'. Built in the constructor.
  std::vector<std::vector<int>> by_initial_;
};

}  // namespace dawg

#endif  // DAWG_CORE_SRC_PINYIN_SYLLABLE_TABLE_H_
