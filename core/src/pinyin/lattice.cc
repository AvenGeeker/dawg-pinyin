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
    bool abbreviated;
  };
  std::vector<Frame> stack;
  stack.push_back(Frame{start, dict_.RootNode(), {}, false});
  std::unordered_set<uint64_t> seen;
  int steps = 0;
  int full_spans = 0;
  int abbrev_spans = 0;
  const auto budget_left = [&]() {
    return full_spans < kMaxFullSpansPerStart ||
           abbrev_spans < kMaxAbbrevSpansPerStart;
  };
  while (!stack.empty() && budget_left() && steps < kMaxStepsPerStart) {
    Frame f = std::move(stack.back());
    stack.pop_back();
    ++steps;
    if (f.pos > start) {
      dict_.ForEachNodeEntry(f.node, [&](uint32_t entry) {
        if (f.abbreviated) {
          if (abbrev_spans >= kMaxAbbrevSpansPerStart) return;
        } else if (full_spans >= kMaxFullSpansPerStart) {
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
        span.abbreviated = f.abbreviated;
        if (f.abbreviated) {
          ++abbrev_spans;
        } else {
          ++full_spans;
        }
        out->push_back(std::move(span));
      });
    }
    if (f.pos >= graph.length) {
      continue;
    }
    // Iterate in reverse: the syllabifier emits full-spelling edges before
    // abbreviated ones and the frame stack is LIFO, so full matches are
    // explored (and emitted into the span budget) first.
    for (auto edge_it = graph.edges_from[f.pos].rbegin();
         edge_it != graph.edges_from[f.pos].rend(); ++edge_it) {
      const SyllableEdge& edge = *edge_it;
      if (edge.abbreviated) {
        dict_.StepByInitial(
            f.node, edge.initial, [&](int syllable_id, int32_t child) {
              Frame nf;
              nf.pos = edge.end;
              nf.node = child;
              nf.syllables = f.syllables;
              nf.syllables.push_back(syllable_id);
              nf.abbreviated = true;
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
          nf.abbreviated = f.abbreviated;
          stack.push_back(std::move(nf));
        }
      }
    }
  }
}

}  // namespace dawg
