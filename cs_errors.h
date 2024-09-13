//
// Created by Dan Evans on 1/11/24.
//
#ifndef CSTAR_CS_ERRORS_H
#define CSTAR_CS_ERRORS_H
#ifdef EXPORT_CS_ERRORS
#define ERRORS_CS_EXPORT
#else
#define ERRORS_CS_EXPORT extern
#endif

#include "cs_global.h"
#include <bitset>

namespace cs {
  void error(int);
  void error_exit();
  void error_message();
  void fatal(int);
  ERRORS_CS_EXPORT std::bitset<ERMAX> errors;
  ERRORS_CS_EXPORT bool fatal_error;
} // namespace Cstar
#endif // CSTAR_CS_ERRORS_H
