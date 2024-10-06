#pragma once
//
// Created by Dan Evans on 5/13/24.
//
#include <fstream>
#include <filesystem>
#include <stdexcept>
#include <streambuf>

#include "cs_defines.h"

namespace fs = std::filesystem;

namespace cs {
/*
 * PreBuffer is used by the function Freadline(), a rewritten version of
 * FREADLINE, which in turn is used by the interpreter to read commands from the
 * console.  PreBuffer can be loaded with a set of prefix commands which will be
 * immediately executed before reading from the console starts.  This supports
 * non-interactive compilation and execution.
 */
class PreBuffer final : public std::streambuf {
  std::ifstream file;
  std::vector<char> buffer;

  public:
    PreBuffer() = default;

    explicit PreBuffer(const fs::path &file_name)
      : file(file_name), buffer(LLNG) {
      if (!file)
        throw std::runtime_error("Attempted to open an invalid file");
      setg(buffer.data(), buffer.data(), buffer.data() + buffer.size());
    }

    /**
     * @brief Read from the file into the buffer
     * @return int
     */
    auto underflow() -> int override {
      if (file.eof()) return traits_type::eof();
      file.read(buffer.data(), static_cast<int64_t>(buffer.size()));
      setg(buffer.data(), buffer.data(), buffer.data() + file.gcount());
      return traits_type::to_int_type(*gptr());
    }
};
} // namespace cs
