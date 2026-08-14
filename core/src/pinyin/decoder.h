// SPDX-License-Identifier: Apache-2.0
#ifndef DAWG_CORE_SRC_PINYIN_DECODER_H_
#define DAWG_CORE_SRC_PINYIN_DECODER_H_

#include <vector>

#include "dawg/candidate.h"
#include "lm/char_lm.h"

namespace dawg {
class DawgReader;
struct SyllableGraph;

struct DecodeOptions {
  int beam_width = 16;
  int max_candidates = 32;
  int context_chars = 8;
};

// Beam decoder over the word lattice. Path score = normalized lexicon weight
// + charlm score conditioned on the committed context plus the chars of the
// partial path, so language-model evidence drives word segmentation choices
// ("后分词") as the lattice is explored.
class Decoder {
 public:
  // `lm` may be null to disable language-model scoring.
  Decoder(const DawgReader& dict, const CharLm* lm) : dict_(dict), lm_(lm) {}

  // Decodes a graph that already covers its full input; yields kOk with an
  // empty list when no complete path exists. Candidates come out best-first,
  // de-duplicated by text and filtered against the beam floor.
  ErrorCode Decode(const SyllableGraph& graph,
                   const std::vector<char32_t>& context,
                   const DecodeOptions& options,
                   std::vector<Candidate>* out) const;

  // Score gap (log10) beyond which a candidate is dropped.
  static constexpr double kCandidateFloor = 6.0;
  static constexpr double kBeamFloor = 12.0;
  // Per end position, only the heaviest spans enter the beam: dictionary
  // weight alone rules out the rest, and the beam must stay cheap enough
  // for on-device keystroke latency.
  static constexpr int kMaxSpansPerEnd = 64;
  // Log10-domain penalty per word matched through initial-only (abbreviated)
  // edges. Keeps shorthand paths available for genuine jianpin input while
  // stopping high-frequency single chars from outranking full-spelling words.
  static constexpr double kAbbreviatedPenalty = -2.5;

 private:
  const DawgReader& dict_;
  const CharLm* lm_;
};

}  // namespace dawg

#endif  // DAWG_CORE_SRC_PINYIN_DECODER_H_
