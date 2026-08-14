// SPDX-License-Identifier: Apache-2.0
// Converts Rime-format dictionaries (word / space-separated pinyin /
// optional weight, plus YAML-ish header lines) into the unified text format
// consumed by dawg_build_dict. License compatibility of the source data is
// the operator's responsibility; imported output must not be committed here.
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "dawg/error.h"
#include "pinyin/syllable_table.h"
#include "util/utf8.h"

namespace {

std::vector<std::string> SplitWhitespace(const std::string& line) {
  std::vector<std::string> tokens;
  std::istringstream iss(line);
  std::string tok;
  while (iss >> tok) tokens.push_back(tok);
  return tokens;
}

bool LooksLikeHeader(const std::string& line) {
  if (line.empty()) return true;
  const char c = line[0];
  return c == '#' || c == '%' || c == '-' || c == '.' || c == ':';
}

bool IsInteger(const std::string& s) {
  if (s.empty()) return false;
  size_t i = (s[0] == '-' || s[0] == '+') ? 1 : 0;
  if (i >= s.size()) return false;
  for (; i < s.size(); ++i) {
    if (!std::isdigit(static_cast<unsigned char>(s[i]))) return false;
  }
  return true;
}

// Parses integer weights, accepting the trailing '%' used by some Rime
// dictionaries (e.g. luna_pinyin "100%").
bool ParseWeight(const std::string& token, int* out) {
  std::string t = token;
  if (!t.empty() && t.back() == '%') t.pop_back();
  if (!IsInteger(t)) return false;
  *out = static_cast<int>(std::strtol(t.c_str(), nullptr, 10));
  return true;
}

}  // namespace

int main(int argc, char** argv) {
  if (argc < 3) {
    std::fprintf(stderr, "usage: %s <rime_dict.txt> <out_dict.txt>\n",
                 argv[0]);
    return 2;
  }
  std::ifstream in(argv[1]);
  if (!in) {
    std::fprintf(stderr, "cannot open %s\n", argv[1]);
    return 1;
  }
  std::ofstream out(argv[2]);
  if (!out) {
    std::fprintf(stderr, "cannot open %s\n", argv[2]);
    return 1;
  }
  const dawg::SyllableTable& table = dawg::SyllableTable::Instance();
  std::string line;
  size_t kept = 0;
  size_t skipped = 0;
  while (std::getline(in, line)) {
    if (!line.empty() && line.back() == '\r') line.pop_back();
    if (LooksLikeHeader(line)) continue;
    const std::vector<std::string> tokens = SplitWhitespace(line);
    if (tokens.size() < 2) {
      ++skipped;
      continue;
    }
    const std::string& word = tokens[0];
    // Reject words containing ASCII letters: they are usually header debris.
    bool ascii = false;
    for (char c : word) {
      if (static_cast<unsigned char>(c) < 0x20 || c == 0x7F) ascii = true;
    }
    if (ascii || word.empty()) {
      ++skipped;
      continue;
    }
    int weight = 1;
    size_t pinyin_end = tokens.size();
    if (tokens.size() >= 3 && ParseWeight(tokens.back(), &weight)) {
      pinyin_end = tokens.size() - 1;
    }
    std::string pinyin;
    bool valid = true;
    for (size_t i = 1; i < pinyin_end; ++i) {
      std::string syllable = tokens[i];
      // Drop trailing tone digits (terra-pinyin style "wo3").
      if (!syllable.empty() && syllable.back() >= '1' &&
          syllable.back() <= '5') {
        syllable.pop_back();
      }
      if (table.Find(syllable) < 0) {
        valid = false;
        break;
      }
      if (!pinyin.empty()) pinyin.push_back(' ');
      pinyin += syllable;
    }
    if (!valid || pinyin.empty()) {
      ++skipped;
      continue;
    }
    const size_t char_count = dawg::util::DecodeUtf8(word).size();
    size_t syllable_count = 1;
    for (char c : pinyin) {
      if (c == ' ') ++syllable_count;
    }
    if (char_count != syllable_count) {
      ++skipped;
      continue;
    }
    out << word << '\t' << pinyin << '\t' << weight << '\n';
    ++kept;
  }
  std::printf("kept=%zu skipped=%zu -> %s\n", kept, skipped, argv[2]);
  return 0;
}
