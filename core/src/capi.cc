// SPDX-License-Identifier: Apache-2.0
#include "dawg/dawg_capi.h"

#include <string>
#include <vector>

#include "dawg/engine.h"

// Completes the opaque handle type declared in dawg_capi.h.
struct DawgEngine {
  std::unique_ptr<dawg::Engine> engine;
  std::vector<dawg::Candidate> results;
};

DawgEngine* dawg_engine_create(const DawgOptions* options, int* err) {
  const auto fail = [&](dawg::ErrorCode code) -> DawgEngine* {
    if (err != nullptr) *err = static_cast<int>(code);
    return nullptr;
  };
  if (options == nullptr || options->dawg_path == nullptr) {
    return fail(dawg::ErrorCode::kInvalidInput);
  }
  dawg::EngineOptions opts;
  opts.dawg_path = options->dawg_path;
  opts.lm_path = options->lm_path != nullptr ? options->lm_path : "";
  opts.max_candidates = options->max_candidates;
  opts.beam_width = options->beam_width;
  opts.context_chars = options->context_chars;
  dawg::ErrorCode code = dawg::ErrorCode::kOk;
  auto engine = dawg::CreateEngine(opts, &code);
  if (engine == nullptr) {
    return fail(code);
  }
  if (err != nullptr) *err = static_cast<int>(dawg::ErrorCode::kOk);
  auto* handle = new DawgEngine();
  handle->engine = std::move(engine);
  return handle;
}

void dawg_engine_destroy(DawgEngine* engine) { delete engine; }

int dawg_engine_search(DawgEngine* engine, const char* input,
                       const char* context, int* err) {
  if (engine == nullptr || engine->engine == nullptr || input == nullptr) {
    if (err != nullptr) *err = static_cast<int>(dawg::ErrorCode::kInvalidInput);
    return -1;
  }
  dawg::ErrorCode code = engine->engine->Search(
      input, context != nullptr ? context : "", &engine->results);
  if (code != dawg::ErrorCode::kOk) {
    if (err != nullptr) *err = static_cast<int>(code);
    return -1;
  }
  if (err != nullptr) *err = static_cast<int>(dawg::ErrorCode::kOk);
  return static_cast<int>(engine->results.size());
}

int dawg_candidate_count(const DawgEngine* engine) {
  if (engine == nullptr) return 0;
  return static_cast<int>(engine->results.size());
}

int dawg_candidate_get(const DawgEngine* engine, int index, const char** text,
                       const char** pinyin, double* score, int* kind) {
  if (engine == nullptr || index < 0 ||
      index >= static_cast<int>(engine->results.size())) {
    return 0;
  }
  const dawg::Candidate& c = engine->results[index];
  if (text != nullptr) *text = c.text.c_str();
  if (pinyin != nullptr) *pinyin = c.pinyin.c_str();
  if (score != nullptr) *score = c.score;
  if (kind != nullptr) *kind = static_cast<int>(c.kind);
  return 1;
}
