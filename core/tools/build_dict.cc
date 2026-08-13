// SPDX-License-Identifier: Apache-2.0
// Builds a .dawg lexicon binary from the unified text dictionary format:
//   word<TAB>pinyin(space separated)<TAB>weight
// Lines starting with '#' and blank lines are ignored. The weight column is
// optional and defaults to 1.
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "dawg/dawg_builder.h"
#include "dawg/error.h"

namespace {

std::vector<std::string> SplitTabs(const std::string& line) {
  std::vector<std::string> fields;
  size_t start = 0;
  while (start <= line.size()) {
    const size_t pos = line.find('\t', start);
    if (pos == std::string::npos) {
      fields.push_back(line.substr(start));
      break;
    }
    fields.push_back(line.substr(start, pos - start));
    start = pos + 1;
  }
  return fields;
}

std::string Trim(const std::string& s) {
  size_t b = s.find_first_not_of(" \t\r");
  if (b == std::string::npos) return "";
  size_t e = s.find_last_not_of(" \t\r");
  return s.substr(b, e - b + 1);
}

}  // namespace

int main(int argc, char** argv) {
  if (argc < 3) {
    std::fprintf(stderr,
                 "usage: %s <dict.txt> <out.dawg>\n"
                 "  dict.txt lines: word<TAB>pinyin(space separated)"
                 "[<TAB>weight]\n",
                 argv[0]);
    return 2;
  }
  std::ifstream in(argv[1]);
  if (!in) {
    std::fprintf(stderr, "cannot open %s\n", argv[1]);
    return 1;
  }
  dawg::DawgBuilder builder;
  std::string line;
  size_t line_no = 0;
  size_t skipped = 0;
  while (std::getline(in, line)) {
    ++line_no;
    line = Trim(line);
    if (line.empty() || line[0] == '#') continue;
    const std::vector<std::string> fields = SplitTabs(line);
    if (fields.size() < 2) {
      std::fprintf(stderr, "line %zu: expected word<TAB>pinyin, skipped\n",
                   line_no);
      ++skipped;
      continue;
    }
    int weight = 1;
    if (fields.size() >= 3 && !fields[2].empty()) {
      char* end = nullptr;
      const long parsed = std::strtol(fields[2].c_str(), &end, 10);
      if (end == fields[2].c_str() || *end != '\0') {
        std::fprintf(stderr, "line %zu: bad weight '%s', skipped\n", line_no,
                     fields[2].c_str());
        ++skipped;
        continue;
      }
      weight = static_cast<int>(parsed);
    }
    const dawg::ErrorCode err =
        builder.AddEntry(fields[0], fields[1], weight);
    if (err != dawg::ErrorCode::kOk) {
      std::fprintf(stderr, "line %zu: rejected (%s): %s\n", line_no,
                   dawg::ErrorCodeToString(err), line.c_str());
      ++skipped;
    }
  }
  const dawg::ErrorCode berr = builder.Build();
  if (berr != dawg::ErrorCode::kOk) {
    std::fprintf(stderr, "build failed: %s\n", dawg::ErrorCodeToString(berr));
    return 1;
  }
  const dawg::ErrorCode werr = builder.WriteFile(argv[2]);
  if (werr != dawg::ErrorCode::kOk) {
    std::fprintf(stderr, "write failed: %s\n", dawg::ErrorCodeToString(werr));
    return 1;
  }
  std::printf("entries=%zu skipped=%zu bytes=%zu -> %s\n",
              builder.entry_count(), skipped, builder.image().size(),
              argv[2]);
  return 0;
}
