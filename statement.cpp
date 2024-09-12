#include <cstring>

#include "cs_block.h"
#include "cs_compile.h"
#include "cs_errors.h"
#include "cs_global.h"
#include "cs_interpret.h"

namespace Cstar {
extern void NEXTCH();
extern void VARIABLEDECLARATION(BlockLocal *);
extern void CONSTANTDECLARATION(BlockLocal *);
extern void TYPEDECLARATION(BlockLocal *);
extern auto LOC(BlockLocal *, ALFA ID) -> int64_t;
extern void SKIP(SymbolSet, int);
extern void TEST(SymbolSet &, SymbolSet &, int N);
extern void CALL(BlockLocal *, SymbolSet &, int);
extern void STANDPROC(BlockLocal *, int);
extern void ENTER(BlockLocal *, ALFA, OBJECTS);
extern void ENTERBLOCK();
extern auto TYPE_COMPATIBLE(Item X, Item Y) -> bool;
extern void BASICEXPRESSION(BlockLocal *, SymbolSet, Item &);
extern void CONSTANT(BlockLocal *, SymbolSet &, CONREC &);

void EXPRESSION(BlockLocal *, SymbolSet FSYS, Item &X);
void COMPOUNDSTATEMENT(BlockLocal * /*bl*/);
void FIXBREAKS(int64_t LC1);
void FIXCONTS(int64_t LC1);
void SWITCHSTATEMENT(BlockLocal * /*block_local*/);
void IFSTATEMENT(BlockLocal * /*bl*/);
void DOSTATEMENT(BlockLocal * /*bl*/);
void WHILESTATEMENT(BlockLocal * /*bl*/);
void FORSTATEMENT(BlockLocal * /*bl*/);
void BLOCKSTATEMENT(BlockLocal * /*bl*/);
void FORALLSTATEMENT(BlockLocal * /*bl*/);
void FORKSTATEMENT(BlockLocal * /*bl*/);
void JOINSTATEMENT(BlockLocal * /*bl*/);
void RETURNSTATEMENT(BlockLocal * /*bl*/);
void BREAKSTATEMENT(BlockLocal * /*bl*/);
void CONTINUESTATEMENT();
void MOVESTATEMENT(BlockLocal * /*bl*/);
void INPUTSTATEMENT(BlockLocal * /*bl*/);
void COUTMETHODS(BlockLocal * /*bl*/);
void OUTPUTSTATEMENT(BlockLocal * /*bl*/);

static int CONTPNT; // range 0..LOOPMAX
static std::array<int64_t, LOOPMAX + 1> CONTLOC;

void STATEMENT(BlockLocal *block_local, SymbolSet &FSYS) {
  int64_t I = 0;
  Item X{};
  int64_t JUMPLC = 0;
  Symbol SYMSAV;
  SymbolSet su;
  SymbolSet sv;
  SYMSAV = symbol;
  // INSYMBOL();
  if (STATBEGSYS[SY] || ASSIGNBEGSYS[SY]) {
    if (symbol == Symbol::LSETBRACK) // BEGINSY ??
    {
      COMPOUNDSTATEMENT(block_local);
    } else if (symbol == Symbol::SWITCHSY) {
      SWITCHSTATEMENT(block_local);
    } else if (symbol == Symbol::IFSY) {
      IFSTATEMENT(block_local);
    } else if (symbol == Symbol::DOSY) {
      DOSTATEMENT(block_local);
    } else if (symbol == Symbol::WHILESY) {
      WHILESTATEMENT(block_local);
    } else if (symbol == Symbol::FORSY) {
      FORSTATEMENT(block_local);
    } else if (symbol == Symbol::FORALLSY) {
      FORALLSTATEMENT(block_local);
    } else if (symbol == Symbol::FORKSY) {
      FORKSTATEMENT(block_local);
    } else if (symbol == Symbol::JOINSY) {
      JOINSTATEMENT(block_local);
    } else if (symbol == Symbol::RETURNSY) {
      RETURNSTATEMENT(block_local);
    } else if (symbol == Symbol::BREAKSY) {
      BREAKSTATEMENT(block_local);
    } else if (symbol == Symbol::CONTSY) {
      CONTINUESTATEMENT();
    } else if (symbol == Symbol::MOVESY) {
      MOVESTATEMENT(block_local);
    } else if (symbol == Symbol::CINSY) {
      INPUTSTATEMENT(block_local);
    } else if (symbol == Symbol::COUTSY) {
      OUTPUTSTATEMENT(block_local);
    } else if (symbol == Symbol::IDENT) {
      I = LOC(block_local, ID);
      if (I != 0) {
        if (TAB[I].object == OBJECTS::PROZEDURE) {
          INSYMBOL();
          if (TAB[I].LEV != 0) {
            CALL(block_local, FSYS, I);
          } else {
            STANDPROC(block_local, TAB[I].address);
          }
        } else {
          EXPRESSION(block_local, FSYS, X);
          if (X.size > 1)
            EMIT1(112, X.size);
          EMIT(111);
        }
      } else
        INSYMBOL();
    } else if (SY == Symbol::LPARENT || SY == Symbol::INCREMENT ||
               SY == Symbol::DECREMENT || SY == Symbol::TIMES) {
      EXPRESSION(block_local, FSYS, X);
      EMIT(111);
    }
  }
  if (ASSIGNBEGSYS[SYMSAV] ||
      (SYMSAV == SYMBOL::DOSY || SYMSAV == SYMBOL::JOINSY ||
       SYMSAV == SYMBOL::RETURNSY || SYMSAV == SYMBOL::BREAKSY ||
       SYMSAV == SYMBOL::CONTSY || SYMSAV == SYMBOL::SEMICOLON ||
       SYMSAV == SYMBOL::MOVESY || SYMSAV == SYMBOL::CINSY ||
       SYMSAV == SYMBOL::COUTSY ||
       SYMSAV == SYMBOL::RSETBRACK)) // RSETBRACK added for while missing
                                     // statement DDE
  {
    if (SY == Symbol::SEMICOLON)
      INSYMBOL();
    else
      error(14);
  }
  su = 0;
  sv = FSYS;
  sv[symbol::ELSESY] = true; // added to fix else DDE
  // TEST(FSYS, su, 14);
  TEST(sv, su, 14);
}

void COMPOUNDSTATEMENT(BlockLocal *bl) {
  int64_t X;
  SymbolSet su, sv;
  INSYMBOL();
  su = DECLBEGSYS | STATBEGSYS | ASSIGNBEGSYS;
  // su[RSETBRACK] = true;  ??
  sv = bl->FSYS;
  sv[RSETBRACK] = true;
  TEST(su, sv, 123);
  while (DECLBEGSYS[SY] || STATBEGSYS[SY] || ASSIGNBEGSYS[SY]) {
    if (SY == SYMBOL::DEFINESY) {
      CONSTANTDECLARATION(bl);
    }
    if (SY == SYMBOL::TYPESY) {
      TYPEDECLARATION(bl);
    }
    if (SY == SYMBOL::STRUCTSY) {
      VARIABLEDECLARATION(bl);
    }
    if (SY == SYMBOL::IDENT) {
      X = LOC(bl, ID);
      if (X != 0) {
        if (TAB[X].OBJ == OBJECTS::TYPE1) {
          VARIABLEDECLARATION(bl);
        } else {
          su = bl->FSYS;
          su[SYMBOL::RSETBRACK] = true;
          su[SYMBOL::SEMICOLON] = true;
          STATEMENT(bl, su);
        }
      } else {
        INSYMBOL();
      }
    } else if (STATBEGSYS[SY] || ASSIGNBEGSYS[SY]) {
      su = bl->FSYS;
      su[SYMBOL::RSETBRACK] = true;
      su[SYMBOL::SEMICOLON] = true;
      STATEMENT(bl, su);
    }
    su = DECLBEGSYS | STATBEGSYS | ASSIGNBEGSYS;
    su[SYMBOL::RSETBRACK] = true;
    TEST(su, bl->FSYS, 6);
  }
  if (symbol_count == 1) {
    LOCATION[LNUM] = line_count;
  }
  if (SY == SYMBOL::RSETBRACK) {
    INSYMBOL();
  } else {
    error(104);
  }
}

void FIXBREAKS(int64_t LC1) {
  int64_t LC2;
  while (BREAKLOC[BREAKPNT] > 0) {
    LC2 = code[BREAKLOC[BREAKPNT]].Y;
    code[BREAKLOC[BREAKPNT]].Y = LC1;
    BREAKLOC[BREAKPNT] = LC2;
  }
  BREAKPNT = BREAKPNT - 1;
}

void FIXCONTS(int64_t LC1) {
  int64_t LC2;
  while (CONTLOC[CONTPNT] > 0) {
    LC2 = code[CONTLOC[CONTPNT]].Y;
    code[CONTLOC[CONTPNT]].Y = LC1;
    CONTLOC[CONTPNT] = LC2;
  }
  CONTPNT = CONTPNT - 1;
}

void IFSTATEMENT(BlockLocal *bl) {
  Item X = {Types::NOTYP, 0};
  int LC1, LC2;
  SymbolSet su;
  INSYMBOL();
  if (SY == Symbol::LPARENT) {
    INSYMBOL();
    su = bl->FSYS;
    su[symbol::RPARENT] = true;
    EXPRESSION(bl, su, X);
    if (!(X.types == Types::BOOLS || X.types == Types::INTS ||
          X.types == Types::NOTYP)) {
      error(17);
    }
    LC1 = line_count;
    EMIT(11);
    if (LOCATION[LNUM] == line_count - 1) {
      LOCATION[LNUM] = line_count;
    }
    if (SY == Symbol::RPARENT) {
      INSYMBOL();
    } else {
      error(4);
    }
    su = bl->FSYS;
    su[symbol::ELSESY] = true;
    STATEMENT(bl, su);
    if (SY == Symbol::ELSESY) {
      LC2 = line_count;
      EMIT(10);
      if (symbol_count == 1) {
        LOCATION[LNUM] = line_count;
      }
      code[LC1].Y = line_count;
      INSYMBOL();
      STATEMENT(bl, bl->FSYS);
      code[LC2].Y = line_count;
    } else {
      code[LC1].Y = line_count;
    }
  } else {
    error(9);
  }
}

void DOSTATEMENT(BlockLocal *bl) {
  Item X;
  int LC1, LC2, LC3;
  LC1 = line_count;
  SymbolSet su;
  INSYMBOL();
  BREAKPNT = BREAKPNT + 1;
  BREAKLOC[BREAKPNT] = 0;
  CONTPNT = CONTPNT + 1;
  CONTLOC[CONTPNT] = 0;
  su = bl->FSYS;
  su[symbol::SEMICOLON] = true;
  su[symbol::WHILESY] = true;
  STATEMENT(bl, su);
  LC2 = line_count;
  if (SY == Symbol::WHILESY) {
    if (symbol_count == 1) {
      LOCATION[LNUM] = line_count;
    }
    INSYMBOL();
    if (SY == Symbol::LPARENT) {
      INSYMBOL();
    } else {
      error(9);
    }
    su = bl->FSYS;
    su[symbol::RPARENT] = true;
    EXPRESSION(bl, su, X);
    if (!(X.types == Types::BOOLS || X.types == Types::INTS ||
          X.types == Types::NOTYP)) {
      error(17);
    }
    if (SY == Symbol::RPARENT) {
      INSYMBOL();
    } else {
      error(4);
    }
    EMIT1(11, line_count + 2);
    EMIT1(10, LC1);
  } else {
    error(53);
  }
  FIXBREAKS(line_count);
  FIXCONTS(LC2);
}

void WHILESTATEMENT(BlockLocal *bl) {
  Item X;
  int LC1, LC2, LC3;
  SymbolSet su;
  INSYMBOL();
  if (SY == Symbol::LPARENT) {
    INSYMBOL();
  } else {
    error(9);
  }
  LC1 = line_count;
  BREAKPNT = BREAKPNT + 1;
  BREAKLOC[BREAKPNT] = 0;
  CONTPNT = CONTPNT + 1;
  CONTLOC[CONTPNT] = 0;
  su = bl->FSYS;
  su[symbol::RPARENT] = true;
  EXPRESSION(bl, su, X);
  if (!(X.types == Types::BOOLS || X.types == Types::INTS ||
        X.types == Types::NOTYP)) {
    error(17);
  }
  LC2 = line_count;
  EMIT(11);
  if (SY == Symbol::RPARENT) {
    INSYMBOL();
  } else {
    error(4);
  }
  STATEMENT(bl, bl->FSYS);
  EMIT1(10, LC1);
  code[LC2].Y = line_count;
  if (symbol_count == 1) {
    LOCATION[LNUM] = line_count;
  }
  FIXBREAKS(line_count);
  FIXCONTS(line_count - 1);
}

void COMMAEXPR(BlockLocal *bl, Symbol TESTSY) {
  Item X;
  SymbolSet su;
  su = bl->FSYS;
  su[symbol::COMMA] = true;
  su[TESTSY] = true;
  EXPRESSION(bl, su, X);
  if (X.Size > 1) {
    EMIT1(112, X.Size);
  }
  EMIT(111);
  while (SY == Symbol::COMMA) {
    INSYMBOL();
    EXPRESSION(bl, su, X);
    if (X.Size > 1) {
      EMIT1(112, X.Size);
    }
    EMIT(111);
  }
}

void FORSTATEMENT(BlockLocal *bl) {
  Item X;
  int LC1, LC2, LC3;
  SymbolSet su;
  INSYMBOL();
  if (SY == Symbol::LPARENT) {
    INSYMBOL();
  } else {
    error(9);
  }
  BREAKPNT = BREAKPNT + 1;
  BREAKLOC[BREAKPNT] = 0;
  CONTPNT = CONTPNT + 1;
  CONTLOC[CONTPNT] = 0;
  if (SY != Symbol::SEMICOLON) {
    COMMAEXPR(bl, Symbol::SEMICOLON);
  }
  if (SY == Symbol::SEMICOLON) {
    INSYMBOL();
  } else {
    error(14);
  }
  LC1 = line_count;
  if (SY != Symbol::SEMICOLON) {
    su = bl->FSYS;
    su[symbol::SEMICOLON] = true;
    EXPRESSION(bl, su, X);
    if (!(X.types == Types::BOOLS || X.types == Types::INTS ||
          X.types == Types::NOTYP)) {
      error(17);
    }
  } else {
    EMIT1(24, 1);
  }
  if (SY == Symbol::SEMICOLON) {
    INSYMBOL();
  } else {
    error(14);
  }
  LC2 = line_count;
  EMIT(11);
  EMIT(10);
  LC3 = line_count;
  if (SY != Symbol::RPARENT) {
    COMMAEXPR(bl, Symbol::RPARENT);
  }
  EMIT1(10, LC1);
  if (SY == Symbol::RPARENT) {
    INSYMBOL();
  } else {
    error(4);
  }
  code[LC2 + 1].Y = line_count;
  STATEMENT(bl, bl->FSYS);
  EMIT1(10, LC3);
  code[LC2].Y = line_count;
  if (symbol_count == 1) {
    LOCATION[LNUM] = line_count;
  }
  FIXBREAKS(line_count);
  FIXCONTS(line_count - 1);
}

void BLOCKSTATEMENT(BlockLocal *bl) {
  Types CVT;
  Item X;
  SymbolSet su, sv;
  int64_t SAVEDX, SAVENUMWITH, SAVEMAXNUMWITH, PRB, PRT, I;
  bool TCRFLAG;
  // INSYMBOL();
  ID = dummy_name;
  ENTER(bl, ID, OBJECTS::PROZEDURE);
  dummy_name[13] = static_cast<char>(dummy_name[13] + 1);
  if (dummy_name[13] == '0') {
    dummy_name[12] = static_cast<char>(dummy_name[12] + 1);
  }
  TAB[tab_index].normal = true;
  SAVEDX = bl->DX;
  SAVENUMWITH = bl->NUMWITH;
  SAVEMAXNUMWITH = bl->MAXNUMWITH;
  bl->LEVEL = bl->LEVEL + 1;
  bl->DX = BASESIZE;
  PRT = tab_index;
  if (bl->LEVEL > LMAX) {
    fatal(5);
  }
  bl->NUMWITH = 0;
  bl->MAXNUMWITH = 0;
  ENTERBLOCK();
  DISPLAY[bl->LEVEL] = btab_index;
  PRB = btab_index;
  TAB[PRT].types = Types::NOTYP;
  TAB[PRT].reference = PRB;
  BTAB[PRB].LASTPAR = tab_index;
  BTAB[PRB].PSIZE = bl->DX;
  su = 0;
  su[symbol::LSETBRACK] = true;
  sv = DECLBEGSYS | STATBEGSYS | ASSIGNBEGSYS;
  TEST(su, sv, 3);
  INSYMBOL();
  EMIT1(18, PRT);
  EMIT1(19, BTAB[PRB].PSIZE - 1);
  TAB[PRT].ADR = line_count;
  su = bl->FSYS;
  su[symbol::RSETBRACK] = true;
  // sv = DECLBEGSYS | STATBEGSYS | ASSIGNBEGSYS;
  TEST(sv, su, 101);
  // while (DECLBEGSYS[SY] || STATBEGSYS[SY] || ASSIGNBEGSYS[SY])
  while (sv[SY]) {
    if (SY == Symbol::DEFINESY) {
      CONSTANTDECLARATION(bl);
    } else if (SY == Symbol::TYPESY) {
      TYPEDECLARATION(bl);
    } else if (SY == Symbol::STRUCTSY) {
      VARIABLEDECLARATION(bl);
    } else if (SY == Symbol::IDENT) {
      I = LOC(bl, ID);
      if (I != 0) {
        if (TAB[I].OBJ == TYPE1) {
          VARIABLEDECLARATION(bl);
        } else {
          su = bl->FSYS;
          su[symbol::SEMICOLON] = true;
          su[symbol::RSETBRACK] = true;
          STATEMENT(bl, su);
        }
      } else {
        INSYMBOL();
      }
    } else if (STATBEGSYS[SY] || ASSIGNBEGSYS[SY]) {
      su = bl->FSYS;
      su[symbol::SEMICOLON] = true;
      su[symbol::RSETBRACK] = true;
      STATEMENT(bl, su);
    }
    su = DECLBEGSYS | STATBEGSYS | ASSIGNBEGSYS;
    su[symbol::RSETBRACK] = true;
    TEST(su, bl->FSYS, 6);
  }
  BTAB[PRB].VSIZE = bl->DX;
  BTAB[PRB].VSIZE = BTAB[PRB].VSIZE + bl->MAXNUMWITH;
  if (symbol_count == 1) {
    LOCATION[LNUM] = line_count;
  }
  if (SY == Symbol::RSETBRACK) {
    INSYMBOL();
  } else {
    error(7);
  }
  su = bl->FSYS;
  su[symbol::PERIOD] = true;
  sv = 0;
  TEST(su, sv, 6);
  EMIT(105);
  bl->DX = SAVEDX;
  bl->NUMWITH = SAVENUMWITH;
  bl->MAXNUMWITH = SAVEMAXNUMWITH;
  bl->LEVEL = bl->LEVEL - 1;
}

void FORALLSTATEMENT(BlockLocal *bl) {
  Types CVT;
  Item X;
  int64_t I, J, LC1, LC2, LC3;
  // bool DEFOUND;
  bool TCRFLAG;
  SymbolSet su;
  bl->FLEVEL = bl->FLEVEL + 1;
  INSYMBOL();
  if (SY == Symbol::IDENT) {
    I = LOC(bl, ID);
    TAB[I].FORLEV = bl->FLEVEL;
    INSYMBOL();
    if (I == 0) {
      CVT = Types::INTS;
    } else {
      if (TAB[I].object == OBJECTS::VARIABLE) {
        CVT = TAB[I].types;
        if (!TAB[I].normal) {
          error(26);
        } else {
          EMIT2(0, TAB[I].LEV, TAB[I].address);
        }
        if (!(CVT == Types::NOTYP || CVT == Types::INTS ||
              CVT == Types::BOOLS || CVT == Types::CHARS)) {
          error(18);
        }
      } else {
        error(37);
        CVT = Types::INTS;
      }
    }
  } else {
    su = bl->FSYS;
    su[symbol::BECOMES] = true;
    su[symbol::TOSY] = true;
    su[symbol::DOSY] = true;
    su[symbol::GROUPINGSY] = true;
    SKIP(su, 2);
    CVT = Types::INTS;
  }
  if (SY == Symbol::BECOMES) {
    INSYMBOL();
    su = bl->FSYS;
    su[symbol::TOSY] = true;
    su[symbol::DOSY] = true;
    su[symbol::GROUPINGSY] = true;
    EXPRESSION(bl, su, X);
    if (X.types != CVT) {
      error(19);
    }
  } else {
    su = bl->FSYS;
    su[symbol::TOSY] = true;
    su[symbol::DOSY] = true;
    su[symbol::GROUPINGSY] = true;
    SKIP(su, 51);
  }
  if (SY == Symbol::TOSY) {
    INSYMBOL();
    su = bl->FSYS;
    su[symbol::ATSY] = true;
    su[symbol::DOSY] = true;
    su[symbol::GROUPINGSY] = true;
    EXPRESSION(bl, su, X);
    if (X.types != CVT) {
      error(19);
    }
  } else {
    su = bl->FSYS;
    su[symbol::ATSY] = true;
    su[symbol::DOSY] = true;
    su[symbol::GROUPINGSY] = true;
    SKIP(su, 55);
  }
  if (SY == Symbol::GROUPINGSY) {
    INSYMBOL();
    su = bl->FSYS;
    su[symbol::ATSY] = true;
    su[symbol::DOSY] = true;
    EXPRESSION(bl, su, X);
    if (X.types != Types::INTS) {
      error(45);
    }
  } else {
    EMIT1(24, 1);
  }
  EMIT(4);
  LC1 = line_count;
  EMIT(75);
  LC2 = line_count;
  TAB[I].FORLEV = -TAB[I].FORLEV;
  if (SY == Symbol::ATSY) {
    INSYMBOL();
    su = bl->FSYS;
    su[symbol::DOSY] = true;
    EXPRESSION(bl, su, X);
    if (!(X.types == Types::INTS || X.types == Types::NOTYP)) {
      error(126);
    }
  } else {
    EMIT(79);
  }
  if (SY == Symbol::DOSY) {
    INSYMBOL();
  } else {
    error(54);
  }
  TAB[I].FORLEV = -TAB[I].FORLEV;
  bl->CREATEFLAG = false;
  if (SY == Symbol::IDENT) {
    J = LOC(bl, ID);
    if (J != 0) {
      if (TAB[J].object == OBJECTS::PROZEDURE && TAB[J].LEV != 0) {
        bl->CREATEFLAG = true;
      }
    }
  }
  if (bl->CREATEFLAG) {
    EMIT1(74, 1);
  } else {
    EMIT1(74, 0);
  }
  TCRFLAG = bl->CREATEFLAG;
  LC3 = line_count;
  EMIT1(10, 0);
  if (SY == Symbol::LSETBRACK) {
    BLOCKSTATEMENT(bl);
  } else {
    su = bl->FSYS;
    STATEMENT(bl, su);
  }
  TAB[I].FORLEV = 0;
  if (TCRFLAG) {
    EMIT2(104, LC3 + 1, 1);
  } else {
    EMIT2(104, LC3 + 1, 0);
  }
  EMIT(70);
  code.at(LC3).Y = line_count;
  EMIT1(76, LC2);
  code.at(LC1).Y = line_count;
  EMIT(5);
  bl->FLEVEL = bl->FLEVEL - 1;
  if (symbol_count == 1) {
    LOCATION[LNUM] = line_count;
  }
}

void FORKSTATEMENT(BlockLocal *bl) {
  Item X;
  int I; // added local (unlikely intended to use global)
  int JUMPLC;
  SymbolSet su;
  bl->CREATEFLAG = false;
  EMIT(106);
  INSYMBOL();
  if (SY == Symbol::LPARENT) {
    INSYMBOL();
    if (SY == Symbol::ATSY) {
      INSYMBOL();
      su = bl->FSYS;
      su[symbol::RPARENT] = true;
      EXPRESSION(bl, su, X);
      if (!(X.types == Types::INTS || X.types == Types::NOTYP)) {
        error(126);
      }
    } else {
      error(127);
    }
    if (SY == Symbol::RPARENT) {
      INSYMBOL();
    } else {
      error(4);
    }
  } else {
    EMIT(79);
  }
  if (SY == Symbol::IDENT) {
    I = LOC(bl, ID);
    if (I != 0) {
      if (TAB[I].object == OBJECTS::PROZEDURE && TAB[I].LEV != 0) {
        bl->CREATEFLAG = true;
      }
    }
  }
  if (bl->CREATEFLAG) {
    EMIT1(67, 1);
  } else {
    EMIT1(67, 0);
  }
  JUMPLC = line_count;
  EMIT1(7, 0);
  STATEMENT(bl, bl->FSYS);
  EMIT(69);
  code.at(JUMPLC).Y = line_count;
  if (symbol_count == 1) {
    LOCATION[LNUM] = line_count;
  }
}

void JOINSTATEMENT(BlockLocal *bl) {
  EMIT(85);
  INSYMBOL();
}

void RETURNSTATEMENT(BlockLocal *bl) {
  Item X;
  INSYMBOL();
  if (RETURNTYPE.types != Types::VOIDS) {
    EXPRESSION(bl, bl->FSYS, X);
    if (X.types == Types::RECS || X.types == Types::ARRAYS) {
      EMIT1(113, X.size);
    }
    if (!TYPE_COMPATIBLE(ReturnType, X)) {
      error(118);
    } else {
      if (RETURNTYPE.types == Types::REALS && X.types == Types::INTS) {
        EMIT(26);
      }
      EMIT(33);
    }
  } else {
    EMIT(32);
  }
}

void BREAKSTATEMENT(BlockLocal *bl) {
  EMIT1(10, BREAKLOC[BREAKPNT]);
  BREAKLOC[BREAKPNT] = line_count - 1;
  INSYMBOL();
}

void CONTINUESTATEMENT() {
  EMIT1(10, CONTLOC[CONTPNT]);
  CONTLOC[CONTPNT] = line_count - 1;
  INSYMBOL();
}

void MOVESTATEMENT(BlockLocal *bl) {
  Item X;
  SymbolSet su;
  INSYMBOL();
  su = bl->FSYS;
  su[symbol::TOSY] = true;
  BASICEXPRESSION(bl, su, X);
  if (!(X.ISADDR && X.types == Types::CHANS)) {
    error(103);
  }
  if (SY == Symbol::TOSY) {
    INSYMBOL();
    su = bl->FSYS;
    su[symbol::SEMICOLON] = true;
    EXPRESSION(bl, su, X);
    if (!(X.types == Types::INTS || X.types == Types::NOTYP)) {
      error(126);
    }
  } else {
    EMIT1(8, 19);
  }
  EMIT(115);
}

//    void INPUTSTATEMENT(BlockLocal *bl)
//    {
//        ITEM X;
//        SYMSET su;
//        INSYMBOL();
//        if (SY == INSTR)
//        {
//            do
//            {
//                INSYMBOL();
//                if (SY == ENDLSY)
//                {
//                    EMIT(63);
//                    INSYMBOL();
//                } else if (SY == STRNG)
//                {
//                    EMIT1(24, SLENG);
//                    EMIT1(28, INUM);
//                    INSYMBOL();
//                } else
//                {
//                    su = bl->FSYS;
//                    su[INSTR] = true;
//                    su[SEMICOLON] = true;
//                    EXPRESSION(bl, su, X);
//                    if (!X.ISADDR)
//                    {
//                        error(131);
//                    } else
//                    {
//                        if (X.TYP == INTS || X.TYP == REALS || X.TYP == CHARS
//                        || X.TYP == NOTYP)
//                        {
//                            EMIT1(27, (int)X.TYP);
//                        } else
//                        {
//                            error(40);
//                        }
//                    }
//                }
//            } while (SY == INSTR);
//        } else
//        {
//            error(129);
//        }
//    }
void INPUTSTATEMENT(BlockLocal *bl) {
  Item X;
  SymbolSet su;
  INSYMBOL();
  if (SY == Symbol::INSTR) {
    do {
      INSYMBOL();
      su = bl->FSYS;
      su[symbol::INSTR] = true;
      su[symbol::SEMICOLON] = true;
      BASICEXPRESSION(bl, su, X);
      if (!X.is_address) {
        error(131);
      } else {
        if (X.types == Types::INTS || X.types == Types::REALS ||
            X.types == Types::CHARS || X.types == Types::NOTYP) {
          EMIT1(27, (int)X.types);
        } else {
          error(40);
        }
      }
    } while (SY == Symbol::INSTR);
  } else {
    error(129);
  }
}

void COUTMETHODS(BlockLocal *bl) {
  ALFA METHOD;
  Item X;
  SymbolSet su;
  INSYMBOL();
  METHOD = ID;
  if (SY != Symbol::IDENT || (strcmp(ID.c_str(), "WIDTH         ") != 0 &&
                              strcmp(ID.c_str(), "PRECISION     ") != 0)) {
    error(133);
  } else {
    INSYMBOL();
    if (SY == Symbol::LPARENT) {
      INSYMBOL();
    } else {
      error(9);
    }
    su = bl->FSYS;
    su[symbol::RPARENT] = true;
    EXPRESSION(bl, su, X);
    if (X.types != Types::INTS) {
      error(134);
    }
    if (SY != Symbol::RPARENT) {
      error(4);
    }
    if (strcmp(METHOD.c_str(), "WIDTH         ") == 0) {
      EMIT1(30, 1);
    } else {
      EMIT1(30, 2);
    }
  }
  INSYMBOL();
}

void OUTPUTSTATEMENT(BlockLocal *bl) {
  Item X;
  SymbolSet su;
  INSYMBOL();
  if (SY == Symbol::PERIOD) {
    COUTMETHODS(bl);
  } else {
    EMIT(89);
    if (SY == Symbol::OUTSTR) {
      do {
        INSYMBOL();
        if (SY == Symbol::ENDLSY) {
          EMIT(63);
          INSYMBOL();
        } else if (SY == Symbol::STRNG) {
          EMIT1(24, SLENG);
          EMIT1(28, INUM);
          INSYMBOL();
        } else {
          su = bl->FSYS;
          su[symbol::OUTSTR] = true;
          su[symbol::SEMICOLON] = true;
          EXPRESSION(bl, su, X);
          if (!(STANTYPS[X.TYP])) {
            error(41);
          }
          if (X.types == Types::REALS) {
            EMIT(37);
          } else {
            EMIT1(29, static_cast<int>(X.types));
          }
        }
      } while (SY == Symbol::OUTSTR);
    } else {
      error(128);
    }
    EMIT(90);
  }
}
/* declared in SWITCHSTATEMENT therefore available to the contained functions
 * ala Pascal */
struct SwitchLocal {
  Item X;
  int I, J, K, LC1, LC2, LC3;
  bool DEFOUND;
  struct {
    Index VAL, line_count;
  } __attribute__((aligned(8))) CASETAB[CSMAX + 1];

  std::array<int, CSMAX + 1> EXITTAB;
  BlockLocal *bl;
} __attribute__((aligned(128))) __attribute__((packed));

static void CASELABEL(SwitchLocal * /*sl*/);
static void ONECASE(SwitchLocal * /*sl*/);

void CASELABEL(SwitchLocal *switch_local) {
  CONREC LAB = {Types::NOTYP, 0L};
  int64_t K = 0;
  SymbolSet su;
  su = switch_local->bl->FSYS;
  su[symbol::COLON] = true;
  CONSTANT(switch_local->bl, su, LAB);
  if (LAB.TP != switch_local->X.types) {
    error(47);
  } else if (switch_local->I == CSMAX) {
    fatal(6);
  } else {
    switch_local->I = switch_local->I + 1;
    K = 0;
    switch_local->CASETAB[switch_local->I].VAL = static_cast<int>(LAB.I);
    switch_local->CASETAB[switch_local->I].line_count = line_count;
    do {
      K = K + 1;
    } while (switch_local->CASETAB[K].VAL != LAB.I);
    if (K < switch_local->I) {
      error(1);
    }
  }
}

void ONECASE(SwitchLocal *switch_local) {
  SymbolSet su;
  if (CONSTBEGSYS[SY]) {
    CASELABEL(switch_local);
    if (SY == Symbol::COLON) {
      INSYMBOL();
    } else {
      error(5);
    }

    su = switch_local->bl->FSYS;
    su[symbol::CASESY] = true;
    su[symbol::SEMICOLON] = true;
    su[symbol::DEFAULTSY] = true;
    su[symbol::RSETBRACK] = true;

    while (STATBEGSYS[SY] || ASSIGNBEGSYS[SY]) {
      STATEMENT(switch_local->bl, su);
    }

    su = 0;
    su[symbol::CASESY] = true;
    su[symbol::DEFAULTSY] = true;
    su[symbol::RSETBRACK] = true;
    TEST(su, switch_local->bl->FSYS, 125);
  }
}

void SWITCHSTATEMENT(BlockLocal *block_local) {
  // int K;
  struct SwitchLocal switch_local {};
  memset(&switch_local, 0, sizeof(struct SwitchLocal));

  switch_local.bl = block_local;
  SymbolSet su;

  BREAKPNT = BREAKPNT + 1;
  BREAKLOC[BREAKPNT] = 0;
  INSYMBOL();

  if (SY == Symbol::LPARENT) {
    INSYMBOL();
  } else {
    error(9);
  }

  switch_local.I = 0;
  su = block_local->FSYS;
  su[symbol::RPARENT] = true;
  su[symbol::COLON] = true;
  su[symbol::LSETBRACK] = true;
  su[symbol::CASESY] = true;
  EXPRESSION(block_local, su, switch_local.X);

  if (switch_local.X.types != Types::INTS &&
      switch_local.X.types != Types::BOOLS &&
      switch_local.X.types != Types::CHARS &&
      switch_local.X.types != Types::NOTYP) {
    error(23);
  }

  if (SY == Symbol::RPARENT) {
    INSYMBOL();
  } else {
    error(4);
  }

  switch_local.LC1 = line_count;
  EMIT(12);

  if (SY == Symbol::LSETBRACK) {
    INSYMBOL();
  } else {
    error(106);
  }
  if (SY == Symbol::CASESY) {
    INSYMBOL();
  } else {
    error(124);
  }

  ONECASE(&switch_local);

  while (SY == Symbol::CASESY) {
    INSYMBOL();
    ONECASE(&switch_local);
  }

  switch_local.DEFOUND = false;
  if (SY == Symbol::DEFAULTSY) {
    INSYMBOL();

    if (SY == Symbol::COLON) {
      INSYMBOL();
    } else {
      error(5);
    }

    switch_local.LC3 = line_count;

    while (STATBEGSYS[SY] || ASSIGNBEGSYS[SY]) {
      su = block_local->FSYS;
      su[symbol::SEMICOLON] = true;
      su[symbol::RSETBRACK] = true;
      STATEMENT(block_local, su);
    }
    switch_local.DEFOUND = true;
  }

  switch_local.LC2 = line_count;
  EMIT(10);
  code.at(switch_local.LC1).Y = line_count;

  for (switch_local.K = 1; switch_local.K <= switch_local.I; switch_local.K++) {
    EMIT1(13, switch_local.CASETAB[switch_local.K].VAL);
    EMIT1(13, switch_local.CASETAB[switch_local.K].line_count);
  }

  if (switch_local.DEFOUND) {
    EMIT2(13, -1, 0);
    EMIT1(13, switch_local.LC3);
  }

  EMIT1(10, 0);
  code.at(switch_local.LC2).Y = line_count;
  if (symbol_count == 1) {
    LOCATION[LNUM] = line_count;
  }

  if (SY == Symbol::RSETBRACK) {
    INSYMBOL();
  } else {
    error(104);
  }

  FIXBREAKS(line_count);
}

} // namespace Cstar
