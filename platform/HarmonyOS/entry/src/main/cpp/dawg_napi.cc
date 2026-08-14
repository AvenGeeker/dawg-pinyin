// SPDX-License-Identifier: Apache-2.0
// Thin NAPI facade over the dawg C API. Type conversion only; no logic here
// (the contract is pinned by core/tests/capi_test.cc).
#include <napi/native_api.h>

#include <cstdint>
#include <string>

#include "dawg/dawg_capi.h"

namespace {

std::string GetStringArg(napi_env env, napi_value value) {
  size_t len = 0;
  if (napi_get_value_string_utf8(env, value, nullptr, 0, &len) != napi_ok) {
    return std::string();
  }
  std::string out(len, '\0');
  if (napi_get_value_string_utf8(env, value, out.data(), len + 1, &len) !=
      napi_ok) {
    return std::string();
  }
  return out;
}

DawgEngine* AsEngine(napi_env env, napi_value value) {
  int64_t handle = 0;
  if (napi_get_value_int64(env, value, &handle) != napi_ok) {
    return nullptr;
  }
  return reinterpret_cast<DawgEngine*>(handle);
}

napi_value Create(napi_env env, napi_callback_info info) {
  constexpr size_t kArgc = 5;
  size_t argc = kArgc;
  napi_value args[kArgc] = {nullptr};
  napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);

  DawgOptions options{};
  const std::string dawg_path = GetStringArg(env, args[0]);
  const std::string lm_path = GetStringArg(env, args[1]);
  options.dawg_path = dawg_path.c_str();
  options.lm_path = lm_path.empty() ? nullptr : lm_path.c_str();
  napi_get_value_int32(env, args[2], &options.max_candidates);
  napi_get_value_int32(env, args[3], &options.beam_width);
  napi_get_value_int32(env, args[4], &options.context_chars);

  int err = 0;
  DawgEngine* handle = dawg_engine_create(&options, &err);
  napi_value result = nullptr;
  napi_create_int64(env, reinterpret_cast<int64_t>(handle), &result);
  return result;
}

napi_value Search(napi_env env, napi_callback_info info) {
  constexpr size_t kArgc = 3;
  size_t argc = kArgc;
  napi_value args[kArgc] = {nullptr};
  napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);

  DawgEngine* engine = AsEngine(env, args[0]);
  const std::string input = GetStringArg(env, args[1]);
  const std::string context = GetStringArg(env, args[2]);
  int err = 0;
  const int count = dawg_engine_search(engine, input.c_str(), context.c_str(),
                                       &err);
  napi_value result = nullptr;
  napi_create_int32(env, count, &result);
  return result;
}

napi_value CandidateText(napi_env env, napi_callback_info info) {
  constexpr size_t kArgc = 2;
  size_t argc = kArgc;
  napi_value args[kArgc] = {nullptr};
  napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);

  DawgEngine* engine = AsEngine(env, args[0]);
  int32_t index = 0;
  napi_get_value_int32(env, args[1], &index);
  const char* text = nullptr;
  if (dawg_candidate_get(engine, index, &text, nullptr, nullptr, nullptr) !=
          1 ||
      text == nullptr) {
    napi_value null_value = nullptr;
    napi_get_null(env, &null_value);
    return null_value;
  }
  napi_value result = nullptr;
  napi_create_string_utf8(env, text, NAPI_AUTO_LENGTH, &result);
  return result;
}

napi_value Destroy(napi_env env, napi_callback_info info) {
  size_t argc = 1;
  napi_value args[1] = {nullptr};
  napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
  dawg_engine_destroy(AsEngine(env, args[0]));
  napi_value undefined = nullptr;
  napi_get_undefined(env, &undefined);
  return undefined;
}

napi_value Init(napi_env env, napi_value exports) {
  const napi_property_descriptor desc[] = {
      {"create", nullptr, Create, nullptr, nullptr, nullptr, napi_default,
       nullptr},
      {"search", nullptr, Search, nullptr, nullptr, nullptr, napi_default,
       nullptr},
      {"candidateText", nullptr, CandidateText, nullptr, nullptr, nullptr,
       napi_default, nullptr},
      {"destroy", nullptr, Destroy, nullptr, nullptr, nullptr, napi_default,
       nullptr},
  };
  napi_define_properties(env, exports, sizeof(desc) / sizeof(desc[0]), desc);
  return exports;
}

}  // namespace

napi_module dawgModule = {
    .nm_version = 1,
    .nm_flags = 0,
    .nm_filename = nullptr,
    .nm_register_func = Init,
    .nm_modname = "dawg_napi",
    .nm_priv = nullptr,
    .reserved = {0},
};

extern "C" __attribute__((constructor)) void RegisterDawgNapiModule(void) {
  napi_module_register(&dawgModule);
}
