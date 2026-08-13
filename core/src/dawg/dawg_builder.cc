// SPDX-License-Identifier: Apache-2.0
#include "dawg/dawg_builder.h"

#include <algorithm>
#include <functional>
#include <unordered_map>

#include "dawg/dawg_format.h"
#include "pinyin/syllable_table.h"
#include "util/mmap_file.h"
#include "util/read_int.h"
#include "util/utf8.h"

namespace dawg {

namespace {

struct TrieNode {
  // Sorted ascending by syllable id.
  std::vector<std::pair<int, uint32_t>> children;
  // Indices into the sorted entry array, weight-descending by construction.
  std::vector<uint32_t> entries;
};

void PutId(std::string* key, uint32_t v) {
  for (int shift = 0; shift < 32; shift += 8) {
    key->push_back(static_cast<char>((v >> shift) & 0xFF));
  }
}

}  // namespace

ErrorCode DawgBuilder::AddEntry(std::string_view text, std::string_view pinyin,
                                int32_t weight) {
  if (built_) {
    return ErrorCode::kInvalidInput;
  }
  const std::vector<char32_t> chars = util::DecodeUtf8(text);
  if (chars.empty() || chars.size() > 32) {
    return ErrorCode::kInvalidInput;
  }
  for (char32_t c : chars) {
    if (c == 0xFFFD || c < 0x20) {
      return ErrorCode::kInvalidInput;
    }
  }
  const SyllableTable& table = SyllableTable::Instance();
  std::vector<int> syllables;
  size_t pos = 0;
  while (pos < pinyin.size()) {
    while (pos < pinyin.size() && pinyin[pos] == ' ') ++pos;
    const size_t start = pos;
    while (pos < pinyin.size() && pinyin[pos] != ' ') ++pos;
    if (pos == start) continue;
    const int id = table.Find(pinyin.substr(start, pos - start));
    if (id < 0) {
      return ErrorCode::kInvalidInput;
    }
    syllables.push_back(id);
  }
  if (syllables.empty() || syllables.size() != chars.size()) {
    return ErrorCode::kInvalidInput;
  }
  entries_.push_back(
      RawEntry{std::move(syllables), std::string(text), weight});
  return ErrorCode::kOk;
}

ErrorCode DawgBuilder::Build() {
  if (built_) {
    return ErrorCode::kOk;
  }
  image_.clear();

  // 1. Sort by (syllables asc, weight desc, text asc) and dedupe.
  std::vector<uint32_t> order(entries_.size());
  for (uint32_t i = 0; i < entries_.size(); ++i) order[i] = i;
  std::sort(order.begin(), order.end(), [&](uint32_t a, uint32_t b) {
    const RawEntry& ea = entries_[a];
    const RawEntry& eb = entries_[b];
    if (ea.syllables != eb.syllables) return ea.syllables < eb.syllables;
    if (ea.weight != eb.weight) return ea.weight > eb.weight;
    return ea.text < eb.text;
  });
  std::vector<RawEntry> sorted;
  sorted.reserve(order.size());
  for (const uint32_t idx : order) {
    const RawEntry& e = entries_[idx];
    if (!sorted.empty() && sorted.back().syllables == e.syllables &&
        sorted.back().text == e.text) {
      continue;  // First kept copy has the largest weight.
    }
    sorted.push_back(e);
  }
  entries_ = std::move(sorted);
  entry_count_ = entries_.size();

  // 2. Trie insertion.
  std::vector<TrieNode> trie(1);
  for (uint32_t i = 0; i < entries_.size(); ++i) {
    uint32_t node = 0;
    for (const int syl : entries_[i].syllables) {
      auto& children = trie[node].children;
      auto it = std::lower_bound(
          children.begin(), children.end(), syl,
          [](const std::pair<int, uint32_t>& c, int id) {
            return c.first < id;
          });
      if (it == children.end() || it->first != syl) {
        it = children.insert(it, {syl, static_cast<uint32_t>(trie.size())});
        trie.emplace_back();
      }
      node = it->second;
    }
    trie[node].entries.push_back(i);
  }

  // 3. Bottom-up minimization. Children are rewritten to canonical ids and
  // the registry keys on (entries, children shape), so merged nodes are
  // structurally identical.
  std::vector<int32_t> canon(trie.size(), -1);
  std::unordered_map<std::string, int32_t> registry;
  int32_t next_canon = 0;
  std::function<int32_t(uint32_t)> assign = [&](uint32_t node) -> int32_t {
    if (canon[node] >= 0) return canon[node];
    TrieNode& t = trie[node];
    for (auto& child : t.children) {
      child.second = static_cast<uint32_t>(assign(child.second));
    }
    std::string key;
    key.reserve(16 + t.entries.size() * 5 + t.children.size() * 9);
    for (const uint32_t e : t.entries) {
      key.push_back('E');
      PutId(&key, e);
    }
    for (const auto& child : t.children) {
      key.push_back('C');
      PutId(&key, static_cast<uint32_t>(child.first));
      PutId(&key, child.second);
    }
    const auto it = registry.find(key);
    if (it != registry.end()) {
      canon[node] = it->second;
    } else {
      canon[node] = next_canon++;
      registry.emplace(std::move(key), canon[node]);
    }
    return canon[node];
  };
  const int32_t root_canon = assign(0);

  // One representative per canonical node.
  std::vector<TrieNode> canonical(static_cast<size_t>(next_canon));
  std::vector<bool> filled(static_cast<size_t>(next_canon), false);
  for (uint32_t i = 0; i < trie.size(); ++i) {
    const int32_t c = canon[i];
    if (c >= 0 && !filled[c]) {
      canonical[c] = std::move(trie[i]);
      filled[c] = true;
    }
  }

  // 4. Emit in DFS pre-order from the canonical root; root becomes node 0.
  std::vector<int32_t> final_index(static_cast<size_t>(next_canon), -1);
  std::vector<int32_t> order_out;
  std::function<void(int32_t)> visit = [&](int32_t c) {
    if (final_index[c] >= 0) return;
    final_index[c] = static_cast<int32_t>(order_out.size());
    order_out.push_back(c);
    for (const auto& child : canonical[c].children) {
      visit(static_cast<int32_t>(child.second));
    }
  };
  visit(root_canon);

  std::vector<uint8_t> node_blob;
  std::vector<uint8_t> child_blob;
  std::vector<uint8_t> entry_blob;
  std::vector<uint8_t> text_blob;
  for (const int32_t c : order_out) {
    const TrieNode& t = canonical[c];
    const uint32_t first_child =
        static_cast<uint32_t>(child_blob.size() / kDawgChildSize);
    const uint32_t first_entry =
        static_cast<uint32_t>(entry_blob.size() / kDawgEntrySize);
    util::PutU32(&node_blob, first_child);
    util::PutU16(&node_blob, static_cast<uint16_t>(t.children.size()));
    util::PutU32(&node_blob, first_entry);
    util::PutU16(&node_blob, static_cast<uint16_t>(t.entries.size()));
    for (const auto& child : t.children) {
      util::PutU16(&child_blob, static_cast<uint16_t>(child.first));
      util::PutU32(&child_blob,
                   static_cast<uint32_t>(final_index[child.second]));
    }
    for (const uint32_t e : t.entries) {
      const RawEntry& entry = entries_[e];
      util::PutU32(&entry_blob, static_cast<uint32_t>(text_blob.size()));
      util::PutU16(&entry_blob, static_cast<uint16_t>(entry.text.size()));
      util::PutU8(&entry_blob, static_cast<uint8_t>(entry.syllables.size()));
      util::PutI32(&entry_blob, entry.weight);
      text_blob.insert(text_blob.end(), entry.text.begin(), entry.text.end());
    }
  }

  std::vector<uint8_t> image;
  image.reserve(kDawgHeaderSize + node_blob.size() + child_blob.size() +
                entry_blob.size() + text_blob.size());
  image.insert(image.end(), kDawgMagic, kDawgMagic + sizeof(kDawgMagic));
  util::PutU32(&image, kDawgVersion);
  util::PutU32(&image, static_cast<uint32_t>(order_out.size()));
  util::PutU32(&image, static_cast<uint32_t>(child_blob.size() / kDawgChildSize));
  util::PutU32(&image, entry_count_);
  util::PutU32(&image, static_cast<uint32_t>(text_blob.size()));
  image.insert(image.end(), node_blob.begin(), node_blob.end());
  image.insert(image.end(), child_blob.begin(), child_blob.end());
  image.insert(image.end(), entry_blob.begin(), entry_blob.end());
  image.insert(image.end(), text_blob.begin(), text_blob.end());
  image_ = std::move(image);
  built_ = true;
  return ErrorCode::kOk;
}

ErrorCode DawgBuilder::WriteFile(const std::string& path) const {
  if (!built_) {
    return ErrorCode::kInvalidInput;
  }
  return util::WriteVectorToFile(path, image_);
}

}  // namespace dawg
