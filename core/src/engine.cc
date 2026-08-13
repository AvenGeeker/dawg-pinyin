// SPDX-License-Identifier: Apache-2.0
#include "dawg/engine.h"

#include <algorithm>

#include "dawg/dawg_reader.h"
#include "lm/char_lm.h"
#include "pinyin/decoder.h"
#include "pinyin/syllabifier.h"
#include "util/utf8.h"

namespace dawg {

namespace {

class EngineImpl : public Engine {
 public:
  ErrorCode Search(std::string_view input, std::string_view context,
                   std::vector<Candidate>* out) override {
    if (out == nullptr) {
      return ErrorCode::kInvalidInput;
    }
    out->clear();
    if (input.empty()) {
      return ErrorCode::kOk;
    }
    if (input.size() > Syllabifier::kMaxInputLength) {
      return ErrorCode::kInvalidInput;
    }
    for (const char c : input) {
      if (c < 'a' || c > 'z') {
        return ErrorCode::kInvalidInput;
      }
    }

    std::vector<char32_t> context_chars = util::DecodeUtf8(context);
    if (static_cast<int>(context_chars.size()) > options_.context_chars) {
      context_chars.erase(
          context_chars.begin(),
          context_chars.begin() +
              (context_chars.size() - options_.context_chars));
    }

    DecodeOptions decode;
    decode.beam_width = options_.beam_width;
    decode.max_candidates = options_.max_candidates;
    decode.context_chars = options_.context_chars;
    const Decoder decoder(dict_, lm_.get());

    // Full input first; when the tail is not segmentable or yields nothing,
    // shrink the prefix so the user still gets candidates while typing.
    for (int len = static_cast<int>(input.size()); len >= 1; --len) {
      SyllableGraph graph;
      const ErrorCode err =
          syllabifier_.Segment(input.substr(0, len), &graph);
      if (err != ErrorCode::kOk) {
        return err;
      }
      if (!graph.CoversFullInput()) {
        continue;
      }
      std::vector<Candidate> candidates;
      const ErrorCode derr =
          decoder.Decode(graph, context_chars, decode, &candidates);
      if (derr != ErrorCode::kOk) {
        return derr;
      }
      if (!candidates.empty()) {
        *out = std::move(candidates);
        return ErrorCode::kOk;
      }
    }
    return ErrorCode::kOk;
  }

  EngineOptions options_;
  DawgReader dict_;
  std::unique_ptr<CharLm> lm_;
  Syllabifier syllabifier_;
};

int Clamp(int value, int lo, int hi, int fallback) {
  if (value <= 0) return fallback;
  return std::clamp(value, lo, hi);
}

}  // namespace

std::unique_ptr<Engine> CreateEngine(const EngineOptions& options,
                                     ErrorCode* error) {
  const auto fail = [&](ErrorCode code) -> std::unique_ptr<Engine> {
    if (error != nullptr) *error = code;
    return nullptr;
  };
  if (options.dawg_path.empty()) {
    return fail(ErrorCode::kInvalidInput);
  }
  auto engine = std::unique_ptr<EngineImpl>(new EngineImpl());
  engine->options_ = options;
  engine->options_.max_candidates =
      Clamp(options.max_candidates, 1, 128, 32);
  engine->options_.beam_width = Clamp(options.beam_width, 1, 64, 16);
  engine->options_.context_chars =
      options.context_chars < 0
          ? 8
          : std::clamp(options.context_chars, 0, 32);

  const ErrorCode derr = engine->dict_.Load(options.dawg_path);
  if (derr != ErrorCode::kOk) {
    return fail(derr);
  }
  if (!options.lm_path.empty()) {
    engine->lm_ = std::unique_ptr<CharLm>(new CharLm());
    const ErrorCode lerr = engine->lm_->Load(options.lm_path);
    if (lerr != ErrorCode::kOk) {
      return fail(lerr);
    }
  }
  if (error != nullptr) *error = ErrorCode::kOk;
  return engine;
}

}  // namespace dawg
