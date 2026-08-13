// SPDX-License-Identifier: Apache-2.0
// Integration tests over the generated testdata assets. Regenerate them via
// core/testdata/regenerate.sh when the formats change.
#include <cstdio>
#include <fstream>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "dawg/dawg_capi.h"
#include "dawg/engine.h"

namespace dawg {
namespace {

constexpr char kDictPath[] = DAWG_TESTDATA_DIR "/dict/tiny.dawg";
constexpr char kLmPath[] = DAWG_TESTDATA_DIR "/lm/tiny.clm";

bool FileExists(const char* path) {
  std::ifstream f(path);
  return f.good();
}

class EngineTest : public ::testing::Test {
 protected:
  static void SetUpTestSuite() {
    ASSERT_TRUE(FileExists(kDictPath))
        << "missing " << kDictPath << "; run core/testdata/regenerate.sh";
    ASSERT_TRUE(FileExists(kLmPath))
        << "missing " << kLmPath << "; run core/testdata/regenerate.sh";
  }

  void SetUp() override {
    EngineOptions options;
    options.dawg_path = kDictPath;
    options.lm_path = kLmPath;
    ErrorCode err = ErrorCode::kOk;
    engine_ = CreateEngine(options, &err);
    ASSERT_NE(nullptr, engine_);
    ASSERT_EQ(ErrorCode::kOk, err);
  }

  std::vector<Candidate> Search(const std::string& input,
                                const std::string& context) {
    std::vector<Candidate> out;
    EXPECT_EQ(ErrorCode::kOk, engine_->Search(input, context, &out));
    return out;
  }

  std::unique_ptr<Engine> engine_;
};

TEST_F(EngineTest, ZhongguorenTopOne) {
  const std::vector<Candidate> candidates = Search("zhongguoren", "");
  ASSERT_FALSE(candidates.empty());
  EXPECT_EQ("中国人", candidates.front().text);
  EXPECT_EQ("zhong guo ren", candidates.front().pinyin);
}

TEST_F(EngineTest, ContextReranksHomophones) {
  // The tiny corpus pairs 综合 with 实力 and 具体 with 事例.
  const std::vector<Candidate> with_zonghe = Search("shili", "综合");
  ASSERT_FALSE(with_zonghe.empty());
  EXPECT_EQ("实力", with_zonghe.front().text);
  const std::vector<Candidate> with_juti = Search("shili", "具体");
  ASSERT_FALSE(with_juti.empty());
  EXPECT_EQ("事例", with_juti.front().text);
}

TEST_F(EngineTest, AbbreviatedInputWorks) {
  const std::vector<Candidate> candidates = Search("zgr", "");
  ASSERT_FALSE(candidates.empty());
  EXPECT_EQ("中国人", candidates.front().text);
}

TEST_F(EngineTest, EmptyInputYieldsNoCandidates) {
  const std::vector<Candidate> candidates = Search("", "");
  EXPECT_TRUE(candidates.empty());
}

TEST_F(EngineTest, RejectsInvalidInput) {
  std::vector<Candidate> out;
  EXPECT_EQ(ErrorCode::kInvalidInput, engine_->Search("abc1", "", &out));
  EXPECT_EQ(ErrorCode::kInvalidInput, engine_->Search("ABC", "", &out));
  EXPECT_EQ(ErrorCode::kInvalidInput, engine_->Search("xi'an", "", &out));
  EXPECT_EQ(ErrorCode::kInvalidInput,
            engine_->Search(std::string(65, 'a'), "", &out));
  EXPECT_EQ(ErrorCode::kInvalidInput, engine_->Search("nihao", "", nullptr));
}

TEST_F(EngineTest, TrimsUnsegmentableTail) {
  // 'v' never starts a syllable; the engine falls back to the "zhongguo"
  // prefix and still returns candidates.
  const std::vector<Candidate> candidates = Search("zhongguov", "");
  ASSERT_FALSE(candidates.empty());
  EXPECT_EQ("中国", candidates.front().text);
}

TEST_F(EngineTest, WorksWithoutLanguageModel) {
  EngineOptions options;
  options.dawg_path = kDictPath;
  ErrorCode err = ErrorCode::kOk;
  auto engine = CreateEngine(options, &err);
  ASSERT_NE(nullptr, engine);
  std::vector<Candidate> out;
  ASSERT_EQ(ErrorCode::kOk, engine->Search("zhongguoren", "", &out));
  ASSERT_FALSE(out.empty());
  EXPECT_EQ("中国人", out.front().text);
}

TEST(CreateEngineTest, ReportsMissingAssets) {
  ErrorCode err = ErrorCode::kOk;
  EngineOptions options;
  options.dawg_path = "/nonexistent/no.dawg";
  EXPECT_EQ(nullptr, CreateEngine(options, &err));
  EXPECT_EQ(ErrorCode::kFileNotFound, err);

  options.dawg_path = kDictPath;
  options.lm_path = "/nonexistent/no.clm";
  EXPECT_EQ(nullptr, CreateEngine(options, &err));
  EXPECT_EQ(ErrorCode::kFileNotFound, err);

  options.dawg_path = "";
  options.lm_path = "";
  EXPECT_EQ(nullptr, CreateEngine(options, &err));
  EXPECT_EQ(ErrorCode::kInvalidInput, err);
}

TEST(CreateEngineTest, ClampsOptions) {
  EngineOptions options;
  options.dawg_path = kDictPath;
  options.max_candidates = 100000;
  options.beam_width = -5;
  options.context_chars = -1;
  ErrorCode err = ErrorCode::kOk;
  auto engine = CreateEngine(options, &err);
  ASSERT_NE(nullptr, engine);
  std::vector<Candidate> out;
  ASSERT_EQ(ErrorCode::kOk, engine->Search("zhongguoren", "", &out));
  EXPECT_LE(static_cast<int>(out.size()), 128);
}

TEST(CApiTest, SmokeRoundTrip) {
  DawgOptions options{};
  options.dawg_path = kDictPath;
  options.lm_path = kLmPath;
  int err = 0;
  DawgEngine* handle = dawg_engine_create(&options, &err);
  ASSERT_NE(nullptr, handle);
  ASSERT_EQ(0, err);

  const int count = dawg_engine_search(handle, "zhongguoren", "", &err);
  ASSERT_GT(count, 0);
  ASSERT_EQ(0, err);
  EXPECT_EQ(count, dawg_candidate_count(handle));

  const char* text = nullptr;
  const char* pinyin = nullptr;
  double score = 0.0;
  int kind = -1;
  ASSERT_EQ(1, dawg_candidate_get(handle, 0, &text, &pinyin, &score, &kind));
  ASSERT_NE(nullptr, text);
  EXPECT_STREQ("中国人", text);
  ASSERT_NE(nullptr, pinyin);
  EXPECT_STREQ("zhong guo ren", pinyin);
  EXPECT_EQ(static_cast<int>(CandidateKind::kPhrase), kind);
  EXPECT_EQ(0, dawg_candidate_get(handle, count, &text, nullptr, nullptr, nullptr));

  dawg_engine_destroy(handle);
}

}  // namespace
}  // namespace dawg
