// SPDX-License-Identifier: Apache-2.0
#include "dawg/dawg_reader.h"

#include <cstring>

#include "dawg/dawg_format.h"
#include "util/read_int.h"

namespace dawg {

using util::ReadI32;
using util::ReadU16;
using util::ReadU32;

ErrorCode DawgReader::Load(const std::string& path) {
  ErrorCode err = file_.Open(path);
  if (err != ErrorCode::kOk) {
    return err;
  }
  return Parse(file_.data(), file_.size());
}

ErrorCode DawgReader::LoadFromBuffer(const uint8_t* data, size_t size) {
  file_.Close();
  return Parse(data, size);
}

ErrorCode DawgReader::Parse(const uint8_t* data, size_t size) {
  data_ = nullptr;
  size_ = 0;
  node_count_ = child_count_ = entry_count_ = 0;
  max_weight_ = 0;
  if (size < kDawgHeaderSize) {
    return ErrorCode::kBadFormat;
  }
  if (std::memcmp(data, kDawgMagic, sizeof(kDawgMagic)) != 0) {
    return ErrorCode::kBadFormat;
  }
  const uint32_t version = ReadU32(data + 4);
  if (version != kDawgVersion) {
    return ErrorCode::kBadFormat;
  }
  node_count_ = ReadU32(data + 8);
  child_count_ = ReadU32(data + 12);
  entry_count_ = ReadU32(data + 16);
  const uint32_t text_size = ReadU32(data + 20);

  nodes_offset_ = kDawgHeaderSize;
  children_offset_ = nodes_offset_ + size_t{node_count_} * kDawgNodeSize;
  entries_offset_ = children_offset_ + size_t{child_count_} * kDawgChildSize;
  text_offset_ = entries_offset_ + size_t{entry_count_} * kDawgEntrySize;
  const size_t total = text_offset_ + text_size;
  if (total != size || node_count_ == 0) {
    return ErrorCode::kBadFormat;
  }
  data_ = data;
  size_ = size;

  // Validate node ranges and cache the maximum entry weight.
  for (uint32_t i = 0; i < node_count_; ++i) {
    const uint8_t* n = nodes() + size_t{i} * kDawgNodeSize;
    const uint32_t first_child = ReadU32(n);
    const uint16_t num_children = ReadU16(n + 4);
    const uint32_t first_entry = ReadU32(n + 6);
    const uint16_t num_entries = ReadU16(n + 10);
    if (size_t{first_child} + num_children > child_count_) {
      return ErrorCode::kBadFormat;
    }
    if (size_t{first_entry} + num_entries > entry_count_) {
      return ErrorCode::kBadFormat;
    }
    for (uint16_t k = 0; k < num_entries; ++k) {
      const int32_t w = EntryWeight(first_entry + k);
      if (k == 0 || w > max_weight_) max_weight_ = w;
    }
  }
  // Children must be sorted per node (binary search relies on it).
  for (uint32_t i = 0; i < node_count_; ++i) {
    const uint8_t* n = nodes() + size_t{i} * kDawgNodeSize;
    const uint32_t first_child = ReadU32(n);
    const uint16_t num_children = ReadU16(n + 4);
    int last = -1;
    for (uint16_t k = 0; k < num_children; ++k) {
      const uint8_t* c = children() + size_t{first_child + k} * kDawgChildSize;
      const int id = ReadU16(c);
      const uint32_t child_node = ReadU32(c + 2);
      if (id <= last || child_node >= node_count_) {
        return ErrorCode::kBadFormat;
      }
      last = id;
    }
  }
  table_ = &SyllableTable::Instance();
  return ErrorCode::kOk;
}

std::string_view DawgReader::EntryText(uint32_t entry) const {
  if (entry >= entry_count_) return {};
  const uint8_t* e = entries() + size_t{entry} * kDawgEntrySize;
  const uint32_t off = ReadU32(e);
  const uint16_t len = ReadU16(e + 4);
  return std::string_view(reinterpret_cast<const char*>(text() + off), len);
}

int32_t DawgReader::EntryWeight(uint32_t entry) const {
  if (entry >= entry_count_) return 0;
  const uint8_t* e = entries() + size_t{entry} * kDawgEntrySize;
  return ReadI32(e + 7);
}

uint8_t DawgReader::EntrySyllableCount(uint32_t entry) const {
  if (entry >= entry_count_) return 0;
  const uint8_t* e = entries() + size_t{entry} * kDawgEntrySize;
  return e[6];
}

int32_t DawgReader::StepExact(int32_t node, int syllable_id) const {
  if (node < 0 || static_cast<uint32_t>(node) >= node_count_) return -1;
  const uint8_t* n = nodes() + static_cast<size_t>(node) * kDawgNodeSize;
  const uint32_t first_child = ReadU32(n);
  const uint16_t num_children = ReadU16(n + 4);
  uint16_t lo = 0;
  uint16_t hi = num_children;
  while (lo < hi) {
    const uint16_t mid = static_cast<uint16_t>((lo + hi) / 2);
    const uint8_t* c = children() + size_t{first_child + mid} * kDawgChildSize;
    const int id = ReadU16(c);
    if (id == syllable_id) {
      return static_cast<int32_t>(ReadU32(c + 2));
    }
    if (id < syllable_id) {
      lo = static_cast<uint16_t>(mid + 1);
    } else {
      hi = mid;
    }
  }
  return -1;
}

void DawgReader::StepByInitial(
    int32_t node, char initial,
    const std::function<void(int, int32_t)>& cb) const {
  if (node < 0 || static_cast<uint32_t>(node) >= node_count_ ||
      table_ == nullptr) {
    return;
  }
  const uint8_t* n = nodes() + static_cast<size_t>(node) * kDawgNodeSize;
  const uint32_t first_child = ReadU32(n);
  const uint16_t num_children = ReadU16(n + 4);
  for (const int syllable_id : table_->ByInitial(initial)) {
    uint16_t lo = 0;
    uint16_t hi = num_children;
    while (lo < hi) {
      const uint16_t mid = static_cast<uint16_t>((lo + hi) / 2);
      const uint8_t* c =
          children() + size_t{first_child + mid} * kDawgChildSize;
      const int id = ReadU16(c);
      if (id == syllable_id) {
        cb(syllable_id, static_cast<int32_t>(ReadU32(c + 2)));
        break;
      }
      if (id < syllable_id) {
        lo = static_cast<uint16_t>(mid + 1);
      } else {
        hi = mid;
      }
    }
  }
}

uint16_t DawgReader::NumEntries(int32_t node) const {
  if (node < 0 || static_cast<uint32_t>(node) >= node_count_) return 0;
  const uint8_t* n = nodes() + static_cast<size_t>(node) * kDawgNodeSize;
  return ReadU16(n + 10);
}

void DawgReader::ForEachNodeEntry(
    int32_t node, const std::function<void(uint32_t)>& cb) const {
  if (node < 0 || static_cast<uint32_t>(node) >= node_count_) return;
  const uint8_t* n = nodes() + static_cast<size_t>(node) * kDawgNodeSize;
  const uint32_t first_entry = ReadU32(n + 6);
  const uint16_t num_entries = ReadU16(n + 10);
  for (uint16_t k = 0; k < num_entries; ++k) {
    cb(first_entry + k);
  }
}

ErrorCode DawgReader::ForEachMatch(
    const std::vector<SyllableEdge>& path,
    const std::function<void(uint32_t)>& cb) const {
  if (!valid()) {
    return ErrorCode::kBadFormat;
  }
  struct Frame {
    int32_t node;
    size_t depth;
  };
  std::vector<Frame> stack{{RootNode(), 0}};
  while (!stack.empty()) {
    const Frame f = stack.back();
    stack.pop_back();
    if (f.depth == path.size()) {
      ForEachNodeEntry(f.node, cb);
      continue;
    }
    const SyllableEdge& step = path[f.depth];
    if (step.abbreviated) {
      StepByInitial(f.node, step.initial,
                    [&](int /*id*/, int32_t child) {
                      stack.push_back({child, f.depth + 1});
                    });
    } else {
      const int32_t child = StepExact(f.node, step.syllable_id);
      if (child >= 0) {
        stack.push_back({child, f.depth + 1});
      }
    }
  }
  return ErrorCode::kOk;
}

}  // namespace dawg
