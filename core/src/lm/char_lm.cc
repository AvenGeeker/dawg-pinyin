// SPDX-License-Identifier: Apache-2.0
#include "lm/char_lm.h"

#include <algorithm>
#include <cstring>

#include "lm/char_lm_format.h"
#include "util/read_int.h"

namespace dawg {

using util::ReadU32;

ErrorCode CharLm::Load(const std::string& path) {
  ErrorCode err = file_.Open(path);
  if (err != ErrorCode::kOk) {
    return err;
  }
  return LoadFromBuffer(file_.data(), file_.size());
}

ErrorCode CharLm::LoadFromBuffer(const uint8_t* data, size_t size) {
  data_ = nullptr;
  size_ = 0;
  order_ = vocab_size_ = node_count_ = 0;
  char_to_id_.clear();
  if (size < kClmHeaderSize) {
    return ErrorCode::kBadFormat;
  }
  if (std::memcmp(data, kClmMagic, sizeof(kClmMagic)) != 0) {
    return ErrorCode::kBadFormat;
  }
  const uint32_t version = ReadU32(data + 4);
  order_ = ReadU32(data + 8);
  vocab_size_ = ReadU32(data + 12);
  node_count_ = ReadU32(data + 16);
  if (version != kClmVersion || order_ < 2 || order_ > 8 || vocab_size_ < 2 ||
      node_count_ < 1) {
    return ErrorCode::kBadFormat;
  }
  vocab_offset_ = kClmHeaderSize;
  nodes_offset_ = vocab_offset_ + size_t{vocab_size_} * sizeof(uint32_t);
  const size_t total = nodes_offset_ + size_t{node_count_} * kClmNodeSize;
  if (total != size) {
    return ErrorCode::kBadFormat;
  }
  // Validate child ranges and ordering before publishing the model.
  for (uint32_t i = 0; i < node_count_; ++i) {
    const uint8_t* n = data + nodes_offset_ + size_t{i} * kClmNodeSize;
    const uint32_t first_child = ReadU32(n + 8);
    const uint32_t num_children = ReadU32(n + 12);
    if (size_t{first_child} + num_children > node_count_) {
      return ErrorCode::kBadFormat;
    }
    uint32_t last = 0;
    bool has_last = false;
    for (uint32_t k = 0; k < num_children; ++k) {
      const uint8_t* c =
          data + nodes_offset_ + size_t{first_child + k} * kClmNodeSize;
      const uint32_t id = ReadU32(c);
      if (id >= vocab_size_ || (has_last && id <= last)) {
        return ErrorCode::kBadFormat;
      }
      last = id;
      has_last = true;
    }
  }
  data_ = data;
  size_ = size;
  char_to_id_.reserve(vocab_size_);
  for (uint32_t id = 2; id < vocab_size_; ++id) {
    const char32_t c = static_cast<char32_t>(ReadU32(data + vocab_offset_ +
                                                     size_t{id} * 4));
    char_to_id_.emplace(c, id);
  }
  return ErrorCode::kOk;
}

uint32_t CharLm::MapChar(char32_t c) const {
  if (c == kBosChar) {
    return kBosCharId;
  }
  const auto it = char_to_id_.find(c);
  return it == char_to_id_.end() ? kUnkCharId : it->second;
}

const uint8_t* CharLm::NodeAt(uint32_t node) const {
  return data_ + nodes_offset_ + size_t{node} * kClmNodeSize;
}

int32_t CharLm::FindChild(uint32_t node, uint32_t char_id) const {
  const uint8_t* n = NodeAt(node);
  const uint32_t first_child = ReadU32(n + 8);
  const uint32_t num_children = ReadU32(n + 12);
  uint32_t lo = 0;
  uint32_t hi = num_children;
  while (lo < hi) {
    const uint32_t mid = (lo + hi) / 2;
    const uint32_t id = ReadU32(NodeAt(first_child + mid));
    if (id == char_id) {
      return static_cast<int32_t>(first_child + mid);
    }
    if (id < char_id) {
      lo = mid + 1;
    } else {
      hi = mid;
    }
  }
  return -1;
}

int32_t CharLm::DescendPath(const std::vector<uint32_t>& ids,
                            size_t begin) const {
  int32_t node = 0;
  for (size_t i = begin; i < ids.size(); ++i) {
    node = FindChild(static_cast<uint32_t>(node), ids[i]);
    if (node < 0) return -1;
  }
  return node;
}

double CharLm::ScoreRec(const std::vector<uint32_t>& ctx, size_t begin,
                        uint32_t target) const {
  if (begin >= ctx.size()) {
    const int32_t child = FindChild(0, target);
    if (child >= 0) {
      return DequantizeProb(NodeAt(child)[4]);
    }
    if (target != kUnkCharId) {
      const int32_t unk = FindChild(0, kUnkCharId);
      if (unk >= 0) {
        return DequantizeProb(NodeAt(unk)[4]);
      }
    }
    return kProbFloorLog10;
  }
  const int32_t node = DescendPath(ctx, begin);
  if (node < 0) {
    return ScoreRec(ctx, begin + 1, target);
  }
  const int32_t child = FindChild(static_cast<uint32_t>(node), target);
  if (child >= 0) {
    return DequantizeProb(NodeAt(child)[4]);
  }
  return DequantizeBackoff(NodeAt(node)[5]) + ScoreRec(ctx, begin + 1, target);
}

double CharLm::ScoreWord(const std::vector<char32_t>& word,
                         const std::vector<char32_t>& context) const {
  if (!valid()) {
    return 0.0;
  }
  const size_t window = order_ - 1;
  std::vector<uint32_t> ctx;
  if (context.empty()) {
    ctx.assign(window, kBosCharId);
  } else {
    ctx.reserve(context.size());
    for (const char32_t c : context) {
      ctx.push_back(MapChar(c));
    }
    if (ctx.size() > window) {
      ctx.erase(ctx.begin(), ctx.begin() + (ctx.size() - window));
    }
  }
  double total = 0.0;
  for (const char32_t c : word) {
    const uint32_t id = MapChar(c);
    const size_t begin = ctx.size() > window ? ctx.size() - window : 0;
    total += ScoreRec(ctx, begin, id);
    ctx.push_back(id);
  }
  return total;
}

}  // namespace dawg
