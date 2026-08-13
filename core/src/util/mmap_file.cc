// SPDX-License-Identifier: Apache-2.0
#include "util/mmap_file.h"

#include <cerrno>
#include <cstdio>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include <vector>

namespace dawg {
namespace util {

MmapFile::~MmapFile() { Close(); }

MmapFile::MmapFile(MmapFile&& other) noexcept
    : data_(other.data_), size_(other.size_) {
  other.data_ = nullptr;
  other.size_ = 0;
}

MmapFile& MmapFile::operator=(MmapFile&& other) noexcept {
  if (this != &other) {
    Close();
    data_ = other.data_;
    size_ = other.size_;
    other.data_ = nullptr;
    other.size_ = 0;
  }
  return *this;
}

ErrorCode MmapFile::Open(const std::string& path) {
  Close();
  int fd = ::open(path.c_str(), O_RDONLY | O_CLOEXEC);
  if (fd < 0) {
    return ErrorCode::kFileNotFound;
  }
  struct stat st;
  if (::fstat(fd, &st) != 0 || st.st_size < 0) {
    ::close(fd);
    return ErrorCode::kBadFormat;
  }
  size_t size = static_cast<size_t>(st.st_size);
  if (size == 0) {
    ::close(fd);
    return ErrorCode::kBadFormat;
  }
  void* base = ::mmap(nullptr, size, PROT_READ, MAP_PRIVATE, fd, 0);
  ::close(fd);
  if (base == MAP_FAILED) {
    return ErrorCode::kNoMemory;
  }
  data_ = static_cast<uint8_t*>(base);
  size_ = size;
  return ErrorCode::kOk;
}

void MmapFile::Close() {
  if (data_ != nullptr) {
    ::munmap(data_, size_);
    data_ = nullptr;
    size_ = 0;
  }
}

ErrorCode ReadFileToVector(const std::string& path,
                           std::vector<uint8_t>* out) {
  FILE* f = std::fopen(path.c_str(), "rb");
  if (f == nullptr) {
    return ErrorCode::kFileNotFound;
  }
  std::fseek(f, 0, SEEK_END);
  long size = std::ftell(f);
  std::fseek(f, 0, SEEK_SET);
  if (size < 0) {
    std::fclose(f);
    return ErrorCode::kBadFormat;
  }
  out->resize(static_cast<size_t>(size));
  size_t got = std::fread(out->data(), 1, out->size(), f);
  std::fclose(f);
  if (got != out->size()) {
    return ErrorCode::kBadFormat;
  }
  return ErrorCode::kOk;
}

ErrorCode WriteVectorToFile(const std::string& path,
                            const std::vector<uint8_t>& data) {
  FILE* f = std::fopen(path.c_str(), "wb");
  if (f == nullptr) {
    return ErrorCode::kFileNotFound;
  }
  size_t put = std::fwrite(data.data(), 1, data.size(), f);
  bool ok = std::fclose(f) == 0;
  return (put == data.size() && ok) ? ErrorCode::kOk : ErrorCode::kBadFormat;
}

}  // namespace util
}  // namespace dawg
