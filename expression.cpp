#include "cs_basic.h"
#include "cs_block.h"
#include "cs_compile.h"
#include "cs_errors.h"
#include "cs_global.h"
#include "cs_interpret.h"
#include <cstring>

namespace cs {
struct TypLocal {
  SymbolSet FSYS;
  Types TP;
  int64_t RF;
  int64_t SZ;
  /* parms above */
  int64_t X, I;
  // int64_t J;
  // TYPES ELTP;
  // int64_t ELRF;
  // int64_t ELSZ;
  int64_t TS;
  ALFA TID;
  BlockLocal *bl;
} __attribute__((aligned(128))) __attribute__((packed));

void TEST(SymbolSet &, SymbolSet &, int);
extern auto HasIncludeFlag() -> bool;
extern void C_PNTCHANTYP(BlockLocal *bl, Types &TP, int64_t &RF, int64_t &SZ);
extern void BASICEXPRESSION(BlockLocal *, SymbolSet, Item &);
void ENTERCHANNEL();
void ENTERVARIABLE(BlockLocal *, Objects);
void EXPRESSION(BlockLocal *block_local, SymbolSet FSYS, Item &X);
void LOADVAL(Item &);
void TYPF(BlockLocal *block_local, SymbolSet FSYS, Types &types, int64_t &RF,
          int64_t &SZ);
void CONSTANT(BlockLocal *, SymbolSet &, CONREC &);
void ENTERARRAY(Types TP, int L, int H);
extern void ENTER(BlockLocal *block_local, const ALFA &identifier,
                  Objects objects);
auto LOC(BlockLocal *bl, const ALFA &ID) -> int64_t;
auto TYPE_COMPATIBLE(Item /*X*/, Item /*Y*/) -> bool;
extern void BLOCK(InterpLocal *il, SymbolSet fsys, bool ISFUN, int LEVEL,
                  int PRT);
void SKIP(SymbolSet, int);
void TESTSEMICOLON(BlockLocal *);
extern int MAINFUNC;
extern void CALL(BlockLocal *, SymbolSet &, int I);
void SELECTOR(BlockLocal * /*bl*/, SymbolSet FSYS, Item &V);
void PNTSELECT(Item & /*V*/);
auto PNT_COMPATIBLE(Item V, Item W) -> bool;
auto ARRAY_COMPATIBLE(Item V, Item W) -> bool;

void RSELECT(Item &V) {
  int A = 0;
  if (symbol == Symbol::PERIOD)
    INSYMBOL();
  if (symbol != Symbol::IDENT) {
    error(2);
  } else if (V.types != Types::RECS) {
    error(31);
  } else {
    A = V.reference;
    TAB[0].name = ID;
    while (strcmp(TAB[A].name.c_str(), ID.c_str()) != 0) {
      A = TAB[A].link;
    }
    if (A == 0)
      error(0);
    else {
      EMIT1(24, TAB[A].address);
      EMIT(15);
      V.types = TAB[A].types;
      V.reference = TAB[A].reference;
      V.size = TAB[A].size;
    }
    INSYMBOL();
  }
}
void PNTSELECT(Item &V) {
  int A;
  INSYMBOL();
  if (V.types != Types::PNTS) {
    error(13);
  } else {
    A = V.reference;
    V.types = CTAB[A].ELTYP;
    V.reference = CTAB[A].ELREF;
    V.size = CTAB[A].ELSIZE;
    EMIT(34);
    RSELECT(V);
  }
}

void SELECTOR(BlockLocal *bl, SymbolSet FSYS, Item &V) {
  Item X{};
  int A = 0;
  SymbolSet su;
  if (V.types == Types::CHANS) {
    error(11);
  }

  su.set(static_cast<int>(Symbol::COMMA), true);
  su.set(static_cast<int>(Symbol::RBRACK), true);
  while (selection_set.test(static_cast<int>(symbol))) {
    if (symbol == Symbol::LBRACK) {
      do {
        INSYMBOL();
        EXPRESSION(bl, FSYS | su, X);
        if (V.types != Types::ARRAYS)
          error(28);
        else {
          A = V.reference;
          if (ATAB[A].INXTYP != X.types)
            error(26);
          else
            EMIT1(21, A);
          V.types = ATAB[A].ELTYP;
          V.reference = ATAB[A].ELREF;
          V.size = ATAB[A].ELSIZE;
        }
      } while (symbol == Symbol::COMMA);
      if (symbol == Symbol::RBRACK)
        INSYMBOL();
      else
        error(12);
    } else if (symbol == Symbol::PERIOD)
      RSELECT(V);
    else
      PNTSELECT(V);
  }
  su = 0;
  TEST(FSYS, su, 6);
}

auto COMPATIBLE(Item V, Item W) -> bool {
  // ITEM Z;
  bool rtn = true;
  if (V.types == W.types) {
    switch (V.types) {
    case Types::INTS:
    case Types::REALS:
    case Types::CHARS:
    case Types::BOOLS:
    case Types::LOCKS:
      rtn = true;
      break;
    case Types::RECS:
      rtn = CTAB[V.reference].ELREF == CTAB[W.reference].ELREF;
      break;
    case Types::CHANS:
      V.types = CTAB[V.reference].ELTYP;
      W.types = CTAB[W.reference].ELTYP;
      V.reference = CTAB[V.reference].ELREF;
      W.reference = CTAB[W.reference].ELREF;
      rtn = COMPATIBLE(V, W);
      break;
    case Types::PNTS:
      if (W.reference != 0) {
        V.types = CTAB[V.reference].ELTYP;
        W.types = CTAB[W.reference].ELTYP;
        if (V.types != Types::VOIDS && W.types != Types::VOIDS) {
          V.reference = CTAB[V.reference].ELREF;
          W.reference = CTAB[W.reference].ELREF;
          rtn = PNT_COMPATIBLE(V, W);
        }
      }
      break;
    case Types::ARRAYS:
      ARRAY_COMPATIBLE(V, W);
      break;
    default:
      break;
    }
  } else {
    rtn = false;
  }
  return rtn;
}

auto ARRAY_COMPATIBLE(Item V, Item W) -> bool {
  bool DIM_OK = true;
  while (V.types == Types::ARRAYS && W.types == Types::ARRAYS) {
    if (ATAB[V.reference].HIGH != ATAB[W.reference].HIGH ||
        ATAB[V.reference].ELTYP != ATAB[W.reference].ELTYP) {
      DIM_OK = false;
    }
    V.types = ATAB[V.reference].ELTYP;
    V.reference = ATAB[V.reference].ELREF;
    W.types = ATAB[W.reference].ELTYP;
    W.reference = ATAB[W.reference].ELREF;
  }
  if (DIM_OK) {
    return COMPATIBLE(V, W);
  }
  return false;
}

auto PNT_COMPATIBLE(Item V, Item W) -> bool {
  while (V.types == Types::ARRAYS) {
    V.types = ATAB[V.reference].ELTYP;
    V.reference = ATAB[V.reference].ELREF;
  }
  while (W.types == Types::ARRAYS) {
    W.types = ATAB[W.reference].ELTYP;
    W.reference = ATAB[W.reference].ELREF;
  }
  return COMPATIBLE(V, W);
}

auto TYPE_COMPATIBLE(Item X, Item Y) -> bool {
  // ITEM W;
  // if ((inset(X.types, STANTYPS + LOCKS)) && (inset(Y.types, STANTYPS +
  // LOCKS)))
  if ((standard_set.test(static_cast<int>(X.types)) || X.types == Types::LOCKS) &&
      (standard_set.test(static_cast<int>(Y.types)) || Y.types == Types::LOCKS)) {
    return X.types == Y.types ||
           (X.types == Types::REALS && Y.types == Types::INTS) ||
           (X.types == Types::INTS && Y.types == Types::CHARS) ||
           (X.types == Types::CHARS && Y.types == Types::INTS);
  }

  if (X.types == Types::RECS && Y.types == Types::RECS) {
    return X.reference == Y.reference;
  }

  if (X.types == Types::ARRAYS && Y.types == Types::ARRAYS) {
    return ARRAY_COMPATIBLE(X, Y);
  }

  if (X.types == Types::PNTS && Y.types == Types::PNTS) {
    return PNT_COMPATIBLE(X, Y);
  }

  if (X.types == Types::PNTS && Y.types == Types::ARRAYS) {
    X.types = CTAB[X.reference].ELTYP;
    X.reference = CTAB[X.reference].ELREF;
    return PNT_COMPATIBLE(X, Y);
  }

  return false;

  if (X.types == Types::NOTYP || Y.types == Types::NOTYP) { // dead code
    return true;
  }
}

void CHANELEMENT(Item &V) {
  int vref = V.reference;
  V.types = CTAB[vref].ELTYP;
  V.size = CTAB[vref].ELSIZE;
  V.reference = CTAB[vref].ELREF;
}

auto RESULTTYPE(Types A, Types B) -> Types {
  if ((A == Types::INTS && B == Types::CHARS) ||
      (A == Types::CHARS && B == Types::INTS)) {
    return Types::CHARS;
  }
  if (A > Types::INTS || B > Types::INTS) {
    error(33);
    return Types::NOTYP;
  }
  if (A == Types::NOTYP || B == Types::NOTYP) {
    return Types::NOTYP;
  }
  if (A == Types::REALS || B == Types::REALS) {
    return Types::REALS;
  }
  return Types::INTS;
}

auto SUMRESULTTYPE(Types A, Types B) -> Types {
  if ((A == Types::INTS && B == Types::CHARS) ||
      (A == Types::CHARS && B == Types::INTS)) {
    return Types::CHARS;
  }
  if ((A == Types::INTS && B == Types::PNTS) ||
      (A == Types::PNTS && B == Types::INTS)) {
    return Types::PNTS;
  }
  if (A > Types::INTS || B > Types::INTS) {
    error(33);
    return Types::NOTYP;
  }
  if (A == Types::NOTYP || B == Types::NOTYP) {
    return Types::NOTYP;
  }
  if (A == Types::REALS || B == Types::REALS) {
    return Types::REALS;
  }
  return Types::INTS;
}

auto FACTOR(BasicLocal *bx, SymbolSet &FSYS, Item &X) -> void {
  int A = 0;
  int I = 0;
  Symbol OP;
  Types TYPCAST;
  SymbolSet su;
  SymbolSet sv;
  su = 0;
  su.set(static_cast<int>(Symbol::DBLQUEST), true);
  X.types = Types::NOTYP;
  X.reference = 0;
  X.is_address = true;

  TEST(FACBEGSYS, FSYS, 8);
  if (symbol == Symbol::IDENT) {
    I = LOC(bx->block_local, ID);
    INSYMBOL();
    switch (TAB[I].object) {
    case OBJECTS::KONSTANT:
      X.types = TAB[I].types;
      X.reference = 0;
      X.is_address = false;
      if (TAB[I].types == Types::REALS) {
        EMIT1(87, TAB[I].address);
      } else {
        EMIT1(24, TAB[I].address);
      }
      break;
    case OBJECTS::VARIABLE:
      X.types = TAB[I].types;
      X.reference = TAB[I].reference;
      X.size = TAB[I].size;
      if (selection_set.test(static_cast<int>(symbol))) {
        bx->factor = (TAB[I].normal) ? 0 : 1;
        EMIT2(bx->factor, TAB[I].LEV, TAB[I].address);
        SELECTOR(bx->block_local, FSYS | su, X);
        if (X.types == Types::CHANS && symbol == Symbol::DBLQUEST) {
          INSYMBOL();
          EMIT(94);
          X.types = Types::BOOLS;
          X.is_address = false;
        }
      } else {
        if (TAB[I].FORLEV == 0) {
          if (X.types == Types::CHANS && symbol == Symbol::DBLQUEST) {
            INSYMBOL();
            X.types = Types::BOOLS;
            X.is_address = false;
            if (TAB[I].normal) {
              bx->factor = 95;
            } else {
              bx->factor = 96;
            }
          } else {
            if (TAB[I].normal) {
              bx->factor = 0;
            } else {
              bx->factor = 1;
            }
          }

          EMIT2(bx->factor, TAB[I].LEV, TAB[I].address);

          if (bx->factor == 0 && TAB[I].PNTPARAM) {
            bx->factor = 1;
          }
        } else {
          X.is_address = false;
          if (TAB[I].FORLEV > 0) {
            EMIT2(73, TAB[I].LEV, TAB[I].address);
          } else {
            EMIT(81);
          }
        }
      }
      break;
    case OBJECTS::TYPE1:
    case OBJECTS::PROZEDURE:
    case OBJECTS::COMPONENT:
      error(44);
      break;
    case OBJECTS::FUNKTION:
      X.types = TAB[I].types;
      X.reference = TAB[I].FREF;
      X.size = TAB[I].size;
      X.is_address = false;
      if (TAB[I].LEV != 0) {
        CALL(bx->block_local, FSYS, I);
      } else if (TAB[I].address == 24) {
        if (symbol == Symbol::LPARENT) {
          INSYMBOL();
        } else {
          error(9);
        }

        su = 0;
        su.set(static_cast<int>(Symbol::IDENT), true);
        su.set(static_cast<int>(Symbol::STRUCTSY), true);
        sv = FSYS;
        sv.set(static_cast<int>(Symbol::RPARENT), true);
        TEST(su, sv, 6);
        if (symbol == Symbol::IDENT) {
          int I = LOC(bx->block_local, ID);
          if (TAB[I].object == OBJECTS::VARIABLE) {
            EMIT1(24, TAB[I].size);
          } else if (TAB[I].object == OBJECTS::TYPE1) {
            EMIT1(24, TAB[I].address);
          } else {
            error(6);
          }
        } else if (symbol == Symbol::STRUCTSY) {
          INSYMBOL();
          if (symbol == Symbol::IDENT) {
            int I = LOC(bx->block_local, ID);
            if (I != 0) {
              if (TAB[I].object != OBJECTS::STRUCTAG) {
                error(108);
              } else {
                EMIT1(24, TAB[I].address);
              }
            }
          } else {
            error(6);
          }
        } else {
          error(6);
        }
        INSYMBOL();
        if (symbol == Symbol::RPARENT) {
          INSYMBOL();
        } else {
          error(4);
        }
      } else {
        if (symbol == Symbol::LPARENT) {
          INSYMBOL();
          if (symbol == Symbol::RPARENT) {
            INSYMBOL();
          } else {
            error(4);
          }
        }
        EMIT1(8, TAB[I].address);
      }
      break;
    default:
      break;
    }
  } else if (symbol == Symbol::CHARCON || symbol == Symbol::INTCON) {
    if (symbol == Symbol::CHARCON) {
      X.types = Types::CHARS;
    } else {
      X.types = Types::INTS;
    }
    EMIT1(24, INUM);
    X.reference = 0;
    X.is_address = false;
    INSYMBOL();
  } else if (symbol == Symbol::STRNG) {
    X.types = Types::PNTS;
    ENTERCHANNEL();
    CTAB[ctab_index].ELTYP = Types::CHARS;
    CTAB[ctab_index].ELREF = 0;
    CTAB[ctab_index].ELSIZE = 1;
    X.reference = ctab_index;
    X.is_address = false;
    EMIT2(13, INUM, STRING_LENGTH);
    INSYMBOL();
  } else if (symbol == Symbol::REALCON) {
    X.types = Types::REALS;
    CONTABLE.at(CPNT) = RNUM;
    EMIT1(87, CPNT);
    if (CPNT >= RCMAX) {
      fatal(9);
    } else {
      CPNT = CPNT + 1;
    }
    X.reference = 0;
    X.is_address = false;
    INSYMBOL();
  } else if (symbol == Symbol::LPARENT) {
    INSYMBOL();
    TYPCAST = Types::NOTYP;
    if (symbol == Symbol::IDENT) {
      int I = LOC(bx->block_local, ID);
      if (TAB[I].object == OBJECTS::TYPE1) {
        TYPCAST = TAB[I].types;
        INSYMBOL();
        if (symbol == Symbol::TIMES) {
          TYPCAST = Types::PNTS;
          INSYMBOL();
        }
        if (symbol != Symbol::RPARENT) {
          error(121);
        } else {
          INSYMBOL();
        }
        if (!standard_set.test(static_cast<int>(TYPCAST)) &&
            TYPCAST != Types::PNTS) {
          error(121);
        }
        FACTOR(bx, FSYS, X);
        if (X.is_address) {
          LOADVAL(X);
        }
        if (TYPCAST == Types::INTS) {
          if (X.types == Types::REALS) {
            EMIT1(8, 10);
          }
        } else if (TYPCAST == Types::BOOLS) {
          if (X.types == Types::REALS) {
            EMIT1(8, 10);
          }
          EMIT(114);
        } else if (TYPCAST == Types::CHARS) {
          if (X.types == Types::REALS) {
            EMIT1(8, 10);
          }
          EMIT1(8, 5);
        } else if (TYPCAST == Types::REALS) {
          EMIT(26);
        } else if (TYPCAST == Types::PNTS) {
          if (X.types != Types::PNTS ||
              CTAB[X.reference].ELTYP != Types::VOIDS) {
            error(121);
          }
        } else {
          error(121);
        }
        X.types = TYPCAST;
      }
    }
    if (TYPCAST == Types::NOTYP) {
      sv = FSYS;
      sv.set(static_cast<int>(Symbol::RPARENT), true);
      BASICEXPRESSION(bx->block_local, sv, X);
      if (symbol == Symbol::RPARENT) {
        INSYMBOL();
      } else {
        error(4);
      }
    }
  } else if (symbol == Symbol::NOTSY) {
    INSYMBOL();
    FACTOR(bx, FSYS, X);
    if (X.is_address) {
      LOADVAL(X);
    }
    if (X.types == Types::BOOLS) {
      EMIT(35);
    } else if (X.types != Types::NOTYP) {
      error(32);
    }
  } else if (symbol == Symbol::TIMES) {
    INSYMBOL();
    FACTOR(bx, FSYS, X);
    if (X.is_address) {
      LOADVAL(X);
    }
    if (X.types == Types::PNTS) {
      int A = X.reference;
      X.types = CTAB[A].ELTYP;
      X.reference = CTAB[A].ELREF;
      X.size = CTAB[A].ELSIZE;
      X.is_address = true;
    } else if (X.types != Types::NOTYP) {
      error(13);
    }
  } else if (symbol == Symbol::PLUS || symbol == Symbol::MINUS) {
    int OP = static_cast<int>(symbol);
    INSYMBOL();
    FACTOR(bx, FSYS, X);
    if (X.is_address) {
      LOADVAL(X);
    }
    if (X.types == Types::INTS || X.types == Types::REALS ||
        X.types == Types::CHARS || X.types == Types::PNTS) {
      if (OP == static_cast<int>(Symbol::MINUS)) {
        EMIT(36);
      }
    } else {
      error(33);
    }
  } else if (symbol == Symbol::INCREMENT) {
    INSYMBOL();
    FACTOR(bx, FSYS, X);
    if (X.types != Types::NOTYP && X.types != Types::INTS &&
        X.types != Types::REALS && X.types != Types::CHARS &&
        X.types != Types::PNTS) {
      error(110);
    } else {
      if ((X.types == Types::NOTYP || X.types == Types::INTS ||
           X.types == Types::REALS || X.types == Types::CHARS) &&
          !X.is_address) {
        error(110);
      }
      int A = 1;
      if (X.types == Types::PNTS) {
        A = CTAB[X.reference].ELSIZE;
      }
      if (X.types != Types::NOTYP) {
        EMIT2(108, A, 0);
      }
      X.is_address = false;
    }
  } else if (symbol == Symbol::DECREMENT) {
    INSYMBOL();
    FACTOR(bx, FSYS, X);
    if (X.types != Types::NOTYP && X.types != Types::INTS &&
        X.types != Types::REALS && X.types != Types::CHARS &&
        X.types != Types::PNTS) {
      error(111);
    } else {
      if ((X.types == Types::NOTYP || X.types == Types::INTS ||
           X.types == Types::REALS || X.types == Types::CHARS) &&
          !X.is_address) {
        error(111);
      }
      A = 1;
      if (X.types == Types::PNTS) {
        A = CTAB[X.reference].ELSIZE;
      }
      if (X.types != Types::NOTYP) {
        EMIT2(109, A, 0);
      }
      X.is_address = false;
    }
  }
  if (selection_set.test(static_cast<int>(symbol)) && X.is_address) {
    su = FSYS;
    su.set(static_cast<int>(Symbol::DBLQUEST), true);
    SELECTOR(bx->block_local, su, X);
  }
  switch (symbol) {
  case Symbol::DBLQUEST:
    if (X.types == Types::CHANS) {
      INSYMBOL();
      EMIT(94);
      X.types = Types::BOOLS;
      X.is_address = false;
    } else
      error(103);
    break;
  case Symbol::INCREMENT:
    if (X.types != Types::NOTYP && X.types != Types::INTS &&
        X.types != Types::REALS && X.types != Types::CHARS &&
        X.types != Types::PNTS) {
      error(110);
    } else {
      if (X.types != Types::NOTYP && X.types != Types::INTS &&
          X.types != Types::REALS && X.types != Types::CHARS && !X.is_address)
        error(110);
      A = 1;
      if (X.types == Types::PNTS)
        A = CTAB[X.reference].ELSIZE;
      if (X.types != Types::NOTYP)
        EMIT2(108, A, 1);
      X.is_address = false;
    }
    INSYMBOL();
    break;
  case Symbol::DECREMENT:
    if (X.types != Types::NOTYP && X.types != Types::INTS &&
        X.types != Types::REALS && X.types != Types::CHARS &&
        X.types != Types::PNTS) {
      error(111);
    } else {
      if (X.types != Types::NOTYP && X.types != Types::INTS &&
          X.types != Types::REALS && X.types != Types::CHARS && !X.is_address)
        error(111);
      A = 1;
      if (X.types == Types::PNTS)
        A = CTAB[X.reference].ELSIZE;
      if (X.types != Types::NOTYP)
        EMIT2(109, A, 1);
      X.is_address = false;
    }
    INSYMBOL();
    break;
  default:
    break;
  }
  TEST(FSYS, FACBEGSYS, 6);
}
void EXPRESSION(BlockLocal *bl, SymbolSet FSYS, Item &X) {
  // ITEM Y;
  // SYMBOL OP;
  bool ADDR_REQUIRED = false;
  int64_t RF = 0;
  int64_t SZ = 0;
  Types TP;
  SymbolSet su;
  SymbolSet sv;

  if (symbol == Symbol::ADDRSY) {
    ADDR_REQUIRED = true;
    INSYMBOL();
  } else {
    ADDR_REQUIRED = false;
  }
  su = FSYS;
  su.set(static_cast<int>(Symbol::TIMES), true);
  su.set(static_cast<int>(Symbol::CHANSY), true);
  su.set(static_cast<int>(Symbol::LBRACK), true);
  if (symbol == Symbol::NEWSY) {
    INSYMBOL();
    TYPF(bl, su, TP, RF, SZ);
    C_PNTCHANTYP(bl, TP, RF, SZ);
    ENTERCHANNEL();
    CTAB[ctab_index].ELTYP = TP;
    CTAB[ctab_index].ELREF = RF;
    CTAB[ctab_index].ELSIZE = SZ;
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
        CTAB[ctab_index].ELSIZE = X.size;
        X.types = Types::PNTS;
        X.size = 1;
        X.reference = ctab_index;
      }
    } else {
      if (X.is_address) {
        LOADVAL(X);
      }
    }
  }
  X.is_address = false;
}

void C_ARRAYTYP(BlockLocal *block_local, int64_t &AREF, int64_t &ARSZ,
                bool FIRSTINDEX, Types ELTP, int64_t ELRF, int64_t ELSZ) {
  CONREC HIGH = {Types::NOTYP, 0l};
  int64_t RF = 0;
  int64_t SZ = 0;
  Types TP;
  SymbolSet su;

  if ((symbol == Symbol::RBRACK) && FIRSTINDEX) {
    HIGH.TP = Types::INTS;
    HIGH.I = 0;
  } else {
    su = block_local->FSYS;
    su.set(static_cast<int>(Symbol::RBRACK), true);
    CONSTANT(block_local, su, HIGH);
    if ((HIGH.TP != Types::INTS) || (HIGH.I < 1)) {
      error(27);
      HIGH.TP = Types::INTS;
      HIGH.I = 0;
    }
  }

  ENTERARRAY(Types::INTS, 0, HIGH.I - 1);
  AREF = atab_index;
  if (symbol == Symbol::RBRACK) {
    INSYMBOL();
  } else {
    error(12);
  }

  if (symbol == Symbol::LBRACK) {
    INSYMBOL();
    TP = Types::ARRAYS;
    C_ARRAYTYP(block_local, RF, SZ, false, ELTP, ELRF, ELSZ);
  } else {
    RF = ELRF;
    SZ = ELSZ;
    TP = ELTP;
  }

  ARSZ = (ATAB[AREF].HIGH + 1) * SZ;
  ATAB[AREF].SIZE = ARSZ;
  ATAB[AREF].ELTYP = TP;
  ATAB[AREF].ELREF = RF;
  ATAB[AREF].ELSIZE = SZ;
}

void C_PNTCHANTYP(BlockLocal *bl, Types &TP, int64_t &RF, int64_t &SZ) {
  bool CHANFOUND;
  int64_t ARF, ASZ;
  CHANFOUND = false;
  while (symbol == Symbol::TIMES || symbol == Symbol::CHANSY ||
         symbol == Symbol::LBRACK) {
    switch (symbol) {
    case Symbol::LBRACK:
      INSYMBOL();
      C_ARRAYTYP(bl, ARF, ASZ, true, TP, RF, SZ);
      TP = Types::ARRAYS;
      RF = ARF;
      SZ = ASZ;
      break;
    case Symbol::TIMES:
    case Symbol::CHANSY:
      ENTERCHANNEL();
      CTAB[ctab_index].ELTYP = TP;
      CTAB[ctab_index].ELREF = RF;
      CTAB[ctab_index].ELSIZE = SZ;
      SZ = 1;
      RF = ctab_index;
      if (symbol == Symbol::TIMES) {
        TP = Types::PNTS;
      }
      if (symbol == Symbol::CHANSY) {
        TP = Types::CHANS;
        if (CHANFOUND) {
          error(11);
        }
        CHANFOUND = true;
      }
      INSYMBOL();
      break;
    default:
      break;
    }
  }
}
void C_COMPONENTDECLARATION(TypLocal *tl) {
  int64_t T0;
  // int64_t T1;
  int64_t RF, SZ;
  int64_t ARF, ASZ, ORF, OSZ;
  Types TP, OTP;
  bool DONE;
  BlockLocal *block_local = tl->bl;
  SymbolSet su;
  SymbolSet TSYS; // {SEMICOLON, COMMA, IDENT, TIMES, CHANSY, LBRACK};
  // TSYS = FSYS;
  TSYS = 0;
  TSYS.set(static_cast<int>(Symbol::SEMICOLON), true);
  TSYS.set(static_cast<int>(Symbol::COMMA), true);
  TSYS.set(static_cast<int>(Symbol::IDENT), true);
  TSYS.set(static_cast<int>(Symbol::TIMES), true);
  TSYS.set(static_cast<int>(Symbol::CHANSY), true);
  TSYS.set(static_cast<int>(Symbol::LBRACK), true);

  TEST(type_set, tl->FSYS, 109);

  while (type_set.test(static_cast<int>(symbol))) {
    TYPF(block_local, TSYS, TP, RF, SZ);
    OTP = TP;
    ORF = RF;
    OSZ = SZ;
    DONE = false;

    do {
      C_PNTCHANTYP(block_local, TP, RF, SZ);
      if (symbol != Symbol::IDENT) {
        error(2);
        ID = "              ";
      }
      T0 = tab_index;
      ENTERVARIABLE(block_local, OBJECTS::COMPONENT);

      if (symbol == Symbol::LBRACK) {
        INSYMBOL();
        C_ARRAYTYP(block_local, ARF, ASZ, true, TP, RF, SZ);
        TP = Types::ARRAYS;
        RF = ARF;
        SZ = ASZ;
      }

      T0 = T0 + 1;
      TAB[T0].types = TP;
      TAB[T0].reference = RF;
      TAB[T0].LEV = block_local->LEVEL;
      TAB[T0].address = block_local->RDX;
      TAB[T0].normal = true;
      block_local->RDX = block_local->RDX + SZ;
      TAB[T0].size = SZ;
      TAB[T0].PNTPARAM = false;

      if (symbol == Symbol::COMMA) {
        INSYMBOL();
      } else {
        DONE = true;
      }

      TP = OTP;
      RF = ORF;
      SZ = OSZ;
    } while (!DONE);

    if (symbol == Symbol::SEMICOLON) {
      INSYMBOL();
    } else {
      error(14);
    }
    su = type_set;
    su.set(static_cast<int>(Symbol::RSETBRACK), true);
    TEST(su, tl->FSYS, 108);
  }
}

void STRUCTTYP(TypLocal *type_local, int64_t &RREF, int64_t &RSZ) {
  SymbolSet su;
  BlockLocal *block_local = type_local->bl;
  if (symbol == Symbol::LSETBRACK) {
    INSYMBOL();
  } else {
    error(106);
  }

  int64_t SAVEDX = block_local->RDX;
  block_local->RDX = 0;
  int64_t SAVELAST = block_local->RLAST;
  block_local->RLAST = 0;
  C_COMPONENTDECLARATION(type_local);

  if (symbol == Symbol::RSETBRACK) {
    INSYMBOL();
  } else {
    error(104);
  }
  su = 0;
  su.set(static_cast<int>(Symbol::IDENT), true);
  su.set(static_cast<int>(Symbol::SEMICOLON), true);
  su.set(static_cast<int>(Symbol::CHANSY), true);
  su.set(static_cast<int>(Symbol::TIMES), true);
  TEST(su, type_local->FSYS, 6);

  RREF = block_local->RLAST;
  block_local->RLAST = SAVELAST;
  RSZ = block_local->RDX;
  block_local->RDX = SAVEDX;
}

void TYPF(BlockLocal *bl, SymbolSet FSYS, Types &types, int64_t &RF,
          int64_t &SZ) {
  //    int64_t X, I, J;
  //    TYPES ELTP;
  //    int64_t ELRF, ELSZ, TS;
  //    ALFA TID;
  struct TypLocal tl = {0};
  SymbolSet su;
  tl.bl = bl;
  tl.TP = Types::NOTYP;
  tl.RF = 0;
  tl.SZ = 0;
  TEST(type_set, FSYS, 10);

  if (symbol == Symbol::CONSTSY) {
    INSYMBOL();
  }

  if (type_set.test(static_cast<int>(symbol))) {
    if (symbol == Symbol::SHORTSY || symbol == Symbol::LONGSY ||
        symbol == Symbol::UNSIGNEDSY) {
      INSYMBOL();
      if (symbol == Symbol::SHORTSY || symbol == Symbol::LONGSY ||
          symbol == Symbol::UNSIGNEDSY) {
        INSYMBOL();
      }
      TEST(type_set, FSYS, 10);
    }

    if (symbol == Symbol::IDENT) {
      tl.X = LOC(bl, ID);
      if (tl.X != 0) {
        if (TAB[tl.X].object != OBJECTS::TYPE1) {
          error(29);
        } else {
          tl.TP = TAB[tl.X].types;
          tl.RF = TAB[tl.X].reference;
          tl.SZ = TAB[tl.X].address;
          if (tl.TP == Types::NOTYP) {
            error(30);
          }
        }
      }
      INSYMBOL();
      if (symbol == Symbol::CONSTSY) {
        INSYMBOL();
      }
    } else if (symbol == Symbol::STRUCTSY) {
      INSYMBOL();
      if (symbol == Symbol::IDENT) {
        tl.TID = ID;
        INSYMBOL();
        if (symbol == Symbol::LSETBRACK) {
          ENTER(bl, tl.TID, OBJECTS::STRUCTAG);
          tl.TS = tab_index;
          TAB[tl.TS].types = Types::RECS;
          tl.TP = Types::RECS;
          TAB[tl.TS].reference = -static_cast<int>(tl.TS);
          STRUCTTYP(&tl, tl.RF, tl.SZ);
          TAB[tl.TS].reference = static_cast<int>(tl.RF);
          TAB[tl.TS].address = static_cast<int>(tl.SZ);
          for (tl.I = 1; tl.I <= ctab_index; tl.I++) {
            if (CTAB[tl.I].ELREF == -tl.TS) {
              CTAB[tl.I].ELREF = static_cast<int>(tl.RF);
              CTAB[tl.I].ELSIZE = static_cast<int>(tl.SZ);
            }
          }
        } else {
          tl.X = LOC(bl, tl.TID);
          if (tl.X != 0) {
            if (TAB[tl.X].object != OBJECTS::STRUCTAG) {
              error(108);
            } else {
              tl.TP = Types::RECS;
              tl.RF = TAB[tl.X].reference;
              tl.SZ = TAB[tl.X].address;
            }
          }
        }
      } else {
        tl.TP = Types::RECS;
        STRUCTTYP(&tl, tl.RF, tl.SZ);
      }
    }
    su = 0;
    TEST(FSYS, su, 6);
  }
  types = tl.TP;
  RF = tl.RF;
  SZ = tl.SZ;
}

void PARAMETERLIST(BlockLocal *bl) {
  SymbolSet su, sv;
  su = type_set;
  su.set(static_cast<int>(Symbol::RPARENT), true);
  su.set(static_cast<int>(Symbol::VALUESY), true);
  INSYMBOL();
  TEST(su, bl->FSYS, 105);

  bool NOPARAMS = true;
  // int PCNT;
  bl->PCNT = 0;

  while (type_set.test(static_cast<int>(symbol)) ||
         symbol == Symbol::VALUESY) {
    bool VALUEFOUND;
    if (symbol == Symbol::VALUESY) {
      VALUEFOUND = true;
      INSYMBOL();
    } else {
      VALUEFOUND = false;
    }

    Types TP;
    int64_t RF, SZ;
    su = bl->FSYS;
    su.set(static_cast<int>(Symbol::SEMICOLON), true);
    su.set(static_cast<int>(Symbol::COMMA), true);
    su.set(static_cast<int>(Symbol::IDENT), true);
    su.set(static_cast<int>(Symbol::TIMES), true);
    su.set(static_cast<int>(Symbol::CHANSY), true);
    su.set(static_cast<int>(Symbol::LBRACK), true);
    su.set(static_cast<int>(Symbol::RPARENT), true);
    TYPF(bl, su, TP, RF, SZ);

    if ((TP == Types::VOIDS) && (symbol == Symbol::RPARENT) && NOPARAMS) {
      goto L56;
    }

    if ((TP == Types::VOIDS) && (symbol != Symbol::TIMES)) {
      error(151);
    }

    C_PNTCHANTYP(bl, TP, RF, SZ);

    if (symbol == Symbol::IDENT) {
      ENTERVARIABLE(bl, OBJECTS::VARIABLE);
    } else if ((symbol == Symbol::COMMA) || (symbol == Symbol::RPARENT)) {
      ID = dummy_name;
      dummy_name[13] = static_cast<char>(dummy_name[13] + 1);
      if (dummy_name[13] == '0') {
        dummy_name[12] = static_cast<char>(dummy_name[12] + 1);
      }
      ENTER(bl, ID, OBJECTS::VARIABLE);
    } else {
      error(2);
      ID = "              ";
    }

    NOPARAMS = false;

    if (symbol == Symbol::LBRACK) {
      INSYMBOL();
      int64_t ARF = 0;
      int64_t ASZ = 0;
      C_ARRAYTYP(bl, ARF, ASZ, true, TP, RF, SZ);
      TP = Types::ARRAYS;
      RF = ARF;
      SZ = ASZ;
    }

    bl->PCNT = bl->PCNT + 1;
    //           IF (TP = ARRAYS) AND VALUEFOUND THEN
    //              IF ATAB[RF].HIGH = 0 THEN error(117);
    //          IF ((TP = ARRAYS) AND NOT VALUEFOUND) OR (TP = CHANS)
    //            THEN NORMAL := FALSE
    //            ELSE NORMAL := TRUE;
    //          IF TP = PNTS THEN PNTPARAM := TRUE ELSE PNTPARAM := FALSE;
    if (TP == Types::ARRAYS && VALUEFOUND) {
      if (ATAB[RF].HIGH == 0) {
        error(117);
      }
    }

    TAB[tab_index].normal =
        (TP != Types::ARRAYS || VALUEFOUND) && TP != Types::CHANS;
    TAB[tab_index].PNTPARAM = TP == Types::PNTS;
    TAB[tab_index].types = TP;
    TAB[tab_index].reference = static_cast<int>(RF);
    TAB[tab_index].LEV = bl->LEVEL;
    TAB[tab_index].address = bl->DX;
    TAB[tab_index].size = SZ;

    if (TAB[tab_index].normal) {
      bl->DX = bl->DX + static_cast<int>(SZ);
    } else {
      bl->DX = bl->DX + 1;
    }

    if (symbol == Symbol::COMMA) {
      INSYMBOL();
    } else {
      su = 0;
      su.set(static_cast<int>(Symbol::RPARENT), true);
      TEST(su, bl->FSYS, 105);
    }
  }

L56:
  if (symbol == Symbol::RPARENT) {
    INSYMBOL();
  } else {
    su = 0;
    su.set(static_cast<int>(Symbol::RPARENT), true);
    sv = bl->FSYS;
    su.set(static_cast<int>(Symbol::LSETBRACK), true);
    TEST(su, sv, 4);
  }

  if (symbol != Symbol::SEMICOLON) {
    if (symbol == Symbol::LSETBRACK) {
      OK_BREAK = true;
      INSYMBOL();
      if (proto_index >= 0) {
        if ((bl->PCNT != BTAB[proto_ref].PARCNT) ||
            (bl->DX != BTAB[proto_ref].PSIZE)) {
          error(152);
        }
      }
    } else {
      sv = 0;
      su.set(static_cast<int>(Symbol::LSETBRACK), true);
      TEST(sv, bl->FSYS, 106);
    }
    OK_BREAK = true;
  } else if (proto_index >= 0) {
    error(153);
  }

  proto_index = -1;
}

auto GETFUNCID(const ALFA &FUNCNAME) -> int {
  for (int K = 1; K <= LIBMAX; K++) {
    if (FUNCNAME == LIBFUNC[K].NAME) {
      return LIBFUNC[K].IDNUM;
    }
  }
  return 0;
}

void FUNCDECLARATION(BlockLocal *bl, Types TP, int64_t RF, int64_t SZ) {
  bool ISFUN = true;
  SymbolSet su, sv;
  if (TP == Types::CHANS) {
    error(119);
    return_type.types = Types::NOTYP;
  } else {
    return_type.types = TP;
  }
  return_type.reference = RF;
  return_type.size = SZ;
  return_type.is_address = false;

  int T0 = tab_index;
  if (proto_index >= 0) {
    T0 = proto_index;
    Item PRORET = {Types::NOTYP, 0, 0l, false};
    PRORET.types = TAB[T0].types;
    PRORET.reference = TAB[T0].FREF;
    PRORET.size = TAB[T0].size;
    PRORET.is_address = false;
    if ((!TYPE_COMPATIBLE(return_type, PRORET)) ||
        (return_type.types != PRORET.types)) {
      error(152);
    }
    proto_ref = TAB[T0].reference;
  }

  TAB[T0].object = OBJECTS::FUNKTION;
  TAB[T0].types = TP;
  TAB[T0].FREF = RF;
  TAB[T0].LEV = bl->LEVEL;
  TAB[T0].normal = true;
  TAB[T0].size = SZ;
  TAB[T0].PNTPARAM = false;
  if (strcmp(TAB[T0].name.c_str(), "MAIN          ") == 0)
    MAINFUNC = T0;
  int LCSAV = line_count;
  EMIT(10);
  su = bl->FSYS | keyword_set | assigners_set | declaration_set;
  su.set(static_cast<int>(Symbol::LSETBRACK), true);
  BLOCK(bl->blkil, su, ISFUN, bl->LEVEL + 1, T0);
  EMIT(32 + ISFUN);
  if (line_count - LCSAV == 2) {
    // no need to generate code for a prototype declaration DE
    line_count = LCSAV;
    execution_count = 0; // no executable content , avoids 6 (noop) for breaks
  } else
    code[LCSAV].Y = line_count;
  OK_BREAK = false;

  if (HasIncludeFlag() && (TAB[T0].address == 0)) {
    TAB[T0].address = -GETFUNCID(TAB[T0].name);
  }

  if (symbol == Symbol::SEMICOLON || symbol == Symbol::RSETBRACK) {
    INSYMBOL();
  } else {
    error(104);
  }
  su = declaration_set;
  su.set(static_cast<int>(Symbol::INCLUDESY), true);
  su.set(static_cast<int>(Symbol::EOFSY), true);
  TEST(su, bl->FSYS, 6);
}

void CONSTANTDECLARATION(BlockLocal *bl) {
  SymbolSet su;
  INSYMBOL();
  su = 0;
  su.set(static_cast<int>(Symbol::IDENT), true);
  TEST(su, bl->FSYS, 2);

  if (symbol == Symbol::IDENT) {
    ENTER(bl, ID, OBJECTS::KONSTANT);
    INSYMBOL();
    CONREC C;
    su = bl->FSYS;
    su.set(static_cast<int>(Symbol::SEMICOLON), true);
    su.set(static_cast<int>(Symbol::COMMA), true);
    su.set(static_cast<int>(Symbol::IDENT), true);
    CONSTANT(bl, su, C);
    TAB[tab_index].types = C.TP;
    TAB[tab_index].reference = 0;
    TAB[tab_index].address = C.I;

    if (symbol == Symbol::SEMICOLON) {
      INSYMBOL();
    }
  }
}

void TYPEDECLARATION(BlockLocal *bl) {
  SymbolSet su;
  INSYMBOL();
  Types TP;
  int64_t RF, SZ, T1, ARF, ASZ;
  // int64_t ORF, OSZ;
  su = bl->FSYS;
  su.set(static_cast<int>(Symbol::SEMICOLON), true);
  su.set(static_cast<int>(Symbol::COMMA), true);
  su.set(static_cast<int>(Symbol::IDENT), true);
  su.set(static_cast<int>(Symbol::TIMES), true);
  su.set(static_cast<int>(Symbol::CHANSY), true);
  su.set(static_cast<int>(Symbol::LBRACK), true);
  TYPF(bl, su, TP, RF, SZ);
  C_PNTCHANTYP(bl, TP, RF, SZ);

  if (symbol != Symbol::IDENT) {
    error(2);
    strcpy(ID.data(), "              ");
  }

  ENTER(bl, ID, OBJECTS::TYPE1);
  T1 = tab_index;
  INSYMBOL();

  if (symbol == Symbol::LBRACK) {
    INSYMBOL();
    C_ARRAYTYP(bl, ARF, ASZ, true, TP, RF, SZ);
    TP = Types::ARRAYS;
    RF = ARF;
    SZ = ASZ;
  }

  TAB[T1].types = TP;
  TAB[T1].reference = RF;
  TAB[T1].address = SZ;

  TESTSEMICOLON(bl);
}

void GETLIST(BlockLocal *bl, Types TP) {
  SymbolSet su;
  if (symbol == Symbol::LSETBRACK) {
    int LBCNT = 1;
    INSYMBOL();

    while ((LBCNT > 0) && (symbol != Symbol::SEMICOLON)) {
      while (symbol == Symbol::LSETBRACK) {
        LBCNT = LBCNT + 1;
        INSYMBOL();
      }

      CONREC LISTVAL = {Types::NOTYP, 0l};
      su = 0;
      su.set(static_cast<int>(Symbol::COMMA), true);
      su.set(static_cast<int>(Symbol::RSETBRACK), true);
      su.set(static_cast<int>(Symbol::LSETBRACK), true);
      su.set(static_cast<int>(Symbol::SEMICOLON), true);
      CONSTANT(bl, su, LISTVAL);

      switch (TP) {
      case Types::REALS:
        if (LISTVAL.TP == Types::INTS) {
          INITABLE[ITPNT].IVAL = RTAG;
          INITABLE[ITPNT].RVAL = LISTVAL.I;
        } else if (LISTVAL.TP == Types::REALS) {
          INITABLE[ITPNT].IVAL = RTAG;
          INITABLE[ITPNT].RVAL = CONTABLE[LISTVAL.I];
        } else {
          error(138);
        }
        break;
      case Types::INTS:
        if ((LISTVAL.TP == Types::INTS) || (LISTVAL.TP == Types::CHARS)) {
          INITABLE[ITPNT].IVAL = LISTVAL.I;
        } else {
          error(138);
        }
        break;
      case Types::CHARS:
        if (LISTVAL.TP == Types::CHARS) {
          INITABLE[ITPNT].IVAL = LISTVAL.I;
        } else {
          error(138);
        }
        break;
      case Types::BOOLS:
        if (LISTVAL.TP == Types::BOOLS) {
          INITABLE[ITPNT].IVAL = LISTVAL.I;
        } else {
          error(138);
        }
        break;
      default:
        break;
      }

      ITPNT = ITPNT + 1;

      while (symbol == Symbol::RSETBRACK) {
        LBCNT = LBCNT - 1;
        INSYMBOL();
      }

      if (LBCNT > 0) {
        if (symbol == Symbol::COMMA) {
          INSYMBOL();
        } else {
          error(135);
        }
      }
    }
  } else if (symbol == Symbol::STRNG) {
    if (TP != Types::CHARS) {
      error(138);
    } else {
      for (int CI = 1; CI <= STRING_LENGTH; CI++) {
        INITABLE[ITPNT].IVAL = STAB[INUM + CI - 1];
        ITPNT = ITPNT + 1;
      }
      INITABLE[ITPNT].IVAL = 0;
      ITPNT = ITPNT + 1;
    }
    INSYMBOL();
  } else {
    su = 0;
    su.set(static_cast<int>(Symbol::SEMICOLON), true);
    su.set(static_cast<int>(Symbol::IDENT), true);
    SKIP(su, 106);
  }
}

void PROCDECLARATION(BlockLocal *bl) {
  SymbolSet su;
  bool ISFUN = false;
  return_type.types = Types::VOIDS;

  if (symbol != Symbol::IDENT) {
    error(2);
    strcpy(ID.data(), "              ");
  }

  ENTER(bl, ID, OBJECTS::PROZEDURE);
  int T0 = tab_index;

  if (proto_index >= 0) {
    T0 = proto_index;
    if (TAB[T0].object != OBJECTS::PROZEDURE) {
      error(152);
    }
    proto_ref = TAB[T0].reference;
  }

  TAB[T0].normal = true;
  INSYMBOL();

  int LCSAV = line_count;
  EMIT(10);
  su = bl->FSYS | keyword_set | assigners_set | declaration_set;
  su.set(static_cast<int>(Symbol::RSETBRACK), true);
  BLOCK(bl->blkil, su, ISFUN, bl->LEVEL + 1, T0);
  EMIT(32 + ISFUN);
  if (line_count - LCSAV == 2) {
    line_count =
        LCSAV; // no need to generate code for a prototype declaration DE
    execution_count = 0; // no executable content , avoids 6 (noop) for breaks
  } else {
    code.at(LCSAV).Y = line_count;
  }
  OK_BREAK = false;

  if (HasIncludeFlag() && (TAB[T0].address == 0)) {
    TAB[T0].address = -GETFUNCID(TAB[T0].name);
  }

  if (symbol == Symbol::SEMICOLON || symbol == Symbol::RSETBRACK) {
    INSYMBOL();
  } else {
    error(104);
  }
  su = declaration_set;
  su.set(static_cast<int>(Symbol::INCLUDESY), true);
  su.set(static_cast<int>(Symbol::EOFSY), true);
  TEST(su, bl->FSYS, 6);
}

void VARIABLEDECLARATION(BlockLocal *bl) {
  bool TYPCALL = true;
  bool PROCDEFN = false;
  bool DONE = false;
  bool FUNCDEC = false;
  SymbolSet su;
  int T0 = 0;
  int64_t RI = 0;
  int64_t RF = 0;
  int64_t SZ = 0;
  int64_t ARF = 0;
  int64_t ASZ = 0;
  int64_t ORF = 0;
  int64_t OSZ = 0;
  int64_t LSTART = 0;
  int64_t LEND = 0;
  Types TP;
  Types OTP;
  Item X{};
  Item Y{};
  TP = Types::NOTYP;
  if (symbol == Symbol::IDENT) {
    bl->UNDEFMSGFLAG = false;
    T0 = LOC(bl, ID);
    bl->UNDEFMSGFLAG = true;
    if (T0 == 0) {
      TYPCALL = false;
      TP = Types::INTS;
      SZ = 1;
      RF = 0;
    }
  }

  if (TYPCALL) {
    su = bl->FSYS;
    su.set(static_cast<int>(Symbol::SEMICOLON), true);
    su.set(static_cast<int>(Symbol::COMMA), true);
    su.set(static_cast<int>(Symbol::IDENT), true);
    su.set(static_cast<int>(Symbol::TIMES), true);
    su.set(static_cast<int>(Symbol::CHANSY), true);
    su.set(static_cast<int>(Symbol::LBRACK), true);
    TYPF(bl, su, TP, RF, SZ);
  }

  if ((TP == Types::VOIDS) && (symbol != Symbol::TIMES)) {
    PROCDECLARATION(bl);
    PROCDEFN = true;
  }

  OTP = TP;
  ORF = RF;
  OSZ = SZ;

  DONE = false;
  FUNCDEC = false;
  if ((TP != Types::RECS || symbol != Symbol::SEMICOLON) && !PROCDEFN) {
    do {
      C_PNTCHANTYP(bl, TP, RF, SZ);
      if (symbol != Symbol::IDENT) {
        error(2);
        strcpy(ID.data(), "              ");
      }
      T0 = tab_index;
      ENTERVARIABLE(bl, OBJECTS::VARIABLE);

      if (!TYPCALL && (symbol != Symbol::LPARENT)) {
        TP = Types::NOTYP;
        SZ = 0;
        TYPCALL = true;
        error(0);
      }

      if (symbol == Symbol::LBRACK) {
        INSYMBOL();
        C_ARRAYTYP(bl, ARF, ASZ, true, TP, RF, SZ);
        TP = Types::ARRAYS;
        RF = ARF;
        SZ = ASZ;
      }

      if (symbol == Symbol::LPARENT) {
        FUNCDEC = true;
        FUNCDECLARATION(bl, TP, RF, SZ);
      } else {
        if (proto_index >= 0) {
          error(1);
          proto_index = -1;
        }

        T0 = T0 + 1;
        TAB[T0].types = TP;
        TAB[T0].reference = RF;
        TAB[T0].LEV = bl->LEVEL;
        TAB[T0].address = bl->DX;
        TAB[T0].normal = true;
        bl->DX += SZ;
        TAB[T0].size = SZ;
        TAB[T0].PNTPARAM = false;

        if (symbol == Symbol::BECOMES) {
          if ((TP == Types::CHANS) || (TP == Types::LOCKS) ||
              (TP == Types::RECS)) {
            error(136);
          } else if (TP == Types::ARRAYS) {
            while (TP == Types::ARRAYS) {
              TP = ATAB[RF].ELTYP;
              RF = ATAB[RF].ELREF;
            }
            if (!standard_set.test(static_cast<int>(TP))) {
              error(137);
            }
            INSYMBOL();
            LSTART = ITPNT;
            GETLIST(bl, TP);
            LEND = ITPNT;
            if (TAB[T0].size == 0) {
              ATAB[TAB[T0].reference].HIGH = LEND - LSTART - 1;
              TAB[T0].size = LEND - LSTART;
              bl->DX = bl->DX + TAB[T0].size;
            }
            if (TAB[T0].size < LEND - LSTART) {
              error(139);
            } else {
              EMIT2(0, TAB[T0].LEV, TAB[T0].address);
              EMIT2(82, LSTART, LEND);
              RI = (TP == Types::REALS) ? 2 : 1;
              EMIT2(83, TAB[T0].size - (LEND - LSTART), RI);
            }
          } else {
            X.types = TP;
            X.reference = RF;
            X.size = SZ;
            X.is_address = true;
            EMIT2(0, bl->LEVEL, TAB[T0].address);
            INSYMBOL();
            su = bl->FSYS;
            su.set(static_cast<int>(Symbol::COMMA), true);
            su.set(static_cast<int>(Symbol::SEMICOLON), true);
            EXPRESSION(bl, su, Y);
            if (!TYPE_COMPATIBLE(X, Y)) {
              error(46);
            } else {
              if ((X.types == Types::REALS) && (Y.types == Types::INTS)) {
                EMIT(91);
              } else {
                EMIT(38);
              }
              EMIT(111); // added in V2.2
            }
          }
        }

        if (symbol == Symbol::COMMA) {
          INSYMBOL();
        } else {
          DONE = true;
        }

        TP = OTP;
        RF = ORF;
        SZ = OSZ;
      }
    } while (!DONE && !FUNCDEC);
  }
  if (!FUNCDEC && !PROCDEFN) {
    TESTSEMICOLON(bl);
  }
}

} // namespace Cstar
