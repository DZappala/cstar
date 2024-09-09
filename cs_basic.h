//
// Created by Dan Evans on 1/17/24.
//

#ifndef CSTAR_CS_BASIC_H
#define CSTAR_CS_BASIC_H
#include "cs_block.h"
#include "cs_compile.h"
namespace Cstar {
typedef struct BasicLocal {
  ITEM Y;
  SYMBOL OP;
  int F; // decl in BASICEXPRESSION, set in FACTOR, used in COMPASSIGNEXP
         // ASSIGNMENTEXP
  BlockLocal *bl;
} BasicLocal;
} // namespace Cstar
#endif // CSTAR_CS_BASIC_H
