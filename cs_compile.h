//
// Created by Dan Evans on 1/7/24.
//
#include "cs_global.h"
#include <bitset>

#ifndef CSTAR_CS_COMPILE_H
#define CSTAR_CS_COMPILE_H
#ifdef EXPORT_CS_COMPILE
#define COMPILE_CS_EXPORT
#else
#define COMPILE_CS_EXPORT extern
#endif
#include "cs_defines.h"
namespace Cstar {
COMPILE_CS_EXPORT int SYMCNT;
COMPILE_CS_EXPORT int EXECNT;
COMPILE_CS_EXPORT int EOFCOUNT;
COMPILE_CS_EXPORT int SAVESYMCNT, SAVEXECNT, SAVELC;
COMPILE_CS_EXPORT bool MPIMODE; // global ?
COMPILE_CS_EXPORT char LINE[LLNG + 1], LINE2[LLNG + 1];
COMPILE_CS_EXPORT int SX, CC, C1, C2, CC2;
COMPILE_CS_EXPORT int LC;
COMPILE_CS_EXPORT int LL, LL2;
COMPILE_CS_EXPORT int A;  // index to ATAB source local?
COMPILE_CS_EXPORT int B;  // index to BTAB source local?
COMPILE_CS_EXPORT int C;  // index to CTAB source local?
COMPILE_CS_EXPORT int Tx; // index to TAB source local?
COMPILE_CS_EXPORT int ERRPOS;
COMPILE_CS_EXPORT int ERRORCOUNT;
COMPILE_CS_EXPORT int CPNT;
COMPILE_CS_EXPORT char STAB[SMAX];

// COMPILE_CS_EXPORT int ERRS;   // range 0..ERMAX
COMPILE_CS_EXPORT int ITPNT; // range 0 ..INITMAX
COMPILE_CS_EXPORT void INSYMBOL();
enum SYMBOL {
  INTCON,
  CHARCON,
  REALCON,
  STRNG,
  CINSY,
  COUTSY,
  UNIONSY,
  DEFINESY,
  NOTSY,
  PLUS,
  MINUS,
  TIMES,
  RDIV,
  IMOD,
  ANDSY,
  ORSY,
  OUTSTR,
  INSTR,
  ENDLSY,
  EQL,
  NEQ,
  GTR,
  GEQ,
  LSS,
  LEQ,
  GRSEMI,
  RARROW,
  ADDRSY,
  INCREMENT,
  DECREMENT,
  LPARENT,
  RPARENT,
  LBRACK,
  RBRACK,
  COMMA,
  SEMICOLON,
  RETURNSY,
  PERIOD,
  ATSY,
  CHANSY,
  QUEST,
  DBLQUEST,
  RSETBRACK,
  LSETBRACK,
  NEWSY,
  COLON,
  BECOMES,
  CONSTSY,
  TYPESY,
  VALUESY,
  FUNCTIONSY,
  IDENT,
  IFSY,
  WHILESY,
  FORSY,
  FORALLSY,
  CASESY,
  SWITCHSY,
  ELSESY,
  DOSY,
  TOSY,
  THENSY,
  CONTSY,
  DEFAULTSY,
  MOVESY,
  GROUPINGSY,
  FORKSY,
  JOINSY,
  STRUCTSY,
  BREAKSY,
  SHAREDSY,
  FULLCONNSY,
  HYPERCUBESY,
  LINESY,
  MESH2SY,
  MESH3SY,
  RINGSY,
  TORUSSY,
  CLUSTERSY,
  ERRSY,
  EOFSY,
  INCLUDESY,
  ENUMSY,
  BITCOMPSY,
  BITANDSY,
  BITXORSY,
  BITINCLSY,
  PLUSEQ,
  MINUSEQ,
  TIMESEQ,
  RDIVEQ,
  IMODEQ,
  SHORTSY,
  LONGSY,
  UNSIGNEDSY
};
enum TYPES {
  NOTYP,
  REALS,
  INTS,
  BOOLS,
  CHARS,
  ARRAYS,
  CHANS,
  RECS,
  PNTS,
  FORWARDS,
  LOCKS,
  VOIDS
};
// typedef int TYPSET[EMAX];
typedef std::bitset<EMAX> TYPSET;
struct TABREC {
  ALFA NAME;
  INDEX LINK;
  OBJECTS OBJ;
  TYPES TYP;
  INDEX REF;
  bool NORMAL;
  int LEV; // range 0..LMAX;
  int ADR;
  long SIZE;
  INDEX FREF;
  int FORLEV;
  bool PNTPARAM;
};
struct INITPAIR {
  int IVAL;
  double RVAL;
};
struct ATABREC {
  TYPES INXTYP;
  TYPES ELTYP;
  int ELREF, LOW, HIGH, ELSIZE, SIZE;
};

struct BTABREC {
  int LAST, LASTPAR, PSIZE, VSIZE, PARCNT;
};

struct CTABREC {
  TYPES ELTYP;
  int ELREF, ELSIZE;
};
struct ITEM {
  enum TYPES TYP;
  int REF;
  long SIZE;
  bool ISADDR;
};
struct LIBREC {
  ALFA NAME;
  int IDNUM;
};
struct CONREC {
  TYPES TP;
  long I;
};
typedef std::bitset<SYMBOLCNT> SYMSET;
typedef int INDEX; // range from -XMAX to +XMAX

// typedef SYMBOL SYMSET[EMAX];
//    std::string KEY[] = {
//            "#DEFINE",
//            "#INCLUDE",
//            "BREAK",
//            "CASES",
//            "CIN",
//            "CONST",
//            "CONTINUE",
//            "COUT",
//            "DEFAULT",
//            "DO",
//            "ELSE",
//            "ENDL",
//            "ENUM",
//            "FOR",
//            "FORALL",
//            "FORK",
//            "FUNCTION",
//            "GROUPING",
//            "IF",
//            "JOIN",
//            "LONG",
//            "MOVE",
//            "NEW",
//            "RETURN",
//            "SHORT",
//            "STREAM",
//            "STRUCT",
//            "SWITCH",
//            "THEN",
//            "TO",
//            "TYPEDEF",
//            "UNION",
//            "UNSIGNED",
//            "VALUE",
//            "WHILE",
//    } ;
//    KEY: ARRAY[1..NKW] OF ALFA;
COMPILE_CS_EXPORT SYMBOL SY;
#ifdef EXPORT_CS_COMPILE
COMPILE_CS_EXPORT char KEY[][NKW + 1] = {
    "              ", "#DEFINE       ", "#INCLUDE      ", "BREAK         ",
    "CASE          ", "CIN           ", "CONST         ", "CONTINUE      ",
    "COUT          ", "DEFAULT       ", "DO            ", "ELSE          ",
    "ENDL          ", "ENUM          ", "FOR           ", "FORALL        ",
    "FORK          ", "FUNCTION      ", "GROUPING      ", "IF            ",
    "JOIN          ", "LONG          ", "MOVE          ", "NEW           ",
    "RETURN        ", "SHORT         ", "STREAM        ", "STRUCT        ",
    "SWITCH        ", "THEN          ", "TO            ", "TYPEDEF       ",
    "UNION         ", "UNSIGNED      ", "VALUE         ", "WHILE         ",
};
#else
COMPILE_CS_EXPORT char KEY[][NKW + 1];
#endif
COMPILE_CS_EXPORT enum SYMBOL KSY[NKW + 1];
COMPILE_CS_EXPORT ALFA DUMMYNAME;
COMPILE_CS_EXPORT INDEX PROTOINDEX;
//  SPS: ARRAY [CHAR] OF SYMBOL;
COMPILE_CS_EXPORT enum SYMBOL SPS[128];
COMPILE_CS_EXPORT SYMSET CONSTBEGSYS, TYPEBEGSYS, BLOCKBEGSYS, FACBEGSYS,
    DECLBEGSYS, STATBEGSYS, ASSIGNBEGSYS, EXECSYS, SELECTSYS, NONMPISYS,
    COMPASGNSYS;
COMPILE_CS_EXPORT TYPSET STANTYPS;
COMPILE_CS_EXPORT struct LIBREC LIBFUNC[LIBMAX + 1];
#ifdef EXPORT_CS_COMPILE
COMPILE_CS_EXPORT std::vector<TABREC> TAB(TMAX + 1);
#else
COMPILE_CS_EXPORT std::vector<TABREC> TAB;
#endif
#ifdef EXPORT_CS_COMPILE
COMPILE_CS_EXPORT std::vector<ATABREC> ATAB(AMAX + 1);
#else
COMPILE_CS_EXPORT std::vector<ATABREC> ATAB;
#endif

#ifdef EXPORT_CS_COMPILE
COMPILE_CS_EXPORT std::vector<BTABREC> BTAB(BMAX + 1);
#else
COMPILE_CS_EXPORT std::vector<BTABREC> BTAB;
#endif
#ifdef EXPORT_CS_COMPILE
COMPILE_CS_EXPORT std::vector<CTABREC> CTAB(CHMAX + 1);
#else
COMPILE_CS_EXPORT std::vector<CTABREC> CTAB;
#endif
#ifdef EXPORT_CS_COMPILE
COMPILE_CS_EXPORT std::vector<INITPAIR> INITABLE(INITMAX + 1);
#else
COMPILE_CS_EXPORT std::vector<INITPAIR> INITABLE;
#endif
COMPILE_CS_EXPORT void INITCOMPILER();
COMPILE_CS_EXPORT int WRITEBLOCK;
COMPILE_CS_EXPORT int HIGHESTPROCESSOR;
COMPILE_CS_EXPORT SYMBOL TOPOLOGY;
COMPILE_CS_EXPORT INDEX TOPDIM;
COMPILE_CS_EXPORT INDEX PROTOREF;
COMPILE_CS_EXPORT ITEM RETURNTYPE;
} // namespace Cstar
#endif // CSTAR_CS_COMPILE_H
