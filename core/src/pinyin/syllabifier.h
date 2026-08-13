// SPDX-License-Identifier: Apache-2.0
#ifndef DAWG_CORE_SRC_PINYIN_SYLLABIFIER_H_
#define DAWG_CORE_SRC_PINYIN_SYLLABIFIER_H_

#include <string>
#include <string_view>
#include <vector>

#include "dawg/error.h"

namespace dawg {

// One edge of the syllable DAG over the raw input letters.
struct SyllableEdge {
  int start = 0;   // Inclusive start offset in the input.
  int end = 0;     // Exclusive end offset.
  int syllable_id = -1;  // Valid for full-spelling edges.
  char initial = 0;      // Valid for abbreviated edges (single letter).
  bool abbreviated = false;  // True for shorthand initial-only edges.
};

// DAG of possible syllable interpretations of a lowercase a-z input.
struct SyllableGraph {
  std::string input;
  int length = 0;
  // edges_from[i] holds every edge starting at position i.
  std::vector<std::vector<SyllableEdge>> edges_from;
  // Positions reachable from 0 following edges; used to decide whether the
  // whole input (or a prefix of it) is segmentable.
  std::vector<bool> reachable;

  bool CoversFullInput() const { return length > 0 && reachable[length]; }
};

// Turns raw letters into a SyllableGraph. Full-spelling edges cover exact
// syllables; abbreviated edges consume one letter and stand for any syllable
// starting with that letter (ambiguous by construction). Ambiguous full
// segmentations (e.g. "xian" vs "xi'an") appear as alternative DAG paths.
class Syllabifier {
 public:
  static constexpr int kMaxInputLength = 64;

  // input must be lowercase a-z only; otherwise returns kInvalidInput.
  // Empty input returns kOk with an empty graph.
  ErrorCode Segment(std::string_view input, SyllableGraph* out) const;
};

}  // namespace dawg

#endif  // DAWG_CORE_SRC_PINYIN_SYLLABIFIER_H_
