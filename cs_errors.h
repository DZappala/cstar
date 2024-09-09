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

namespace Cstar {
void ERROR(int);
void ERROREXIT();
void ERRORMSG();
void FATAL(int);
ERRORS_CS_EXPORT std::bitset<ERMAX> ERRS;
ERRORS_CS_EXPORT bool FATALERROR;
} // namespace Cstar
#endif // CSTAR_CS_ERRORS_H
