// SPDX-License-Identifier: Apache-2.0
#ifndef DAWG_CORE_SRC_PINYIN_LATTICE_H_
#define DAWG_CORE_SRC_PINYIN_LATTICE_H_

#include <cstdint>
#include <vector>

#include "dawg/dawg_reader.h"
#include "pinyin/syllabifier.h"

namespace dawg {

// One dictionary word matched against a syllable-span of the input.
struct WordSpan {
  int start = 0;   // Inclusive input offset.
  int end = 0;     // Exclusive input offset.
  uint32_t entry = 0;
  std::vector<int> syllables;      // Concrete syllable ids matched.
  std::vector<char32_t> chars;     // Decoded entry text.
};

// Walks the syllable DAG and the lexicon DAWG simultaneously, emitting every
// (word, span) pair reachable from a start position.
class LatticeBuilder {
 public:
  explicit LatticeBuilder(const DawgReader& dict) : dict_(dict) {}

  // Appends spans to *out; de-duplicates (end, entry) pairs so ambiguous
  // syllable paths do not multiply identical candidates.
  void CollectFrom(const SyllableGraph& graph, int start,
                   std::vector<WordSpan>* out) const;

  static constexpr int kMaxSpansPerStart = 256;
  static constexpr int kMaxStepsPerStart = 20000;

 private:
  const DawgReader& dict_;
};

}  // namespace dawg

#endif  // DAWG_CORE_SRC_PINYIN_LATTICE_H_
