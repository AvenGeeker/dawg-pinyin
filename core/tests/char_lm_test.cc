// SPDX-License-Identifier: Apache-2.0
#include <cstdint>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "lm/char_lm.h"
#include "lm/char_lm_format.h"

namespace dawg {
namespace {

// Hand-assembles a tiny order-3 .clm image so quantization and backoff
// behaviour can be checked against known values.
class ClmImage {
 public:
  ClmImage() {
    U32(static_cast<uint32_t>(kClmMagic[0]) |
        (static_cast<uint32_t>(kClmMagic[1]) << 8) |
        (static_cast<uint32_t>(kClmMagic[2]) << 16) |
        (static_cast<uint32_t>(kClmMagic[3]) << 24));
    U32(kClmVersion);
    U32(3);  // order
    U32(5);  // vocab: unk, <s>, 中, 国, 人
    U32(7);  // nodes
    // Vocab code points.
    U32(0);
    U32(static_cast<uint32_t>(kBosChar));
    U32(0x4E2D);  // 中
    U32(0x56FD);  // 国
    U32(0x4EBA);  // 人
    // Nodes: {char_id, prob_q, backoff_q, reserved, first_child,
    //         num_children}. Children are contiguous in the node array.
    // 0 root <s>: children unk->1, <s>->2, 中->3
    Node(1, 0.0, 0.0, 1, 3);
    // 1 unk node (leaf)
    Node(0, -2.0, 0.0, 0, 0);
    // 2 <s>-context node: child 中->4
    Node(1, 0.0, -0.2, 4, 1);
    // 3 中-context node (vocab id 2): child 国->5
    Node(2, 0.0, -0.1, 5, 1);
    // 4 中 after <s> (leaf)
    Node(2, -0.4, 0.0, 0, 0);
    // 5 国 node (vocab id 3): child 人->6
    Node(3, -0.2, 0.0, 6, 1);
    // 6 人 leaf (vocab id 4)
    Node(4, -0.25, 0.0, 0, 0);
  }

  const std::vector<uint8_t>& bytes() const { return buf_; }

 private:
  void U32(uint32_t v) {
    for (int shift = 0; shift < 32; shift += 8) {
      buf_.push_back(static_cast<uint8_t>((v >> shift) & 0xFF));
    }
  }
  void Node(uint32_t char_id, double prob, double backoff, uint32_t first_child,
            uint32_t num_children) {
    U32(char_id);
    buf_.push_back(QuantizeProb(prob));
    buf_.push_back(QuantizeBackoff(backoff));
    buf_.push_back(0);
    buf_.push_back(0);
    U32(first_child);
    U32(num_children);
  }
  std::vector<uint8_t> buf_;
};

class CharLmTest : public ::testing::Test {
 protected:
  void SetUp() override {
    image_ = ClmImage().bytes();
    ASSERT_EQ(ErrorCode::kOk, lm_.LoadFromBuffer(image_.data(), image_.size()));
    ASSERT_EQ(3, lm_.order());
  }

  CharLm lm_;
  std::vector<uint8_t> image_;
  // Two quantization steps of slack (prob + backoff).
  static constexpr double kTol = 0.09;
};

TEST_F(CharLmTest, MapsVocabulary) {
  EXPECT_EQ(2u, lm_.MapChar(U'中'));
  EXPECT_EQ(3u, lm_.MapChar(U'国'));
  EXPECT_EQ(kUnkCharId, lm_.MapChar(U'龟'));
  EXPECT_EQ(kBosCharId, lm_.MapChar(kBosChar));
}

TEST_F(CharLmTest, ExactNgramHit) {
  EXPECT_NEAR(-0.2, lm_.ScoreWord({U'国'}, {U'中'}), kTol);
}

TEST_F(CharLmTest, BacksOffWhenNgramMissing) {
  // P(人|中) missing -> backoff(中) + P(人|root) = -0.1 + P(unk|root).
  EXPECT_NEAR(-2.1, lm_.ScoreWord({U'人'}, {U'中'}), kTol);
}

TEST_F(CharLmTest, EmptyContextUsesBosPadding) {
  // P(中|<s>) exists in the trie.
  EXPECT_NEAR(-0.4, lm_.ScoreWord({U'中'}, {}), kTol);
  // P(人|<s>) missing -> backoff(<s>) + P(unk|root).
  EXPECT_NEAR(-2.2, lm_.ScoreWord({U'人'}, {}), kTol);
}

TEST_F(CharLmTest, UnknownWordCharsUseUnk) {
  EXPECT_NEAR(-2.1, lm_.ScoreWord({U'龟'}, {U'中'}), kTol);
}

TEST_F(CharLmTest, ScoresAccumulateOverWord) {
  // 国人: P(国|中) + P(人|国)
  const double expected = -0.2 + -0.25;
  EXPECT_NEAR(expected, lm_.ScoreWord({U'国', U'人'}, {U'中'}), kTol * 2);
}

TEST_F(CharLmTest, CorruptImagesRejected) {
  CharLm bad_magic;
  std::vector<uint8_t> corrupted = image_;
  corrupted[0] = 'X';
  EXPECT_EQ(ErrorCode::kBadFormat,
            bad_magic.LoadFromBuffer(corrupted.data(), corrupted.size()));

  CharLm truncated;
  EXPECT_EQ(ErrorCode::kBadFormat,
            truncated.LoadFromBuffer(image_.data(), image_.size() - 5));

  CharLm missing;
  EXPECT_EQ(ErrorCode::kFileNotFound, missing.Load("/nonexistent/x.clm"));
}

TEST_F(CharLmTest, QuantizationHelpersAreSymmetric) {
  for (const double p : {-9.5, -4.2, -1.0, -0.05}) {
    EXPECT_NEAR(p, DequantizeProb(QuantizeProb(p)), 1.0 / kProbQuantScale / 2 + 1e-9);
  }
  for (const double b : {-1.9, -0.5, 0.0, 1.7}) {
    EXPECT_NEAR(b, DequantizeBackoff(QuantizeBackoff(b)),
                1.0 / kBackoffQuantScale / 2 + 1e-9);
  }
  EXPECT_EQ(0, QuantizeProb(-50.0));
  EXPECT_EQ(255, QuantizeProb(0.5));
}

}  // namespace
}  // namespace dawg
