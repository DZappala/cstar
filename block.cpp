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

namespace Cstar {
typedef struct {
  SYMBOL MYSY;
} OptionsLocal;
extern bool inCLUDEFLAG();
extern TYPES RESULTTYPE(TYPES A, TYPES B);
extern TYPES SUMRESULTTYPE(TYPES A, TYPES B);
extern bool COMPATIBLE(ITEM X, ITEM Y);
extern bool TYPE_COMPATIBLE(ITEM X, ITEM Y);
extern bool ARRAY_COMPATIBLE(ITEM X, ITEM Y);
extern bool PNT_COMPATIBLE(ITEM X, ITEM Y);
extern void CONSTANTDECLARATION(BlockLocal *);
extern void TYPEDECLARATION(BlockLocal *);
extern void VARIABLEDECLARATION(BlockLocal *);
extern bool INCLUDEDIRECTIVE();
extern void STATEMENT(BlockLocal *, SYMSET &);
extern void CHANELEMENT(ITEM &);
extern void ENTERCHANNEL();
extern void C_PNTCHANTYP(BlockLocal *, TYPES &, long &, long &);
extern void C_ARRAYTYP(BlockLocal *, long &, long &, bool FIRSTINDEX, TYPES,
                       long, long);
extern void TYPF(BlockLocal *bl, SYMSET FSYS, TYPES &TP, long &RF, long &SZ);
void FACTOR(BasicLocal *, SYMSET &, ITEM &);
void EXPRESSION(BlockLocal *, SYMSET &, ITEM &);
void PARAMETERLIST(BlockLocal *);
void ASSIGNEXPRESSION(BasicLocal *, SYMSET &, ITEM &);
static const int NWORDS = 9;
// static int F;  // decl in BASICEXPRESSION, set in FACTOR

void ENTERBLOCK() {
  if (B == BMAX) {
    FATAL(2);
  } else {
    B++;
    BTAB[B].LAST = 0;
    BTAB[B].LASTPAR = 0;
  }
}
void SKIP(SYMSET FSYS, int N) {
  ERROR(N);
  while (!(SY == EOFSY || FSYS[SY])) {
    INSYMBOL();
  }
}
void TEST(SYMSET &S1, SYMSET &S2, int N) {
  if (!S1[SY]) {
    SYMSET su;
    // std::set_union(S1.begin(), S1.end(), S2.begin(), S2.end(), su);
    su = S1 | S2;
    // SKIP(S1 + S2, N);
    SKIP(su, N);
  }
}
void TESTSEMICOLON(BlockLocal *bl) {
  SYMSET su;
  if (SY == SEMICOLON) {
    INSYMBOL();
  } else {
    ERROR(14);
    if (SY == COMMA || SY == COLON) {
      INSYMBOL();
    }
  }
  su = DECLBEGSYS | STATBEGSYS | ASSIGNBEGSYS;
  su[INCLUDESY] = true;
  TEST(su, bl->FSYS, 6);
}
void ENTER(BlockLocal *bl, ALFA ID, OBJECTS K) {
  // int ix;
  int J, L;
  if (Tx == TMAX) {
    FATAL(1);
  } else {
    // std::cout << ID << std::endl;
    strcpy(TAB[0].NAME, ID);
    if (K == COMPONENT) {
      J = bl->RLAST;
    } else {
      J = BTAB[DISPLAY[bl->LEVEL]].LAST;
    }
    L = J;
    while (strcmp(TAB[J].NAME, ID) != 0) {
      J = TAB[J].LINK;
    }
    if (J != 0) {
      if (((TAB[J].OBJ == FUNKTION) || (TAB[J].OBJ == PROZEDURE)) &&
          (TAB[J].ADR == 0)) {
        PROTOINDEX = J;
      } else {
        ERROR(1);
      }
    } else {
      Tx = Tx + 1;
      strcpy(TAB[Tx].NAME, ID);
      TAB[Tx].LINK = L;
      TAB[Tx].OBJ = K;
      TAB[Tx].TYP = NOTYP;
      TAB[Tx].FORLEV = 0;
      TAB[Tx].REF = 0;
      TAB[Tx].LEV = bl->LEVEL;
      TAB[Tx].ADR = 0;
      if (K == COMPONENT) {
        bl->RLAST = Tx;
      } else {
        BTAB[DISPLAY[bl->LEVEL]].LAST = Tx;
      }
    }
  }
}
int LOC(BlockLocal *bl, ALFA ID) {
  int I, J;
  bool FOUND;
  strcpy(TAB[0].NAME, ID);

  I = bl->NUMWITH;
  FOUND = false;
  while (I > 0 && !FOUND) {
    J = WITHTAB[I];
    while (strcmp(TAB[J].NAME, ID) != 0) {
      J = TAB[J].LINK;
    }
    if (J != 0) {
      FOUND = true;
      EMIT2(1, bl->LEVEL, bl->DX + I - 1);
      EMIT1(24, TAB[J].ADR);
      EMIT(15);
    }
    I = I - 1;
  }
  if (!FOUND) {
    I = bl->LEVEL;
    do {
      J = BTAB[DISPLAY[I]].LAST;
      while (strcmp(TAB[J].NAME, ID) != 0) {
        J = TAB[J].LINK;
      }
      I = I - 1;
    } while (I >= 0 && J == 0);
  }
  //        fprintf(STDOUT, "lookup %s\n", ID);
  //        if (J != 0 && strcmp(ID, "SIZEOF        ") == 0)
  //            fprintf(STDOUT, "sizeof %d\n", J);
  if (J == 0 && bl->UNDEFMSGFLAG) {
    ERROR(0);
  }
  return J;
}
void ENTERVARIABLE(BlockLocal *bl, OBJECTS KIND) {
  if (SY == IDENT) {
    ENTER(bl, ID, KIND);
    INSYMBOL();
  } else {
    ERROR(2);
  }
}
void CONSTANT(BlockLocal *bl, SYMSET &FSYS, CONREC &C) {
  int X;
  int SIGN;
  CONREC NEXTC = {NOTYP, 0l};
  SYMBOL OPSYM;
  SYMSET su;
  C.TP = NOTYP;
  C.I = 0;
  TEST(CONSTBEGSYS, FSYS, 50);
  if (CONSTBEGSYS[SY]) {
    if (SY == CHARCON) {
      C.TP = CHARS;
      C.I = INUM;
      INSYMBOL();
    } else {
      SIGN = 1;
      if (SY == PLUS || SY == MINUS) {
        if (SY == MINUS) {
          SIGN = -1;
        }
        INSYMBOL();
      }
      if (SY == IDENT) {
        X = LOC(bl, ID);
        if (X != 0) {
          if (TAB[X].OBJ != KONSTANT) {
            ERROR(25);
          } else {
            C.TP = TAB[X].TYP;
            if (C.TP == REALS) {
              C.I = CPNT;
              CONTABLE[CPNT] = SIGN * CONTABLE[TAB[X].ADR];
              if (CPNT >= RCMAX) {
                FATAL(10);
              } else {
                CPNT = CPNT + 1;
              }
            } else {
              C.I = SIGN * TAB[X].ADR;
            }
          }
        }
        INSYMBOL();
      } else if (SY == INTCON) {
        C.TP = INTS;
        C.I = SIGN * INUM;
        INSYMBOL();
      } else if (SY == REALCON) {
        C.TP = REALS;
        CONTABLE[CPNT] = SIGN * RNUM;
        C.I = CPNT;
        if (CPNT >= RCMAX) {
          FATAL(10);
        } else {
          CPNT = CPNT + 1;
        }
        INSYMBOL();
      } else {
        SKIP(FSYS, 50);
      }
      if (C.TP == INTS && (SY == PLUS || SY == MINUS || SY == TIMES)) {
        OPSYM = SY;
        INSYMBOL();
        CONSTANT(bl, FSYS, NEXTC);
        if (NEXTC.TP != INTS) {
          ERROR(25);
        } else {
          switch (OPSYM) {
          case PLUS:
            C.I = C.I + NEXTC.I;
            break;
          case MINUS:
            C.I = C.I - NEXTC.I;
            break;
          case TIMES:
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
void IDENTIFY(OptionsLocal *ol, const ALFA *WORD, const SYMBOL *SYM) {
  long I = 1;
  bool FOUND = false;
  do {
    if (strcmp(WORD[I], ID) == 0) {
      FOUND = true;
    } else {
      I = I + 1;
    }
  } while (!FOUND && I <= NWORDS);
  if (I <= NWORDS) {
    ol->MYSY = SYM[I];
  } else {
    ol->MYSY = ERRSY;
  }
}
void OPTIONS(BlockLocal *bl) {
  ALFA WORD[NWORDS + 1];
  SYMBOL SYM[NWORDS + 1];
  CONREC C = {NOTYP, 0l};
  long J, K;
  OptionsLocal ol;
  SYMSET su;
  ol.MYSY = INTCON; // initial value only
  TOPOLOGY = SHAREDSY;
  TOPDIM = PMAX;
  if (SY == IDENT && strcmp(ID, "ARCHITECTURE  ") == 0) {
    strcpy(WORD[1], "SHARED        ");
    SYM[1] = SHAREDSY;
    strcpy(WORD[2], "FULLCONNECT   ");
    SYM[2] = FULLCONNSY;
    strcpy(WORD[3], "HYPERCUBE     ");
    SYM[3] = HYPERCUBESY;
    strcpy(WORD[4], "LINE          ");
    SYM[4] = LINESY;
    strcpy(WORD[5], "MESH2         ");
    SYM[5] = MESH2SY;
    strcpy(WORD[6], "MESH3         ");
    SYM[6] = MESH3SY;
    strcpy(WORD[7], "RING          ");
    SYM[7] = RINGSY;
    strcpy(WORD[8], "TORUS         ");
    SYM[8] = TORUSSY;
    strcpy(WORD[9], "CLUSTER       ");
    SYM[9] = CLUSTERSY;
    INSYMBOL();
    if (SY == IDENT) {
      IDENTIFY(&ol, WORD, SYM);
      if (ol.MYSY != ERRSY) {
        INSYMBOL();
        TOPOLOGY = ol.MYSY;
        if (SY == LPARENT) {
          INSYMBOL();
          su = bl->FSYS;
          su[SEMICOLON] = true;
          su[COMMA] = true;
          su[RPARENT] = true;
          su[IDENT] = true;
          CONSTANT(bl, su, C);
          if (C.TP == INTS && C.I >= 0) {
            TOPDIM = C.I;
          } else {
            ERROR(43);
          }
          if (SY == RPARENT) {
            INSYMBOL();
          } else {
            ERROR(4);
          }
        } else {
          su = bl->FSYS;
          su[RPARENT] = true;
          su[SEMICOLON] = true;
          su[IDENT] = true;
          SKIP(su, 9);
        }
      } else {
        INSYMBOL();
        su = 0;
        su[SEMICOLON] = true;
        SKIP(su | DECLBEGSYS, 42);
      }
      TESTSEMICOLON(bl);
    } else {
      SKIP(DECLBEGSYS, 42);
    }
  }
  switch (TOPOLOGY) {
  case SHAREDSY:
  case FULLCONNSY:
  case CLUSTERSY:
    HIGHESTPROCESSOR = TOPDIM - 1;
    break;
  case HYPERCUBESY:
    K = 1;
    for (J = 1; J <= TOPDIM; J++) {
      K = K * 2;
    }
    HIGHESTPROCESSOR = K - 1;
    break;
  case LINESY:
  case RINGSY:
    HIGHESTPROCESSOR = TOPDIM - 1;
    break;
  case MESH2SY:
  case TORUSSY:
    HIGHESTPROCESSOR = TOPDIM * TOPDIM - 1;
    break;
  case MESH3SY:
    HIGHESTPROCESSOR = TOPDIM * TOPDIM * TOPDIM - 1;
    break;
  default:
    break;
  }
  if (MPIMODE && TOPOLOGY == SHAREDSY) {
    ERROR(144);
  }
}
void LOADVAL(ITEM &X) {
  if (STANTYPS[X.TYP] || X.TYP == PNTS || X.TYP == LOCKS) {
    EMIT(34);
  }
  if (X.TYP == CHANS) {
    ERROR(143);
  }
  X.ISADDR = false;
}
void BASICEXPRESSION(BlockLocal *bl, SYMSET FSYS, ITEM &X) {
  // int F;
  // ITEM Y;
  // SYMBOL OP;
  BasicLocal bx;
  bx.F = 0;
  bx.Y = {NOTYP, 0, 0, false};
  bx.OP = INTCON; // initial value only, no meaning
  bx.bl = bl;
  SYMSET su = FSYS;
  ASSIGNEXPRESSION(&bx, su, X);
}

void EXPRESSION(BlockLocal *bl, SYMSET &FSYS, ITEM &X) {
  ITEM Y = {NOTYP, 0, 0, false};
  SYMBOL OP;
  bool ADDR_REQUIRED;
  SYMSET NXT;
  long RF, SZ;
  TYPES TP;
  if (SY == ADDRSY) {
    ADDR_REQUIRED = true;
    INSYMBOL();
  } else
    ADDR_REQUIRED = false;
  if (SY == NEWSY) {
    INSYMBOL();
    NXT = FSYS;
    NXT[TIMES] = true;
    NXT[CHANSY] = true;
    NXT[LBRACK] = true;
    TYPF(bl, NXT, TP, RF, SZ);
    C_PNTCHANTYP(bl, TP, RF, SZ);
    ENTERCHANNEL();
    CTAB[C].ELTYP = X.TYP;
    CTAB[C].ELREF = X.REF;
    CTAB[C].ELSIZE = (int)X.SIZE;
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
        CTAB[C].ELSIZE = (int)X.SIZE;
        X.TYP = PNTS;
        X.SIZE = 1;
        X.REF = C;
      }
    } else {
      if (X.ISADDR)
        LOADVAL(X);
    }
  }
  X.ISADDR = false;
}

void TERM(BasicLocal *bx, SYMSET &FSYS, ITEM &X) {
  ITEM Y = {NOTYP, 0, 0, false};
  SYMBOL OP;
  SYMSET NXT;
  long A, B;
  SYMSET su, sv;
  su = 0;
  su[TIMES] = true;
  su[IMOD] = true;
  su[RDIV] = true;
  sv = su | FSYS;
  FACTOR(bx, sv, X);
  if (su[SY] && X.ISADDR)
    LOADVAL(X);
  while (su[SY]) {
    OP = SY;
    INSYMBOL();
    FACTOR(bx, sv, Y);
    if (Y.ISADDR)
      LOADVAL(Y);
    if (OP == TIMES) {
      X.TYP = RESULTTYPE(X.TYP, Y.TYP);
      if (X.TYP == INTS || X.TYP == REALS)
        EMIT(57);
    } else if (OP == RDIV) {
      if (X.TYP == INTS && Y.TYP == INTS)
        EMIT(58);
      else {
        X.TYP = RESULTTYPE(X.TYP, Y.TYP);
        if (X.TYP != NOTYP)
          X.TYP = REALS;
        EMIT(88);
      }
    } else {
      if (X.TYP == INTS && Y.TYP == INTS)
        EMIT(59);
      else {
        if (X.TYP != NOTYP && Y.TYP != NOTYP)
          ERROR(34);
        X.TYP = NOTYP;
      }
    }
  }
}

void SIMPLEEXPRESSION(BasicLocal *bx, SYMSET &FSYS, ITEM &X) {
  ITEM Y = {NOTYP, 0, 0, false};
  SYMBOL OP;
  long I, J;
  SYMSET su, sv;
  su = 0;
  su[PLUS] = true;
  su[MINUS] = true;
  sv = FSYS | su;
  TERM(bx, sv, X);
  if ((SY == PLUS || SY == MINUS) && X.ISADDR)
    LOADVAL(X);
  while (SY == PLUS || SY == MINUS) {
    OP = SY;
    INSYMBOL();
    TERM(bx, sv, Y);
    if (Y.ISADDR)
      LOADVAL(Y);
    if (X.TYP == PNTS && Y.TYP == INTS) {
      if (OP == MINUS)
        EMIT(36);
      EMIT2(110, CTAB[X.REF].ELSIZE, 0);
    }
    if (X.TYP == INTS && Y.TYP == PNTS) {
      if (OP == MINUS) {
        ERROR(113);
        X.TYP = NOTYP;
      } else
        EMIT2(110, CTAB[Y.REF].ELSIZE, 1);
    }
    if (X.TYP == PNTS && Y.TYP == PNTS && OP == MINUS) {
      X.TYP = INTS;
      EMIT(53);
    } else {
      X.TYP = SUMRESULTTYPE(X.TYP, Y.TYP);
      if (X.TYP == INTS || X.TYP == REALS || X.TYP == CHARS) {
        if (OP == PLUS)
          EMIT(52);
        else
          EMIT(53);
      }
    }
  }
}

void BOOLEXPRESSION(BasicLocal *bx, SYMSET &FSYS, ITEM &X) {
  ITEM Y = {NOTYP, 0, 0, false};
  SYMBOL OP;
  SYMSET su, sv, sz;
  su = 0;
  su[EQL] = true;
  su[NEQ] = true;
  su[LSS] = true;
  su[LEQ] = true;
  su[GTR] = true;
  su[GEQ] = true;
  sv = 0;
  sv[NOTYP] = true;
  sv[BOOLS] = true;
  sv[INTS] = true;
  sv[REALS] = true;
  sv[CHARS] = true;
  sv[PNTS] = true;
  sz = su | FSYS;
  SIMPLEEXPRESSION(bx, sz, X);
  if (su[SY]) {
    if (X.ISADDR)
      LOADVAL(X);
    OP = SY;
    INSYMBOL();
    SIMPLEEXPRESSION(bx, FSYS, Y);
    if (Y.ISADDR)
      LOADVAL(Y);
    if (sv[X.TYP]) {
      if (X.TYP == Y.TYP || (X.TYP == INTS && Y.TYP == REALS) ||
          (X.TYP == REALS && Y.TYP == INTS)) {
        switch (OP) {
        case EQL:
          EMIT(45);
          break;
        case NEQ:
          EMIT(46);
          break;
        case LSS:
          EMIT(47);
          break;
        case LEQ:
          EMIT(48);
          break;
        case GTR:
          EMIT(49);
          break;
        case GEQ:
          EMIT(50);
          break;
        default:
          break;
        }
      } else
        ERROR(35);
    }
    X.TYP = BOOLS;
  }
}

void ANDEXPRESSION(BasicLocal *bx, SYMSET &FSYS, ITEM &X) {
  ITEM Y = {NOTYP, 0, 0, false};
  SYMBOL OP;
  SYMSET su;
  su = FSYS;
  su[ANDSY] = true;
  BOOLEXPRESSION(bx, su, X);
  if (SY == ANDSY && X.ISADDR)
    LOADVAL(X);
  while (SY == ANDSY) {
    OP = SY;
    INSYMBOL();
    BOOLEXPRESSION(bx, su, Y);
    if (Y.ISADDR)
      LOADVAL(Y);
    if (X.TYP == BOOLS && Y.TYP == BOOLS)
      EMIT(56);
    else {
      if (X.TYP != NOTYP && Y.TYP != NOTYP)
        ERROR(32);
      X.TYP = NOTYP;
    }
  }
}

void OREXPRESSION(BasicLocal *bx, SYMSET FSYS, ITEM &X) {
  ITEM Y = {NOTYP, 0, 0, false};
  SYMBOL OP;
  SYMSET su;
  su = FSYS;
  su[ORSY] = true;
  ANDEXPRESSION(bx, su, X);
  while (SY == ORSY) {
    if (X.ISADDR)
      LOADVAL(X);
    OP = SY;
    INSYMBOL();
    ANDEXPRESSION(bx, su, Y);
    if (Y.ISADDR)
      LOADVAL(Y);
    if (X.TYP == BOOLS && Y.TYP == BOOLS)
      EMIT(51);
    else {
      if (X.TYP != NOTYP && Y.TYP != NOTYP)
        ERROR(32);
      X.TYP = NOTYP;
    }
  }
}

void COMPASSIGNEXP(BasicLocal *bx, SYMSET &FSYS, ITEM &X) {
  ITEM Y = {NOTYP, 0, 0, false};
  ITEM Z = {NOTYP, 0, 0, false};
  SYMBOL OP;
  int SF;
  OP = SY;
  SF = bx->F;
  if (!X.ISADDR) {
    ERROR(114);
    X.TYP = NOTYP;
  }
  if (X.TYP == CHANS) {
    ERROR(114);
    X.TYP = NOTYP;
  }
  EMIT(14);
  Z = X;
  Z.ISADDR = false;
  INSYMBOL();
  EXPRESSION(bx->bl, FSYS, Y);
  switch (OP) {
  case PLUSEQ:
  case MINUSEQ:
    if (Y.ISADDR)
      LOADVAL(Y);
    if (Z.TYP == PNTS && Y.TYP == INTS) {
      if (OP == MINUSEQ)
        EMIT(36);
      EMIT2(110, CTAB[Z.REF].ELSIZE, 0);
    }
    if (Z.TYP == INTS && Y.TYP == PNTS) {
      if (OP == MINUSEQ) {
        ERROR(113);
        Z.TYP = NOTYP;
      } else
        EMIT2(110, CTAB[Y.REF].ELSIZE, 1);
    }
    if (Z.TYP == PNTS && Y.TYP == PNTS && OP == MINUSEQ) {
      Z.TYP = INTS;
      EMIT(53);
    } else {
      Z.TYP = SUMRESULTTYPE(Z.TYP, Y.TYP);
      if (Z.TYP == INTS || Z.TYP == REALS || Z.TYP == CHARS) {
        if (OP == PLUSEQ)
          EMIT(52);
        else
          EMIT(53);
      }
    }
    break;
  case TIMESEQ:
  case RDIVEQ:
  case IMODEQ:
    if (Y.ISADDR)
      LOADVAL(Y);
    if (OP == TIMESEQ) {
      Z.TYP = RESULTTYPE(Z.TYP, Y.TYP);
      if (Z.TYP == INTS || Z.TYP == REALS)
        EMIT(57);
    } else if (OP == RDIVEQ) {
      if (Z.TYP == INTS && Y.TYP == INTS)
        EMIT(58);
      else {
        Z.TYP = RESULTTYPE(Z.TYP, Y.TYP);
        if (Z.TYP != NOTYP)
          Z.TYP = REALS;
        EMIT(88);
      }
    } else {
      if (Z.TYP == INTS && Y.TYP == INTS)
        EMIT(59);
      else {
        if (Z.TYP != NOTYP && Y.TYP != NOTYP)
          ERROR(34);
        Z.TYP = NOTYP;
      }
    }
    break;
  default:
    break;
  }
  if (!TYPE_COMPATIBLE(X, Z))
    ERROR(46);
  else {
    if (X.TYP == REALS && Z.TYP == INTS)
      EMIT1(91, SF);
    EMIT1(38, SF);
  }
  X.ISADDR = false;
}

void ASSIGNEXPRESSION(BasicLocal *bx, SYMSET &FSYS, ITEM &X) {
  ITEM Y;
  SYMBOL OP;
  int SF, SZ;
  SYMSET su, sv;
  su = FSYS | COMPASGNSYS;
  su[BECOMES] = true;
  OREXPRESSION(bx, su, X);
  if (COMPASGNSYS[SY])
    COMPASSIGNEXP(bx, FSYS, X);
  else if (SY == BECOMES) {
    SF = bx->F;
    if (!X.ISADDR) {
      ERROR(114);
      X.TYP = NOTYP;
    }
    if (X.TYP == CHANS) {
      ERROR(114);
      X.TYP = NOTYP;
    }
    INSYMBOL();
    su = FSYS;
    su[BECOMES] = true;
    EXPRESSION(bx->bl, su, Y);
    if (!TYPE_COMPATIBLE(X, Y))
      ERROR(46);
    else {
      if (X.TYP == REALS && Y.TYP == INTS)
        EMIT1(91, SF);
      else if (STANTYPS[X.TYP] || X.TYP == PNTS || X.TYP == LOCKS)
        EMIT1(38, SF);
      else {
        if (X.TYP == ARRAYS)
          SZ = ATAB[X.REF].SIZE;
        else
          SZ = X.SIZE;
        EMIT2(23, SZ, SF);
      }
    }
    X.ISADDR = false;
  }
}

void STANDPROC(BlockLocal *bl, int N) {
  ITEM X, Y, Z;
  int SZ;
  SYMSET su;
  switch (N) {
  case 3: // send
    if (SY == LPARENT) {
      INSYMBOL();
      su = bl->FSYS;
      su[COMMA] = true;
      su[RPARENT] = true;
      BASICEXPRESSION(bl, su, X);
      if ((!X.ISADDR) || (!(X.TYP == CHANS))) {
        ERROR(140);
        X.TYP = NOTYP;
      } else {
        CHANELEMENT(X);
        if (SY == COMMA) {
          INSYMBOL();
        } else {
          ERROR(135);
        }
        su = bl->FSYS;
        su[RPARENT] = true;
        EXPRESSION(bl, su, Y);
        if (!TYPE_COMPATIBLE(X, Y)) {
          ERROR(141);
        } else {
          if ((X.TYP == REALS) && (Y.TYP == INTS)) {
            EMIT1(92, 1);
          } else if (STANTYPS[X.TYP] || X.TYP == PNTS) {
            EMIT1(66, 1);
          } else {
            if (X.TYP == ARRAYS) {
              SZ = ATAB[X.REF].SIZE;
            } else {
              SZ = X.SIZE;
            }
            EMIT1(98, SZ);
            EMIT1(66, SZ);
          }
        }
      }
      if (SY == RPARENT) {
        INSYMBOL();
      } else {
        ERROR(4);
      }
    } else {
      ERROR(9);
    }
    break;
  case 4: // recv
    if (SY == LPARENT) {
      INSYMBOL();
      su = bl->FSYS;
      su[COMMA] = true;
      su[RPARENT] = true;
      BASICEXPRESSION(bl, su, X);
      if ((!X.ISADDR) || X.TYP != CHANS) {
        ERROR(140);
        X.TYP = NOTYP;
      } else {
        EMIT(71);
        CHANELEMENT(X);
        if (SY == COMMA) {
          INSYMBOL();
        } else {
          ERROR(135);
        }
        su = bl->FSYS;
        su[RPARENT] = true;
        BASICEXPRESSION(bl, su, Y);
        if (!Y.ISADDR) {
          ERROR(142);
          Y.TYP = NOTYP;
        }
        if (!TYPE_COMPATIBLE(X, Y)) {
          ERROR(141);
        } else {
          EMIT(20);
          Z = X;
          X = Y;
          Y = Z;
          if ((X.TYP == REALS) && (Y.TYP == INTS)) {
            EMIT1(91, 0);
          } else if (STANTYPS[X.TYP] || X.TYP == PNTS) {
            EMIT1(38, 0);
            EMIT(111);
          } else {
            if (X.TYP == ARRAYS) {
              SZ = ATAB[X.REF].SIZE;
            } else {
              SZ = X.SIZE;
            }
            EMIT2(23, SZ, 0);
            EMIT1(112, X.SIZE);
            EMIT(111);
          }
        }
      }
      if (SY == RPARENT) {
        INSYMBOL();
      } else {
        ERROR(4);
      }
    } else {
      ERROR(9);
    }
    break;
  case 6: // delete
    su = bl->FSYS;
    su[SEMICOLON] = true;
    BASICEXPRESSION(bl, su, X);
    if (!X.ISADDR) {
      ERROR(120);
    } else if (X.TYP == PNTS) {
      EMIT(100);
    } else {
      ERROR(16);
    }
    break;
  case 7: // lock
    if (SY == LPARENT) {
      INSYMBOL();
      su = bl->FSYS;
      su[RPARENT] = true;
      BASICEXPRESSION(bl, su, X);
      if (!X.ISADDR) {
        ERROR(15);
      } else {
        if (X.TYP == LOCKS) {
          EMIT(101);
        } else {
          ERROR(15);
        }
        if (SY == RPARENT) {
          INSYMBOL();
        } else {
          ERROR(4);
        }
      }
    } else {
      ERROR(9);
    }
    break;
  case 8: // unlock
    if (SY == LPARENT) {
      INSYMBOL();
      su = bl->FSYS;
      su[RPARENT] = true;
      BASICEXPRESSION(bl, su, X);
      if (!X.ISADDR) {
        ERROR(15);
      } else {
        if (X.TYP == LOCKS) {
          EMIT(102);
        } else {
          ERROR(15);
        }
        if (SY == RPARENT) {
          INSYMBOL();
        } else {
          ERROR(4);
        }
      }
    } else {
      ERROR(9);
    }
    break;
  case 9: // duration
    if (SY == LPARENT) {
      INSYMBOL();
      su = bl->FSYS;
      su[RPARENT] = true;
      EXPRESSION(bl, su, X);
      if (X.TYP != INTS) {
        ERROR(34);
      }
      EMIT(93);
      if (SY == RPARENT) {
        INSYMBOL();
      } else {
        ERROR(4);
      }
    } else {
      ERROR(9);
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
void CALL(BlockLocal *bl, SYMSET &FSYS, int I) {
  ITEM X, Y;
  int LASTP, CP;
  // int K;
  bool DONE;
  SYMSET su, sv;
  if (strcmp(TAB[I].NAME, "MPI_INIT      ") == 0 && !MPIMODE)
    ERROR(146);
  su = FSYS;
  su[COMMA] = true;
  su[COLON] = true;
  su[RPARENT] = true;
  EMIT1(18, I);
  LASTP = BTAB[TAB[I].REF].LASTPAR;
  CP = LASTP - BTAB[TAB[I].REF].PARCNT;
  if (SY == LPARENT)
    INSYMBOL();
  else
    ERROR(9);
  if (SY != RPARENT) {
    DONE = false;
    do {
      if (CP >= LASTP)
        ERROR(39);
      else {
        CP++;
        if (TAB[CP].NORMAL) {
          EXPRESSION(bl, su, X);
          Y.TYP = TAB[CP].TYP;
          Y.REF = TAB[CP].REF;
          Y.ISADDR = true;
          if (TYPE_COMPATIBLE(Y, X)) {
            if (X.TYP == ARRAYS && Y.TYP != PNTS)
              EMIT1(22, ATAB[X.REF].SIZE);
            if (X.TYP == RECS)
              EMIT1(22, (int)X.SIZE);
            if (Y.TYP == REALS && X.TYP == INTS)
              EMIT(26);
          } else
            ERROR(36);
        } else if (SY != IDENT)
          ERROR(2);
        else {
          sv = FSYS | su;
          BASICEXPRESSION(bl, sv, X);
          Y.TYP = TAB[CP].TYP;
          Y.REF = TAB[CP].REF;
          Y.ISADDR = true;
          if (!X.ISADDR)
            ERROR(116);
          else {
            switch (Y.TYP) {
            case CHANS:
              if (X.TYP != CHANS)
                ERROR(116);
              else if (!COMPATIBLE(Y, X))
                ERROR(36);
              break;
            case ARRAYS:
              EMIT(86);
              if (ATAB[Y.REF].HIGH > 0) {
                if (!ARRAY_COMPATIBLE(Y, X))
                  ERROR(36);
              } else if (!PNT_COMPATIBLE(Y, X))
                ERROR(36);
              break;
            default:
              break;
            }
          }
        }
      }
      su = 0;
      su[COMMA] = true;
      su[RPARENT] = true;
      TEST(su, FSYS, 6);
      if (SY == COMMA)
        INSYMBOL();
      else
        DONE = true;
    } while (!DONE);
  }
  if (SY == RPARENT)
    INSYMBOL();
  else
    ERROR(4);
  if (CP < LASTP)
    ERROR(39);
  if (bl->CREATEFLAG) {
    EMIT1(78, BTAB[TAB[I].REF].PSIZE - BASESIZE);
    bl->CREATEFLAG = false;
  }
  EMIT2(19, I, BTAB[TAB[I].REF].PSIZE - 1);
  if (TAB[I].LEV < bl->LEVEL && TAB[I].ADR >= 0)
    EMIT2(3, TAB[I].LEV, bl->LEVEL);
}
void BLOCK(InterpLocal *il, SYMSET FSYS, bool ISFUN, int LEVEL, int PRT) {
  // fprintf(stdout, "BLOCK not yet implemented");
  //    int DX;
  //    int FLEVEL;   shadows global
  //    int NUMWITH;
  //    int MAXNUMWITH;
  //    bool UNDEFMSGFLAG;
  BlockLocal bl;
  bl.blkil = il;
  bl.FSYS = FSYS;
  bl.ISFUN = ISFUN;
  bl.LEVEL = LEVEL;
  bl.PRT = PRT;
  /* parameters above */
  SYMSET su, sv;
  bl.DX = BASESIZE;
  bl.FLEVEL = 0;
  if (il->LEVEL > LMAX)
    FATAL(5);
  if (bl.LEVEL > 2)
    ERROR(122);
  bl.NUMWITH = 0;
  bl.MAXNUMWITH = 0;
  bl.UNDEFMSGFLAG = true;
  bl.RDX = 0;
  bl.RLAST = 0;
  bl.PCNT = 0;
  bl.X = 0;
  bl.V = 0;
  // bl.F = 0;   BASICEXPRESSION
  bl.CREATEFLAG = false;
  bl.ISDECLARATION = false;
  ENTERBLOCK();
  DISPLAY[bl.LEVEL] = B;
  bl.PRB = B;
  if (bl.LEVEL == 1) {
    TAB[PRT].TYP = NOTYP;
  } else {
    su = 0;
    su[LPARENT] = true;
    TEST(su, FSYS, 9);
  }
  TAB[PRT].REF = bl.PRB;
  if (SY == LPARENT && bl.LEVEL > 1) {
    PARAMETERLIST(&bl);
  }
  BTAB[bl.PRB].LASTPAR = Tx;
  BTAB[bl.PRB].PSIZE = bl.DX;
  BTAB[bl.PRB].PARCNT = bl.PCNT;
  if (bl.LEVEL == 1) {
    su = DECLBEGSYS;
    su[INCLUDESY] = true;
    TEST(su, FSYS, 102);
    OPTIONS(&bl);
    do {
      if (SY == DEFINESY)
        CONSTANTDECLARATION(&bl);
      else if (SY == TYPESY)
        TYPEDECLARATION(&bl);
      else if (TYPEBEGSYS[SY])
        VARIABLEDECLARATION(&bl);
      else if (SY == INCLUDESY)
        INCLUDEDIRECTIVE();
      su = DECLBEGSYS;
      su[INCLUDESY] = true;
      su[EOFSY] = true;
      TEST(su, FSYS, 6);
    } while (SY != EOFSY);
    TAB[PRT].ADR = LC;
  } else {
    if (SY != SEMICOLON) {
      TAB[PRT].ADR = LC;
      su = DECLBEGSYS | STATBEGSYS | ASSIGNBEGSYS;
      sv = FSYS;
      sv[RSETBRACK] = true;
      TEST(su, sv, 101);
      bl.CREATEFLAG = false;
      while (DECLBEGSYS[SY] || STATBEGSYS[SY] || ASSIGNBEGSYS[SY]) {
        if (SY == DEFINESY)
          CONSTANTDECLARATION(&bl);
        else if (SY == TYPESY)
          TYPEDECLARATION(&bl);
        else if (SY == STRUCTSY || SY == CONSTSY)
          VARIABLEDECLARATION(&bl);
        if (SY == IDENT) {
          bl.X = LOC(&bl, ID);
          if (bl.X != 0) {
            if (TAB[bl.X].OBJ == TYPE1) {
              VARIABLEDECLARATION(&bl);
            } else {
              su = FSYS;
              su[SEMICOLON] = true;
              su[RSETBRACK] = true;
              STATEMENT(&bl, su);
            }
          } else {
            INSYMBOL();
          }
        } else {
          if (STATBEGSYS[SY] || ASSIGNBEGSYS[SY]) {
            su = FSYS;
            su[SEMICOLON] = true;
            su[RSETBRACK] = true;
            STATEMENT(&bl, su);
          }
        }
        su = DECLBEGSYS | STATBEGSYS | ASSIGNBEGSYS;
        su[RSETBRACK] = true;
        TEST(su, FSYS, 6);
      }
    }
  }
  BTAB[bl.PRB].VSIZE = bl.DX;
  BTAB[bl.PRB].VSIZE = BTAB[bl.PRB].VSIZE + bl.MAXNUMWITH;
  if ((SYMCNT == 1) && (!inCLUDEFLAG()))
    LOCATION[LNUM] = LC;
}

} // namespace Cstar
