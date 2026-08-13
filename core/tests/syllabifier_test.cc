// SPDX-License-Identifier: Apache-2.0
#include <string>

#include <gtest/gtest.h>

#include "pinyin/syllabifier.h"
#include "pinyin/syllable_table.h"

namespace dawg {
namespace {

class SyllabifierTest : public ::testing::Test {
 protected:
  Syllabifier syllabifier;

  static const SyllableEdge* FindFullEdge(const SyllableGraph& graph,
                                          int start, int end) {
    for (const SyllableEdge& e : graph.edges_from[start]) {
      if (!e.abbreviated && e.end == end) return &e;
    }
    return nullptr;
  }

  static const SyllableEdge* FindAbbrevEdge(const SyllableGraph& graph,
                                            int start, char initial) {
    for (const SyllableEdge& e : graph.edges_from[start]) {
      if (e.abbreviated && e.initial == initial) return &e;
    }
    return nullptr;
  }
};

TEST_F(SyllabifierTest, SegmentsFullPinyin) {
  SyllableGraph graph;
  ASSERT_EQ(ErrorCode::kOk, syllabifier.Segment("zhongguo", &graph));
  EXPECT_EQ(8, graph.length);
  const SyllableEdge* zhong = FindFullEdge(graph, 0, 5);
  ASSERT_NE(nullptr, zhong);
  EXPECT_EQ("zhong", SyllableTable::Instance().text(zhong->syllable_id));
  const SyllableEdge* guo = FindFullEdge(graph, 5, 8);
  ASSERT_NE(nullptr, guo);
  EXPECT_EQ("guo", SyllableTable::Instance().text(guo->syllable_id));
  EXPECT_TRUE(graph.CoversFullInput());
}

TEST_F(SyllabifierTest, KeepsAmbiguousSplits) {
  // "xian" is both one syllable and "xi" + "an".
  SyllableGraph graph;
  ASSERT_EQ(ErrorCode::kOk, syllabifier.Segment("xian", &graph));
  EXPECT_NE(nullptr, FindFullEdge(graph, 0, 4));
  EXPECT_NE(nullptr, FindFullEdge(graph, 0, 2));
  EXPECT_NE(nullptr, FindFullEdge(graph, 2, 4));
  EXPECT_TRUE(graph.CoversFullInput());
}

TEST_F(SyllabifierTest, AddsAbbreviatedEdges) {
  SyllableGraph graph;
  ASSERT_EQ(ErrorCode::kOk, syllabifier.Segment("zg", &graph));
  const SyllableEdge* z = FindAbbrevEdge(graph, 0, 'z');
  ASSERT_NE(nullptr, z);
  EXPECT_EQ(1, z->end);
  EXPECT_NE(nullptr, FindAbbrevEdge(graph, 1, 'g'));
  EXPECT_TRUE(graph.CoversFullInput());
}

TEST_F(SyllabifierTest, RejectsNonLowercaseLetters) {
  SyllableGraph graph;
  EXPECT_EQ(ErrorCode::kInvalidInput, syllabifier.Segment("zhong1", &graph));
  EXPECT_EQ(ErrorCode::kInvalidInput, syllabifier.Segment("Zhong", &graph));
  EXPECT_EQ(ErrorCode::kInvalidInput, syllabifier.Segment("xi'an", &graph));
}

TEST_F(SyllabifierTest, RejectsOverlongInput) {
  SyllableGraph graph;
  const std::string too_long(Syllabifier::kMaxInputLength + 1, 'a');
  EXPECT_EQ(ErrorCode::kInvalidInput, syllabifier.Segment(too_long, &graph));
}

TEST_F(SyllabifierTest, EmptyInputYieldsEmptyGraph) {
  SyllableGraph graph;
  ASSERT_EQ(ErrorCode::kOk, syllabifier.Segment("", &graph));
  EXPECT_EQ(0, graph.length);
  EXPECT_FALSE(graph.CoversFullInput());
}

TEST_F(SyllabifierTest, DetectsUnsegmentableTail) {
  // 'v' never starts a syllable and cannot finish one after "zhong", so the
  // tail cannot be covered.
  SyllableGraph graph;
  ASSERT_EQ(ErrorCode::kOk, syllabifier.Segment("zhongv", &graph));
  EXPECT_TRUE(graph.reachable[5]);
  EXPECT_FALSE(graph.CoversFullInput());
}

TEST_F(SyllabifierTest, SyllableTableLookups) {
  const SyllableTable& table = SyllableTable::Instance();
  EXPECT_GT(table.size(), 350);
  EXPECT_GE(0, table.Find("zhon"));
  EXPECT_EQ(-1, table.Find(""));
  EXPECT_EQ(-1, table.Find("1abc"));
  EXPECT_TRUE(table.IsValidInitial('z'));
  EXPECT_FALSE(table.IsValidInitial('u'));
  EXPECT_FALSE(table.ByInitial('u').empty() && table.IsValidInitial('u'));
  EXPECT_TRUE(table.ByInitial('a').size() >= 5);  // a ai an ang ao
}

}  // namespace
}  // namespace dawg
