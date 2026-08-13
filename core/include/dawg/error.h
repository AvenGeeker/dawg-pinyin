// SPDX-License-Identifier: Apache-2.0
#ifndef DAWG_INCLUDE_DAWG_ERROR_H_
#define DAWG_INCLUDE_DAWG_ERROR_H_

namespace dawg {

// Error codes shared across the public API. The engine never throws.
enum class ErrorCode : int {
  kOk = 0,
  kFileNotFound = 1,   // Asset file missing on disk.
  kBadFormat = 2,      // Asset exists but failed parsing/validation.
  kInvalidInput = 3,   // Caller input rejected (charset, length, ...).
  kNoMemory = 4,       // Allocation or mapping failure.
};

// Stable human-readable name, e.g. "kOk". Never returns nullptr.
const char* ErrorCodeToString(ErrorCode code);

}  // namespace dawg

#endif  // DAWG_INCLUDE_DAWG_ERROR_H_
