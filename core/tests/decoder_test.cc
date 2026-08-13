// SPDX-License-Identifier: Apache-2.0
#include <cstdint>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "dawg/dawg_builder.h"
#include "dawg/dawg_reader.h"
#include "pinyin/decoder.h"
#include "pinyin/syllabifier.h"

namespace dawg {
namespace {

// Hermetic decoder tests: in-memory DAWG, no language model. LM-driven
// ordering is covered by engine_test with the generated testdata assets.
class DecoderTest : public ::testing::Test {
 protected:
  void SetUp() override {
    ASSERT_EQ(ErrorCode::kOk, builder_.AddEntry("中国人", "zhong guo ren", 1000));
    ASSERT_EQ(ErrorCode::kOk, builder_.AddEntry("中国", "zhong guo", 900));
    ASSERT_EQ(ErrorCode::kOk, builder_.AddEntry("国人", "guo ren", 200));
    ASSERT_EQ(ErrorCode::kOk, builder_.AddEntry("中", "zhong", 100));
    ASSERT_EQ(ErrorCode::kOk, builder_.AddEntry("国", "guo", 90));
    ASSERT_EQ(ErrorCode::kOk, builder_.AddEntry("人", "ren", 80));
    ASSERT_EQ(ErrorCode::kOk, builder_.AddEntry("实力", "shi li", 500));
    ASSERT_EQ(ErrorCode::kOk, builder_.AddEntry("事例", "shi li", 480));
    ASSERT_EQ(ErrorCode::kOk, builder_.AddEntry("你", "ni", 300));
    ASSERT_EQ(ErrorCode::kOk, builder_.AddEntry("好", "hao", 300));
    ASSERT_EQ(ErrorCode::kOk, builder_.Build());
    image_ = builder_.Serialize();
    ASSERT_EQ(ErrorCode::kOk,
              reader_.LoadFromBuffer(image_.data(), image_.size()));
  }

  std::vector<Candidate> Decode(const std::string& input) {
    SyllableGraph graph;
    EXPECT_EQ(ErrorCode::kOk, syllabifier_.Segment(input, &graph));
    std::vector<Candidate> out;
    DecodeOptions options;
    EXPECT_EQ(ErrorCode::kOk,
              decoder_.Decode(graph, {}, options, &out));
    return out;
  }

  DawgBuilder builder_;
  std::vector<uint8_t> image_;
  DawgReader reader_;
  Syllabifier syllabifier_;
  Decoder decoder_{reader_, nullptr};
};

TEST_F(DecoderTest, BestPathPrefersHeavyWord) {
  const std::vector<Candidate> candidates = Decode("zhongguoren");
  ASSERT_FALSE(candidates.empty());
  EXPECT_EQ("中国人", candidates.front().text);
  EXPECT_EQ(CandidateKind::kPhrase, candidates.front().kind);
  EXPECT_EQ("zhong guo ren", candidates.front().pinyin);
}

TEST_F(DecoderTest, ProducesSentenceCandidate) {
  const std::vector<Candidate> candidates = Decode("nihao");
  ASSERT_FALSE(candidates.empty());
  EXPECT_EQ("你好", candidates.front().text);
  EXPECT_EQ(CandidateKind::kSentence, candidates.front().kind);
  EXPECT_EQ("ni hao", candidates.front().pinyin);
}

TEST_F(DecoderTest, OrdersHomophonesByWeightWithoutLm) {
  const std::vector<Candidate> candidates = Decode("shili");
  ASSERT_GE(candidates.size(), 2u);
  EXPECT_EQ("实力", candidates[0].text);
  EXPECT_EQ("事例", candidates[1].text);
}

TEST_F(DecoderTest, SingleCharCandidateIsCharKind) {
  const std::vector<Candidate> candidates = Decode("ni");
  ASSERT_FALSE(candidates.empty());
  EXPECT_EQ("你", candidates.front().text);
  EXPECT_EQ(CandidateKind::kChar, candidates.front().kind);
}

TEST_F(DecoderTest, DecodesAbbreviatedInput) {
  const std::vector<Candidate> candidates = Decode("zg");
  ASSERT_FALSE(candidates.empty());
  EXPECT_EQ("中国", candidates.front().text);
}

TEST_F(DecoderTest, EmptyWhenLexiconHasNoCoverage) {
  const std::vector<Candidate> candidates = Decode("ka");
  EXPECT_TRUE(candidates.empty());
}

TEST_F(DecoderTest, ScoresDecreaseWithinResultList) {
  const std::vector<Candidate> candidates = Decode("zhongguoren");
  for (size_t i = 1; i < candidates.size(); ++i) {
    EXPECT_LE(candidates[i].score, candidates[i - 1].score);
  }
}

}  // namespace
}  // namespace dawg
