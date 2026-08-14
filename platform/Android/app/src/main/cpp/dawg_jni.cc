// SPDX-License-Identifier: Apache-2.0
// Thin JNI facade over the dawg C API. Type conversion only; no logic here
// (the contract is pinned by core/tests/capi_test.cc).
#include <jni.h>

#include <string>

#include "dawg/dawg_capi.h"

namespace {

std::string ToUtf8(JNIEnv* env, jstring value) {
  if (value == nullptr) {
    return std::string();
  }
  const char* chars = env->GetStringUTFChars(value, nullptr);
  if (chars == nullptr) {
    return std::string();
  }
  std::string out(chars);
  env->ReleaseStringUTFChars(value, chars);
  return out;
}

void StoreError(JNIEnv* env, jintArray err_out, int err) {
  if (err_out != nullptr) {
    env->SetIntArrayRegion(err_out, 0, 1, &err);
  }
}

DawgEngine* AsEngine(jlong handle) {
  return reinterpret_cast<DawgEngine*>(handle);
}

}  // namespace

extern "C" JNIEXPORT jlong JNICALL
Java_com_avengeeker_dawgime_engine_DawgEngineNative_nativeCreate(
    JNIEnv* env, jclass, jstring dawg_path, jstring lm_path,
    jint max_candidates, jint beam_width, jint context_chars,
    jintArray err_out) {
  const std::string dawg = ToUtf8(env, dawg_path);
  const std::string lm = ToUtf8(env, lm_path);
  DawgOptions options{};
  options.dawg_path = dawg.c_str();
  options.lm_path = lm.empty() ? nullptr : lm.c_str();
  options.max_candidates = max_candidates;
  options.beam_width = beam_width;
  options.context_chars = context_chars;
  int err = 0;
  DawgEngine* handle = dawg_engine_create(&options, &err);
  StoreError(env, err_out, err);
  return reinterpret_cast<jlong>(handle);
}

extern "C" JNIEXPORT jint JNICALL
Java_com_avengeeker_dawgime_engine_DawgEngineNative_nativeSearch(
    JNIEnv* env, jclass, jlong handle, jstring input, jstring context,
    jintArray err_out) {
  int err = 0;
  const std::string in = ToUtf8(env, input);
  const std::string ctx = ToUtf8(env, context);
  const jint count = dawg_engine_search(AsEngine(handle), in.c_str(),
                                        ctx.c_str(), &err);
  StoreError(env, err_out, err);
  return count;
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_avengeeker_dawgime_engine_DawgEngineNative_nativeCandidateText(
    JNIEnv* env, jclass, jlong handle, jint index) {
  const char* text = nullptr;
  if (dawg_candidate_get(AsEngine(handle), index, &text, nullptr, nullptr,
                         nullptr) != 1 ||
      text == nullptr) {
    return nullptr;
  }
  return env->NewStringUTF(text);
}

extern "C" JNIEXPORT void JNICALL
Java_com_avengeeker_dawgime_engine_DawgEngineNative_nativeDestroy(
    JNIEnv*, jclass, jlong handle) {
  dawg_engine_destroy(AsEngine(handle));
}
