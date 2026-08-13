// SPDX-License-Identifier: Apache-2.0
#ifndef DAWG_INCLUDE_DAWG_ENGINE_H_
#define DAWG_INCLUDE_DAWG_ENGINE_H_

#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "dawg/candidate.h"
#include "dawg/error.h"

namespace dawg {

struct EngineOptions {
  // Path of the .dawg lexicon binary (required).
  std::string dawg_path;
  // Path of the .clm character n-gram binary; empty disables the LM and the
  // engine falls back to lexicon-weight ranking.
  std::string lm_path;
  // Maximum number of candidates returned by Search(). Clamped to [1, 128].
  int max_candidates = 32;
  // Beam width kept per lattice position. Clamped to [1, 64].
  int beam_width = 16;
  // How many trailing characters of the supplied context influence scoring.
  // Clamped to [0, 32].
  int context_chars = 8;
};

// Pinyin input engine. Create via CreateEngine(); all methods are
// thread-compatible for concurrent Search() calls on one instance only if
// callers serialize access (v1 keeps no internal locks).
class Engine {
 public:
  virtual ~Engine() = default;

  // Decodes `input` (lowercase a-z, length 1..64) into ranked candidates.
  // `context` is the already-committed preceding text (UTF-8, may be empty);
  // its trailing characters condition the language model. Empty `input`
  // yields kOk with an empty output list. Output ordering is best-first.
  virtual ErrorCode Search(std::string_view input, std::string_view context,
                           std::vector<Candidate>* out) = 0;
};

// Factory. On failure returns nullptr and stores the reason in *error
// (error may be nullptr only when the caller ignores the reason).
std::unique_ptr<Engine> CreateEngine(const EngineOptions& options,
                                     ErrorCode* error);

}  // namespace dawg

#endif  // DAWG_INCLUDE_DAWG_ENGINE_H_
