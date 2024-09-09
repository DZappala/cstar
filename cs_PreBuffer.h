//
// Created by Dan Evans on 5/13/24.
//
#ifndef CSTAR_CS_PREBUFFER_H
#define CSTAR_CS_PREBUFFER_H
#include "cs_defines.h"
#include <cstdio>
#include <cstring>
namespace Cstar {
/*
 * PreBuffer is used by the function Freadline(), a rewritten version of
 * FREADLINE, which in turn is used by the interpreter to read commands from the
 * console.  PreBuffer can be loaded with a set of prefix commands which will be
 * immediately executed before reading from the console starts.  This supports
 * non-interactive compilation and execution.
 */
struct PreBuffer {
  FILE *fp;          // file pointer
  int bl;            // buffer low
  int bh;            // buffer high
  char buffer[LLNG]; // buffer
  explicit PreBuffer(FILE *fp) {
    this->fp = fp;
    bl = 0;
    bh = 0;
    buffer[0] = '\0';
  }

  int getc() {
    if (bh == bl)
      return fgetc(fp);
    return buffer[bl++];
  }

  void setBuffer(const char *data, int size) {
    if (size < LLNG) {
      memmove(buffer, data, size);
      bl = 0;
      bh = size;
    }
  }
};
} // namespace Cstar
#endif // CSTAR_CS_PREBUFFER_H
