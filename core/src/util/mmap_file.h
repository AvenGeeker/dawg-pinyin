// SPDX-License-Identifier: Apache-2.0
#ifndef DAWG_CORE_SRC_UTIL_MMAP_FILE_H_
#define DAWG_CORE_SRC_UTIL_MMAP_FILE_H_

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "dawg/error.h"

namespace dawg {
namespace util {

// Read-only, privately mapped file. Clean pages may be evicted by the OS
// under memory pressure, which is what keeps large assets affordable inside
// the iOS keyboard extension memory budget.
class MmapFile {
 public:
  MmapFile() = default;
  ~MmapFile();
  MmapFile(const MmapFile&) = delete;
  MmapFile& operator=(const MmapFile&) = delete;
  MmapFile(MmapFile&& other) noexcept;
  MmapFile& operator=(MmapFile&& other) noexcept;

  ErrorCode Open(const std::string& path);
  void Close();

  const uint8_t* data() const { return data_; }
  size_t size() const { return size_; }
  bool valid() const { return data_ != nullptr; }

 private:
  uint8_t* data_ = nullptr;
  size_t size_ = 0;
};

// Reads a whole file into memory (tools/tests only, not the runtime path).
ErrorCode ReadFileToVector(const std::string& path,
                           std::vector<uint8_t>* out);

// Writes a buffer to disk, replacing any existing file.
ErrorCode WriteVectorToFile(const std::string& path,
                            const std::vector<uint8_t>& data);

}  // namespace util
}  // namespace dawg

#endif  // DAWG_CORE_SRC_UTIL_MMAP_FILE_H_
