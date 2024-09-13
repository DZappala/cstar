//
// Created by Dan Evans on 4/5/24.
//

#ifndef CSTAR_CS_EXEC_H
#define CSTAR_CS_EXEC_H
#include <sstream>

#include "cs_global.h"
#include "cs_interpret.h"

struct ExLocal {
  ExLocal() = default;
  int I = 0;
  int J = 0;
  int K = 0;
  int H1 = 0;
  int H2 = 0;
  int H3 = 0;
  int H4 = 0;
  int TGAP = 0;
  double RH1 = 0;

  cs::ORDER IR{};
  bool B1 = false;
  cs::PROCPNT NEWPROC = nullptr;
  // InterpLocal *il;
  double log10 = 0.0;
  std::stringbuf buf;
} __attribute__((aligned(128))) __attribute__((packed));

#endif // CSTAR_CS_EXEC_H
