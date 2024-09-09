//
// Created by Dan Evans on 4/5/24.
//

#ifndef CSTAR_CS_EXEC_H
#define CSTAR_CS_EXEC_H
#include "cs_global.h"
#include "cs_interpret.h"
struct ExLocal {
  int I, J, K, H1, H2, H3, H4, TGAP;
  double RH1;
  Cstar::ORDER IR;
  bool B1;
  Cstar::PROCPNT NEWPROC;
  // InterpLocal *il;
  double log10;
  char buf[24];
};
#endif // CSTAR_CS_EXEC_H
