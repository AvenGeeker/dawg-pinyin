// SPDX-License-Identifier: Apache-2.0
#include "dawg/error.h"

namespace dawg {

const char* ErrorCodeToString(ErrorCode code) {
  switch (code) {
    case ErrorCode::kOk:
      return "kOk";
    case ErrorCode::kFileNotFound:
      return "kFileNotFound";
    case ErrorCode::kBadFormat:
      return "kBadFormat";
    case ErrorCode::kInvalidInput:
      return "kInvalidInput";
    case ErrorCode::kNoMemory:
      return "kNoMemory";
  }
  return "kUnknown";
}

}  // namespace dawg
