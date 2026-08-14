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
  bool abbreviated = false;        // True when any edge was initial-only.
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

  // Sized for production lexicons (~60k entries). Full-spelling spans are
  // emitted first (see CollectFrom); abbreviated-origin spans get their own
  // smaller budget so initial-only junk cannot starve the decoder.
  static constexpr int kMaxFullSpansPerStart = 2048;
  static constexpr int kMaxAbbrevSpansPerStart = 128;
  static constexpr int kMaxStepsPerStart = 100000;

 private:
  const DawgReader& dict_;
};

}  // namespace dawg

#endif  // DAWG_CORE_SRC_PINYIN_LATTICE_H_
