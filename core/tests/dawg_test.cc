// SPDX-License-Identifier: Apache-2.0
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "dawg/dawg_builder.h"
#include "dawg/dawg_reader.h"
#include "util/mmap_file.h"

namespace dawg {
namespace {

class DawgTest : public ::testing::Test {
 protected:
  void SetUp() override {
    ASSERT_EQ(ErrorCode::kOk, builder_.AddEntry("中国人", "zhong guo ren", 1000));
    ASSERT_EQ(ErrorCode::kOk, builder_.AddEntry("中国", "zhong guo", 500));
    ASSERT_EQ(ErrorCode::kOk, builder_.AddEntry("美国", "mei guo", 300));
    ASSERT_EQ(ErrorCode::kOk, builder_.AddEntry("中", "zhong", 100));
    ASSERT_EQ(ErrorCode::kOk, builder_.AddEntry("国", "guo", 90));
    ASSERT_EQ(ErrorCode::kOk, builder_.Build());
    image_ = builder_.Serialize();
    ASSERT_FALSE(image_.empty());
    ASSERT_EQ(ErrorCode::kOk, reader_.LoadFromBuffer(image_.data(), image_.size()));
  }

  // Walks the automaton and returns the node reached, or -1.
  int32_t Walk(const std::vector<std::string>& spellings) {
    const SyllableTable& table = SyllableTable::Instance();
    int32_t node = reader_.RootNode();
    for (const std::string& s : spellings) {
      node = reader_.StepExact(node, table.Find(s));
      if (node < 0) return -1;
    }
    return node;
  }

  std::vector<uint32_t> MatchPath(const std::vector<SyllableEdge>& path) {
    std::vector<uint32_t> entries;
    EXPECT_EQ(ErrorCode::kOk, reader_.ForEachMatch(path, [&](uint32_t e) {
      entries.push_back(e);
    }));
    return entries;
  }

  DawgBuilder builder_;
  std::vector<uint8_t> image_;
  DawgReader reader_;
};

TEST_F(DawgTest, RoundTripsEntries) {
  EXPECT_EQ(5u, reader_.entry_count());
  EXPECT_EQ(1000, reader_.max_weight());
  bool found = false;
  for (uint32_t i = 0; i < reader_.entry_count(); ++i) {
    if (reader_.EntryText(i) == "中国人") {
      EXPECT_EQ(1000, reader_.EntryWeight(i));
      EXPECT_EQ(3, reader_.EntrySyllableCount(i));
      found = true;
    }
  }
  EXPECT_TRUE(found);
}

TEST_F(DawgTest, ExactPathLookup) {
  const int32_t node = Walk({"zhong", "guo", "ren"});
  ASSERT_GE(node, 0);
  std::vector<std::string> texts;
  reader_.ForEachNodeEntry(node, [&](uint32_t e) {
    texts.emplace_back(reader_.EntryText(e));
  });
  ASSERT_EQ(1u, texts.size());
  EXPECT_EQ("中国人", texts[0]);
  // Prefix node carries the shorter word.
  const int32_t prefix = Walk({"zhong", "guo"});
  ASSERT_GE(prefix, 0);
  std::vector<std::string> prefix_texts;
  reader_.ForEachNodeEntry(prefix, [&](uint32_t e) {
    prefix_texts.emplace_back(reader_.EntryText(e));
  });
  ASSERT_EQ(1u, prefix_texts.size());
  EXPECT_EQ("中国", prefix_texts[0]);
}

TEST_F(DawgTest, MatchFullAndAbbreviatedPaths) {
  const SyllableTable& table = SyllableTable::Instance();
  // Full path: zhong guo -> 中国
  std::vector<SyllableEdge> full(2);
  full[0].syllable_id = table.Find("zhong");
  full[1].syllable_id = table.Find("guo");
  std::vector<uint32_t> entries = MatchPath(full);
  ASSERT_EQ(1u, entries.size());
  EXPECT_EQ("中国", reader_.EntryText(entries[0]));

  // Abbreviated path: z g -> 中国 (zhong guo)
  std::vector<SyllableEdge> abbrev(2);
  abbrev[0].abbreviated = true;
  abbrev[0].initial = 'z';
  abbrev[1].abbreviated = true;
  abbrev[1].initial = 'g';
  entries = MatchPath(abbrev);
  ASSERT_EQ(1u, entries.size());
  EXPECT_EQ("中国", reader_.EntryText(entries[0]));
}

TEST_F(DawgTest, EntriesSortedByWeightWithinNode) {
  DawgBuilder builder;
  ASSERT_EQ(ErrorCode::kOk, builder.AddEntry("低", "di", 5));
  ASSERT_EQ(ErrorCode::kOk, builder.AddEntry("地", "di", 500));
  ASSERT_EQ(ErrorCode::kOk, builder.AddEntry("的", "di", 9000));
  ASSERT_EQ(ErrorCode::kOk, builder.Build());
  const std::vector<uint8_t> image = builder.Serialize();
  DawgReader reader;
  ASSERT_EQ(ErrorCode::kOk, reader.LoadFromBuffer(image.data(), image.size()));
  const int32_t node = [&] {
    const SyllableTable& table = SyllableTable::Instance();
    return reader.StepExact(reader.RootNode(), table.Find("di"));
  }();
  ASSERT_GE(node, 0);
  std::vector<int32_t> weights;
  reader.ForEachNodeEntry(node, [&](uint32_t e) {
    weights.push_back(reader.EntryWeight(e));
  });
  ASSERT_EQ(3u, weights.size());
  EXPECT_GE(weights[0], weights[1]);
  EXPECT_GE(weights[1], weights[2]);
}

TEST_F(DawgTest, MissingFileReportsFileNotFound) {
  DawgReader reader;
  EXPECT_EQ(ErrorCode::kFileNotFound,
            reader.Load("/nonexistent/path/no.dawg"));
}

TEST_F(DawgTest, CorruptImagesRejected) {
  DawgReader reader;
  std::vector<uint8_t> bad_magic = image_;
  bad_magic[0] = 'X';
  EXPECT_EQ(ErrorCode::kBadFormat,
            reader.LoadFromBuffer(bad_magic.data(), bad_magic.size()));

  DawgReader truncated;
  EXPECT_EQ(ErrorCode::kBadFormat,
            truncated.LoadFromBuffer(image_.data(), image_.size() - 3));

  DawgReader tiny;
  const uint8_t few[4] = {0, 1, 2, 3};
  EXPECT_EQ(ErrorCode::kBadFormat, tiny.LoadFromBuffer(few, sizeof(few)));
}

TEST_F(DawgTest, BuilderValidatesEntries) {
  DawgBuilder builder;
  EXPECT_EQ(ErrorCode::kInvalidInput, builder.AddEntry("中", "zhon", 1));
  EXPECT_EQ(ErrorCode::kInvalidInput, builder.AddEntry("中中", "zhong", 1));
  EXPECT_EQ(ErrorCode::kInvalidInput, builder.AddEntry("", "zhong", 1));
  EXPECT_EQ(ErrorCode::kInvalidInput, builder.AddEntry("中", "", 1));
}

TEST_F(DawgTest, DuplicateEntriesKeepMaxWeight) {
  DawgBuilder builder;
  ASSERT_EQ(ErrorCode::kOk, builder.AddEntry("中", "zhong", 10));
  ASSERT_EQ(ErrorCode::kOk, builder.AddEntry("中", "zhong", 77));
  ASSERT_EQ(ErrorCode::kOk, builder.Build());
  EXPECT_EQ(1u, builder.entry_count());
  const std::vector<uint8_t> image = builder.Serialize();
  DawgReader reader;
  ASSERT_EQ(ErrorCode::kOk, reader.LoadFromBuffer(image.data(), image.size()));
  ASSERT_EQ(1u, reader.entry_count());
  EXPECT_EQ(77, reader.EntryWeight(0));
}

TEST_F(DawgTest, FileLoadRoundTrip) {
  const std::string path = std::string(::testing::TempDir()) + "rt.dawg";
  ASSERT_EQ(ErrorCode::kOk, builder_.WriteFile(path));
  DawgReader reader;
  ASSERT_EQ(ErrorCode::kOk, reader.Load(path));
  EXPECT_EQ(reader_.entry_count(), reader.entry_count());
  std::remove(path.c_str());
}

// Regression: the max-weight scan used to reset per node, so max_weight()
// returned the last node's first entry weight instead of the global max.
// Put the large weight on the syllable whose subtree is visited first in
// the DFS node order and the small one on the last visited subtree.
TEST(DawgMaxWeightTest, GlobalMaxAcrossNodes) {
  const SyllableTable& table = SyllableTable::Instance();
  const int id_a = table.Find("a");
  const int id_z = table.Find("zuo");
  ASSERT_GE(id_a, 0);
  ASSERT_GE(id_z, 0);
  const std::string_view first = table.text(std::min(id_a, id_z));
  const std::string_view last = table.text(std::max(id_a, id_z));

  DawgBuilder builder;
  ASSERT_EQ(ErrorCode::kOk, builder.AddEntry("阿", first, 1000));
  ASSERT_EQ(ErrorCode::kOk, builder.AddEntry("作", last, 5));
  ASSERT_EQ(ErrorCode::kOk, builder.Build());
  const std::vector<uint8_t> image = builder.Serialize();

  DawgReader reader;
  ASSERT_EQ(ErrorCode::kOk,
            reader.LoadFromBuffer(image.data(), image.size()));
  EXPECT_EQ(1000, reader.max_weight());
}

}  // namespace
}  // namespace dawg
