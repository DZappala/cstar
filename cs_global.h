//
// Created by Dan Evans on 1/1/24.
//
#ifndef CSTAR_CS_GLOBAL_H
#define CSTAR_CS_GLOBAL_H
#include <cstdio>
#ifdef EXPORT_CS_GLOBAL
#define GLOBAL_CS_EXPORT
#else
#define GLOBAL_CS_EXPORT extern
#endif
#include "cs_defines.h"
#include <vector>
namespace Cstar {
#define ArraySize(A) ((sizeof(A)) / (sizeof(A)[0]))

//    GLOBAL_CS_EXPORT std::istream *SRC;
//    GLOBAL_CS_EXPORT std::ofstream *LIS;
//    GLOBAL_CS_EXPORT std::ifstream *INP;
//    GLOBAL_CS_EXPORT std::ofstream *OUTP;
//    GLOBAL_CS_EXPORT std::ifstream *INSRC;

GLOBAL_CS_EXPORT FILE *SRC;
GLOBAL_CS_EXPORT FILE *LIS;
GLOBAL_CS_EXPORT FILE *INP;
GLOBAL_CS_EXPORT FILE *OUTP;
GLOBAL_CS_EXPORT FILE *INSRC;
GLOBAL_CS_EXPORT FILE *STDIN;
GLOBAL_CS_EXPORT FILE *STDOUT;

GLOBAL_CS_EXPORT bool LISTFILE, INPUTFILE, OUTPUTFILE, INPUTOPEN, OUTPUTOPEN,
    MUSTRUN, SRCOPEN, LISTDEF;
GLOBAL_CS_EXPORT bool ENDFLAG;
GLOBAL_CS_EXPORT int LNUM;

struct ORDER {
  int F = 0;
  int X = 0;
  int Y = 0;
} __attribute__((aligned(16)));

enum class OBJECTS : std::uint8_t {
  KONSTANT,
  VARIABLE,
  TYPE1,
  PROZEDURE,
  FUNKTION,
  COMPONENT,
  STRUCTAG
};
//    enum TYPES
//    {
//        NOTYP, REALS, INTS, BOOLS, CHARS, ARRAYS, CHANS, RECS, PNTS, FORWARDS,
//        LOCKS, VOIDS
//    };

GLOBAL_CS_EXPORT std::array<int, LMAX + 1> DISPLAY;

using ALFA = std::string;
using OLDALFA = std::string;
using MESSAGEALFA = std::string;
using Index = int;

#ifdef EXPORT_CS_GLOBAL
GLOBAL_CS_EXPORT std::vector<int> LOCATION;
#else
GLOBAL_CS_EXPORT std::vector<int> LOCATION;
#endif
#ifdef EXPORT_CS_GLOBAL
GLOBAL_CS_EXPORT std::vector<bool> BREAKALLOW;
#else
GLOBAL_CS_EXPORT std::vector<bool> BREAKALLOW;
#endif
#ifdef EXPORT_CS_GLOBAL
GLOBAL_CS_EXPORT std::vector<int> BREAKLOC;
#else
GLOBAL_CS_EXPORT std::vector<int> BREAKLOC;
#endif
GLOBAL_CS_EXPORT int BREAKPNT;

// char *KEY[NKW];

// std::vector<char> STAB(SMAX + 1);
#ifdef EXPORT_CS_GLOBAL
GLOBAL_CS_EXPORT std::array<ORDER, CMAX + 1> code;
#else
GLOBAL_CS_EXPORT std::array<ORDER, CMAX + 1> code;
#endif
#ifdef EXPORT_CS_GLOBAL
GLOBAL_CS_EXPORT std::vector<Index> WITHTAB;
#else
GLOBAL_CS_EXPORT std::vector<Index> WITHTAB;
#endif
GLOBAL_CS_EXPORT bool INCLUDEFLAG;
GLOBAL_CS_EXPORT bool OKBREAK;
GLOBAL_CS_EXPORT ALFA ID;
GLOBAL_CS_EXPORT int INUM;
GLOBAL_CS_EXPORT double RNUM;
GLOBAL_CS_EXPORT int SLENG;
GLOBAL_CS_EXPORT char CH;
GLOBAL_CS_EXPORT std::array<double, RCMAX + 1> CONTABLE;
void EMIT(int FCT);
void EMIT1(int FCT, int B);
void EMIT2(int FCT, int A, int B);
} // namespace Cstar

#endif // CSTAR_CS_GLOBAL_H
