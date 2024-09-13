//
// Created by Dan Evans on 1/7/24.
//
#include "cs_global.h"
#include <bitset>
#include <sstream>

#ifndef CSTAR_CS_COMPILE_H
#define CSTAR_CS_COMPILE_H
#ifdef EXPORT_CS_COMPILE
#define COMPILE_CS_EXPORT
#else
#define COMPILE_CS_EXPORT extern
#endif
#include "cs_defines.h"

namespace Cstar {
COMPILE_CS_EXPORT int symbol_count;
COMPILE_CS_EXPORT int execution_count;
COMPILE_CS_EXPORT int eof_count;
COMPILE_CS_EXPORT int save_symbol_count, save_execution_count, save_line_count;

// nb: Message Passing Interface, see https://www.mpi-forum.org
COMPILE_CS_EXPORT bool mpi_mode; // global ?

COMPILE_CS_EXPORT std::stringbuf line, line2;
COMPILE_CS_EXPORT int SX, CC, C1, C2, CC2;
COMPILE_CS_EXPORT int line_count;
COMPILE_CS_EXPORT int LL, LL2;
COMPILE_CS_EXPORT int atab_index; // index to ATAB source local?
COMPILE_CS_EXPORT int btab_index; // index to BTAB source local?
COMPILE_CS_EXPORT int ctab_index; // index to CTAB source local?
COMPILE_CS_EXPORT int tab_index;  // index to TAB source local?
COMPILE_CS_EXPORT int error_position;
COMPILE_CS_EXPORT int error_count;
COMPILE_CS_EXPORT int CPNT;
COMPILE_CS_EXPORT char STAB[SMAX];

// COMPILE_CS_EXPORT int ERRS;   // range 0..ERMAX
COMPILE_CS_EXPORT int ITPNT; // range 0 ..INITMAX
COMPILE_CS_EXPORT void INSYMBOL();

using SymbolSet = std::bitset<95>;
using Index = int; // range from -XMAX to +XMAX
using TypeSet = std::bitset<127>;

enum class Symbol : uint8_t {
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

enum class Types : uint8_t {
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

struct TABREC {
  ALFA name;
  Index link;
  OBJECTS object;
  Types types;
  Index reference;
  bool normal;
  int LEV; // range 0..LMAX;
  int address;
  int64_t size;
  Index FREF;
  int FORLEV;
  bool PNTPARAM;
} __attribute__((aligned(64))) __attribute__((packed));

struct INITPAIR {
  int IVAL;
  double RVAL;
} __attribute__((aligned(16)));

struct ATABREC {
  Types INXTYP;
  Types ELTYP;
  int ELREF, LOW, HIGH, ELSIZE, SIZE;
} __attribute__((aligned(32))) __attribute__((packed));

struct BTABREC {
  int LAST, LASTPAR, PSIZE, VSIZE, PARCNT;
} __attribute__((aligned(32)));

struct CTABREC {
  Types ELTYP;
  int ELREF, ELSIZE;
} __attribute__((aligned(16))) __attribute__((packed));

struct Item {
  enum Types types;
  int reference;
  int64_t size;
  bool is_address;
} __attribute__((aligned(16))) __attribute__((packed));

struct LIBREC {
  ALFA NAME;
  int IDNUM;
} __attribute__((aligned(32)));

struct CONREC {
  Types TP;
  int64_t I;
} __attribute__((aligned(16)));

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
COMPILE_CS_EXPORT Symbol symbol;
#ifdef EXPORT_CS_COMPILE
COMPILE_CS_EXPORT char key[][NKW + 1] = {
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
COMPILE_CS_EXPORT char key[][NKW + 1];
#endif
COMPILE_CS_EXPORT enum Symbol ksy[NKW + 1];
COMPILE_CS_EXPORT ALFA dummy_name;
COMPILE_CS_EXPORT Index proto_index;
//  SPS: ARRAY [CHAR] OF SYMBOL;
COMPILE_CS_EXPORT enum Symbol SPS[128];
COMPILE_CS_EXPORT SymbolSet CONSTBEGSYS, TYPEBEGSYS, BLOCKBEGSYS, FACBEGSYS,
    DECLBEGSYS, STATBEGSYS, ASSIGNBEGSYS, EXECSYS, SELECTSYS, NONMPISYS,
    COMPASGNSYS;
COMPILE_CS_EXPORT TypeSet STANTYPS;
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
COMPILE_CS_EXPORT void initialize_compiler();
COMPILE_CS_EXPORT int write_block;
COMPILE_CS_EXPORT int highest_processor;
COMPILE_CS_EXPORT Symbol topology;
COMPILE_CS_EXPORT Index TOPDIM;
COMPILE_CS_EXPORT Index proto_ref;
COMPILE_CS_EXPORT Item return_type;
} // namespace Cstar
#endif // CSTAR_CS_COMPILE_H
