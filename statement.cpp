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
extern long LOC(BlockLocal *, ALFA ID);
extern void SKIP(SYMSET, int);
extern void TEST(SYMSET &, SYMSET &, int N);
extern void CALL(BlockLocal *, SYMSET &, int);
extern void STANDPROC(BlockLocal *, int);
extern void ENTER(BlockLocal *, ALFA, OBJECTS);
extern void ENTERBLOCK();
extern bool TYPE_COMPATIBLE(ITEM X, ITEM Y);
extern void BASICEXPRESSION(BlockLocal *, SYMSET, ITEM &);
extern void CONSTANT(BlockLocal *, SYMSET &, CONREC &);
void EXPRESSION(BlockLocal *, SYMSET FSYS, ITEM &X);
void COMPOUNDSTATEMENT(BlockLocal *);
void FIXBREAKS(long LC1);
void FIXCONTS(long LC1);
void SWITCHSTATEMENT(BlockLocal *);
void IFSTATEMENT(BlockLocal *);
void DOSTATEMENT(BlockLocal *);
void WHILESTATEMENT(BlockLocal *);
void FORSTATEMENT(BlockLocal *);
void BLOCKSTATEMENT(BlockLocal *);
void FORALLSTATEMENT(BlockLocal *);
void FORKSTATEMENT(BlockLocal *);
void JOINSTATEMENT(BlockLocal *);
void RETURNSTATEMENT(BlockLocal *);
void BREAKSTATEMENT(BlockLocal *);
void CONTINUESTATEMENT();
void MOVESTATEMENT(BlockLocal *);
void INPUTSTATEMENT(BlockLocal *);
void COUTMETHODS(BlockLocal *);
void OUTPUTSTATEMENT(BlockLocal *);
static int CONTPNT; // range 0..LOOPMAX
static long CONTLOC[LOOPMAX + 1];

void STATEMENT(BlockLocal *bl, SYMSET &FSYS) {
  long int I;
  ITEM X;
  long int JUMPLC;
  SYMBOL SYMSAV;
  SYMSET su, sv;
  SYMSAV = SY;
  // INSYMBOL();
  if (STATBEGSYS[SY] || ASSIGNBEGSYS[SY]) {
    if (SY == LSETBRACK) // BEGINSY ??
    {
      COMPOUNDSTATEMENT(bl);
    } else if (SY == SWITCHSY) {
      SWITCHSTATEMENT(bl);
    } else if (SY == IFSY) {
      IFSTATEMENT(bl);
    } else if (SY == DOSY) {
      DOSTATEMENT(bl);
    } else if (SY == WHILESY) {
      WHILESTATEMENT(bl);
    } else if (SY == FORSY) {
      FORSTATEMENT(bl);
    } else if (SY == FORALLSY) {
      FORALLSTATEMENT(bl);
    } else if (SY == FORKSY) {
      FORKSTATEMENT(bl);
    } else if (SY == JOINSY) {
      JOINSTATEMENT(bl);
    } else if (SY == RETURNSY) {
      RETURNSTATEMENT(bl);
    } else if (SY == BREAKSY) {
      BREAKSTATEMENT(bl);
    } else if (SY == CONTSY) {
      CONTINUESTATEMENT();
    } else if (SY == MOVESY) {
      MOVESTATEMENT(bl);
    } else if (SY == CINSY) {
      INPUTSTATEMENT(bl);
    } else if (SY == COUTSY) {
      OUTPUTSTATEMENT(bl);
    } else if (SY == IDENT) {
      I = LOC(bl, ID);
      if (I != 0) {
        if (TAB[I].OBJ == PROZEDURE) {
          INSYMBOL();
          if (TAB[I].LEV != 0) {
            CALL(bl, FSYS, I);
          } else {
            STANDPROC(bl, TAB[I].ADR);
          }
        } else {
          EXPRESSION(bl, FSYS, X);
          if (X.SIZE > 1)
            EMIT1(112, X.SIZE);
          EMIT(111);
        }
      } else
        INSYMBOL();
    } else if (SY == LPARENT || SY == INCREMENT || SY == DECREMENT ||
               SY == TIMES) {
      EXPRESSION(bl, FSYS, X);
      EMIT(111);
    }
  }
  if (ASSIGNBEGSYS[SYMSAV] ||
      (SYMSAV == DOSY || SYMSAV == JOINSY || SYMSAV == RETURNSY ||
       SYMSAV == BREAKSY || SYMSAV == CONTSY || SYMSAV == SEMICOLON ||
       SYMSAV == MOVESY || SYMSAV == CINSY || SYMSAV == COUTSY ||
       SYMSAV == RSETBRACK)) // RSETBRACK added for while missing statement DDE
  {
    if (SY == SEMICOLON)
      INSYMBOL();
    else
      ERROR(14);
  }
  su = 0;
  sv = FSYS;
  sv[ELSESY] = true; // added to fix else DDE
  // TEST(FSYS, su, 14);
  TEST(sv, su, 14);
}

void COMPOUNDSTATEMENT(BlockLocal *bl) {
  long int X;
  SYMSET su, sv;
  INSYMBOL();
  su = DECLBEGSYS | STATBEGSYS | ASSIGNBEGSYS;
  // su[RSETBRACK] = true;  ??
  sv = bl->FSYS;
  sv[RSETBRACK] = true;
  TEST(su, sv, 123);
  while (DECLBEGSYS[SY] || STATBEGSYS[SY] || ASSIGNBEGSYS[SY]) {
    if (SY == DEFINESY) {
      CONSTANTDECLARATION(bl);
    }
    if (SY == TYPESY) {
      TYPEDECLARATION(bl);
    }
    if (SY == STRUCTSY) {
      VARIABLEDECLARATION(bl);
    }
    if (SY == IDENT) {
      X = LOC(bl, ID);
      if (X != 0) {
        if (TAB[X].OBJ == TYPE1) {
          VARIABLEDECLARATION(bl);
        } else {
          su = bl->FSYS;
          su[RSETBRACK] = true;
          su[SEMICOLON] = true;
          STATEMENT(bl, su);
        }
      } else {
        INSYMBOL();
      }
    } else if (STATBEGSYS[SY] || ASSIGNBEGSYS[SY]) {
      su = bl->FSYS;
      su[RSETBRACK] = true;
      su[SEMICOLON] = true;
      STATEMENT(bl, su);
    }
    su = DECLBEGSYS | STATBEGSYS | ASSIGNBEGSYS;
    su[RSETBRACK] = true;
    TEST(su, bl->FSYS, 6);
  }
  if (SYMCNT == 1) {
    LOCATION[LNUM] = LC;
  }
  if (SY == RSETBRACK) {
    INSYMBOL();
  } else {
    ERROR(104);
  }
}

void FIXBREAKS(long LC1) {
  long LC2;
  while (BREAKLOC[BREAKPNT] > 0) {
    LC2 = CODE[BREAKLOC[BREAKPNT]].Y;
    CODE[BREAKLOC[BREAKPNT]].Y = LC1;
    BREAKLOC[BREAKPNT] = LC2;
  }
  BREAKPNT = BREAKPNT - 1;
}

void FIXCONTS(long int LC1) {
  long int LC2;
  while (CONTLOC[CONTPNT] > 0) {
    LC2 = CODE[CONTLOC[CONTPNT]].Y;
    CODE[CONTLOC[CONTPNT]].Y = LC1;
    CONTLOC[CONTPNT] = LC2;
  }
  CONTPNT = CONTPNT - 1;
}

void IFSTATEMENT(BlockLocal *bl) {
  ITEM X = {NOTYP, 0};
  int LC1, LC2;
  SYMSET su;
  INSYMBOL();
  if (SY == LPARENT) {
    INSYMBOL();
    su = bl->FSYS;
    su[RPARENT] = true;
    EXPRESSION(bl, su, X);
    if (!(X.TYP == BOOLS || X.TYP == INTS || X.TYP == NOTYP)) {
      ERROR(17);
    }
    LC1 = LC;
    EMIT(11);
    if (LOCATION[LNUM] == LC - 1) {
      LOCATION[LNUM] = LC;
    }
    if (SY == RPARENT) {
      INSYMBOL();
    } else {
      ERROR(4);
    }
    su = bl->FSYS;
    su[ELSESY] = true;
    STATEMENT(bl, su);
    if (SY == ELSESY) {
      LC2 = LC;
      EMIT(10);
      if (SYMCNT == 1) {
        LOCATION[LNUM] = LC;
      }
      CODE[LC1].Y = LC;
      INSYMBOL();
      STATEMENT(bl, bl->FSYS);
      CODE[LC2].Y = LC;
    } else {
      CODE[LC1].Y = LC;
    }
  } else {
    ERROR(9);
  }
}

void DOSTATEMENT(BlockLocal *bl) {
  ITEM X;
  int LC1, LC2, LC3;
  LC1 = LC;
  SYMSET su;
  INSYMBOL();
  BREAKPNT = BREAKPNT + 1;
  BREAKLOC[BREAKPNT] = 0;
  CONTPNT = CONTPNT + 1;
  CONTLOC[CONTPNT] = 0;
  su = bl->FSYS;
  su[SEMICOLON] = true;
  su[WHILESY] = true;
  STATEMENT(bl, su);
  LC2 = LC;
  if (SY == WHILESY) {
    if (SYMCNT == 1) {
      LOCATION[LNUM] = LC;
    }
    INSYMBOL();
    if (SY == LPARENT) {
      INSYMBOL();
    } else {
      ERROR(9);
    }
    su = bl->FSYS;
    su[RPARENT] = true;
    EXPRESSION(bl, su, X);
    if (!(X.TYP == BOOLS || X.TYP == INTS || X.TYP == NOTYP)) {
      ERROR(17);
    }
    if (SY == RPARENT) {
      INSYMBOL();
    } else {
      ERROR(4);
    }
    EMIT1(11, LC + 2);
    EMIT1(10, LC1);
  } else {
    ERROR(53);
  }
  FIXBREAKS(LC);
  FIXCONTS(LC2);
}

void WHILESTATEMENT(BlockLocal *bl) {
  ITEM X;
  int LC1, LC2, LC3;
  SYMSET su;
  INSYMBOL();
  if (SY == LPARENT) {
    INSYMBOL();
  } else {
    ERROR(9);
  }
  LC1 = LC;
  BREAKPNT = BREAKPNT + 1;
  BREAKLOC[BREAKPNT] = 0;
  CONTPNT = CONTPNT + 1;
  CONTLOC[CONTPNT] = 0;
  su = bl->FSYS;
  su[RPARENT] = true;
  EXPRESSION(bl, su, X);
  if (!(X.TYP == BOOLS || X.TYP == INTS || X.TYP == NOTYP)) {
    ERROR(17);
  }
  LC2 = LC;
  EMIT(11);
  if (SY == RPARENT) {
    INSYMBOL();
  } else {
    ERROR(4);
  }
  STATEMENT(bl, bl->FSYS);
  EMIT1(10, LC1);
  CODE[LC2].Y = LC;
  if (SYMCNT == 1) {
    LOCATION[LNUM] = LC;
  }
  FIXBREAKS(LC);
  FIXCONTS(LC - 1);
}

void COMMAEXPR(BlockLocal *bl, SYMBOL TESTSY) {
  ITEM X;
  SYMSET su;
  su = bl->FSYS;
  su[COMMA] = true;
  su[TESTSY] = true;
  EXPRESSION(bl, su, X);
  if (X.SIZE > 1) {
    EMIT1(112, X.SIZE);
  }
  EMIT(111);
  while (SY == COMMA) {
    INSYMBOL();
    EXPRESSION(bl, su, X);
    if (X.SIZE > 1) {
      EMIT1(112, X.SIZE);
    }
    EMIT(111);
  }
}

void FORSTATEMENT(BlockLocal *bl) {
  ITEM X;
  int LC1, LC2, LC3;
  SYMSET su;
  INSYMBOL();
  if (SY == LPARENT) {
    INSYMBOL();
  } else {
    ERROR(9);
  }
  BREAKPNT = BREAKPNT + 1;
  BREAKLOC[BREAKPNT] = 0;
  CONTPNT = CONTPNT + 1;
  CONTLOC[CONTPNT] = 0;
  if (SY != SEMICOLON) {
    COMMAEXPR(bl, SEMICOLON);
  }
  if (SY == SEMICOLON) {
    INSYMBOL();
  } else {
    ERROR(14);
  }
  LC1 = LC;
  if (SY != SEMICOLON) {
    su = bl->FSYS;
    su[SEMICOLON] = true;
    EXPRESSION(bl, su, X);
    if (!(X.TYP == BOOLS || X.TYP == INTS || X.TYP == NOTYP)) {
      ERROR(17);
    }
  } else {
    EMIT1(24, 1);
  }
  if (SY == SEMICOLON) {
    INSYMBOL();
  } else {
    ERROR(14);
  }
  LC2 = LC;
  EMIT(11);
  EMIT(10);
  LC3 = LC;
  if (SY != RPARENT) {
    COMMAEXPR(bl, RPARENT);
  }
  EMIT1(10, LC1);
  if (SY == RPARENT) {
    INSYMBOL();
  } else {
    ERROR(4);
  }
  CODE[LC2 + 1].Y = LC;
  STATEMENT(bl, bl->FSYS);
  EMIT1(10, LC3);
  CODE[LC2].Y = LC;
  if (SYMCNT == 1) {
    LOCATION[LNUM] = LC;
  }
  FIXBREAKS(LC);
  FIXCONTS(LC - 1);
}

void BLOCKSTATEMENT(BlockLocal *bl) {
  TYPES CVT;
  ITEM X;
  SYMSET su, sv;
  long int SAVEDX, SAVENUMWITH, SAVEMAXNUMWITH, PRB, PRT, I;
  bool TCRFLAG;
  // INSYMBOL();
  strcpy(ID, DUMMYNAME);
  ENTER(bl, ID, PROZEDURE);
  DUMMYNAME[13] = (char)(DUMMYNAME[13] + 1);
  if (DUMMYNAME[13] == '0') {
    DUMMYNAME[12] = (char)(DUMMYNAME[12] + 1);
  }
  TAB[Tx].NORMAL = true;
  SAVEDX = bl->DX;
  SAVENUMWITH = bl->NUMWITH;
  SAVEMAXNUMWITH = bl->MAXNUMWITH;
  bl->LEVEL = bl->LEVEL + 1;
  bl->DX = BASESIZE;
  PRT = Tx;
  if (bl->LEVEL > LMAX) {
    FATAL(5);
  }
  bl->NUMWITH = 0;
  bl->MAXNUMWITH = 0;
  ENTERBLOCK();
  DISPLAY[bl->LEVEL] = B;
  PRB = B;
  TAB[PRT].TYP = NOTYP;
  TAB[PRT].REF = PRB;
  BTAB[PRB].LASTPAR = Tx;
  BTAB[PRB].PSIZE = bl->DX;
  su = 0;
  su[LSETBRACK] = true;
  sv = DECLBEGSYS | STATBEGSYS | ASSIGNBEGSYS;
  TEST(su, sv, 3);
  INSYMBOL();
  EMIT1(18, PRT);
  EMIT1(19, BTAB[PRB].PSIZE - 1);
  TAB[PRT].ADR = LC;
  su = bl->FSYS;
  su[RSETBRACK] = true;
  // sv = DECLBEGSYS | STATBEGSYS | ASSIGNBEGSYS;
  TEST(sv, su, 101);
  // while (DECLBEGSYS[SY] || STATBEGSYS[SY] || ASSIGNBEGSYS[SY])
  while (sv[SY]) {
    if (SY == DEFINESY) {
      CONSTANTDECLARATION(bl);
    } else if (SY == TYPESY) {
      TYPEDECLARATION(bl);
    } else if (SY == STRUCTSY) {
      VARIABLEDECLARATION(bl);
    } else if (SY == IDENT) {
      I = LOC(bl, ID);
      if (I != 0) {
        if (TAB[I].OBJ == TYPE1) {
          VARIABLEDECLARATION(bl);
        } else {
          su = bl->FSYS;
          su[SEMICOLON] = true;
          su[RSETBRACK] = true;
          STATEMENT(bl, su);
        }
      } else {
        INSYMBOL();
      }
    } else if (STATBEGSYS[SY] || ASSIGNBEGSYS[SY]) {
      su = bl->FSYS;
      su[SEMICOLON] = true;
      su[RSETBRACK] = true;
      STATEMENT(bl, su);
    }
    su = DECLBEGSYS | STATBEGSYS | ASSIGNBEGSYS;
    su[RSETBRACK] = true;
    TEST(su, bl->FSYS, 6);
  }
  BTAB[PRB].VSIZE = bl->DX;
  BTAB[PRB].VSIZE = BTAB[PRB].VSIZE + bl->MAXNUMWITH;
  if (SYMCNT == 1) {
    LOCATION[LNUM] = LC;
  }
  if (SY == RSETBRACK) {
    INSYMBOL();
  } else {
    ERROR(7);
  }
  su = bl->FSYS;
  su[PERIOD] = true;
  sv = 0;
  TEST(su, sv, 6);
  EMIT(105);
  bl->DX = SAVEDX;
  bl->NUMWITH = SAVENUMWITH;
  bl->MAXNUMWITH = SAVEMAXNUMWITH;
  bl->LEVEL = bl->LEVEL - 1;
}

void FORALLSTATEMENT(BlockLocal *bl) {
  TYPES CVT;
  ITEM X;
  long int I, J, LC1, LC2, LC3;
  // bool DEFOUND;
  bool TCRFLAG;
  SYMSET su;
  bl->FLEVEL = bl->FLEVEL + 1;
  INSYMBOL();
  if (SY == IDENT) {
    I = LOC(bl, ID);
    TAB[I].FORLEV = bl->FLEVEL;
    INSYMBOL();
    if (I == 0) {
      CVT = INTS;
    } else {
      if (TAB[I].OBJ == VARIABLE) {
        CVT = TAB[I].TYP;
        if (!TAB[I].NORMAL) {
          ERROR(26);
        } else {
          EMIT2(0, TAB[I].LEV, TAB[I].ADR);
        }
        if (!(CVT == NOTYP || CVT == INTS || CVT == BOOLS || CVT == CHARS)) {
          ERROR(18);
        }
      } else {
        ERROR(37);
        CVT = INTS;
      }
    }
  } else {
    su = bl->FSYS;
    su[BECOMES] = true;
    su[TOSY] = true;
    su[DOSY] = true;
    su[GROUPINGSY] = true;
    SKIP(su, 2);
    CVT = INTS;
  }
  if (SY == BECOMES) {
    INSYMBOL();
    su = bl->FSYS;
    su[TOSY] = true;
    su[DOSY] = true;
    su[GROUPINGSY] = true;
    EXPRESSION(bl, su, X);
    if (X.TYP != CVT) {
      ERROR(19);
    }
  } else {
    su = bl->FSYS;
    su[TOSY] = true;
    su[DOSY] = true;
    su[GROUPINGSY] = true;
    SKIP(su, 51);
  }
  if (SY == TOSY) {
    INSYMBOL();
    su = bl->FSYS;
    su[ATSY] = true;
    su[DOSY] = true;
    su[GROUPINGSY] = true;
    EXPRESSION(bl, su, X);
    if (X.TYP != CVT) {
      ERROR(19);
    }
  } else {
    su = bl->FSYS;
    su[ATSY] = true;
    su[DOSY] = true;
    su[GROUPINGSY] = true;
    SKIP(su, 55);
  }
  if (SY == GROUPINGSY) {
    INSYMBOL();
    su = bl->FSYS;
    su[ATSY] = true;
    su[DOSY] = true;
    EXPRESSION(bl, su, X);
    if (X.TYP != INTS) {
      ERROR(45);
    }
  } else {
    EMIT1(24, 1);
  }
  EMIT(4);
  LC1 = LC;
  EMIT(75);
  LC2 = LC;
  TAB[I].FORLEV = -TAB[I].FORLEV;
  if (SY == ATSY) {
    INSYMBOL();
    su = bl->FSYS;
    su[DOSY] = true;
    EXPRESSION(bl, su, X);
    if (!(X.TYP == INTS || X.TYP == NOTYP)) {
      ERROR(126);
    }
  } else {
    EMIT(79);
  }
  if (SY == DOSY) {
    INSYMBOL();
  } else {
    ERROR(54);
  }
  TAB[I].FORLEV = -TAB[I].FORLEV;
  bl->CREATEFLAG = false;
  if (SY == IDENT) {
    J = LOC(bl, ID);
    if (J != 0) {
      if (TAB[J].OBJ == PROZEDURE && TAB[J].LEV != 0) {
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
  LC3 = LC;
  EMIT1(10, 0);
  if (SY == LSETBRACK) {
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
  CODE[LC3].Y = LC;
  EMIT1(76, LC2);
  CODE[LC1].Y = LC;
  EMIT(5);
  bl->FLEVEL = bl->FLEVEL - 1;
  if (SYMCNT == 1) {
    LOCATION[LNUM] = LC;
  }
}

void FORKSTATEMENT(BlockLocal *bl) {
  ITEM X;
  int I; // added local (unlikely intended to use global)
  int JUMPLC;
  SYMSET su;
  bl->CREATEFLAG = false;
  EMIT(106);
  INSYMBOL();
  if (SY == LPARENT) {
    INSYMBOL();
    if (SY == ATSY) {
      INSYMBOL();
      su = bl->FSYS;
      su[RPARENT] = true;
      EXPRESSION(bl, su, X);
      if (!(X.TYP == INTS || X.TYP == NOTYP)) {
        ERROR(126);
      }
    } else {
      ERROR(127);
    }
    if (SY == RPARENT) {
      INSYMBOL();
    } else {
      ERROR(4);
    }
  } else {
    EMIT(79);
  }
  if (SY == IDENT) {
    I = LOC(bl, ID);
    if (I != 0) {
      if (TAB[I].OBJ == PROZEDURE && TAB[I].LEV != 0) {
        bl->CREATEFLAG = true;
      }
    }
  }
  if (bl->CREATEFLAG) {
    EMIT1(67, 1);
  } else {
    EMIT1(67, 0);
  }
  JUMPLC = LC;
  EMIT1(7, 0);
  STATEMENT(bl, bl->FSYS);
  EMIT(69);
  CODE[JUMPLC].Y = LC;
  if (SYMCNT == 1) {
    LOCATION[LNUM] = LC;
  }
}

void JOINSTATEMENT(BlockLocal *bl) {
  EMIT(85);
  INSYMBOL();
}

void RETURNSTATEMENT(BlockLocal *bl) {
  ITEM X;
  INSYMBOL();
  if (RETURNTYPE.TYP != VOIDS) {
    EXPRESSION(bl, bl->FSYS, X);
    if (X.TYP == RECS || X.TYP == ARRAYS) {
      EMIT1(113, X.SIZE);
    }
    if (!TYPE_COMPATIBLE(RETURNTYPE, X)) {
      ERROR(118);
    } else {
      if (RETURNTYPE.TYP == REALS && X.TYP == INTS) {
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
  BREAKLOC[BREAKPNT] = LC - 1;
  INSYMBOL();
}

void CONTINUESTATEMENT() {
  EMIT1(10, CONTLOC[CONTPNT]);
  CONTLOC[CONTPNT] = LC - 1;
  INSYMBOL();
}

void MOVESTATEMENT(BlockLocal *bl) {
  ITEM X;
  SYMSET su;
  INSYMBOL();
  su = bl->FSYS;
  su[TOSY] = true;
  BASICEXPRESSION(bl, su, X);
  if (!(X.ISADDR && X.TYP == CHANS)) {
    ERROR(103);
  }
  if (SY == TOSY) {
    INSYMBOL();
    su = bl->FSYS;
    su[SEMICOLON] = true;
    EXPRESSION(bl, su, X);
    if (!(X.TYP == INTS || X.TYP == NOTYP)) {
      ERROR(126);
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
//                        ERROR(131);
//                    } else
//                    {
//                        if (X.TYP == INTS || X.TYP == REALS || X.TYP == CHARS
//                        || X.TYP == NOTYP)
//                        {
//                            EMIT1(27, (int)X.TYP);
//                        } else
//                        {
//                            ERROR(40);
//                        }
//                    }
//                }
//            } while (SY == INSTR);
//        } else
//        {
//            ERROR(129);
//        }
//    }
void INPUTSTATEMENT(BlockLocal *bl) {
  ITEM X;
  SYMSET su;
  INSYMBOL();
  if (SY == INSTR) {
    do {
      INSYMBOL();
      su = bl->FSYS;
      su[INSTR] = true;
      su[SEMICOLON] = true;
      BASICEXPRESSION(bl, su, X);
      if (!X.ISADDR) {
        ERROR(131);
      } else {
        if (X.TYP == INTS || X.TYP == REALS || X.TYP == CHARS ||
            X.TYP == NOTYP) {
          EMIT1(27, (int)X.TYP);
        } else {
          ERROR(40);
        }
      }
    } while (SY == INSTR);
  } else {
    ERROR(129);
  }
}

void COUTMETHODS(BlockLocal *bl) {
  ALFA METHOD;
  ITEM X;
  SYMSET su;
  INSYMBOL();
  strcpy(METHOD, ID);
  if (SY != IDENT || (strcmp(ID, "WIDTH         ") != 0 &&
                      strcmp(ID, "PRECISION     ") != 0)) {
    ERROR(133);
  } else {
    INSYMBOL();
    if (SY == LPARENT) {
      INSYMBOL();
    } else {
      ERROR(9);
    }
    su = bl->FSYS;
    su[RPARENT] = true;
    EXPRESSION(bl, su, X);
    if (X.TYP != INTS) {
      ERROR(134);
    }
    if (SY != RPARENT) {
      ERROR(4);
    }
    if (strcmp(METHOD, "WIDTH         ") == 0) {
      EMIT1(30, 1);
    } else {
      EMIT1(30, 2);
    }
  }
  INSYMBOL();
}

void OUTPUTSTATEMENT(BlockLocal *bl) {
  ITEM X;
  SYMSET su;
  INSYMBOL();
  if (SY == PERIOD) {
    COUTMETHODS(bl);
  } else {
    EMIT(89);
    if (SY == OUTSTR) {
      do {
        INSYMBOL();
        if (SY == ENDLSY) {
          EMIT(63);
          INSYMBOL();
        } else if (SY == STRNG) {
          EMIT1(24, SLENG);
          EMIT1(28, INUM);
          INSYMBOL();
        } else {
          su = bl->FSYS;
          su[OUTSTR] = true;
          su[SEMICOLON] = true;
          EXPRESSION(bl, su, X);
          if (!(STANTYPS[X.TYP])) {
            ERROR(41);
          }
          if (X.TYP == REALS) {
            EMIT(37);
          } else {
            EMIT1(29, (int)X.TYP);
          }
        }
      } while (SY == OUTSTR);
    } else {
      ERROR(128);
    }
    EMIT(90);
  }
}
/* declared in SWITCHSTATEMENT therefore available to the contained functions
 * ala Pascal */
struct SwitchLocal {
  ITEM X;
  int I, J, K, LC1, LC2, LC3;
  bool DEFOUND;
  struct {
    INDEX VAL, LC;
  } CASETAB[CSMAX + 1];
  int EXITTAB[CSMAX + 1];
  BlockLocal *bl;
};
static void CASELABEL(SwitchLocal *);
static void ONECASE(SwitchLocal *);
void CASELABEL(SwitchLocal *sl) {
  CONREC LAB = {NOTYP, 0l};
  long K;
  SYMSET su;
  su = sl->bl->FSYS;
  su[COLON] = true;
  CONSTANT(sl->bl, su, LAB);
  if (LAB.TP != sl->X.TYP) {
    ERROR(47);
  } else if (sl->I == CSMAX) {
    FATAL(6);
  } else {
    sl->I = sl->I + 1;
    K = 0;
    sl->CASETAB[sl->I].VAL = (int)LAB.I;
    sl->CASETAB[sl->I].LC = LC;
    do {
      K = K + 1;
    } while (sl->CASETAB[K].VAL != LAB.I);
    if (K < sl->I) {
      ERROR(1);
    }
  }
}

void ONECASE(SwitchLocal *sl) {
  SYMSET su;
  if (CONSTBEGSYS[SY]) {
    CASELABEL(sl);
    if (SY == COLON) {
      INSYMBOL();
    } else {
      ERROR(5);
    }
    su = sl->bl->FSYS;
    su[CASESY] = true;
    su[SEMICOLON] = true;
    su[DEFAULTSY] = true;
    su[RSETBRACK] = true;
    while (STATBEGSYS[SY] || ASSIGNBEGSYS[SY]) {
      STATEMENT(sl->bl, su);
    }
    su = 0;
    su[CASESY] = true;
    su[DEFAULTSY] = true;
    su[RSETBRACK] = true;
    TEST(su, sl->bl->FSYS, 125);
  }
}

void SWITCHSTATEMENT(BlockLocal *bl) {
  // int K;
  struct SwitchLocal sl;
  memset(&sl, 0, sizeof(struct SwitchLocal));
  sl.bl = bl;
  SYMSET su;
  BREAKPNT = BREAKPNT + 1;
  BREAKLOC[BREAKPNT] = 0;
  INSYMBOL();
  if (SY == LPARENT) {
    INSYMBOL();
  } else {
    ERROR(9);
  }
  sl.I = 0;
  su = bl->FSYS;
  su[RPARENT] = true;
  su[COLON] = true;
  su[LSETBRACK] = true;
  su[CASESY] = true;
  EXPRESSION(bl, su, sl.X);
  if (!(sl.X.TYP == INTS || sl.X.TYP == BOOLS || sl.X.TYP == CHARS ||
        sl.X.TYP == NOTYP)) {
    ERROR(23);
  }
  if (SY == RPARENT) {
    INSYMBOL();
  } else {
    ERROR(4);
  }
  sl.LC1 = LC;
  EMIT(12);
  if (SY == LSETBRACK) {
    INSYMBOL();
  } else {
    ERROR(106);
  }
  if (SY == CASESY) {
    INSYMBOL();
  } else {
    ERROR(124);
  }
  ONECASE(&sl);
  while (SY == CASESY) {
    INSYMBOL();
    ONECASE(&sl);
  }
  sl.DEFOUND = false;
  if (SY == DEFAULTSY) {
    INSYMBOL();
    if (SY == COLON) {
      INSYMBOL();
    } else {
      ERROR(5);
    }
    sl.LC3 = LC;
    while (STATBEGSYS[SY] || ASSIGNBEGSYS[SY]) {
      su = bl->FSYS;
      su[SEMICOLON] = true;
      su[RSETBRACK] = true;
      STATEMENT(bl, su);
    }
    sl.DEFOUND = true;
  }
  sl.LC2 = LC;
  EMIT(10);
  CODE[sl.LC1].Y = LC;
  for (sl.K = 1; sl.K <= sl.I; sl.K++) {
    EMIT1(13, sl.CASETAB[sl.K].VAL);
    EMIT1(13, sl.CASETAB[sl.K].LC);
  }
  if (sl.DEFOUND) {
    EMIT2(13, -1, 0);
    EMIT1(13, sl.LC3);
  }
  EMIT1(10, 0);
  CODE[sl.LC2].Y = LC;
  if (SYMCNT == 1) {
    LOCATION[LNUM] = LC;
  }
  if (SY == RSETBRACK) {
    INSYMBOL();
  } else {
    ERROR(104);
  }
  FIXBREAKS(LC);
}

} // namespace Cstar
