// #include <iostream>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <print>
#include <string>

#include "cs_compile.h"
#include "cs_errors.h"
#include "cs_global.h"

namespace Cstar {
using std::println;
using std::string;

//    void fatal(int);
//    void error(int);
//    void errorMSG();
//    void errorEXIT();
//    extern bool fatalerror;

extern void INTERPRET();
extern void dumpInst(int);

// extern bool eoln(FILE *);
//  double RNUM; // source local ?
//  int SLENG;  // source local ?
//  int eof_count = 0;  // source local ?

void NEXTCH();
string FILENAME;

struct IncludeStack {
  FILE *inc{};
  string fname;
} __attribute__((aligned(32)));

struct InsymbolLocal {
  int I, J, K, E;
  bool ISDEC;
} __attribute__((aligned(32))) __attribute__((packed));

static const int maxiStack = 10;
static struct std::array<IncludeStack, maxiStack> iStack;
static int i_stack_index = -1;

auto inCLUDEFLAG() -> bool { return i_stack_index >= 0; }

static int look_ahead = -2;
static bool console_list = false;
static bool code_list = false;

void showConsoleList(bool flg) { console_list = flg; }

static void showCodeList(bool flg) { code_list = flg; }

auto is_eoln(FILE *inf) -> bool {
  return (look_ahead == '\r' || look_ahead == '\n');
}

void do_eoln(FILE *inf) {
  if (look_ahead == '\r') {
    look_ahead = fgetc(inf);
  }
  if (look_ahead == '\n') {
    look_ahead = fgetc(inf);
  }
}
static auto is_eof(FILE *inf) -> bool {
  if (look_ahead == -2) {
    look_ahead = fgetc(inf);
  }

  return look_ahead < 0;
}
static auto readc(FILE *inf) -> char {
  char ltr = 0;
  ltr = static_cast<char>(look_ahead);
  look_ahead = fgetc(inf);
  return ltr;
}
static auto eoln(FILE *inf) -> bool {
  int ltr = 0;
  bool eln = false;
  // //        if ((*SRC).eof())
  // //            return true;
  //        ch = (*SRC).peek();
  ltr = fgetc(inf);
  if (ltr >= 0) {
    if (ltr == '\r') {
      // (*SRC).ignore();
      ltr = fgetc(inf);
      eln = true;
    }
    if (ltr == '\n') {
      // (*SRC).ignore(2);
      ltr = fgetc(inf);
      eln = true;
    }
    if (ltr >= 0) {
      auto unget = ungetc(ltr, inf);
    } else {
      eln = true;
    }
  } else {
    eln = true;
  }
  return eln;
}

void EMIT(int FCT) {
  if (line_count == CMAX) {
    fatal(6);
  } else {
    code.at(line_count).F = FCT;
    code.at(line_count).X = 0;
    code.at(line_count).Y = 0;
    // std::cout << "Code0:" << CODE[line_count].F << " " << CODE[line_count].X
    // << "," << CODE[line_count].Y << std::endl;

    if (code_list) {
      dumpInst(line_count);
    }

    line_count++;
  }
}

void EMIT1(int FCT, int lB) {
  if (line_count == CMAX) {
    fatal(6);
  } else {
    code.at(line_count).F = FCT;
    code.at(line_count).Y = lB;
    code.at(line_count).X = 0;
    // std::cout << "Code1:" << CODE[line_count].F << " " << CODE[line_count].X
    // << "," << CODE[line_count].Y << std::endl;

    if (code_list) {
      dumpInst(line_count);
    }

    line_count++;
  }
}

void EMIT2(int FCT, int lA, int lB) {
  if (line_count == CMAX) {
    fatal(6);
  } else {
    code.at(line_count).F = FCT;
    code.at(line_count).X = lA;
    code.at(line_count).Y = lB;
    // std::cout << "Code2:" << CODE[line_count].F << " " << CODE[line_count].X
    // << "," << CODE[line_count].Y << std::endl;

    if (code_list) {
      dumpInst(line_count);
    }
    line_count++;
  }
}

void INCLUDEDIRECTIVE() {
  // int CHVAL;
  int len = 0;
  INSYMBOL();
  if (symbol == Symbol::LSS) {
    FILENAME = "";
    len = 0;
    // while (((CH >= 'a' && CH <= 'z') || (CH >= 'A' && CH <= 'Z') || (CH >=
    // '0' && CH <= '9') ||
    //         (CH == '\\' || CH == '.' || CH == '_' || CH == ':')) && (LENG <=
    //         FILMAX))
    while (((std::isalnum(CH) != 0) || CH == '/' || CH == '\\' || CH == '.' ||
            CH == '_' || CH == ':') &&
           (len <= FILMAX)) {
      // if (CH >= 'a' && CH <= 'z') CH = CH - 32;  // DE
      FILENAME += CH;
      len++;
      NEXTCH();
    }
    INSYMBOL();
    if (symbol == Symbol::GTR || symbol == Symbol::GRSEMI) {
      // #I-
      // needs fixup
      // ASSIGN(INSRC, FILENAME.c_str());
      // RESET(INSRC);
      // #I-
      if (++i_stack_index == maxiStack) {
        error(149);
      }
      //                if (INSRC != nullptr) // || IOResult() != 0)
      //                    error(149);
      else {
        INSRC = fopen(FILENAME.c_str(), "r");
        if (INSRC == nullptr) {
          error(150);
        } else {
          iStack.at(i_stack_index).fname = FILENAME;
          iStack.at(i_stack_index).inc = INSRC;
          INCLUDEFLAG = true;
          if (i_stack_index == 0) {
            save_symbol_count = symbol_count;
            save_execution_count = execution_count;
            // SAVELC = LC;  // DE
          }
          INSYMBOL();
        }
      }
    } else
      error(6);
  } else
    error(6);
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
    if (error_position != 0) {
      // std::cout << std::endl;
      if (console_list)
        fputc('\n', STDOUT);
      // (*LIS) << std::endl;
      fputc('\n', LIS);
      error_position = 0;
    }
    if (LNUM > 0) {
      if (LOCATION[LNUM] == line_count && execution_count > 0) {
        EMIT(6);
      }
    }
    LNUM++;
    symbol_count = 0;
    execution_count = 0;
    // std::cout << LNUM << "     ";
    if (console_list)
      fprintf(STDOUT, "%5d ", LNUM);
    fprintf(LIS, "%5d ", LNUM);
    LOCATION[LNUM] = line_count;
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
      line[LL] = CH;
    }
    if (console_list)
      fputc('\n', STDOUT);
    // std::cout << std::endl;
    fputc('\n', LIS);
    LL++;
    // do_eoln(SRC);
    line[LL] = ' ';
  }
  CC++;
  CH = line[CC];
label37:;
}

void ALTNEXTCH() {
  if (CC2 == LL2) {
    // if ((*INSRC).eof())
    if (feof(INSRC)) {
      //(*INSRC).close();
      fclose(INSRC);
      if (--i_stack_index < 0) {
        INCLUDEFLAG = false;
        INSRC = nullptr;
        execution_count = save_execution_count;
        symbol_count = save_symbol_count;
        // line_count = SAVELC;  // DE
        MAINNEXTCH();
        return;
      }
      INSRC = iStack[i_stack_index].inc;
      FILENAME = iStack[i_stack_index].fname;
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
      line2[LL2] = CH;
    }
    LL2++;
    //(*INSRC).ignore();
    line2[LL2] = ' ';
  }
  CC2++;
  CH = line2[CC2];
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
    error(21);
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
  symbol = Symbol::REALCON;
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
    error(132);
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
        error(130);
    } else {
      error(24);
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
      error(130);
  }
  symbol = Symbol::CHARCON;
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
      if (strcmp(ID, key[nl.K]) <= 0) {
        nl.J = nl.K - 1;
      }
      if (strcmp(ID, key[nl.K]) >= 0) {
        nl.I = nl.K + 1;
      }
    } while (nl.I <= nl.J);
    if (nl.I - 1 > nl.J) {
      symbol = ksy[nl.K];
    } else {
      symbol = Symbol::IDENT;
    }
    if (mpi_mode && (NONMPISYS[SY] == 1)) {
      error(145);
    }
  } else if (isdigit(CH)) {
    nl.K = 0;
    INUM = 0;
    nl.ISDEC = true;
    symbol = Symbol::INTCON;
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
      error(21);
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
        symbol = Symbol::REALCON;
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
      symbol = Symbol::LEQ;
      NEXTCH();
    } else if (CH == '>') {
      symbol = Symbol::NEQ;
      NEXTCH();
    } else if (CH == '<') {
      symbol = Symbol::OUTSTR;
      NEXTCH();
    } else {
      symbol = Symbol::LSS;
    }
  } else if (CH == '=') {
    NEXTCH();
    if (CH == '=') {
      symbol = Symbol::EQL;
      NEXTCH();
    } else {
      symbol = Symbol::BECOMES;
    }
  } else if (CH == '!') {
    NEXTCH();
    if (CH == '=') {
      symbol = Symbol::NEQ;
      NEXTCH();
    } else {
      symbol = Symbol::NOTSY;
    }
  } else if (CH == '&') {
    NEXTCH();
    if (CH == '&') {
      SY = Symbol::ANDSY;
      NEXTCH();
    } else if (CH == ' ') {
      SY = Symbol::BITANDSY;
    } else {
      SY = Symbol::ADDRSY;
    }
  } else if (CH == '|') {
    NEXTCH();
    if (CH == '|') {
      SY = Symbol::ORSY;
      NEXTCH();
    } else {
      SY = Symbol::BITINCLSY;
    }
  } else if (CH == '>') {
    NEXTCH();
    if (CH == '=') {
      SY = Symbol::GEQ;
      NEXTCH();
    } else if (CH == '>') {
      SY = Symbol::INSTR;
      NEXTCH();
    } else if (CH == ';') {
      SY = Symbol::GRSEMI;
      NEXTCH();
    } else {
      SY = Symbol::GTR;
    }
  } else if (CH == '+') {
    NEXTCH();
    if (CH == '+') {
      SY = Symbol::INCREMENT;
      NEXTCH();
    } else if (CH == '=') {
      SY = Symbol::PLUSEQ;
      NEXTCH();
    } else {
      SY = Symbol::PLUS;
    }
  } else if (CH == '-') {
    NEXTCH();
    if (CH == '-') {
      SY = Symbol::DECREMENT;
      NEXTCH();
    } else if (CH == '>') {
      SY = Symbol::RARROW;
      NEXTCH();
    } else if (CH == '=') {
      SY = Symbol::MINUSEQ;
      NEXTCH();
    } else {
      SY = Symbol::MINUS;
    }
  } else if (CH == '.') {
    NEXTCH();
    if (CH == '.') {
      SY = Symbol::COLON;
      NEXTCH();
    } else if (isdigit(CH)) {
      INUM = 0;
      nl.K = 0;
      FRACTION(&nl);
    } else {
      SY = Symbol::PERIOD;
    }
  } else if (CH == '\'') {
    NEXTCH();
    if (CH == '\'') {
      error(38);
      SY = Symbol::CHARCON;
      INUM = 0;
    } else {
      if (SX + 1 == SMAX) {
        fatal(3);
      }
      if (CH == '\\') {
        ESCAPECHAR2();
      } else {
        STAB[SX] = CH;
        SY = Symbol::CHARCON;
        INUM = (unsigned char)STAB[SX];
        SX++;
        NEXTCH();
        if (CH == '\'') {
          NEXTCH();
        } else {
          error(130);
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
      fatal(3);
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
      error(38);
      SY = Symbol::CHARCON;
      INUM = 0;
    } else {
      SY = Symbol::STRNG;
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
        if (console_list) {
          println(STDOUT, "\n\n FAILURE TO END A COMMENT");
        }
        println(LIS, "\n\n FAILURE TO END A COMMENT");
        error_message();
        fatal_error = true;
        error_exit();
      }
      NEXTCH();
      goto label1;
    } else if (CH == '=') {
      SY = Symbol::RDIVEQ;
      NEXTCH();
    } else {
      SY = Symbol::RDIV;
    }
  } else if (CH == '?') {
    NEXTCH();
    if (CH == '?') {
      SY = Symbol::DBLQUEST;
      NEXTCH();
    } else {
      SY = Symbol::QUEST;
    }
  } else if (CH == '*') {
    NEXTCH();
    if (CH == '=') {
      SY = Symbol::TIMESEQ;
      NEXTCH();
    } else {
      SY = Symbol::TIMES;
    }
  } else if (CH == '%') {
    NEXTCH();
    if (CH == '=') {
      SY = Symbol::IMODEQ;
      NEXTCH();
    } else {
      SY = Symbol::IMOD;
    }
  } else if (CH == '(' || CH == ')' || CH == ':' || CH == ',' || CH == '[' ||
             CH == ']' || CH == ';' || CH == '@' || CH == '{' || CH == '}' ||
             CH == '~' || CH == '^') {
    symbol = SPS[CH];
    NEXTCH();
  } else if (CH == static_cast<char>(28)) {
    SY = Symbol::EOFSY;
    eof_count++;
    if (eof_count > 5) {
      //                std::cout << std::endl;
      //                std::cout << std::endl;
      //                std::cout << " PROGRAM INCOMPLETE" << std::endl;
      if (console_list) {
        println(STDOUT, "\n\n PROGRAM INCOMPLETE");
      }
      println(LIS, "\n\n PROGRAM INCOMPLETE");
      error_message();
      fatal_error = true;
      error_exit();
    }
  } else {
    error(24);
    NEXTCH();
    goto label1;
  }
  symbol_count++;
  if (EXECSYS[SY] == 1) {
    execution_count++;
  }
  if (SY == Symbol::UNIONSY) {
    error(154);
  }
  if (SY == Symbol::ENUMSY) {
    error(155);
  }
  if (SY == Symbol::BITCOMPSY || SY == Symbol::BITANDSY ||
      SY == Symbol::BITXORSY || SY == Symbol::BITINCLSY) {
    error(156);
  }
  if (SY == Symbol::QUEST) {
    error(157);
  }
}

void ENTERARRAY(Types TP, int L, int H) {
  if (abs(L) > XMAX || abs(H) > XMAX) {
    error(27);
    L = 0;
    H = 0;
  }
  if (atab_index == AMAX) {
    fatal(4);
  } else {
    atab_index++;
    ATAB[atab_index].INXTYP = TP;
    ATAB[atab_index].LOW = L;
    ATAB[atab_index].HIGH = H;
  }
}

void ENTERCHANNEL() {
  if (ctab_index == CMAX) {
    fatal(9);
  } else {
    ctab_index++;
  }
}
} // namespace Cstar
