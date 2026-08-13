// SPDX-License-Identifier: Apache-2.0
// Thin extern "C" facade over the C++ engine for JNI / Swift / HarmonyOS
// NAPI bridges. The returned strings stay valid until the next search on the
// same handle or until the handle is destroyed.
#ifndef DAWG_INCLUDE_DAWG_CAPI_H_
#define DAWG_INCLUDE_DAWG_CAPI_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef struct DawgEngine DawgEngine;

typedef struct DawgOptions {
  const char* dawg_path;      // required, NUL-terminated
  const char* lm_path;        // optional, NULL or "" disables the LM
  int max_candidates;         // <=0 selects default 32
  int beam_width;             // <=0 selects default 16
  int context_chars;          // <0 selects default 8
} DawgOptions;

// Creates an engine; returns NULL on failure and stores the ErrorCode
// (see dawg::ErrorCode) in *err when err != NULL.
DawgEngine* dawg_engine_create(const DawgOptions* options, int* err);

void dawg_engine_destroy(DawgEngine* engine);

// Runs a search; returns the candidate count (>=0) or -1 on error with the
// ErrorCode stored in *err when err != NULL. `input` / `context` are UTF-8.
int dawg_engine_search(DawgEngine* engine, const char* input,
                       const char* context, int* err);

// Accessors over the cached results of the last search.
int dawg_candidate_count(const DawgEngine* engine);
// Returns 1 on success, 0 when index is out of range. Any out pointer may be
// NULL. `kind` receives the dawg::CandidateKind value.
int dawg_candidate_get(const DawgEngine* engine, int index, const char** text,
                       const char** pinyin, double* score, int* kind);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // DAWG_INCLUDE_DAWG_CAPI_H_
