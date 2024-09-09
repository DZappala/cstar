// #include <iostream>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>

#include "cs_compile.h"
#include "cs_errors.h"
#include "cs_global.h"

namespace Cstar {
//    void FATAL(int);
//    void ERROR(int);
//    void ERRORMSG();
//    void ERROREXIT();
//    extern bool FATALERROR;
extern void INTERPRET();
extern void dumpInst(int);
// extern bool eoln(FILE *);
//  double RNUM; // source local ?
//  int SLENG;  // source local ?
//  int EOFCOUNT = 0;  // source local ?
void NEXTCH();
std::string FILENAME;
struct IncludeStack {
  FILE *inc;
  std::string fname;
};
struct InsymbolLocal {
  int I, J, K, E;
  bool ISDEC;
};
static const int maxiStack = 10;
static struct IncludeStack iStack[maxiStack];
static int iStackIndex = -1;
bool inCLUDEFLAG() { return iStackIndex >= 0; }
static int lookAhead = -2;
static bool console_list = false;
static bool code_list = false;
void showConsoleList(bool flg) { console_list = flg; }
static void showCodeList(bool flg) { code_list = flg; }
bool is_eoln(FILE *inf) { return (lookAhead == '\r' || lookAhead == '\n'); }
void do_eoln(FILE *inf) {
  if (lookAhead == '\r') {
    lookAhead = fgetc(inf);
  }
  if (lookAhead == '\n') {
    lookAhead = fgetc(inf);
  }
}
static bool is_eof(FILE *inf) {
  if (lookAhead == -2)
    lookAhead = fgetc(inf);
  return lookAhead < 0;
}
static char readc(FILE *inf) {
  char ch;
  ch = (char)lookAhead;
  lookAhead = fgetc(inf);
  return ch;
}
static bool eoln(FILE *inf) {
  int ch;
  bool eln = false;
  // //        if ((*SRC).eof())
  // //            return true;
  //        ch = (*SRC).peek();
  ch = fgetc(inf);
  if (ch >= 0) {
    if (ch == '\r') {
      // (*SRC).ignore();
      ch = fgetc(inf);
      eln = true;
    }
    if (ch == '\n') {
      // (*SRC).ignore(2);
      ch = fgetc(inf);
      eln = true;
    }
    if (ch >= 0)
      ungetc(ch, inf);
    else
      eln = true;
  } else
    eln = true;
  return eln;
}
void EMIT(int FCT) {
  if (LC == CMAX) {
    FATAL(6);
  } else {
    CODE[LC].F = FCT;
    CODE[LC].X = 0;
    CODE[LC].Y = 0;
    // std::cout << "Code0:" << CODE[LC].F << " " << CODE[LC].X << "," <<
    // CODE[LC].Y << std::endl;
    if (code_list)
      dumpInst(LC);
    LC++;
  }
}

void EMIT1(int FCT, int lB) {
  if (LC == CMAX) {
    FATAL(6);
  } else {
    CODE[LC].F = FCT;
    CODE[LC].Y = lB;
    CODE[LC].X = 0;
    // std::cout << "Code1:" << CODE[LC].F << " " << CODE[LC].X << "," <<
    // CODE[LC].Y << std::endl;
    if (code_list)
      dumpInst(LC);
    LC++;
  }
}

void EMIT2(int FCT, int lA, int lB) {
  if (LC == CMAX) {
    FATAL(6);
  } else {
    CODE[LC].F = FCT;
    CODE[LC].X = lA;
    CODE[LC].Y = lB;
    // std::cout << "Code2:" << CODE[LC].F << " " << CODE[LC].X << "," <<
    // CODE[LC].Y << std::endl;
    if (code_list)
      dumpInst(LC);
    LC++;
  }
}

void INCLUDEDIRECTIVE() {
  // int CHVAL;
  int LENG;
  INSYMBOL();
  if (SY == LSS) {
    FILENAME = "";
    LENG = 0;
    // while (((CH >= 'a' && CH <= 'z') || (CH >= 'A' && CH <= 'Z') || (CH >=
    // '0' && CH <= '9') ||
    //         (CH == '\\' || CH == '.' || CH == '_' || CH == ':')) && (LENG <=
    //         FILMAX))
    while ((std::isalnum(CH) || CH == '/' || CH == '\\' || CH == '.' ||
            CH == '_' || CH == ':') &&
           (LENG <= FILMAX)) {
      // if (CH >= 'a' && CH <= 'z') CH = CH - 32;  // DE
      FILENAME += CH;
      LENG = LENG + 1;
      NEXTCH();
    }
    INSYMBOL();
    if (SY == GTR || SY == GRSEMI) {
      // #I-
      // needs fixup
      // ASSIGN(INSRC, FILENAME.c_str());
      // RESET(INSRC);
      // #I-
      if (++iStackIndex == maxiStack) {
        ERROR(149);
      }
      //                if (INSRC != nullptr) // || IOResult() != 0)
      //                    ERROR(149);
      else {
        INSRC = fopen(FILENAME.c_str(), "r");
        if (!INSRC) {
          ERROR(150);
        } else {
          iStack[iStackIndex].fname = FILENAME;
          iStack[iStackIndex].inc = INSRC;
          INCLUDEFLAG = true;
          if (iStackIndex == 0) {
            SAVESYMCNT = SYMCNT;
            SAVEXECNT = EXECNT;
            // SAVELC = LC;  // DE
          }
          INSYMBOL();
        }
      }
    } else
      ERROR(6);
  } else
    ERROR(6);
}
void MAINNEXTCH() {
  if (CC == LL) {
    // end of the buffered line
    // read and buffer another line
    // if ((*SRC).eof())
    if (feof(SRC)) {
      CH = (char)(28);
      goto label37;
    }
    if (ERRPOS != 0) {
      // std::cout << std::endl;
      if (console_list)
        fputc('\n', STDOUT);
      // (*LIS) << std::endl;
      fputc('\n', LIS);
      ERRPOS = 0;
    }
    if (LNUM > 0) {
      if (LOCATION[LNUM] == LC && EXECNT > 0) {
        EMIT(6);
      }
    }
    LNUM++;
    SYMCNT = 0;
    EXECNT = 0;
    // std::cout << LNUM << "     ";
    if (console_list)
      fprintf(STDOUT, "%5d ", LNUM);
    fprintf(LIS, "%5d ", LNUM);
    LOCATION[LNUM] = LC;
    BREAKALLOW[LNUM] = OKBREAK;
    LL = 0;
    CC = 0;
    while (!eoln(SRC)) {
      LL++;
      // CH = (*SRC).get();
      CH = (char)fgetc(SRC);
      // std::cout << CH;
      if (console_list)
        fputc(CH, STDOUT);
      fputc(CH, LIS);
      if (CH == 9) {
        CH = ' ';
      }
      LINE[LL] = CH;
    }
    if (console_list)
      fputc('\n', STDOUT);
    // std::cout << std::endl;
    fputc('\n', LIS);
    LL++;
    // do_eoln(SRC);
    LINE[LL] = ' ';
  }
  CC++;
  CH = LINE[CC];
label37:;
}

void ALTNEXTCH() {
  if (CC2 == LL2) {
    // if ((*INSRC).eof())
    if (feof(INSRC)) {
      //(*INSRC).close();
      fclose(INSRC);
      if (--iStackIndex < 0) {
        INCLUDEFLAG = false;
        INSRC = nullptr;
        EXECNT = SAVEXECNT;
        SYMCNT = SAVESYMCNT;
        // LC = SAVELC;  // DE
        MAINNEXTCH();
        return;
      }
      INSRC = iStack[iStackIndex].inc;
      FILENAME = iStack[iStackIndex].fname;
    }
    LL2 = 0;
    CC2 = 0;
    while (!eoln(INSRC)) {
      LL2++;
      // CH = (*INSRC).get();
      CH = (int)fgetc(INSRC);
      if (CH == '\x09') {
        CH = ' ';
      }
      LINE2[LL2] = CH;
    }
    LL2++;
    //(*INSRC).ignore();
    LINE2[LL2] = ' ';
  }
  CC2++;
  CH = LINE2[CC2];
}

void NEXTCH() {
  if (inCLUDEFLAG()) {
    ALTNEXTCH();
  } else {
    MAINNEXTCH();
  }
}

void ADJUSTSCALE(struct InsymbolLocal *nl) {
  int S;
  float D, T;
  if (nl->K + nl->E > EMAX)
    ERROR(21);
  else if (nl->K + nl->E < EMIN)
    RNUM = 0.0;
  else {
    S = abs(nl->E);
    T = 1.0;
    D = 10.0;
    do {
      while ((S & 0x01) == 0) {
        S >>= 1;
        D *= D;
      }
      S -= 1;
      T *= D;
    } while (S != 0);
    if (nl->E >= 0)
      RNUM = RNUM * T;
    else
      RNUM = RNUM / T;
  }
}

void READSCALE(struct InsymbolLocal *nl) {
  int S, SIGN;
  NEXTCH();
  SIGN = 1;
  S = 0;
  if (CH == '+') {
    NEXTCH();
  } else if (CH == '-') {
    NEXTCH();
    SIGN = -1;
  }
  while (isdigit(CH)) {
    S = 10 * S + (CH - '0');
    NEXTCH();
  }
  nl->E = S * SIGN + nl->E;
}

void FRACTION(struct InsymbolLocal *nl) {
  SY = REALCON;
  RNUM = INUM;
  nl->E = 0;
  while (isdigit(CH)) {
    nl->E -= 1;
    RNUM = RNUM * 10.0 + (CH - '0');
    NEXTCH();
  }
  if (CH == 'E')
    READSCALE(nl);
  if (nl->E != 0)
    ADJUSTSCALE(nl);
}

void ESCAPECHAR() {
  switch (CH) {
  case 'a':
    CH = 7;
    break;
  case 'b':
    CH = 8;
    break;
  case 'f':
    CH = 12;
    break;
  case 'n':
    CH = 10;
    break;
  case 'r':
    CH = 13;
    break;
  case 't':
    CH = 9;
    break;
  case 'v':
    CH = 11;
    break;
  case '\\':
  case '?':
  case '\'':
  case '"':
    break;
  default:
    ERROR(132);
    break;
  }
}

static bool isoctal(char ch) { return ch >= '0' && ch <= '7'; }

void ESCAPECHAR2() {
  NEXTCH();
  if (isoctal(CH)) {
    INUM = 0;
    while (isoctal(CH)) {
      INUM = INUM * 8 + (CH - '0');
      NEXTCH();
    }
    if (INUM < 128) {
      STAB[SX] = (char)INUM;
      SX += 1;
      if (CH == '\'')
        NEXTCH();
      else
        ERROR(130);
    } else {
      ERROR(24);
      if (CH == '\'')
        NEXTCH();
    }
  } else {
    ESCAPECHAR();
    STAB[SX] = CH;
    INUM = CH;
    SX += 1;
    NEXTCH();
    if (CH == '\'')
      NEXTCH();
    else
      ERROR(130);
  }
  SY = CHARCON;
}

void INSYMBOL() {
  // int I, J, K, E;
  // bool ISDEC;
  struct InsymbolLocal nl = {0, 0, 0, 0, false};
label1:
  while (CH == ' ')
    NEXTCH();
  if (isalpha(CH) || CH == '#') {
    nl.K = -1;
    strcpy(ID, "              ");
    do {
      if (nl.K < ALNG - 1) {
        nl.K++;
        if (islower(CH)) {
          CH = toupper(CH);
        }
        ID[nl.K] = CH;
      } else {
        // fprintf(STDOUT, "long symbol %s\n", ID);
      }
      NEXTCH();
    } while (isalnum(CH) || CH == '_');
    nl.I = 1;
    nl.J = NKW;
    do {
      nl.K = (nl.I + nl.J) / 2;
      if (strcmp(ID, KEY[nl.K]) <= 0) {
        nl.J = nl.K - 1;
      }
      if (strcmp(ID, KEY[nl.K]) >= 0) {
        nl.I = nl.K + 1;
      }
    } while (nl.I <= nl.J);
    if (nl.I - 1 > nl.J) {
      SY = KSY[nl.K];
    } else {
      SY = IDENT;
    }
    if (MPIMODE && (NONMPISYS[SY] == 1)) {
      ERROR(145);
    }
  } else if (isdigit(CH)) {
    nl.K = 0;
    INUM = 0;
    nl.ISDEC = true;
    SY = INTCON;
    if (CH == '0') {
      nl.ISDEC = false;
      NEXTCH();
      if (CH == 'X' || CH == 'x') {
        NEXTCH();
        while (isxdigit(CH)) {
          if (nl.K <= KMAX) {
            INUM = INUM * 16;
            if (isdigit(CH)) {
              INUM += CH - '0';
            } else if (isupper(CH)) {
              INUM += CH - 'A' + 10;
            } else if (islower(CH)) {
              INUM += CH - 'a' + 10;
            }
          }
          nl.K++;
          NEXTCH();
        }
      } else {
        while (isdigit(CH)) {
          if (nl.K <= KMAX) {
            INUM = INUM * 8 + CH - '0';
          }
          nl.K++;
          NEXTCH();
        }
      }
    } else {
      do {
        if (nl.K <= KMAX) {
          INUM = INUM * 10 + CH - '0';
        }
        nl.K++;
        NEXTCH();
      } while (isdigit(CH));
    }
    if (nl.K > KMAX || abs(INUM) > NMAX) {
      ERROR(21);
      INUM = 0;
      nl.K = 0;
    }
    if (nl.ISDEC || INUM == 0) {
      if (CH == '.') {
        NEXTCH();
        if (CH == '.') {
          CH = ':';
        } else {
          FRACTION(&nl);
        }
      } else if (CH == 'E') {
        SY = REALCON;
        RNUM = INUM;
        nl.E = 0;
        READSCALE(&nl);
        if (nl.E != 0) {
          ADJUSTSCALE(&nl);
        }
      }
    }
  } else if (CH == '<') {
    NEXTCH();
    if (CH == '=') {
      SY = LEQ;
      NEXTCH();
    } else if (CH == '>') {
      SY = NEQ;
      NEXTCH();
    } else if (CH == '<') {
      SY = OUTSTR;
      NEXTCH();
    } else {
      SY = LSS;
    }
  } else if (CH == '=') {
    NEXTCH();
    if (CH == '=') {
      SY = EQL;
      NEXTCH();
    } else {
      SY = BECOMES;
    }
  } else if (CH == '!') {
    NEXTCH();
    if (CH == '=') {
      SY = NEQ;
      NEXTCH();
    } else {
      SY = NOTSY;
    }
  } else if (CH == '&') {
    NEXTCH();
    if (CH == '&') {
      SY = ANDSY;
      NEXTCH();
    } else if (CH == ' ') {
      SY = BITANDSY;
    } else {
      SY = ADDRSY;
    }
  } else if (CH == '|') {
    NEXTCH();
    if (CH == '|') {
      SY = ORSY;
      NEXTCH();
    } else {
      SY = BITINCLSY;
    }
  } else if (CH == '>') {
    NEXTCH();
    if (CH == '=') {
      SY = GEQ;
      NEXTCH();
    } else if (CH == '>') {
      SY = INSTR;
      NEXTCH();
    } else if (CH == ';') {
      SY = GRSEMI;
      NEXTCH();
    } else {
      SY = GTR;
    }
  } else if (CH == '+') {
    NEXTCH();
    if (CH == '+') {
      SY = INCREMENT;
      NEXTCH();
    } else if (CH == '=') {
      SY = PLUSEQ;
      NEXTCH();
    } else {
      SY = PLUS;
    }
  } else if (CH == '-') {
    NEXTCH();
    if (CH == '-') {
      SY = DECREMENT;
      NEXTCH();
    } else if (CH == '>') {
      SY = RARROW;
      NEXTCH();
    } else if (CH == '=') {
      SY = MINUSEQ;
      NEXTCH();
    } else {
      SY = MINUS;
    }
  } else if (CH == '.') {
    NEXTCH();
    if (CH == '.') {
      SY = COLON;
      NEXTCH();
    } else if (isdigit(CH)) {
      INUM = 0;
      nl.K = 0;
      FRACTION(&nl);
    } else {
      SY = PERIOD;
    }
  } else if (CH == '\'') {
    NEXTCH();
    if (CH == '\'') {
      ERROR(38);
      SY = CHARCON;
      INUM = 0;
    } else {
      if (SX + 1 == SMAX) {
        FATAL(3);
      }
      if (CH == '\\') {
        ESCAPECHAR2();
      } else {
        STAB[SX] = CH;
        SY = CHARCON;
        INUM = (unsigned char)STAB[SX];
        SX++;
        NEXTCH();
        if (CH == '\'') {
          NEXTCH();
        } else {
          ERROR(130);
        }
      }
    }
  } else if (CH == '"') {
    nl.K = 0;
  label4:
    NEXTCH();
    if (CH == '"') {
      NEXTCH();
      if (CH != '"') {
        goto label5;
      }
    }
    if (SX + nl.K == SMAX) {
      FATAL(3);
    }
    if (CH == '\\') {
      NEXTCH();
      ESCAPECHAR();
    }
    STAB[SX + nl.K] = CH;
    nl.K++;
    if (CC == 1) {
      nl.K = 0;
    } else {
      goto label4;
    }
  label5:
    if (nl.K == 0) {
      ERROR(38);
      SY = CHARCON;
      INUM = 0;
    } else {
      SY = STRNG;
      INUM = SX;
      SLENG = nl.K;
      SX += nl.K;
    }
  } else if (CH == '/') {
    NEXTCH();
    if (CH == '*') {
      NEXTCH();
      do {
        while (CH != '*' && CH != char(28)) {
          NEXTCH();
        }
        NEXTCH();
      } while (CH != '/' && CH != char(28));
      if (CH == char(28)) {
        //                    std::cout << std::endl;
        //                    std::cout << std::endl;
        //                    std::cout << " FAILURE TO END A COMMENT" <<
        //                    std::endl;
        if (console_list)
          fprintf(STDOUT, "\n\n FAILURE TO END A COMMENT\n");
        fprintf(LIS, "\n\n FAILURE TO END A COMMENT\n");
        ERRORMSG();
        FATALERROR = true;
        ERROREXIT();
      }
      NEXTCH();
      goto label1;
    } else if (CH == '=') {
      SY = RDIVEQ;
      NEXTCH();
    } else {
      SY = RDIV;
    }
  } else if (CH == '?') {
    NEXTCH();
    if (CH == '?') {
      SY = DBLQUEST;
      NEXTCH();
    } else {
      SY = QUEST;
    }
  } else if (CH == '*') {
    NEXTCH();
    if (CH == '=') {
      SY = TIMESEQ;
      NEXTCH();
    } else {
      SY = TIMES;
    }
  } else if (CH == '%') {
    NEXTCH();
    if (CH == '=') {
      SY = IMODEQ;
      NEXTCH();
    } else {
      SY = IMOD;
    }
  } else if (CH == '(' || CH == ')' || CH == ':' || CH == ',' || CH == '[' ||
             CH == ']' || CH == ';' || CH == '@' || CH == '{' || CH == '}' ||
             CH == '~' || CH == '^') {
    SY = SPS[CH];
    NEXTCH();
  } else if (CH == char(28)) {
    SY = EOFSY;
    EOFCOUNT++;
    if (EOFCOUNT > 5) {
      //                std::cout << std::endl;
      //                std::cout << std::endl;
      //                std::cout << " PROGRAM INCOMPLETE" << std::endl;
      if (console_list)
        fprintf(STDOUT, "\n\n PROGRAM INCOMPLETE\n");
      fprintf(LIS, "\n\n PROGRAM INCOMPLETE\n");
      ERRORMSG();
      FATALERROR = true;
      ERROREXIT();
    }
  } else {
    ERROR(24);
    NEXTCH();
    goto label1;
  }
  SYMCNT++;
  if (EXECSYS[SY] == 1) {
    EXECNT++;
  }
  if (SY == UNIONSY) {
    ERROR(154);
  }
  if (SY == ENUMSY) {
    ERROR(155);
  }
  if (SY == BITCOMPSY || SY == BITANDSY || SY == BITXORSY || SY == BITINCLSY) {
    ERROR(156);
  }
  if (SY == QUEST) {
    ERROR(157);
  }
}

void ENTERARRAY(TYPES TP, int L, int H) {
  if (abs(L) > XMAX || abs(H) > XMAX) {
    ERROR(27);
    L = 0;
    H = 0;
  }
  if (A == AMAX) {
    FATAL(4);
  } else {
    A++;
    ATAB[A].INXTYP = TP;
    ATAB[A].LOW = L;
    ATAB[A].HIGH = H;
  }
}

void ENTERCHANNEL() {
  if (C == CMAX) {
    FATAL(9);
  } else {
    C++;
  }
}
} // namespace Cstar
