#include "cs_basic.h"
#include "cs_block.h"
#include "cs_compile.h"
#include "cs_errors.h"
#include "cs_global.h"
#include "cs_interpret.h"
#include <cstring>

namespace Cstar {
struct TypLocal {
  SYMSET FSYS;
  TYPES TP;
  long RF;
  long SZ;
  /* parms above */
  long X, I;
  // long J;
  // TYPES ELTP;
  // long ELRF;
  // long ELSZ;
  long TS;
  ALFA TID;
  BlockLocal *bl;
};
void TEST(SYMSET &, SYMSET &, int);
extern bool inCLUDEFLAG();
extern void C_PNTCHANTYP(BlockLocal *, TYPES &, long &, long &);
extern void BASICEXPRESSION(BlockLocal *, SYMSET, ITEM &);
void ENTERCHANNEL();
void ENTERVARIABLE(BlockLocal *, OBJECTS);
void EXPRESSION(BlockLocal *bl, SYMSET FSYS, ITEM &X);
void LOADVAL(ITEM &);
void TYPF(BlockLocal *, SYMSET FSYS, TYPES &TP, long &RF, long &SZ);
void CONSTANT(BlockLocal *, SYMSET &, CONREC &);
void ENTERARRAY(TYPES TP, int L, int H);
extern void ENTER(BlockLocal *bl, ALFA, OBJECTS);
int LOC(BlockLocal *, ALFA);
bool TYPE_COMPATIBLE(ITEM, ITEM);
extern void BLOCK(InterpLocal *il, SYMSET fsys, bool ISFUN, int LEVEL, int PRT);
void SKIP(SYMSET, int);
void TESTSEMICOLON(BlockLocal *);
extern int MAINFUNC;
extern void CALL(BlockLocal *, SYMSET &, int I);
void SELECTOR(BlockLocal *, SYMSET FSYS, ITEM &V);
void PNTSELECT(ITEM &);
bool PNT_COMPATIBLE(ITEM V, ITEM W);
bool ARRAY_COMPATIBLE(ITEM V, ITEM W);
void RSELECT(ITEM &V) {
  int A;
  if (SY == PERIOD)
    INSYMBOL();
  if (SY != IDENT) {
    ERROR(2);
  } else if (V.TYP != RECS) {
    ERROR(31);
  } else {
    A = V.REF;
    strcpy(TAB[0].NAME, ID);
    while (strcmp(TAB[A].NAME, ID) != 0)
      A = TAB[A].LINK;
    if (A == 0)
      ERROR(0);
    else {
      EMIT1(24, TAB[A].ADR);
      EMIT(15);
      V.TYP = TAB[A].TYP;
      V.REF = TAB[A].REF;
      V.SIZE = TAB[A].SIZE;
    }
    INSYMBOL();
  }
}
void PNTSELECT(ITEM &V) {
  int A;
  INSYMBOL();
  if (V.TYP != PNTS) {
    ERROR(13);
  } else {
    A = V.REF;
    V.TYP = CTAB[A].ELTYP;
    V.REF = CTAB[A].ELREF;
    V.SIZE = CTAB[A].ELSIZE;
    EMIT(34);
    RSELECT(V);
  }
}

void SELECTOR(BlockLocal *bl, SYMSET FSYS, ITEM &V) {
  ITEM X;
  int A;
  SYMSET su;
  if (V.TYP == CHANS)
    ERROR(11);
  su[COMMA] = true;
  su[RBRACK] = true;
  while (SELECTSYS[SY]) {
    if (SY == LBRACK) {
      do {
        INSYMBOL();
        EXPRESSION(bl, FSYS | su, X);
        if (V.TYP != ARRAYS)
          ERROR(28);
        else {
          A = V.REF;
          if (ATAB[A].INXTYP != X.TYP)
            ERROR(26);
          else
            EMIT1(21, A);
          V.TYP = ATAB[A].ELTYP;
          V.REF = ATAB[A].ELREF;
          V.SIZE = ATAB[A].ELSIZE;
        }
      } while (SY == COMMA);
      if (SY == RBRACK)
        INSYMBOL();
      else
        ERROR(12);
    } else if (SY == PERIOD)
      RSELECT(V);
    else
      PNTSELECT(V);
  }
  su = 0;
  TEST(FSYS, su, 6);
}

bool COMPATIBLE(ITEM V, ITEM W) {
  // ITEM Z;
  bool rtn = true;
  if (V.TYP == W.TYP) {
    switch (V.TYP) {
    case INTS:
    case REALS:
    case CHARS:
    case BOOLS:
    case LOCKS:
      rtn = true;
      break;
    case RECS:
      if (CTAB[V.REF].ELREF == CTAB[W.REF].ELREF)
        rtn = true;
      else
        rtn = false;
      break;
    case CHANS:
      V.TYP = CTAB[V.REF].ELTYP;
      W.TYP = CTAB[W.REF].ELTYP;
      V.REF = CTAB[V.REF].ELREF;
      W.REF = CTAB[W.REF].ELREF;
      rtn = COMPATIBLE(V, W);
      break;
    case PNTS:
      if (W.REF != 0) {
        V.TYP = CTAB[V.REF].ELTYP;
        W.TYP = CTAB[W.REF].ELTYP;
        if (V.TYP != VOIDS && W.TYP != VOIDS) {
          V.REF = CTAB[V.REF].ELREF;
          W.REF = CTAB[W.REF].ELREF;
          rtn = PNT_COMPATIBLE(V, W);
        }
      }
      break;
    case ARRAYS:
      ARRAY_COMPATIBLE(V, W);
      break;
    default:
      break;
    }
  } else
    rtn = false;
  return rtn;
}

bool ARRAY_COMPATIBLE(ITEM V, ITEM W) {
  bool DIM_OK = true;
  while (V.TYP == ARRAYS && W.TYP == ARRAYS) {
    if (ATAB[V.REF].HIGH != ATAB[W.REF].HIGH ||
        ATAB[V.REF].ELTYP != ATAB[W.REF].ELTYP)
      DIM_OK = false;
    V.TYP = ATAB[V.REF].ELTYP;
    V.REF = ATAB[V.REF].ELREF;
    W.TYP = ATAB[W.REF].ELTYP;
    W.REF = ATAB[W.REF].ELREF;
  }
  if (DIM_OK)
    return COMPATIBLE(V, W);
  else
    return false;
}

bool PNT_COMPATIBLE(ITEM V, ITEM W) {
  while (V.TYP == ARRAYS) {
    V.TYP = ATAB[V.REF].ELTYP;
    V.REF = ATAB[V.REF].ELREF;
  }
  while (W.TYP == ARRAYS) {
    W.TYP = ATAB[W.REF].ELTYP;
    W.REF = ATAB[W.REF].ELREF;
  }
  return COMPATIBLE(V, W);
}

bool TYPE_COMPATIBLE(ITEM X, ITEM Y) {
  // ITEM W;
  // if ((inset(X.TYP, STANTYPS + LOCKS)) && (inset(Y.TYP, STANTYPS + LOCKS)))
  if ((STANTYPS[X.TYP] || X.TYP == LOCKS) &&
      (STANTYPS[Y.TYP] || Y.TYP == LOCKS)) {
    if (X.TYP == Y.TYP || (X.TYP == REALS && Y.TYP == INTS) ||
        (X.TYP == INTS && Y.TYP == CHARS) || (X.TYP == CHARS && Y.TYP == INTS))
      return true;
    else
      return false;
  } else if (X.TYP == RECS && Y.TYP == RECS) {
    if (X.REF == Y.REF)
      return true;
    else
      return false;
  } else if (X.TYP == ARRAYS && Y.TYP == ARRAYS)
    return ARRAY_COMPATIBLE(X, Y);
  else if (X.TYP == PNTS && Y.TYP == PNTS)
    return PNT_COMPATIBLE(X, Y);
  else if (X.TYP == PNTS && Y.TYP == ARRAYS) {
    X.TYP = CTAB[X.REF].ELTYP;
    X.REF = CTAB[X.REF].ELREF;
    return PNT_COMPATIBLE(X, Y);
  } else
    return false;
  if (X.TYP == NOTYP || Y.TYP == NOTYP) // dead code
    return true;
}

void CHANELEMENT(ITEM &V) {
  int vref = V.REF;
  V.TYP = CTAB[vref].ELTYP;
  V.SIZE = CTAB[vref].ELSIZE;
  V.REF = CTAB[vref].ELREF;
}

TYPES RESULTTYPE(TYPES A, TYPES B) {
  if ((A == INTS && B == CHARS) || (A == CHARS && B == INTS))
    return CHARS;
  else if (A > INTS || B > INTS) {
    ERROR(33);
    return NOTYP;
  } else if (A == NOTYP || B == NOTYP)
    return NOTYP;
  else if (A == REALS || B == REALS)
    return REALS;
  else
    return INTS;
}

TYPES SUMRESULTTYPE(TYPES A, TYPES B) {
  if ((A == INTS && B == CHARS) || (A == CHARS && B == INTS))
    return CHARS;
  else if ((A == INTS && B == PNTS) || (A == PNTS && B == INTS))
    return PNTS;
  else if (A > INTS || B > INTS) {
    ERROR(33);
    return NOTYP;
  } else if (A == NOTYP || B == NOTYP)
    return NOTYP;
  else if (A == REALS || B == REALS)
    return REALS;
  else
    return INTS;
}

void FACTOR(BasicLocal *bx, SYMSET &FSYS, ITEM &X) {
  int A, I;
  SYMBOL OP;
  TYPES TYPCAST;
  SYMSET su, sv;
  su = 0;
  su[DBLQUEST] = true;
  X.TYP = NOTYP;
  X.REF = 0;
  X.ISADDR = true;
  TEST(FACBEGSYS, FSYS, 8);
  if (SY == IDENT) {
    I = LOC(bx->bl, ID);
    INSYMBOL();
    switch (TAB[I].OBJ) {
    case KONSTANT:
      X.TYP = TAB[I].TYP;
      X.REF = 0;
      X.ISADDR = false;
      if (TAB[I].TYP == REALS) {
        EMIT1(87, TAB[I].ADR);
      } else {
        EMIT1(24, TAB[I].ADR);
      }
      break;
    case VARIABLE:
      X.TYP = TAB[I].TYP;
      X.REF = TAB[I].REF;
      X.SIZE = TAB[I].SIZE;
      if (SELECTSYS[SY]) {
        bx->F = (TAB[I].NORMAL) ? 0 : 1;
        EMIT2(bx->F, TAB[I].LEV, TAB[I].ADR);
        SELECTOR(bx->bl, FSYS | su, X);
        if (X.TYP == CHANS && SY == DBLQUEST) {
          INSYMBOL();
          EMIT(94);
          X.TYP = BOOLS;
          X.ISADDR = false;
        }
      } else {
        if (TAB[I].FORLEV == 0) {
          if (X.TYP == CHANS && SY == DBLQUEST) {
            INSYMBOL();
            X.TYP = BOOLS;
            X.ISADDR = false;
            if (TAB[I].NORMAL)
              bx->F = 95;
            else
              bx->F = 96;
          } else {
            if (TAB[I].NORMAL)
              bx->F = 0;
            else
              bx->F = 1;
          }
          EMIT2(bx->F, TAB[I].LEV, TAB[I].ADR);
          if (bx->F == 0 && TAB[I].PNTPARAM)
            bx->F = 1;
        } else {
          X.ISADDR = false;
          if (TAB[I].FORLEV > 0) {
            EMIT2(73, TAB[I].LEV, TAB[I].ADR);
          } else {
            EMIT(81);
          }
        }
      }
      break;
    case TYPE1:
    case PROZEDURE:
    case COMPONENT:
      ERROR(44);
      break;
    case FUNKTION:
      X.TYP = TAB[I].TYP;
      X.REF = TAB[I].FREF;
      X.SIZE = TAB[I].SIZE;
      X.ISADDR = false;
      if (TAB[I].LEV != 0) {
        CALL(bx->bl, FSYS, I);
      } else if (TAB[I].ADR == 24) {
        if (SY == LPARENT)
          INSYMBOL();
        else
          ERROR(9);
        su = 0;
        su[IDENT] = true;
        su[STRUCTSY] = true;
        sv = FSYS;
        sv[RPARENT] = true;
        TEST(su, sv, 6);
        if (SY == IDENT) {
          int I = LOC(bx->bl, ID);
          if (TAB[I].OBJ == VARIABLE) {
            EMIT1(24, TAB[I].SIZE);
          } else if (TAB[I].OBJ == TYPE1) {
            EMIT1(24, TAB[I].ADR);
          } else {
            ERROR(6);
          }
        } else if (SY == STRUCTSY) {
          INSYMBOL();
          if (SY == IDENT) {
            int I = LOC(bx->bl, ID);
            if (I != 0) {
              if (TAB[I].OBJ != STRUCTAG)
                ERROR(108);
              else
                EMIT1(24, TAB[I].ADR);
            }
          } else {
            ERROR(6);
          }
        } else {
          ERROR(6);
        }
        INSYMBOL();
        if (SY == RPARENT)
          INSYMBOL();
        else
          ERROR(4);
      } else {
        if (SY == LPARENT) {
          INSYMBOL();
          if (SY == RPARENT)
            INSYMBOL();
          else
            ERROR(4);
        }
        EMIT1(8, TAB[I].ADR);
      }
      break;
    default:
      break;
    }
  } else if (SY == CHARCON || SY == INTCON) {
    if (SY == CHARCON) {
      X.TYP = CHARS;
    } else {
      X.TYP = INTS;
    }
    EMIT1(24, INUM);
    X.REF = 0;
    X.ISADDR = false;
    INSYMBOL();
  } else if (SY == STRNG) {
    X.TYP = PNTS;
    ENTERCHANNEL();
    CTAB[C].ELTYP = CHARS;
    CTAB[C].ELREF = 0;
    CTAB[C].ELSIZE = 1;
    X.REF = C;
    X.ISADDR = false;
    EMIT2(13, INUM, SLENG);
    INSYMBOL();
  } else if (SY == REALCON) {
    X.TYP = REALS;
    CONTABLE[CPNT] = RNUM;
    EMIT1(87, CPNT);
    if (CPNT >= RCMAX) {
      FATAL(9);
    } else {
      CPNT = CPNT + 1;
    }
    X.REF = 0;
    X.ISADDR = false;
    INSYMBOL();
  } else if (SY == LPARENT) {
    INSYMBOL();
    TYPCAST = NOTYP;
    if (SY == IDENT) {
      int I = LOC(bx->bl, ID);
      if (TAB[I].OBJ == TYPE1) {
        TYPCAST = TAB[I].TYP;
        INSYMBOL();
        if (SY == TIMES) {
          TYPCAST = PNTS;
          INSYMBOL();
        }
        if (SY != RPARENT) {
          ERROR(121);
        } else {
          INSYMBOL();
        }
        if (!(STANTYPS[TYPCAST] || TYPCAST == PNTS)) {
          ERROR(121);
        }
        FACTOR(bx, FSYS, X);
        if (X.ISADDR) {
          LOADVAL(X);
        }
        if (TYPCAST == INTS) {
          if (X.TYP == REALS) {
            EMIT1(8, 10);
          }
        } else if (TYPCAST == BOOLS) {
          if (X.TYP == REALS) {
            EMIT1(8, 10);
          }
          EMIT(114);
        } else if (TYPCAST == CHARS) {
          if (X.TYP == REALS) {
            EMIT1(8, 10);
          }
          EMIT1(8, 5);
        } else if (TYPCAST == REALS) {
          EMIT(26);
        } else if (TYPCAST == PNTS) {
          if (X.TYP != PNTS || CTAB[X.REF].ELTYP != VOIDS) {
            ERROR(121);
          }
        } else {
          ERROR(121);
        }
        X.TYP = TYPCAST;
      }
    }
    if (TYPCAST == NOTYP) {
      sv = FSYS;
      sv[RPARENT] = true;
      BASICEXPRESSION(bx->bl, sv, X);
      if (SY == RPARENT) {
        INSYMBOL();
      } else {
        ERROR(4);
      }
    }
  } else if (SY == NOTSY) {
    INSYMBOL();
    FACTOR(bx, FSYS, X);
    if (X.ISADDR) {
      LOADVAL(X);
    }
    if (X.TYP == BOOLS) {
      EMIT(35);
    } else if (X.TYP != NOTYP) {
      ERROR(32);
    }
  } else if (SY == TIMES) {
    INSYMBOL();
    FACTOR(bx, FSYS, X);
    if (X.ISADDR) {
      LOADVAL(X);
    }
    if (X.TYP == PNTS) {
      int A = X.REF;
      X.TYP = CTAB[A].ELTYP;
      X.REF = CTAB[A].ELREF;
      X.SIZE = CTAB[A].ELSIZE;
      X.ISADDR = true;
    } else if (X.TYP != NOTYP) {
      ERROR(13);
    }
  } else if (SY == PLUS || SY == MINUS) {
    int OP = SY;
    INSYMBOL();
    FACTOR(bx, FSYS, X);
    if (X.ISADDR) {
      LOADVAL(X);
    }
    if (X.TYP == INTS || X.TYP == REALS || X.TYP == CHARS || X.TYP == PNTS) {
      if (OP == MINUS) {
        EMIT(36);
      }
    } else {
      ERROR(33);
    }
  } else if (SY == INCREMENT) {
    INSYMBOL();
    FACTOR(bx, FSYS, X);
    if (!(X.TYP == NOTYP || X.TYP == INTS || X.TYP == REALS || X.TYP == CHARS ||
          X.TYP == PNTS)) {
      ERROR(110);
    } else {
      if ((X.TYP == NOTYP || X.TYP == INTS || X.TYP == REALS ||
           X.TYP == CHARS) &&
          !X.ISADDR) {
        ERROR(110);
      }
      int A = 1;
      if (X.TYP == PNTS) {
        A = CTAB[X.REF].ELSIZE;
      }
      if (X.TYP != NOTYP) {
        EMIT2(108, A, 0);
      }
      X.ISADDR = false;
    }
  } else if (SY == DECREMENT) {
    INSYMBOL();
    FACTOR(bx, FSYS, X);
    if (!(X.TYP == NOTYP || X.TYP == INTS || X.TYP == REALS || X.TYP == CHARS ||
          X.TYP == PNTS)) {
      ERROR(111);
    } else {
      if ((X.TYP == NOTYP || X.TYP == INTS || X.TYP == REALS ||
           X.TYP == CHARS) &&
          !X.ISADDR) {
        ERROR(111);
      }
      A = 1;
      if (X.TYP == PNTS) {
        A = CTAB[X.REF].ELSIZE;
      }
      if (X.TYP != NOTYP) {
        EMIT2(109, A, 0);
      }
      X.ISADDR = false;
    }
  }
  if (SELECTSYS[SY] && X.ISADDR) {
    su = FSYS;
    su[DBLQUEST] = true;
    SELECTOR(bx->bl, su, X);
  }
  switch (SY) {
  case DBLQUEST:
    if (X.TYP == CHANS) {
      INSYMBOL();
      EMIT(94);
      X.TYP = BOOLS;
      X.ISADDR = false;
    } else
      ERROR(103);
    break;
  case INCREMENT:
    if (!(X.TYP == NOTYP || X.TYP == INTS || X.TYP == REALS || X.TYP == CHARS ||
          X.TYP == PNTS)) {
      ERROR(110);
    } else {
      if (!(X.TYP == NOTYP || X.TYP == INTS || X.TYP == REALS ||
            X.TYP == CHARS) &&
          !X.ISADDR)
        ERROR(110);
      A = 1;
      if (X.TYP == PNTS)
        A = CTAB[X.REF].ELSIZE;
      if (X.TYP != NOTYP)
        EMIT2(108, A, 1);
      X.ISADDR = false;
    }
    INSYMBOL();
    break;
  case DECREMENT:
    if (!(X.TYP == NOTYP || X.TYP == INTS || X.TYP == REALS || X.TYP == CHARS ||
          X.TYP == PNTS)) {
      ERROR(111);
    } else {
      if (!(X.TYP == NOTYP || X.TYP == INTS || X.TYP == REALS ||
            X.TYP == CHARS) &&
          !X.ISADDR)
        ERROR(111);
      A = 1;
      if (X.TYP == PNTS)
        A = CTAB[X.REF].ELSIZE;
      if (X.TYP != NOTYP)
        EMIT2(109, A, 1);
      X.ISADDR = false;
    }
    INSYMBOL();
    break;
  default:
    break;
  }
  TEST(FSYS, FACBEGSYS, 6);
}
void EXPRESSION(BlockLocal *bl, SYMSET FSYS, ITEM &X) {
  // ITEM Y;
  // SYMBOL OP;
  bool ADDR_REQUIRED;
  long RF, SZ;
  TYPES TP;
  SYMSET su, sv;
  if (SY == ADDRSY) {
    ADDR_REQUIRED = true;
    INSYMBOL();
  } else {
    ADDR_REQUIRED = false;
  }
  su = FSYS;
  su[TIMES] = true;
  su[CHANSY] = true;
  su[LBRACK] = true;
  if (SY == NEWSY) {
    INSYMBOL();
    TYPF(bl, su, TP, RF, SZ);
    C_PNTCHANTYP(bl, TP, RF, SZ);
    ENTERCHANNEL();
    CTAB[C].ELTYP = TP;
    CTAB[C].ELREF = RF;
    CTAB[C].ELSIZE = SZ;
    X.TYP = PNTS;
    X.SIZE = 1;
    X.REF = C;
    EMIT1(99, SZ);
  } else {
    BASICEXPRESSION(bl, FSYS, X);
    if (ADDR_REQUIRED) {
      if (!X.ISADDR) {
        ERROR(115);
      } else {
        ENTERCHANNEL();
        CTAB[C].ELTYP = X.TYP;
        CTAB[C].ELREF = X.REF;
        CTAB[C].ELSIZE = X.SIZE;
        X.TYP = PNTS;
        X.SIZE = 1;
        X.REF = C;
      }
    } else {
      if (X.ISADDR) {
        LOADVAL(X);
      }
    }
  }
  X.ISADDR = false;
}

void C_ARRAYTYP(BlockLocal *bl, long &AREF, long &ARSZ, bool FIRSTINDEX,
                TYPES ELTP, long ELRF, long ELSZ) {
  CONREC HIGH = {NOTYP, 0l};
  long RF, SZ;
  TYPES TP;
  SYMSET su;
  if ((SY == RBRACK) && FIRSTINDEX) {
    HIGH.TP = INTS;
    HIGH.I = 0;
  } else {
    su = bl->FSYS;
    su[RBRACK] = true;
    CONSTANT(bl, su, HIGH);
    if ((HIGH.TP != INTS) || (HIGH.I < 1)) {
      ERROR(27);
      HIGH.TP = INTS;
      HIGH.I = 0;
    }
  }

  ENTERARRAY(INTS, 0, HIGH.I - 1);
  AREF = A;
  if (SY == RBRACK) {
    INSYMBOL();
  } else {
    ERROR(12);
  }

  if (SY == LBRACK) {
    INSYMBOL();
    TP = ARRAYS;
    C_ARRAYTYP(bl, RF, SZ, false, ELTP, ELRF, ELSZ);
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

void C_PNTCHANTYP(BlockLocal *bl, TYPES &TP, long &RF, long &SZ) {
  bool CHANFOUND;
  long ARF, ASZ;
  CHANFOUND = false;
  while (SY == TIMES || SY == CHANSY || SY == LBRACK) {
    switch (SY) {
    case LBRACK:
      INSYMBOL();
      C_ARRAYTYP(bl, ARF, ASZ, true, TP, RF, SZ);
      TP = ARRAYS;
      RF = ARF;
      SZ = ASZ;
      break;
    case TIMES:
    case CHANSY:
      ENTERCHANNEL();
      CTAB[C].ELTYP = TP;
      CTAB[C].ELREF = RF;
      CTAB[C].ELSIZE = SZ;
      SZ = 1;
      RF = C;
      if (SY == TIMES) {
        TP = PNTS;
      }
      if (SY == CHANSY) {
        TP = CHANS;
        if (CHANFOUND) {
          ERROR(11);
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
  long T0;
  // long T1;
  long RF, SZ;
  long ARF, ASZ, ORF, OSZ;
  TYPES TP, OTP;
  bool DONE;
  BlockLocal *bl = tl->bl;
  SYMSET su;
  SYMSET TSYS; // {SEMICOLON, COMMA, IDENT, TIMES, CHANSY, LBRACK};
  // TSYS = FSYS;
  TSYS = 0;
  TSYS[SEMICOLON] = true;
  TSYS[COMMA] = true;
  TSYS[IDENT] = true;
  TSYS[TIMES] = true;
  TSYS[CHANSY] = true;
  TSYS[LBRACK] = true;

  TEST(TYPEBEGSYS, tl->FSYS, 109);

  while (TYPEBEGSYS[SY]) {
    TYPF(bl, TSYS, TP, RF, SZ);
    OTP = TP;
    ORF = RF;
    OSZ = SZ;
    DONE = false;

    do {
      C_PNTCHANTYP(bl, TP, RF, SZ);
      if (SY != IDENT) {
        ERROR(2);
        strcpy(ID, "              ");
      }
      T0 = Tx;
      ENTERVARIABLE(bl, COMPONENT);

      if (SY == LBRACK) {
        INSYMBOL();
        C_ARRAYTYP(bl, ARF, ASZ, true, TP, RF, SZ);
        TP = ARRAYS;
        RF = ARF;
        SZ = ASZ;
      }

      T0 = T0 + 1;
      TAB[T0].TYP = TP;
      TAB[T0].REF = RF;
      TAB[T0].LEV = bl->LEVEL;
      TAB[T0].ADR = bl->RDX;
      TAB[T0].NORMAL = true;
      bl->RDX = bl->RDX + SZ;
      TAB[T0].SIZE = SZ;
      TAB[T0].PNTPARAM = false;

      if (SY == COMMA) {
        INSYMBOL();
      } else {
        DONE = true;
      }

      TP = OTP;
      RF = ORF;
      SZ = OSZ;
    } while (!DONE);

    if (SY == SEMICOLON) {
      INSYMBOL();
    } else {
      ERROR(14);
    }
    su = TYPEBEGSYS;
    su[RSETBRACK] = true;
    TEST(su, tl->FSYS, 108);
  }
}

void STRUCTTYP(TypLocal *tl, long &RREF, long &RSZ) {
  SYMSET su;
  BlockLocal *bl = tl->bl;
  if (SY == LSETBRACK) {
    INSYMBOL();
  } else {
    ERROR(106);
  }

  long SAVEDX = bl->RDX;
  bl->RDX = 0;
  long SAVELAST = bl->RLAST;
  bl->RLAST = 0;
  C_COMPONENTDECLARATION(tl);

  if (SY == RSETBRACK) {
    INSYMBOL();
  } else {
    ERROR(104);
  }
  su = 0;
  su[IDENT] = true;
  su[SEMICOLON] = true;
  su[CHANSY] = true;
  su[TIMES] = true;
  TEST(su, tl->FSYS, 6);

  RREF = bl->RLAST;
  bl->RLAST = SAVELAST;
  RSZ = bl->RDX;
  bl->RDX = SAVEDX;
}

void TYPF(BlockLocal *bl, SYMSET FSYS, TYPES &TP, long &RF, long &SZ) {
  //    long X, I, J;
  //    TYPES ELTP;
  //    long ELRF, ELSZ, TS;
  //    ALFA TID;
  struct TypLocal tl = {0};
  SYMSET su;
  tl.bl = bl;
  tl.TP = NOTYP;
  tl.RF = 0;
  tl.SZ = 0;
  TEST(TYPEBEGSYS, FSYS, 10);

  if (SY == CONSTSY) {
    INSYMBOL();
  }

  if (TYPEBEGSYS[SY]) {
    if (SY == SHORTSY || SY == LONGSY || SY == UNSIGNEDSY) {
      INSYMBOL();
      if (SY == SHORTSY || SY == LONGSY || SY == UNSIGNEDSY) {
        INSYMBOL();
      }
      TEST(TYPEBEGSYS, FSYS, 10);
    }

    if (SY == IDENT) {
      tl.X = LOC(bl, ID);
      if (tl.X != 0) {
        if (TAB[tl.X].OBJ != TYPE1) {
          ERROR(29);
        } else {
          tl.TP = TAB[tl.X].TYP;
          tl.RF = TAB[tl.X].REF;
          tl.SZ = TAB[tl.X].ADR;
          if (tl.TP == NOTYP) {
            ERROR(30);
          }
        }
      }
      INSYMBOL();
      if (SY == CONSTSY) {
        INSYMBOL();
      }
    } else if (SY == STRUCTSY) {
      INSYMBOL();
      if (SY == IDENT) {
        strcpy(tl.TID, ID);
        INSYMBOL();
        if (SY == LSETBRACK) {
          ENTER(bl, tl.TID, STRUCTAG);
          tl.TS = Tx;
          TAB[tl.TS].TYP = RECS;
          tl.TP = RECS;
          TAB[tl.TS].REF = -(int)tl.TS;
          STRUCTTYP(&tl, tl.RF, tl.SZ);
          TAB[tl.TS].REF = (int)tl.RF;
          TAB[tl.TS].ADR = (int)tl.SZ;
          for (tl.I = 1; tl.I <= C; tl.I++) {
            if (CTAB[tl.I].ELREF == -tl.TS) {
              CTAB[tl.I].ELREF = (int)tl.RF;
              CTAB[tl.I].ELSIZE = (int)tl.SZ;
            }
          }
        } else {
          tl.X = LOC(bl, tl.TID);
          if (tl.X != 0) {
            if (TAB[tl.X].OBJ != STRUCTAG) {
              ERROR(108);
            } else {
              tl.TP = RECS;
              tl.RF = TAB[tl.X].REF;
              tl.SZ = TAB[tl.X].ADR;
            }
          }
        }
      } else {
        tl.TP = RECS;
        STRUCTTYP(&tl, tl.RF, tl.SZ);
      }
    }
    su = 0;
    TEST(FSYS, su, 6);
  }
  TP = tl.TP;
  RF = tl.RF;
  SZ = tl.SZ;
}

void PARAMETERLIST(BlockLocal *bl) {
  SYMSET su, sv;
  su = TYPEBEGSYS;
  su[RPARENT] = true;
  su[VALUESY] = true;
  INSYMBOL();
  TEST(su, bl->FSYS, 105);

  bool NOPARAMS = true;
  // int PCNT;
  bl->PCNT = 0;

  while (TYPEBEGSYS[SY] || SY == VALUESY) {
    bool VALUEFOUND;
    if (SY == VALUESY) {
      VALUEFOUND = true;
      INSYMBOL();
    } else {
      VALUEFOUND = false;
    }

    TYPES TP;
    long RF, SZ;
    su = bl->FSYS;
    su[SEMICOLON] = true;
    su[COMMA] = true;
    su[IDENT] = true;
    su[TIMES] = true;
    su[CHANSY] = true;
    su[LBRACK] = true;
    su[RPARENT] = true;
    TYPF(bl, su, TP, RF, SZ);

    if ((TP == VOIDS) && (SY == RPARENT) && NOPARAMS) {
      goto L56;
    }

    if ((TP == VOIDS) && (SY != TIMES)) {
      ERROR(151);
    }

    C_PNTCHANTYP(bl, TP, RF, SZ);

    if (SY == IDENT) {
      ENTERVARIABLE(bl, VARIABLE);
    } else if ((SY == COMMA) || (SY == RPARENT)) {
      strcpy(ID, DUMMYNAME);
      DUMMYNAME[13] = (char)(DUMMYNAME[13] + 1);
      if (DUMMYNAME[13] == '0') {
        DUMMYNAME[12] = (char)(DUMMYNAME[12] + 1);
      }
      ENTER(bl, ID, VARIABLE);
    } else {
      ERROR(2);
      strcpy(ID, "              ");
    }

    NOPARAMS = false;

    if (SY == LBRACK) {
      INSYMBOL();
      long ARF, ASZ;
      C_ARRAYTYP(bl, ARF, ASZ, true, TP, RF, SZ);
      TP = ARRAYS;
      RF = ARF;
      SZ = ASZ;
    }

    bl->PCNT = bl->PCNT + 1;
    //           IF (TP = ARRAYS) AND VALUEFOUND THEN
    //              IF ATAB[RF].HIGH = 0 THEN ERROR(117);
    //          IF ((TP = ARRAYS) AND NOT VALUEFOUND) OR (TP = CHANS)
    //            THEN NORMAL := FALSE
    //            ELSE NORMAL := TRUE;
    //          IF TP = PNTS THEN PNTPARAM := TRUE ELSE PNTPARAM := FALSE;
    if (TP == ARRAYS && VALUEFOUND)
      if (ATAB[RF].HIGH == 0)
        ERROR(117);
    if ((TP == ARRAYS && !VALUEFOUND) || TP == CHANS)
      TAB[Tx].NORMAL = false;
    else
      TAB[Tx].NORMAL = true;
    if (TP == PNTS)
      TAB[Tx].PNTPARAM = true;
    else
      TAB[Tx].PNTPARAM = false;
    TAB[Tx].TYP = TP;
    TAB[Tx].REF = (int)RF;
    TAB[Tx].LEV = bl->LEVEL;
    TAB[Tx].ADR = bl->DX;
    TAB[Tx].SIZE = SZ;

    if (TAB[Tx].NORMAL) {
      bl->DX = bl->DX + (int)SZ;
    } else {
      bl->DX = bl->DX + 1;
    }

    if (SY == COMMA) {
      INSYMBOL();
    } else {
      su = 0;
      su[RPARENT] = true;
      TEST(su, bl->FSYS, 105);
    }
  }

L56:
  if (SY == RPARENT) {
    INSYMBOL();
  } else {
    su = 0;
    su[RPARENT] = true;
    sv = bl->FSYS;
    sv[LSETBRACK] = true;
    TEST(su, sv, 4);
  }

  if (SY != SEMICOLON) {
    if (SY == LSETBRACK) {
      OKBREAK = true;
      INSYMBOL();
      if (PROTOINDEX >= 0) {
        if ((bl->PCNT != BTAB[PROTOREF].PARCNT) ||
            (bl->DX != BTAB[PROTOREF].PSIZE)) {
          ERROR(152);
        }
      }
    } else {
      sv = 0;
      sv[LSETBRACK] = true;
      TEST(sv, bl->FSYS, 106);
    }
    OKBREAK = true;
  } else if (PROTOINDEX >= 0) {
    ERROR(153);
  }

  PROTOINDEX = -1;
}

int GETFUNCID(ALFA FUNCNAME) {
  for (int K = 1; K <= LIBMAX; K++) {
    if (strcmp(FUNCNAME, LIBFUNC[K].NAME) == 0) {
      return LIBFUNC[K].IDNUM;
    }
  }
  return 0;
}

void FUNCDECLARATION(BlockLocal *bl, TYPES TP, long RF, long SZ) {
  bool ISFUN = true;
  SYMSET su, sv;
  if (TP == CHANS) {
    ERROR(119);
    RETURNTYPE.TYP = NOTYP;
  } else {
    RETURNTYPE.TYP = TP;
  }
  RETURNTYPE.REF = RF;
  RETURNTYPE.SIZE = SZ;
  RETURNTYPE.ISADDR = false;

  int T0 = Tx;
  if (PROTOINDEX >= 0) {
    T0 = PROTOINDEX;
    ITEM PRORET = {NOTYP, 0, 0l, false};
    PRORET.TYP = TAB[T0].TYP;
    PRORET.REF = TAB[T0].FREF;
    PRORET.SIZE = TAB[T0].SIZE;
    PRORET.ISADDR = false;
    if ((!TYPE_COMPATIBLE(RETURNTYPE, PRORET)) ||
        (RETURNTYPE.TYP != PRORET.TYP)) {
      ERROR(152);
    }
    PROTOREF = TAB[T0].REF;
  }

  TAB[T0].OBJ = FUNKTION;
  TAB[T0].TYP = TP;
  TAB[T0].FREF = RF;
  TAB[T0].LEV = bl->LEVEL;
  TAB[T0].NORMAL = true;
  TAB[T0].SIZE = SZ;
  TAB[T0].PNTPARAM = false;
  if (strcmp(TAB[T0].NAME, "MAIN          ") == 0)
    MAINFUNC = T0;
  int LCSAV = LC;
  EMIT(10);
  su = bl->FSYS | STATBEGSYS | ASSIGNBEGSYS | DECLBEGSYS;
  su[RSETBRACK] = true;
  BLOCK(bl->blkil, su, ISFUN, bl->LEVEL + 1, T0);
  EMIT(32 + ISFUN);
  if (LC - LCSAV == 2) {
    LC = LCSAV; // no need to generate code for a prototype declaration DE
    EXECNT = 0; // no executable content , avoids 6 (noop) for breaks
  } else
    CODE[LCSAV].Y = LC;
  OKBREAK = false;

  if (inCLUDEFLAG() && (TAB[T0].ADR == 0)) {
    TAB[T0].ADR = -GETFUNCID(TAB[T0].NAME);
  }

  if (SY == SEMICOLON || SY == RSETBRACK) {
    INSYMBOL();
  } else {
    ERROR(104);
  }
  su = DECLBEGSYS;
  su[INCLUDESY] = true;
  su[EOFSY] = true;
  TEST(su, bl->FSYS, 6);
}

void CONSTANTDECLARATION(BlockLocal *bl) {
  SYMSET su;
  INSYMBOL();
  su = 0;
  su[IDENT] = true;
  TEST(su, bl->FSYS, 2);

  if (SY == IDENT) {
    ENTER(bl, ID, KONSTANT);
    INSYMBOL();
    CONREC C;
    su = bl->FSYS;
    su[SEMICOLON] = true;
    su[COMMA] = true;
    su[IDENT] = true;
    CONSTANT(bl, su, C);
    TAB[Tx].TYP = C.TP;
    TAB[Tx].REF = 0;
    TAB[Tx].ADR = C.I;

    if (SY == SEMICOLON) {
      INSYMBOL();
    }
  }
}

void TYPEDECLARATION(BlockLocal *bl) {
  SYMSET su;
  INSYMBOL();
  TYPES TP;
  long RF, SZ, T1, ARF, ASZ;
  // long ORF, OSZ;
  su = bl->FSYS;
  su[SEMICOLON] = true;
  su[COMMA] = true;
  su[IDENT] = true;
  su[TIMES] = true;
  su[CHANSY] = true;
  su[LBRACK] = true;
  TYPF(bl, su, TP, RF, SZ);
  C_PNTCHANTYP(bl, TP, RF, SZ);

  if (SY != IDENT) {
    ERROR(2);
    strcpy(ID, "              ");
  }

  ENTER(bl, ID, TYPE1);
  T1 = Tx;
  INSYMBOL();

  if (SY == LBRACK) {
    INSYMBOL();
    C_ARRAYTYP(bl, ARF, ASZ, true, TP, RF, SZ);
    TP = ARRAYS;
    RF = ARF;
    SZ = ASZ;
  }

  TAB[T1].TYP = TP;
  TAB[T1].REF = RF;
  TAB[T1].ADR = SZ;

  TESTSEMICOLON(bl);
}

void GETLIST(BlockLocal *bl, TYPES TP) {
  SYMSET su;
  if (SY == LSETBRACK) {
    int LBCNT = 1;
    INSYMBOL();

    while ((LBCNT > 0) && (SY != SEMICOLON)) {
      while (SY == LSETBRACK) {
        LBCNT = LBCNT + 1;
        INSYMBOL();
      }

      CONREC LISTVAL = {NOTYP, 0l};
      su = 0;
      su[COMMA] = true;
      su[RSETBRACK] = true;
      su[LSETBRACK] = true;
      su[SEMICOLON] = true;
      CONSTANT(bl, su, LISTVAL);

      switch (TP) {
      case REALS:
        if (LISTVAL.TP == INTS) {
          INITABLE[ITPNT].IVAL = RTAG;
          INITABLE[ITPNT].RVAL = LISTVAL.I;
        } else if (LISTVAL.TP == REALS) {
          INITABLE[ITPNT].IVAL = RTAG;
          INITABLE[ITPNT].RVAL = CONTABLE[LISTVAL.I];
        } else {
          ERROR(138);
        }
        break;
      case INTS:
        if ((LISTVAL.TP == INTS) || (LISTVAL.TP == CHARS)) {
          INITABLE[ITPNT].IVAL = LISTVAL.I;
        } else {
          ERROR(138);
        }
        break;
      case CHARS:
        if (LISTVAL.TP == CHARS) {
          INITABLE[ITPNT].IVAL = LISTVAL.I;
        } else {
          ERROR(138);
        }
        break;
      case BOOLS:
        if (LISTVAL.TP == BOOLS) {
          INITABLE[ITPNT].IVAL = LISTVAL.I;
        } else {
          ERROR(138);
        }
        break;
      default:
        break;
      }

      ITPNT = ITPNT + 1;

      while (SY == RSETBRACK) {
        LBCNT = LBCNT - 1;
        INSYMBOL();
      }

      if (LBCNT > 0) {
        if (SY == COMMA) {
          INSYMBOL();
        } else {
          ERROR(135);
        }
      }
    }
  } else if (SY == STRNG) {
    if (TP != CHARS) {
      ERROR(138);
    } else {
      for (int CI = 1; CI <= SLENG; CI++) {
        INITABLE[ITPNT].IVAL = STAB[INUM + CI - 1];
        ITPNT = ITPNT + 1;
      }
      INITABLE[ITPNT].IVAL = 0;
      ITPNT = ITPNT + 1;
    }
    INSYMBOL();
  } else {
    su = 0;
    su[SEMICOLON] = true;
    su[IDENT] = true;
    SKIP(su, 106);
  }
}

void PROCDECLARATION(BlockLocal *bl) {
  SYMSET su;
  bool ISFUN = false;
  RETURNTYPE.TYP = VOIDS;

  if (SY != IDENT) {
    ERROR(2);
    strcpy(ID, "              ");
  }

  ENTER(bl, ID, PROZEDURE);
  int T0 = Tx;

  if (PROTOINDEX >= 0) {
    T0 = PROTOINDEX;
    if (TAB[T0].OBJ != PROZEDURE) {
      ERROR(152);
    }
    PROTOREF = TAB[T0].REF;
  }

  TAB[T0].NORMAL = true;
  INSYMBOL();

  int LCSAV = LC;
  EMIT(10);
  su = bl->FSYS | STATBEGSYS | ASSIGNBEGSYS | DECLBEGSYS;
  su[RSETBRACK] = true;
  BLOCK(bl->blkil, su, ISFUN, bl->LEVEL + 1, T0);
  EMIT(32 + ISFUN);
  if (LC - LCSAV == 2) {
    LC = LCSAV; // no need to generate code for a prototype declaration DE
    EXECNT = 0; // no executable content , avoids 6 (noop) for breaks
  } else
    CODE[LCSAV].Y = LC;
  OKBREAK = false;

  if (inCLUDEFLAG() && (TAB[T0].ADR == 0)) {
    TAB[T0].ADR = -GETFUNCID(TAB[T0].NAME);
  }

  if (SY == SEMICOLON || SY == RSETBRACK) {
    INSYMBOL();
  } else {
    ERROR(104);
  }
  su = DECLBEGSYS;
  su[INCLUDESY] = true;
  su[EOFSY] = true;
  TEST(su, bl->FSYS, 6);
}

void VARIABLEDECLARATION(BlockLocal *bl) {
  bool TYPCALL = true;
  bool PROCDEFN = false;
  bool DONE;
  bool FUNCDEC;
  SYMSET su;
  int T0;
  long RI, RF, SZ = 0, ARF, ASZ, ORF, OSZ, LSTART, LEND;
  TYPES TP, OTP;
  ITEM X, Y;
  TP = NOTYP;
  if (SY == IDENT) {
    bl->UNDEFMSGFLAG = false;
    T0 = LOC(bl, ID);
    bl->UNDEFMSGFLAG = true;
    if (T0 == 0) {
      TYPCALL = false;
      TP = INTS;
      SZ = 1;
      RF = 0;
    }
  }

  if (TYPCALL) {
    su = bl->FSYS;
    su[SEMICOLON] = true;
    su[COMMA] = true;
    su[IDENT] = true;
    su[TIMES] = true;
    su[CHANSY] = true;
    su[LBRACK] = true;
    TYPF(bl, su, TP, RF, SZ);
  }

  if ((TP == VOIDS) && (SY != TIMES)) {
    PROCDECLARATION(bl);
    PROCDEFN = true;
  }

  OTP = TP;
  ORF = RF;
  OSZ = SZ;

  DONE = false;
  FUNCDEC = false;
  if (!(TP == RECS && SY == SEMICOLON) && !PROCDEFN) {
    do {
      C_PNTCHANTYP(bl, TP, RF, SZ);
      if (SY != IDENT) {
        ERROR(2);
        strcpy(ID, "              ");
      }
      T0 = Tx;
      ENTERVARIABLE(bl, VARIABLE);

      if (!TYPCALL && (SY != LPARENT)) {
        TP = NOTYP;
        SZ = 0;
        TYPCALL = true;
        ERROR(0);
      }

      if (SY == LBRACK) {
        INSYMBOL();
        C_ARRAYTYP(bl, ARF, ASZ, true, TP, RF, SZ);
        TP = ARRAYS;
        RF = ARF;
        SZ = ASZ;
      }

      if (SY == LPARENT) {
        FUNCDEC = true;
        FUNCDECLARATION(bl, TP, RF, SZ);
      } else {
        if (PROTOINDEX >= 0) {
          ERROR(1);
          PROTOINDEX = -1;
        }

        T0 = T0 + 1;
        TAB[T0].TYP = TP;
        TAB[T0].REF = RF;
        TAB[T0].LEV = bl->LEVEL;
        TAB[T0].ADR = bl->DX;
        TAB[T0].NORMAL = true;
        bl->DX += SZ;
        TAB[T0].SIZE = SZ;
        TAB[T0].PNTPARAM = false;

        if (SY == BECOMES) {
          if ((TP == CHANS) || (TP == LOCKS) || (TP == RECS)) {
            ERROR(136);
          } else if (TP == ARRAYS) {
            while (TP == ARRAYS) {
              TP = ATAB[RF].ELTYP;
              RF = ATAB[RF].ELREF;
            }
            if (!STANTYPS[TP]) {
              ERROR(137);
            }
            INSYMBOL();
            LSTART = ITPNT;
            GETLIST(bl, TP);
            LEND = ITPNT;
            if (TAB[T0].SIZE == 0) {
              ATAB[TAB[T0].REF].HIGH = LEND - LSTART - 1;
              TAB[T0].SIZE = LEND - LSTART;
              bl->DX = bl->DX + TAB[T0].SIZE;
            }
            if (TAB[T0].SIZE < LEND - LSTART) {
              ERROR(139);
            } else {
              EMIT2(0, TAB[T0].LEV, TAB[T0].ADR);
              EMIT2(82, LSTART, LEND);
              RI = (TP == REALS) ? 2 : 1;
              EMIT2(83, TAB[T0].SIZE - (LEND - LSTART), RI);
            }
          } else {
            X.TYP = TP;
            X.REF = RF;
            X.SIZE = SZ;
            X.ISADDR = true;
            EMIT2(0, bl->LEVEL, TAB[T0].ADR);
            INSYMBOL();
            su = bl->FSYS;
            su[COMMA] = true;
            su[SEMICOLON] = true;
            EXPRESSION(bl, su, Y);
            if (!TYPE_COMPATIBLE(X, Y)) {
              ERROR(46);
            } else {
              if ((X.TYP == REALS) && (Y.TYP == INTS)) {
                EMIT(91);
              } else {
                EMIT(38);
              }
              EMIT(111); // added in V2.2
            }
          }
        }

        if (SY == COMMA) {
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
