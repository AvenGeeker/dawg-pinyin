// SPDX-License-Identifier: Apache-2.0
#include "pinyin/syllabifier.h"

#include <algorithm>

#include "pinyin/syllable_table.h"

namespace dawg {

ErrorCode Syllabifier::Segment(std::string_view input,
                               SyllableGraph* out) const {
  out->input.assign(input);
  out->length = static_cast<int>(input.size());
  out->edges_from.assign(input.size() + 1, {});
  out->reachable.assign(input.size() + 1, false);
  if (input.empty()) {
    return ErrorCode::kOk;
  }
  if (input.size() > kMaxInputLength) {
    return ErrorCode::kInvalidInput;
  }
  for (char c : input) {
    if (c < 'a' || c > 'z') {
      return ErrorCode::kInvalidInput;
    }
  }

  const SyllableTable& table = SyllableTable::Instance();
  const int n = out->length;
  for (int i = 0; i < n; ++i) {
    // Full-spelling edges: longest syllable is 6 letters ("zhuang").
    const int max_len = std::min(6, n - i);
    for (int len = 1; len <= max_len; ++len) {
      const int id = table.Find(input.substr(i, len));
      if (id >= 0) {
        SyllableEdge e;
        e.start = i;
        e.end = i + len;
        e.syllable_id = id;
        e.abbreviated = false;
        out->edges_from[i].push_back(e);
      }
    }
    // Abbreviated edge: one letter standing for any syllable with the same
    // initial, including zero-initial syllables such as "a"/"ai".
    const char letter = input[i];
    if (table.IsValidInitial(letter)) {
      SyllableEdge e;
      e.start = i;
      e.end = i + 1;
      e.initial = letter;
      e.abbreviated = true;
      out->edges_from[i].push_back(e);
    }
  }

  out->reachable[0] = true;
  for (int i = 0; i < n; ++i) {
    if (!out->reachable[i]) continue;
    for (const SyllableEdge& e : out->edges_from[i]) {
      out->reachable[e.end] = true;
    }
  }
  return ErrorCode::kOk;
}

}  // namespace dawg
