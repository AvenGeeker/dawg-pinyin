// SPDX-License-Identifier: Apache-2.0
#include "pinyin/lattice.h"

#include <unordered_set>

#include "util/utf8.h"

namespace dawg {

void LatticeBuilder::CollectFrom(const SyllableGraph& graph, int start,
                                 std::vector<WordSpan>* out) const {
  if (!dict_.valid() || start < 0 || start >= graph.length) {
    return;
  }
  struct Frame {
    int pos;
    int32_t node;
    std::vector<int> syllables;
  };
  std::vector<Frame> stack;
  stack.push_back(Frame{start, dict_.RootNode(), {}});
  std::unordered_set<uint64_t> seen;
  int steps = 0;
  const size_t base = out->size();
  while (!stack.empty() &&
         static_cast<int>(out->size()) - static_cast<int>(base) <
             kMaxSpansPerStart &&
         steps < kMaxStepsPerStart) {
    Frame f = std::move(stack.back());
    stack.pop_back();
    ++steps;
    if (f.pos > start) {
      dict_.ForEachNodeEntry(f.node, [&](uint32_t entry) {
        if (static_cast<int>(out->size()) - static_cast<int>(base) >=
            kMaxSpansPerStart) {
          return;
        }
        const uint64_t key =
            (static_cast<uint64_t>(f.pos) << 32) | entry;
        if (!seen.insert(key).second) {
          return;
        }
        WordSpan span;
        span.start = start;
        span.end = f.pos;
        span.entry = entry;
        span.syllables = f.syllables;
        span.chars = util::DecodeUtf8(dict_.EntryText(entry));
        out->push_back(std::move(span));
      });
    }
    if (f.pos >= graph.length) {
      continue;
    }
    for (const SyllableEdge& edge : graph.edges_from[f.pos]) {
      if (edge.abbreviated) {
        dict_.StepByInitial(
            f.node, edge.initial, [&](int syllable_id, int32_t child) {
              Frame nf;
              nf.pos = edge.end;
              nf.node = child;
              nf.syllables = f.syllables;
              nf.syllables.push_back(syllable_id);
              stack.push_back(std::move(nf));
            });
      } else {
        const int32_t child = dict_.StepExact(f.node, edge.syllable_id);
        if (child >= 0) {
          Frame nf;
          nf.pos = edge.end;
          nf.node = child;
          nf.syllables = f.syllables;
          nf.syllables.push_back(edge.syllable_id);
          stack.push_back(std::move(nf));
        }
      }
    }
  }
}

}  // namespace dawg
