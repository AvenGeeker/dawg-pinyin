// SPDX-License-Identifier: Apache-2.0
// Tests for the extern "C" facade consumed by the JNI / Swift / NAPI
// bridges. The engine behavior itself is covered by engine_test.cc; here we
// pin the C ABI contract: handle lifecycle, result accessors, error mapping.
#include <gtest/gtest.h>

#include "dawg/dawg_capi.h"
#include "dawg/error.h"

namespace {

constexpr char kDictPath[] = DAWG_TESTDATA_DIR "/dict/tiny.dawg";
constexpr char kLmPath[] = DAWG_TESTDATA_DIR "/lm/tiny.clm";

class DawgCapiTest : public ::testing::Test {
 protected:
  void SetUp() override {
    DawgOptions options{};
    options.dawg_path = kDictPath;
    options.lm_path = kLmPath;
    int err = -1;
    engine_ = dawg_engine_create(&options, &err);
    ASSERT_NE(nullptr, engine_);
    ASSERT_EQ(static_cast<int>(dawg::ErrorCode::kOk), err);
  }

  void TearDown() override {
    if (engine_ != nullptr) {
      dawg_engine_destroy(engine_);
      engine_ = nullptr;
    }
  }

  DawgEngine* engine_ = nullptr;
};

TEST_F(DawgCapiTest, SearchReturnsCandidateCount) {
  int err = -1;
  const int count = dawg_engine_search(engine_, "zhongguoren", "", &err);
  ASSERT_GT(count, 0);
  EXPECT_EQ(static_cast<int>(dawg::ErrorCode::kOk), err);
  EXPECT_EQ(count, dawg_candidate_count(engine_));
}

TEST_F(DawgCapiTest, CandidateGetFillsAllOutputs) {
  ASSERT_GT(dawg_engine_search(engine_, "zhongguo", "", nullptr), 0);
  const char* text = nullptr;
  const char* pinyin = nullptr;
  double score = 0.0;
  int kind = -1;
  ASSERT_EQ(1, dawg_candidate_get(engine_, 0, &text, &pinyin, &score, &kind));
  ASSERT_NE(nullptr, text);
  EXPECT_STREQ("\xe4\xb8\xad\xe5\x9b\xbd", text);  // 中国
  ASSERT_NE(nullptr, pinyin);
  EXPECT_STREQ("zhong guo", pinyin);
  EXPECT_GE(kind, 0);
  EXPECT_LE(kind, 2);
}

TEST_F(DawgCapiTest, CandidateGetOutOfRangeReturnsZero) {
  const int count = dawg_engine_search(engine_, "zhong", "", nullptr);
  ASSERT_GT(count, 0);
  EXPECT_EQ(0, dawg_candidate_get(engine_, count, nullptr, nullptr, nullptr,
                                  nullptr));
  EXPECT_EQ(0, dawg_candidate_get(engine_, -1, nullptr, nullptr, nullptr,
                                  nullptr));
  EXPECT_EQ(0, dawg_candidate_get(nullptr, 0, nullptr, nullptr, nullptr,
                                  nullptr));
}

TEST_F(DawgCapiTest, EmptyInputYieldsZeroCandidates) {
  int err = -1;
  EXPECT_EQ(0, dawg_engine_search(engine_, "", "", &err));
  EXPECT_EQ(static_cast<int>(dawg::ErrorCode::kOk), err);
  EXPECT_EQ(0, dawg_candidate_count(engine_));
}

TEST_F(DawgCapiTest, SearchRejectsNullEngineAndInput) {
  int err = -1;
  EXPECT_EQ(-1, dawg_engine_search(nullptr, "zhong", "", &err));
  EXPECT_EQ(static_cast<int>(dawg::ErrorCode::kInvalidInput), err);
  err = -1;
  EXPECT_EQ(-1, dawg_engine_search(engine_, nullptr, "", &err));
  EXPECT_EQ(static_cast<int>(dawg::ErrorCode::kInvalidInput), err);
}

TEST_F(DawgCapiTest, CreateFailsOnMissingDict) {
  DawgOptions options{};
  options.dawg_path = "/nonexistent/missing.dawg";
  options.lm_path = nullptr;
  int err = -1;
  DawgEngine* engine = dawg_engine_create(&options, &err);
  EXPECT_EQ(nullptr, engine);
  EXPECT_EQ(static_cast<int>(dawg::ErrorCode::kFileNotFound), err);
}

TEST_F(DawgCapiTest, CreateRejectsNullOptions) {
  int err = -1;
  EXPECT_EQ(nullptr, dawg_engine_create(nullptr, &err));
  EXPECT_EQ(static_cast<int>(dawg::ErrorCode::kInvalidInput), err);
}

}  // namespace
