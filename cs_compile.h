//
// Created by Dan Evans on 1/7/24.
//
#ifndef CSTAR_CS_COMPILE_H
#define CSTAR_CS_COMPILE_H

#include <bitset>
#include <sstream>

#include "cs_global.h"

namespace cs {
  inline int symbol_count;
  inline int execution_count;
  inline int eof_count;
  inline int SAVE_SYMBOL_COUNT;
  inline int save_execution_count;
  inline int save_line_count;

  // nb: Message Passing Interface, see https://www.mpi-forum.org
  inline bool mpi_mode; // global ?

  inline std::vector<char> LINE;
  inline std::vector<char> LINE2;
  inline int SX;
  inline int CC;
  inline int C1;
  inline int C2;
  inline int CC2;
  inline int line_count;
  inline int LL;
  inline int LL2;
  inline int atab_index; // index to ATAB source local?
  inline int btab_index; // index to BTAB source local?
  inline int ctab_index; // index to CTAB source local?
  inline int tab_index; // index to TAB source local?
  inline int error_position;
  inline int error_count;
  inline int CPNT;
  char STAB[SMAX];

  // COMPILE_CS_EXPORT int ERRS;   // range 0..ERMAX
  int ITPNT; // range 0 ..INITMAX
  void in_symbol();

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
    Objects object;
    Types types;
    Index reference;
    bool normal;
    int LEV; // range 0..LMAX;
    int address;
    int64_t size;
    Index FREF;
    int FORLEV;
    bool PNTPARAM;
  };

  struct INITPAIR {
    int IVAL;
    double RVAL;
  };

  struct ATABREC {
    Types INXTYP;
    Types ELTYP;
    int ELREF, LOW, HIGH, ELSIZE, SIZE;
  };

  struct BTABREC {
    int LAST, LASTPAR, PSIZE, VSIZE, PARCNT;
  };

  struct CTABREC {
    Types ELTYP;
    int ELREF, ELSIZE;
  };

  struct Item {
    Types types;
    int reference;
    int64_t size;
    bool is_address;
  };

  struct LIBREC {
    ALFA NAME;
    int IDNUM;
  };

  struct CONREC {
    Types TP;
    int64_t I;
  };

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
  inline Symbol symbol;
  inline std::vector<char> key;
  inline std::vector<Symbol> ksy;
  inline ALFA dummy_name;
  inline Index proto_index;
  //  SPS: ARRAY [CHAR] OF SYMBOL;
  inline std::vector<Symbol> SPS;
  inline SymbolSet base_set;
  inline SymbolSet type_set;
  inline SymbolSet block_set;
  inline SymbolSet FACBEGSYS;
  inline SymbolSet declaration_set;
  inline SymbolSet keyword_set;
  inline SymbolSet assigners_set;
  inline SymbolSet execution_set;
  inline SymbolSet selection_set;
  inline SymbolSet non_mpi_set;
  inline SymbolSet COMPARATORS_SET;
  inline TypeSet standard_set;
  inline std::vector<LIBREC> LIBFUNC;
  inline std::vector<TABREC> TAB;
  inline std::vector<ATABREC> ATAB;
  inline std::vector<BTABREC> BTAB;
  inline std::vector<CTABREC> CTAB;
  inline std::vector<INITPAIR> INITABLE;
  void initialize_compiler();
  inline int write_block;
  inline int highest_processor;
  inline Symbol topology;
  inline Index TOPDIM;
  inline Index proto_ref;
  inline Item return_type;
} // namespace cs
#endif // CSTAR_CS_COMPILE_H
