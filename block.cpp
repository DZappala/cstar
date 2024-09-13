//
// Created by Dan Evans on 1/1/24.
//
#include "cs_basic.h"
#include "cs_block.h"
#include "cs_compile.h"
#include "cs_errors.h"
#include "cs_global.h"
#include "cs_interpret.h"
#include <cstring>

namespace cs {
using OptionsLocal = struct {
  Symbol MYSY;
};
extern auto inCLUDEFLAG() -> bool;
extern auto RESULTTYPE(Types A, Types B) -> Types;
extern auto SUMRESULTTYPE(Types A, Types B) -> Types;
extern auto COMPATIBLE(Item X, Item Y) -> bool;
extern auto TYPE_COMPATIBLE(Item X, Item Y) -> bool;
extern auto ARRAY_COMPATIBLE(Item X, Item Y) -> bool;
extern auto PNT_COMPATIBLE(Item X, Item Y) -> bool;
extern void CONSTANTDECLARATION(BlockLocal *);
extern void TYPEDECLARATION(BlockLocal *);
extern void VARIABLEDECLARATION(BlockLocal *);
extern auto INCLUDEDIRECTIVE() -> bool;
extern void STATEMENT(BlockLocal *, SymbolSet &);
extern void CHANELEMENT(Item &);
extern void ENTERCHANNEL();
extern void C_PNTCHANTYP(BlockLocal *, Types &, int64_t &, int64_t &);
extern void C_ARRATYP(BlockLocal *, int64_t &, int64_t &, bool FIRSTINDEX,
                      Types, int64_t, int64_t);
extern void TYPF(BlockLocal *block_local, SymbolSet FSYS, Types &types,
                 int64_t &RF, int64_t &SZ);
void FACTOR(BasicLocal *, SymbolSet &, Item &);
void EXPRESSION(BlockLocal * /*bl*/, SymbolSet & /*FSYS*/, Item & /*X*/);
void PARAMETERLIST(BlockLocal *);
void ASSIGNEXPRESSION(BasicLocal * /*bx*/, SymbolSet & /*FSYS*/, Item & /*X*/);
static const int NWORDS = 9;
// static int F;  // decl in BASICEXPRESSION, set in FACTOR

void ENTERBLOCK() {
  if (btab_index == BMAX) {
    fatal(2);
  } else {
    btab_index++;
    BTAB[btab_index].LAST = 0;
    BTAB[btab_index].LASTPAR = 0;
  }
}
void SKIP(SymbolSet FSYS, int num) {
  error(num);
  while (symbol != Symbol::EOFSY && !FSYS[static_cast<int>(symbol)]) {
    INSYMBOL();
  }
}
void TEST(SymbolSet &S1, SymbolSet &S2, int num) {
  if (!S1[static_cast<int>(symbol)]) {
    SymbolSet su;
    // std::set_union(S1.begin(), S1.end(), S2.begin(), S2.end(), su);
    su = S1 | S2;
    // SKIP(S1 + S2, N);
    SKIP(su, num);
  }
}
void TESTSEMICOLON(BlockLocal *block_local) {
  SymbolSet su;
  if (symbol == Symbol::SEMICOLON) {
    INSYMBOL();
  } else {
    error(14);
    if (symbol == Symbol::COMMA || symbol == Symbol::COLON) {
      INSYMBOL();
    }
  }
  su = declaration_set | keyword_set | assigners_set;
  su.set(static_cast<int>(Symbol::INCLUDESY), true);
  TEST(su, block_local->FSYS, 6);
}
void ENTER(BlockLocal *block_local, const ALFA &identifier, OBJECTS objects) {
  // int ix;
  int J, L;
  if (tab_index == TMAX) {
    fatal(1);
  } else {
    // std::cout << ID << std::endl;
    TAB[0].name = identifier;
    if (objects == OBJECTS::COMPONENT) {
      J = block_local->RLAST;
    } else {
      J = BTAB[DISPLAY[block_local->LEVEL]].LAST;
    }
    L = J;
    while (strcmp(TAB[J].name.c_str(), identifier.c_str()) != 0) {
      J = TAB[J].link;
    }
    if (J != 0) {
      if (((TAB[J].object == OBJECTS::FUNKTION) ||
           (TAB[J].object == OBJECTS::PROZEDURE)) &&
          (TAB[J].address == 0)) {
        proto_index = J;
      } else {
        error(1);
      }
    } else {
      tab_index++;
      TAB[tab_index].name = identifier;
      TAB[tab_index].link = L;
      TAB[tab_index].object = objects;
      TAB[tab_index].types = Types::NOTYP;
      TAB[tab_index].FORLEV = 0;
      TAB[tab_index].reference = 0;
      TAB[tab_index].LEV = block_local->LEVEL;
      TAB[tab_index].address = 0;
      if (objects == OBJECTS::COMPONENT) {
        block_local->RLAST = tab_index;
      } else {
        BTAB[DISPLAY[block_local->LEVEL]].LAST = tab_index;
      }
    }
  }
}
auto LOC(BlockLocal *bl, const ALFA &ID) -> int64_t {
  int I, J;
  bool FOUND;
  TAB[0].name = ID;

  I = bl->NUMWITH;
  FOUND = false;
  while (I > 0 && !FOUND) {
    J = WITHTAB[I];
    while (strcmp(TAB[J].name.c_str(), ID.c_str()) != 0) {
      J = TAB[J].link;
    }
    if (J != 0) {
      FOUND = true;
      EMIT2(1, bl->LEVEL, bl->DX + I - 1);
      EMIT1(24, TAB[J].address);
      EMIT(15);
    }
    I = I - 1;
  }
  if (!FOUND) {
    I = bl->LEVEL;
    do {
      J = BTAB[DISPLAY[I]].LAST;
      while (strcmp(TAB[J].name.c_str(), ID.c_str()) != 0) {
        J = TAB[J].link;
      }
      I = I - 1;
    } while (I >= 0 && J == 0);
  }
  //        fprintf(STDOUT, "lookup %s\n", ID);
  //        if (J != 0 && strcmp(ID, "SIZEOF        ") == 0)
  //            fprintf(STDOUT, "sizeof %d\n", J);
  if (J == 0 && bl->UNDEFMSGFLAG) {
    error(0);
  }
  return J;
}
void ENTERVARIABLE(BlockLocal *bl, OBJECTS KIND) {
  if (symbol == Symbol::IDENT) {
    ENTER(bl, ID, KIND);
    INSYMBOL();
  } else {
    error(2);
  }
}
void CONSTANT(BlockLocal *bl, SymbolSet &FSYS, CONREC &C) {
  int X;
  int sign;
  CONREC NEXTC = {Types::NOTYP, 0l};
  Symbol OPSYM;
  SymbolSet su;
  C.TP = Types::NOTYP;
  C.I = 0;
  TEST(base_set, FSYS, 50);
  if (base_set[static_cast<int>(symbol)]) {
    if (symbol == Symbol::CHARCON) {
      C.TP = Types::CHARS;
      C.I = INUM;
      INSYMBOL();
    } else {
      sign = 1;
      if (symbol == Symbol::PLUS || symbol == Symbol::MINUS) {
        if (symbol == Symbol::MINUS) {
          sign = -1;
        }
        INSYMBOL();
      }
      if (symbol == Symbol::IDENT) {
        X = LOC(bl, ID);
        if (X != 0) {
          if (TAB[X].object != OBJECTS::KONSTANT) {
            error(25);
          } else {
            C.TP = TAB[X].types;
            if (C.TP == Types::REALS) {
              C.I = CPNT;
              CONTABLE[CPNT] = sign * CONTABLE[TAB[X].address];
              if (CPNT >= RCMAX) {
                fatal(10);
              } else {
                CPNT = CPNT + 1;
              }
            } else {
              C.I = sign * TAB[X].address;
            }
          }
        }
        INSYMBOL();
      } else if (symbol == Symbol::INTCON) {
        C.TP = Types::INTS;
        C.I = sign * INUM;
        INSYMBOL();
      } else if (symbol == Symbol::REALCON) {
        C.TP = Types::REALS;
        CONTABLE[CPNT] = sign * RNUM;
        C.I = CPNT;
        if (CPNT >= RCMAX) {
          fatal(10);
        } else {
          CPNT = CPNT + 1;
        }
        INSYMBOL();
      } else {
        SKIP(FSYS, 50);
      }
      if (C.TP == Types::INTS &&
          (symbol == Symbol::PLUS || symbol == Symbol::MINUS ||
           symbol == Symbol::TIMES)) {
        OPSYM = symbol;
        INSYMBOL();
        CONSTANT(bl, FSYS, NEXTC);
        if (NEXTC.TP != Types::INTS) {
          error(25);
        } else {
          switch (OPSYM) {
          case Symbol::PLUS:
            C.I = C.I + NEXTC.I;
            break;
          case Symbol::MINUS:
            C.I = C.I - NEXTC.I;
            break;
          case Symbol::TIMES:
            C.I = C.I * NEXTC.I;
            break;
          default:
            break;
          }
        }
      }
    }
    su = 0;
    TEST(FSYS, su, 6);
  }
}
void IDENTIFY(OptionsLocal *ol, const ALFA *WORD, const Symbol *SYM) {
  int64_t I = 1;
  bool FOUND = false;
  do {
    if (strcmp(WORD[I].c_str(), ID.c_str()) == 0) {
      FOUND = true;
    } else {
      I = I + 1;
    }
  } while (!FOUND && I <= NWORDS);
  if (I <= NWORDS) {
    ol->MYSY = SYM[I];
  } else {
    ol->MYSY = Symbol::ERRSY;
  }
}
void OPTIONS(BlockLocal *bl) {
  ALFA WORD[NWORDS + 1];
  Symbol SYM[NWORDS + 1];
  CONREC C = {Types::NOTYP, 0l};
  int64_t J, K;
  OptionsLocal ol;
  SymbolSet su;
  ol.MYSY = Symbol::INTCON; // initial value only
  topology = Symbol::SHAREDSY;
  TOPDIM = PMAX;
  if (symbol == Symbol::IDENT && strcmp(ID.c_str(), "ARCHITECTURE  ") == 0) {
    WORD[1] = "SHARED        ";
    SYM[1] = Symbol::SHAREDSY;
    WORD[2] = "FULLCONNECT   ";
    SYM[2] = Symbol::FULLCONNSY;
    WORD[3] = "HYPERCUBE     ";
    SYM[3] = Symbol::HYPERCUBESY;
    WORD[4] = "LINE          ";
    SYM[4] = Symbol::LINESY;
    WORD[5] = "MESH2         ";
    SYM[5] = Symbol::MESH2SY;
    WORD[6] = "MESH3         ";
    SYM[6] = Symbol::MESH3SY;
    WORD[7] = "RING          ";
    SYM[7] = Symbol::RINGSY;
    WORD[8] = "TORUS         ";
    SYM[8] = Symbol::TORUSSY;
    WORD[9] = "CLUSTER       ";
    SYM[9] = Symbol::CLUSTERSY;
    INSYMBOL();
    if (symbol == Symbol::IDENT) {
      IDENTIFY(&ol, WORD, SYM);
      if (ol.MYSY != Symbol::ERRSY) {
        INSYMBOL();
        topology = ol.MYSY;
        if (symbol == Symbol::LPARENT) {
          INSYMBOL();
          su = bl->FSYS;
          su.set(static_cast<int>(Symbol::SEMICOLON), true);
          su.set(static_cast<int>(Symbol::COMMA), true);
          su.set(static_cast<int>(Symbol::RPARENT), true);
          su.set(static_cast<int>(Symbol::IDENT), true);
          CONSTANT(bl, su, C);
          if (C.TP == Types::INTS && C.I >= 0) {
            TOPDIM = C.I;
          } else {
            error(43);
          }
          if (symbol == Symbol::RPARENT) {
            INSYMBOL();
          } else {
            error(4);
          }
        } else {
          su = bl->FSYS;
          su.set(static_cast<int>(Symbol::RPARENT), true);
          su.set(static_cast<int>(Symbol::SEMICOLON), true);
          su.set(static_cast<int>(Symbol::IDENT), true);
          SKIP(su, 9);
        }
      } else {
        INSYMBOL();
        su = 0;
        su.set(static_cast<int>(Symbol::SEMICOLON), true);
        SKIP(su | declaration_set, 42);
      }
      TESTSEMICOLON(bl);
    } else {
      SKIP(declaration_set, 42);
    }
  }
  switch (topology) {
  case Symbol::SHAREDSY:
  case Symbol::FULLCONNSY:
  case Symbol::CLUSTERSY:
    highest_processor = TOPDIM - 1;
    break;
  case Symbol::HYPERCUBESY:
    K = 1;
    for (J = 1; J <= TOPDIM; J++) {
      K = K * 2;
    }
    highest_processor = K - 1;
    break;
  case Symbol::LINESY:
  case Symbol::RINGSY:
    highest_processor = TOPDIM - 1;
    break;
  case Symbol::MESH2SY:
  case Symbol::TORUSSY:
    highest_processor = TOPDIM * TOPDIM - 1;
    break;
  case Symbol::MESH3SY:
    highest_processor = TOPDIM * TOPDIM * TOPDIM - 1;
    break;
  default:
    break;
  }
  if (mpi_mode && topology == Symbol::SHAREDSY) {
    error(144);
  }
}
void LOADVAL(Item &X) {
  if (standard_set.test(static_cast<int>(X.types)) || X.types == Types::PNTS ||
      X.types == Types::LOCKS) {
    EMIT(34);
  }
  if (X.types == Types::CHANS) {
    error(143);
  }
  X.is_address = false;
}
void BASICEXPRESSION(BlockLocal *bl, SymbolSet FSYS, Item &X) {
  // int F;
  // ITEM Y;
  // SYMBOL OP;
  BasicLocal bx;
  bx.factor = 0;
  bx.item = {Types::NOTYP, 0, 0, false};
  bx.operation = Symbol::INTCON; // initial value only, no meaning
  bx.block_local = bl;
  SymbolSet su = FSYS;
  ASSIGNEXPRESSION(&bx, su, X);
}

void EXPRESSION(BlockLocal *bl, SymbolSet &FSYS, Item &X) {
  Item Y = {Types::NOTYP, 0, 0, false};
  Symbol OP;
  bool ADDR_REQUIRED;
  SymbolSet NXT;
  int64_t RF, SZ;
  Types TP;
  if (symbol == Symbol::ADDRSY) {
    ADDR_REQUIRED = true;
    INSYMBOL();
  } else
    ADDR_REQUIRED = false;
  if (symbol == Symbol::NEWSY) {
    INSYMBOL();
    NXT = FSYS;
    NXT.set(static_cast<int>(Symbol::TIMES), true);
    NXT.set(static_cast<int>(Symbol::CHANSY), true);
    NXT.set(static_cast<int>(Symbol::LBRACK), true);
    TYPF(bl, NXT, TP, RF, SZ);
    C_PNTCHANTYP(bl, TP, RF, SZ);
    ENTERCHANNEL();
    CTAB[ctab_index].ELTYP = X.types;
    CTAB[ctab_index].ELREF = X.reference;
    CTAB[ctab_index].ELSIZE = (int)X.size;
    X.types = Types::PNTS;
    X.size = 1;
    X.reference = ctab_index;
    EMIT1(99, SZ);
  } else {
    BASICEXPRESSION(bl, FSYS, X);
    if (ADDR_REQUIRED) {
      if (!X.is_address) {
        error(115);
      } else {
        ENTERCHANNEL();
        CTAB[ctab_index].ELTYP = X.types;
        CTAB[ctab_index].ELREF = X.reference;
        CTAB[ctab_index].ELSIZE = (int)X.size;
        X.types = Types::PNTS;
        X.size = 1;
        X.reference = ctab_index;
      }
    } else {
      if (X.is_address)
        LOADVAL(X);
    }
  }
  X.is_address = false;
}

void TERM(BasicLocal *bx, SymbolSet &FSYS, Item &X) {
  Item Y = {Types::NOTYP, 0, 0, false};
  Symbol OP;
  SymbolSet NXT;
  int64_t A, B;
  SymbolSet su, sv;
  su = 0;
  su.set(static_cast<int>(Symbol::TIMES), true);
  su.set(static_cast<int>(Symbol::IMOD), true);
  su.set(static_cast<int>(Symbol::RDIV), true);
  sv = su | FSYS;
  FACTOR(bx, sv, X);
  if (su.test(static_cast<int>(symbol)) && X.is_address)
    LOADVAL(X);
  while (su.test(static_cast<int>(symbol))) {
    OP = symbol;
    INSYMBOL();
    FACTOR(bx, sv, Y);
    if (Y.is_address)
      LOADVAL(Y);
    if (OP == Symbol::TIMES) {
      X.types = RESULTTYPE(X.types, Y.types);
      if (X.types == Types::INTS || X.types == Types::REALS)
        EMIT(57);
    } else if (OP == Symbol::RDIV) {
      if (X.types == Types::INTS && Y.types == Types::INTS)
        EMIT(58);
      else {
        X.types = RESULTTYPE(X.types, Y.types);
        if (X.types != Types::NOTYP)
          X.types = Types::REALS;
        EMIT(88);
      }
    } else {
      if (X.types == Types::INTS && Y.types == Types::INTS)
        EMIT(59);
      else {
        if (X.types != Types::NOTYP && Y.types != Types::NOTYP)
          error(34);
        X.types = Types::NOTYP;
      }
    }
  }
}

void SIMPLEEXPRESSION(BasicLocal *bx, SymbolSet &FSYS, Item &X) {
  Item Y = {Types::NOTYP, 0, 0, false};
  Symbol OP;
  int64_t I = 0;
  int64_t J = 0;
  SymbolSet su, sv;
  su = 0;
  su.set(static_cast<int>(Symbol::PLUS), true);
  su.set(static_cast<int>(Symbol::MINUS), true);
  sv = FSYS | su;
  TERM(bx, sv, X);
  if ((symbol == Symbol::PLUS || symbol == Symbol::MINUS) && X.is_address)
    LOADVAL(X);
  while (symbol == Symbol::PLUS || symbol == Symbol::MINUS) {
    OP = symbol;
    INSYMBOL();
    TERM(bx, sv, Y);
    if (Y.is_address)
      LOADVAL(Y);
    if (X.types == Types::PNTS && Y.types == Types::INTS) {
      if (OP == Symbol::MINUS)
        EMIT(36);
      EMIT2(110, CTAB[X.reference].ELSIZE, 0);
    }
    if (X.types == Types::INTS && Y.types == Types::PNTS) {
      if (OP == Symbol::MINUS) {
        error(113);
        X.types = Types::NOTYP;
      } else
        EMIT2(110, CTAB[Y.reference].ELSIZE, 1);
    }
    if (X.types == Types::PNTS && Y.types == Types::PNTS &&
        OP == Symbol::MINUS) {
      X.types = Types::INTS;
      EMIT(53);
    } else {
      X.types = SUMRESULTTYPE(X.types, Y.types);
      if (X.types == Types::INTS || X.types == Types::REALS ||
          X.types == Types::CHARS) {
        if (OP == Symbol::PLUS)
          EMIT(52);
        else
          EMIT(53);
      }
    }
  }
}

void BOOLEXPRESSION(BasicLocal *bx, SymbolSet &FSYS, Item &X) {
  Item Y = {Types::NOTYP, 0, 0, false};
  Symbol OP;
  SymbolSet su, sv, sz;
  su = 0;
  su.set(static_cast<int>(Symbol::EQL), true);
  su.set(static_cast<int>(Symbol::NEQ), true);
  su.set(static_cast<int>(Symbol::LSS), true);
  su.set(static_cast<int>(Symbol::LEQ), true);
  su.set(static_cast<int>(Symbol::GTR), true);
  su.set(static_cast<int>(Symbol::GEQ), true);
  sv = 0;
  sv[static_cast<int>(Types::NOTYP)] = true;
  sv[static_cast<int>(Types::BOOLS)] = true;
  sv[static_cast<int>(Types::INTS)] = true;
  sv[static_cast<int>(Types::REALS)] = true;
  sv[static_cast<int>(Types::CHARS)] = true;
  sv[static_cast<int>(Types::PNTS)] = true;
  sz = su | FSYS;
  SIMPLEEXPRESSION(bx, sz, X);
  if (su.test(static_cast<int>(symbol))) {
    if (X.is_address)
      LOADVAL(X);
    OP = symbol;
    INSYMBOL();
    SIMPLEEXPRESSION(bx, FSYS, Y);
    if (Y.is_address)
      LOADVAL(Y);
    if (sv[static_cast<int>(X.types)]) {
      if (X.types == Y.types ||
          (X.types == Types::INTS && Y.types == Types::REALS) ||
          (X.types == Types::REALS && Y.types == Types::INTS)) {
        switch (OP) {
        case Symbol::EQL:
          EMIT(45);
          break;
        case Symbol::NEQ:
          EMIT(46);
          break;
        case Symbol::LSS:
          EMIT(47);
          break;
        case Symbol::LEQ:
          EMIT(48);
          break;
        case Symbol::GTR:
          EMIT(49);
          break;
        case Symbol::GEQ:
          EMIT(50);
          break;
        default:
          break;
        }
      } else
        error(35);
    }
    X.types = Types::BOOLS;
  }
}

void ANDEXPRESSION(BasicLocal *bx, SymbolSet &FSYS, Item &X) {
  Item Y = {Types::NOTYP, 0, 0, false};
  Symbol OP;
  SymbolSet su;
  su = FSYS;
  su.set(static_cast<int>(Symbol::ANDSY), true);
  BOOLEXPRESSION(bx, su, X);
  if (symbol == Symbol::ANDSY && X.is_address)
    LOADVAL(X);
  while (symbol == Symbol::ANDSY) {
    OP = symbol;
    INSYMBOL();
    BOOLEXPRESSION(bx, su, Y);
    if (Y.is_address)
      LOADVAL(Y);
    if (X.types == Types::BOOLS && Y.types == Types::BOOLS)
      EMIT(56);
    else {
      if (X.types != Types::NOTYP && Y.types != Types::NOTYP)
        error(32);
      X.types = Types::NOTYP;
    }
  }
}

void OREXPRESSION(BasicLocal *bx, SymbolSet FSYS, Item &X) {
  Item Y = {Types::NOTYP, 0, 0, false};
  Symbol OP;
  SymbolSet su;
  su = FSYS;
  su.set(static_cast<int>(Symbol::ORSY), true);
  ANDEXPRESSION(bx, su, X);
  while (symbol == Symbol::ORSY) {
    if (X.is_address)
      LOADVAL(X);
    OP = symbol;
    INSYMBOL();
    ANDEXPRESSION(bx, su, Y);
    if (Y.is_address)
      LOADVAL(Y);
    if (X.types == Types::BOOLS && Y.types == Types::BOOLS)
      EMIT(51);
    else {
      if (X.types != Types::NOTYP && Y.types != Types::NOTYP)
        error(32);
      X.types = Types::NOTYP;
    }
  }
}

void COMPASSIGNEXP(BasicLocal *bx, SymbolSet &FSYS, Item &X) {
  Item Y = {Types::NOTYP, 0, 0, false};
  Item Z = {Types::NOTYP, 0, 0, false};
  Symbol OP;
  int SF;
  OP = symbol;
  SF = bx->factor;
  if (!X.is_address) {
    error(114);
    X.types = Types::NOTYP;
  }
  if (X.types == Types::CHANS) {
    error(114);
    X.types = Types::NOTYP;
  }
  EMIT(14);
  Z = X;
  Z.is_address = false;
  INSYMBOL();
  EXPRESSION(bx->block_local, FSYS, Y);
  switch (OP) {
  case Symbol::PLUSEQ:
  case Symbol::MINUSEQ:
    if (Y.is_address)
      LOADVAL(Y);
    if (Z.types == Types::PNTS && Y.types == Types::INTS) {
      if (OP == Symbol::MINUSEQ)
        EMIT(36);
      EMIT2(110, CTAB[Z.reference].ELSIZE, 0);
    }
    if (Z.types == Types::INTS && Y.types == Types::PNTS) {
      if (OP == Symbol::MINUSEQ) {
        error(113);
        Z.types = Types::NOTYP;
      } else
        EMIT2(110, CTAB[Y.reference].ELSIZE, 1);
    }
    if (Z.types == Types::PNTS && Y.types == Types::PNTS &&
        OP == Symbol::MINUSEQ) {
      Z.types = Types::INTS;
      EMIT(53);
    } else {
      Z.types = SUMRESULTTYPE(Z.types, Y.types);
      if (Z.types == Types::INTS || Z.types == Types::REALS ||
          Z.types == Types::CHARS) {
        if (OP == Symbol::PLUSEQ)
          EMIT(52);
        else
          EMIT(53);
      }
    }
    break;
  case Symbol::TIMESEQ:
  case Symbol::RDIVEQ:
  case Symbol::IMODEQ:
    if (Y.is_address)
      LOADVAL(Y);
    if (OP == Symbol::TIMESEQ) {
      Z.types = RESULTTYPE(Z.types, Y.types);
      if (Z.types == Types::INTS || Z.types == Types::REALS)
        EMIT(57);
    } else if (OP == Symbol::RDIVEQ) {
      if (Z.types == Types::INTS && Y.types == Types::INTS)
        EMIT(58);
      else {
        Z.types = RESULTTYPE(Z.types, Y.types);
        if (Z.types != Types::NOTYP)
          Z.types = Types::REALS;
        EMIT(88);
      }
    } else {
      if (Z.types == Types::INTS && Y.types == Types::INTS)
        EMIT(59);
      else {
        if (Z.types != Types::NOTYP && Y.types != Types::NOTYP)
          error(34);
        Z.types = Types::NOTYP;
      }
    }
    break;
  default:
    break;
  }
  if (TYPE_COMPATIBLE(X, Z))
    error(46);
  else {
    if (X.types == Types::REALS && Z.types == Types::INTS)
      EMIT1(91, SF);
    EMIT1(38, SF);
  }
  X.is_address = false;
}

void ASSIGNEXPRESSION(BasicLocal *bx, SymbolSet &FSYS, Item &X) {
  Item Y;
  Symbol OP;
  int SF, SZ;
  SymbolSet su, sv;
  su = FSYS | comparators_set;
  su.set(static_cast<int>(Symbol::BECOMES), true);
  OREXPRESSION(bx, su, X);
  if (comparators_set.test(static_cast<int>(symbol)))
    COMPASSIGNEXP(bx, FSYS, X);
  else if (symbol == Symbol::BECOMES) {
    SF = bx->factor;
    if (!X.is_address) {
      error(114);
      X.types = Types::NOTYP;
    }
    if (X.types == Types::CHANS) {
      error(114);
      X.types = Types::NOTYP;
    }
    INSYMBOL();
    su = FSYS;
    su.set(static_cast<int>(Symbol::BECOMES), true);
    EXPRESSION(bx->block_local, su, Y);
    if (TYPE_COMPATIBLE(X, Y))
      error(46);
    else {
      if (X.types == Types::REALS && Y.types == Types::INTS)
        EMIT1(91, SF);
      else if (standard_set[static_cast<int>(X.types)] || X.types == Types::PNTS ||
               X.types == Types::LOCKS)
        EMIT1(38, SF);
      else {
        if (X.types == Types::ARRAYS)
          SZ = ATAB[X.reference].SIZE;
        else
          SZ = X.size;
        EMIT2(23, SZ, SF);
      }
    }
    X.is_address = false;
  }
}

void STANDPROC(BlockLocal *bl, int N) {
  Item X, Y, Z;
  int SZ;
  SymbolSet su;
  switch (N) {
  case 3: // send
    if (symbol == Symbol::LPARENT) {
      INSYMBOL();
      su = bl->FSYS;
      su.set(static_cast<int>(Symbol::COMMA), true);
      su.set(static_cast<int>(Symbol::RPARENT), true);
      BASICEXPRESSION(bl, su, X);
      if ((!X.is_address) || (!(X.types == Types::CHANS))) {
        error(140);
        X.types = Types::NOTYP;
      } else {
        CHANELEMENT(X);
        if (symbol == Symbol::COMMA) {
          INSYMBOL();
        } else {
          error(135);
        }
        su = bl->FSYS;
        su.set(static_cast<int>(Symbol::RPARENT), true);
        EXPRESSION(bl, su, Y);
        if (TYPE_COMPATIBLE(X, Y)) {
          error(141);
        } else {
          if ((X.types == Types::REALS) && (Y.types == Types::INTS)) {
            EMIT1(92, 1);
          } else if (standard_set[static_cast<int>(X.types)] ||
                     X.types == Types::PNTS) {
            EMIT1(66, 1);
          } else {
            if (X.types == Types::ARRAYS) {
              SZ = ATAB[X.reference].SIZE;
            } else {
              SZ = X.size;
            }
            EMIT1(98, SZ);
            EMIT1(66, SZ);
          }
        }
      }
      if (symbol == Symbol::RPARENT) {
        INSYMBOL();
      } else {
        error(4);
      }
    } else {
      error(9);
    }
    break;
  case 4: // recv
    if (symbol == Symbol::LPARENT) {
      INSYMBOL();
      su = bl->FSYS;
      su.set(static_cast<int>(Symbol::COMMA), true);
      su.set(static_cast<int>(Symbol::RPARENT), true);
      BASICEXPRESSION(bl, su, X);
      if ((!X.is_address) || X.types != Types::CHANS) {
        error(140);
        X.types = Types::NOTYP;
      } else {
        EMIT(71);
        CHANELEMENT(X);
        if (symbol == Symbol::COMMA) {
          INSYMBOL();
        } else {
          error(135);
        }
        su = bl->FSYS;
        su.set(static_cast<int>(Symbol::RPARENT), true);
        BASICEXPRESSION(bl, su, Y);
        if (!Y.is_address) {
          error(142);
          Y.types = Types::NOTYP;
        }
        if (TYPE_COMPATIBLE(X, Y)) {
          error(141);
        } else {
          EMIT(20);
          Z = X;
          X = Y;
          Y = Z;
          if ((X.types == Types::REALS) && (Y.types == Types::INTS)) {
            EMIT1(91, 0);
          } else if (standard_set[static_cast<int>(X.types)] ||
                     X.types == Types::PNTS) {
            EMIT1(38, 0);
            EMIT(111);
          } else {
            if (X.types == Types::ARRAYS) {
              SZ = ATAB[X.reference].SIZE;
            } else {
              SZ = X.size;
            }
            EMIT2(23, SZ, 0);
            EMIT1(112, X.size);
            EMIT(111);
          }
        }
      }
      if (symbol == Symbol::RPARENT) {
        INSYMBOL();
      } else {
        error(4);
      }
    } else {
      error(9);
    }
    break;
  case 6: // delete
    su = bl->FSYS;
    su.set(static_cast<int>(Symbol::SEMICOLON), true);
    BASICEXPRESSION(bl, su, X);
    if (!X.is_address) {
      error(120);
    } else if (X.types == Types::PNTS) {
      EMIT(100);
    } else {
      error(16);
    }
    break;
  case 7: // lock
    if (symbol == Symbol::LPARENT) {
      INSYMBOL();
      su = bl->FSYS;
      su.set(static_cast<int>(Symbol::RPARENT), true);
      BASICEXPRESSION(bl, su, X);
      if (!X.is_address) {
        error(15);
      } else {
        if (X.types == Types::LOCKS) {
          EMIT(101);
        } else {
          error(15);
        }
        if (symbol == Symbol::RPARENT) {
          INSYMBOL();
        } else {
          error(4);
        }
      }
    } else {
      error(9);
    }
    break;
  case 8: // unlock
    if (symbol == Symbol::LPARENT) {
      INSYMBOL();
      su = bl->FSYS;
      su.set(static_cast<int>(Symbol::RPARENT), true);
      BASICEXPRESSION(bl, su, X);
      if (!X.is_address) {
        error(15);
      } else {
        if (X.types == Types::LOCKS) {
          EMIT(102);
        } else {
          error(15);
        }
        if (symbol == Symbol::RPARENT) {
          INSYMBOL();
        } else {
          error(4);
        }
      }
    } else {
      error(9);
    }
    break;
  case 9: // duration
    if (symbol == Symbol::LPARENT) {
      INSYMBOL();
      su = bl->FSYS;
      su.set(static_cast<int>(Symbol::RPARENT), true);
      EXPRESSION(bl, su, X);
      if (X.types != Types::INTS) {
        error(34);
      }
      EMIT(93);
      if (symbol == Symbol::RPARENT) {
        INSYMBOL();
      } else {
        error(4);
      }
    } else {
      error(9);
    }
    break;
  case 10: // seqon
    EMIT(106);
    break;
  case 11: // seqoff
    EMIT(107);
    break;
  }
}

void CALL(BlockLocal *bl, SymbolSet &FSYS, int I) {
  Item X, Y;
  int LASTP, CP;
  // int K;
  bool DONE;
  SymbolSet su, sv;
  if (strcmp(TAB[I].name.c_str(), "MPI_INIT      ") == 0 && !mpi_mode)
    error(146);
  su = FSYS;
  su.set(static_cast<int>(Symbol::COMMA), true);
  su.set(static_cast<int>(Symbol::COLON), true);
  su.set(static_cast<int>(Symbol::RPARENT), true);
  EMIT1(18, I);
  LASTP = BTAB[TAB[I].reference].LASTPAR;
  CP = LASTP - BTAB[TAB[I].reference].PARCNT;
  if (symbol == Symbol::LPARENT)
    INSYMBOL();
  else
    error(9);
  if (symbol != Symbol::RPARENT) {
    DONE = false;
    do {
      if (CP >= LASTP)
        error(39);
      else {
        CP++;
        if (TAB[CP].normal) {
          EXPRESSION(bl, su, X);
          Y.types = TAB[CP].types;
          Y.reference = TAB[CP].reference;
          Y.is_address = true;
          if (TYPE_COMPATIBLE(Y, X)) {
            if (X.types == Types::ARRAYS && Y.types != Types::PNTS)
              EMIT1(22, ATAB[X.reference].SIZE);
            if (X.types == Types::RECS)
              EMIT1(22, (int)X.size);
            if (Y.types == Types::REALS && X.types == Types::INTS)
              EMIT(26);
          } else
            error(36);
        } else if (symbol != Symbol::IDENT)
          error(2);
        else {
          sv = FSYS | su;
          BASICEXPRESSION(bl, sv, X);
          Y.types = TAB[CP].types;
          Y.reference = TAB[CP].reference;
          Y.is_address = true;
          if (!X.is_address)
            error(116);
          else {
            switch (Y.types) {
            case Types::CHANS:
              if (X.types != Types::CHANS)
                error(116);
              else if (!COMPATIBLE(Y, X))
                error(36);
              break;
            case Types::ARRAYS:
              EMIT(86);
              if (ATAB[Y.reference].HIGH > 0) {
                if (!ARRAY_COMPATIBLE(Y, X))
                  error(36);
              } else if (!PNT_COMPATIBLE(Y, X))
                error(36);
              break;
            default:
              break;
            }
          }
        }
      }
      su = 0;
      su.set(static_cast<int>(Symbol::COMMA), true);
      su.set(static_cast<int>(Symbol::RPARENT), true);
      TEST(su, FSYS, 6);
      if (symbol == Symbol::COMMA)
        INSYMBOL();
      else
        DONE = true;
    } while (!DONE);
  }
  if (symbol == Symbol::RPARENT)
    INSYMBOL();
  else
    error(4);
  if (CP < LASTP)
    error(39);
  if (bl->CREATEFLAG) {
    EMIT1(78, BTAB[TAB[I].reference].PSIZE - BASESIZE);
    bl->CREATEFLAG = false;
  }
  EMIT2(19, I, BTAB[TAB[I].reference].PSIZE - 1);
  if (TAB[I].LEV < bl->LEVEL && TAB[I].address >= 0)
    EMIT2(3, TAB[I].LEV, bl->LEVEL);
}

void BLOCK(InterpLocal *interp_local, SymbolSet FSYS, bool is_function,
           int level, int PRT) {
  // fprintf(stdout, "BLOCK not yet implemented");
  //    int DX;
  //    int FLEVEL;   shadows global
  //    int NUMWITH;
  //    int MAXNUMWITH;
  //    bool UNDEFMSGFLAG;
  BlockLocal block_local;
  block_local.blkil = interp_local;
  block_local.FSYS = FSYS;
  block_local.ISFUN = is_function;
  block_local.LEVEL = level;
  block_local.PRT = PRT;
  /* parameters above */
  SymbolSet su, sv;
  block_local.DX = BASESIZE;
  block_local.FLEVEL = 0;
  if (interp_local->LEVEL > LMAX)
    fatal(5);
  if (block_local.LEVEL > 2)
    error(122);
  block_local.NUMWITH = 0;
  block_local.MAXNUMWITH = 0;
  block_local.UNDEFMSGFLAG = true;
  block_local.RDX = 0;
  block_local.RLAST = 0;
  block_local.PCNT = 0;
  block_local.X = 0;
  block_local.V = 0;
  // bl.F = 0;   BASICEXPRESSION
  block_local.CREATEFLAG = false;
  block_local.ISDECLARATION = false;
  ENTERBLOCK();

  DISPLAY[block_local.LEVEL] = btab_index;
  block_local.PRB = btab_index;
  if (block_local.LEVEL == 1) {
    TAB[PRT].types = Types::NOTYP;
  } else {
    su = 0;
    su.set(static_cast<int>(Symbol::LPARENT), true);
    TEST(su, FSYS, 9);
  }

  TAB[PRT].reference = block_local.PRB;
  if (symbol == Symbol::LPARENT && block_local.LEVEL > 1) {
    PARAMETERLIST(&block_local);
  }

  BTAB[block_local.PRB].LASTPAR = tab_index;
  BTAB[block_local.PRB].PSIZE = block_local.DX;
  BTAB[block_local.PRB].PARCNT = block_local.PCNT;
  if (block_local.LEVEL == 1) {
    su = declaration_set;
    su.set(static_cast<int>(Symbol::INCLUDESY), true);
    TEST(su, FSYS, 102);
    OPTIONS(&block_local);
    do {
      if (symbol == Symbol::DEFINESY) {
        CONSTANTDECLARATION(&block_local);
      } else if (symbol == Symbol::TYPESY) {
        TYPEDECLARATION(&block_local);
      } else if (type_set.test(static_cast<int>(symbol))) {
        VARIABLEDECLARATION(&block_local);
      } else if (symbol == Symbol::INCLUDESY) {
        INCLUDEDIRECTIVE();
      }
      su = declaration_set;
      su.set(static_cast<int>(Symbol::INCLUDESY), true);
      su.set(static_cast<int>(Symbol::EOFSY), true);
      TEST(su, FSYS, 6);
    } while (symbol != Symbol::EOFSY);
    TAB[PRT].address = line_count;
  } else {
    if (symbol != Symbol::SEMICOLON) {
      TAB[PRT].address = line_count;
      su = declaration_set | keyword_set | assigners_set;
      sv = FSYS;
      sv.set(static_cast<int>(Symbol::RSETBRACK), true);
      TEST(su, sv, 101);
      block_local.CREATEFLAG = false;
      while (declaration_set.test(static_cast<int>(symbol)) ||
             keyword_set.test(static_cast<int>(symbol)) ||
             assigners_set.test(static_cast<int>(symbol))) {
        if (symbol == Symbol::DEFINESY)
          CONSTANTDECLARATION(&block_local);
        else if (symbol == Symbol::TYPESY)
          TYPEDECLARATION(&block_local);
        else if (symbol == Symbol::STRUCTSY || symbol == Symbol::CONSTSY)
          VARIABLEDECLARATION(&block_local);
        if (symbol == Symbol::IDENT) {
          block_local.X = LOC(&block_local, ID);
          if (block_local.X != 0) {
            if (TAB[block_local.X].object == OBJECTS::TYPE1) {
              VARIABLEDECLARATION(&block_local);
            } else {
              su = FSYS;
              su.set(static_cast<int>(Symbol::SEMICOLON), true);
              su.set(static_cast<int>(Symbol::RSETBRACK), true);
              STATEMENT(&block_local, su);
            }
          } else {
            INSYMBOL();
          }
        } else {
          if (keyword_set.test(static_cast<int>(symbol)) ||
              assigners_set.test(static_cast<int>(symbol))) {
            su = FSYS;
            su.set(static_cast<int>(Symbol::SEMICOLON), true);
            su.set(static_cast<int>(Symbol::RSETBRACK), true);
            STATEMENT(&block_local, su);
          }
        }
        su = declaration_set | keyword_set | assigners_set;
        su.set(static_cast<int>(Symbol::RSETBRACK), true);
        TEST(su, FSYS, 6);
      }
    }
  }

  BTAB[block_local.PRB].VSIZE = block_local.DX;
  BTAB[block_local.PRB].VSIZE =
      BTAB[block_local.PRB].VSIZE + block_local.MAXNUMWITH;
  if ((symbol_count == 1) && (!inCLUDEFLAG()))
    LOCATION[LNUM] = line_count;
}

} // namespace Cstar
