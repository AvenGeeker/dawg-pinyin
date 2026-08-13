// SPDX-License-Identifier: Apache-2.0
#include "pinyin/decoder.h"

#include <algorithm>
#include <cmath>
#include <unordered_map>

#include "dawg/dawg_reader.h"
#include "pinyin/lattice.h"
#include "pinyin/syllable_table.h"
#include "pinyin/syllabifier.h"
#include "util/utf8.h"

namespace dawg {

namespace {

struct State {
  double score = 0.0;
  std::vector<char32_t> tail;   // Chars conditioning the next LM step.
  std::vector<int> path;        // WordSpan indices in visit order.
};

std::string JoinPinyin(const std::vector<int>& syllables) {
  const SyllableTable& table = SyllableTable::Instance();
  std::string out;
  for (size_t i = 0; i < syllables.size(); ++i) {
    if (i > 0) out.push_back(' ');
    out.append(table.text(syllables[i]));
  }
  return out;
}

}  // namespace

ErrorCode Decoder::Decode(const SyllableGraph& graph,
                          const std::vector<char32_t>& context,
                          const DecodeOptions& options,
                          std::vector<Candidate>* out) const {
  if (out == nullptr) {
    return ErrorCode::kInvalidInput;
  }
  out->clear();
  const int n = graph.length;
  if (n == 0 || !graph.CoversFullInput() || !dict_.valid()) {
    return ErrorCode::kOk;
  }

  // 1. Word lattice: spans grouped by their end position.
  LatticeBuilder lattice(dict_);
  std::vector<WordSpan> spans;
  std::vector<std::vector<int>> spans_by_end(n + 1);
  for (int i = 0; i < n; ++i) {
    if (!graph.reachable[i]) continue;
    const size_t before = spans.size();
    lattice.CollectFrom(graph, i, &spans);
    for (size_t k = before; k < spans.size(); ++k) {
      spans_by_end[spans[k].end].push_back(static_cast<int>(k));
    }
  }

  // 2. Beam search left to right.
  const double dict_norm =
      std::log10(static_cast<double>(dict_.max_weight()) + 1.0);
  const size_t tail_window =
      lm_ != nullptr ? static_cast<size_t>(lm_->order() - 1) : 0;

  std::vector<State> initial;
  {
    State s;
    s.score = 0.0;
    s.tail = context;
    if (tail_window > 0 && s.tail.size() > tail_window) {
      s.tail.erase(s.tail.begin(),
                   s.tail.begin() + (s.tail.size() - tail_window));
    }
    initial.push_back(std::move(s));
  }

  std::vector<std::vector<State>> states(n + 1);
  states[0] = std::move(initial);

  for (int pos = 1; pos <= n; ++pos) {
    std::vector<State> next;
    for (const int span_id : spans_by_end[pos]) {
      const WordSpan& span = spans[span_id];
      const std::vector<State>& from = states[span.start];
      if (from.empty()) continue;
      const double dict_score =
          std::log10(static_cast<double>(dict_.EntryWeight(span.entry)) +
                     1.0) - dict_norm;
      for (const State& s : from) {
        State ns;
        ns.score = s.score + dict_score +
                   (lm_ != nullptr ? lm_->ScoreWord(span.chars, s.tail) : 0.0);
        ns.path = s.path;
        ns.path.push_back(span_id);
        if (tail_window > 0) {
          ns.tail = s.tail;
          ns.tail.insert(ns.tail.end(), span.chars.begin(), span.chars.end());
          if (ns.tail.size() > tail_window) {
            ns.tail.erase(ns.tail.begin(),
                          ns.tail.begin() + (ns.tail.size() - tail_window));
          }
        }
        next.push_back(std::move(ns));
      }
    }
    if (next.empty()) continue;
    std::sort(next.begin(), next.end(),
              [](const State& a, const State& b) { return a.score > b.score; });
    const double floor = next.front().score - kBeamFloor;
    size_t keep = 0;
    while (keep < next.size() && keep < static_cast<size_t>(options.beam_width) &&
           next[keep].score >= floor) {
      ++keep;
    }
    next.resize(keep);
    states[pos] = std::move(next);
  }

  if (states[n].empty()) {
    return ErrorCode::kOk;
  }

  // 3. Assemble, dedupe and filter candidates.
  std::unordered_map<std::string, size_t> dedupe;
  std::vector<Candidate> candidates;
  for (const State& s : states[n]) {
    Candidate c;
    c.score = s.score;
    c.kind = s.path.size() > 1 ? CandidateKind::kSentence : CandidateKind::kPhrase;
    std::vector<int> syllables;
    size_t char_count = 0;
    for (const int span_id : s.path) {
      const WordSpan& span = spans[span_id];
      for (const char32_t ch : span.chars) {
        util::AppendUtf8(ch, &c.text);
      }
      syllables.insert(syllables.end(), span.syllables.begin(),
                       span.syllables.end());
      char_count += span.chars.size();
    }
    if (s.path.size() == 1 && char_count == 1) {
      c.kind = CandidateKind::kChar;
    }
    c.pinyin = JoinPinyin(syllables);
    auto it = dedupe.find(c.text);
    if (it == dedupe.end()) {
      dedupe.emplace(c.text, candidates.size());
      candidates.push_back(std::move(c));
    } else if (candidates[it->second].score < c.score) {
      candidates[it->second] = std::move(c);
    }
  }
  std::sort(candidates.begin(), candidates.end(),
            [](const Candidate& a, const Candidate& b) {
              return a.score > b.score;
            });
  const double best = candidates.front().score;
  const double cutoff = best - kCandidateFloor;
  size_t keep = 0;
  while (keep < candidates.size() && candidates[keep].score >= cutoff &&
         keep < static_cast<size_t>(options.max_candidates)) {
    ++keep;
  }
  candidates.resize(keep);
  *out = std::move(candidates);
  return ErrorCode::kOk;
}

}  // namespace dawg
