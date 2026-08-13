// SPDX-License-Identifier: Apache-2.0
// Decode latency sanity probe (recorded, not gating). Runs repeated searches
// and reports p50/p95 in milliseconds.
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <string>
#include <vector>

#include "dawg/engine.h"

int main(int argc, char** argv) {
  if (argc < 2) {
    std::fprintf(stderr, "usage: %s <dict.dawg> [lm.clm] [input]\n", argv[0]);
    return 2;
  }
  dawg::EngineOptions options;
  options.dawg_path = argv[1];
  options.lm_path = argc > 2 ? argv[2] : "";
  dawg::ErrorCode err = dawg::ErrorCode::kOk;
  auto engine = dawg::CreateEngine(options, &err);
  if (engine == nullptr) {
    std::fprintf(stderr, "engine init failed: %s\n", dawg::ErrorCodeToString(err));
    return 1;
  }
  const std::string text =
      argc > 3 ? std::string(argv[3])
               : std::string("zhonghuarenmingongheguo");
  constexpr int kRuns = 300;
  std::vector<double> ms;
  ms.reserve(kRuns);
  std::vector<dawg::Candidate> candidates;
  for (int i = 0; i < kRuns; ++i) {
    const auto t0 = std::chrono::steady_clock::now();
    const dawg::ErrorCode code = engine->Search(text, "", &candidates);
    const auto t1 = std::chrono::steady_clock::now();
    if (code != dawg::ErrorCode::kOk) {
      std::fprintf(stderr, "search failed: %s\n", dawg::ErrorCodeToString(code));
      return 1;
    }
    ms.push_back(std::chrono::duration<double, std::milli>(t1 - t0).count());
  }
  std::sort(ms.begin(), ms.end());
  std::printf("input='%s' runs=%d p50=%.3fms p95=%.3fms max=%.3fms top='%s'\n",
              text.c_str(), kRuns, ms[ms.size() / 2],
              ms[static_cast<size_t>(ms.size() * 0.95)], ms.back(),
              candidates.empty() ? "" : candidates.front().text.c_str());
  return 0;
}
