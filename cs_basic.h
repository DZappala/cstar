//
// Created by Dan Evans on 1/17/24.
//

#ifndef CSTAR_CS_BASIC_H
#define CSTAR_CS_BASIC_H
#include "cs_block.h"
#include "cs_compile.h"

namespace cs {
  using BasicLocal = struct BasicLocal {
    Item item;
    Symbol operation;
    int factor; // decl in BASICEXPRESSION, set in FACTOR, used in COMPASSIGNEXP
    // ASSIGNMENTEXP
    BlockLocal* block_local;
  } __attribute__((aligned(64))) __attribute__((packed));
} // namespace Cstar
#endif // CSTAR_CS_BASIC_H
