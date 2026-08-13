// SPDX-License-Identifier: Apache-2.0
#ifndef DAWG_INCLUDE_DAWG_CANDIDATE_H_
#define DAWG_INCLUDE_DAWG_CANDIDATE_H_

#include <string>

namespace dawg {

// Category of a produced candidate.
enum class CandidateKind : int {
  kSentence = 0,  // Multi-word path assembled by the decoder.
  kPhrase = 1,    // Single lexicon entry covering the whole input.
  kChar = 2,      // Single-character candidate (per-syllable fallback).
};

struct Candidate {
  std::string text;    // UTF-8 candidate text.
  std::string pinyin;  // Space-separated syllables, e.g. "zhong guo ren".
  double score = 0.0;  // Combined log10-domain score; higher is better.
  CandidateKind kind = CandidateKind::kPhrase;
};

}  // namespace dawg

#endif  // DAWG_INCLUDE_DAWG_CANDIDATE_H_
