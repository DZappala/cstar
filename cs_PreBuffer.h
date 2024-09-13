//
// Created by Dan Evans on 5/13/24.
//
#ifndef CSTAR_CS_PREBUFFER_H
#define CSTAR_CS_PREBUFFER_H
#include "cs_defines.h"
#include <cstdio>
#include <cstring>
#include <sstream>
namespace Cstar {
/*
 * PreBuffer is used by the function Freadline(), a rewritten version of
 * FREADLINE, which in turn is used by the interpreter to read commands from the
 * console.  PreBuffer can be loaded with a set of prefix commands which will be
 * immediately executed before reading from the console starts.  This supports
 * non-interactive compilation and execution.
 */
struct PreBuffer {
  explicit PreBuffer(FILE *file_ptr) : file_ptr(file_ptr) {
    buffer.sputc('\0');
  }

  FILE *file_ptr;
  int buffer_low{};
  int buffer_high{};
  std::stringbuf buffer;

  auto getc() -> int {
    if (buffer_high == buffer_low) {
      return fgetc(file_ptr);
    }
    return buffer.str().at(buffer_low++);
  }

  void setBuffer(const char *data, int size) {
    if (size < LLNG) {
      buffer.sputn(data, size);
      buffer_low = 0;
      buffer_high = size;
    }
  }
} __attribute__((aligned(128))) __attribute__((packed));
} // namespace Cstar
#endif // CSTAR_CS_PREBUFFER_H
