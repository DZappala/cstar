//
// Created by Dan Evans on 4/5/24.
//

#ifndef CSTAR_CS_EXEC_H
#define CSTAR_CS_EXEC_H

#include <string>

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

  cs::Order IR{};
  bool B1 = false;
  cs::PROCPNT NEWPROC = nullptr;
  // InterpLocal *il;
  double log10 = 0.0;
  std::string buf;
};

#endif // CSTAR_CS_EXEC_H
