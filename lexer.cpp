// #include <iostream>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <print>
#include <string>

#include "cs_compile.h"
#include "cs_errors.h"
#include "cs_global.h"

namespace cs {
using std::isdigit;
using std::print;
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
        INSRC = fopen(FILENAME.c_str(), "re");
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
    } else {
      error(6);
    }
  } else {
    error(6);
  }
}
void MAINNEXTCH() {
  if (CC == LL) {
    // end of the buffered line
    // read and buffer another line
    // if ((*SRC).eof())
    if (feof(SRC) != 0) {
      CH = static_cast<char>(28);
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
    if (console_list) {
      print(STDOUT, "{:5} ", LNUM);
    }
    print(LIS, "{:5} ", LNUM);
    LOCATION[LNUM] = line_count;
    BREAKALLOW[LNUM] = OKBREAK;
    LL = 0;
    CC = 0;
    while (!eoln(SRC)) {
      LL++;
      // CH = (*SRC).get();
      CH = static_cast<char>(fgetc(SRC));
      // std::cout << CH;
      if (console_list) {
        fputc(CH, STDOUT);
      }
      fputc(CH, LIS);
      if (CH == 9) {
        CH = ' ';
      }
      line.str().at(LL) = CH;
    }
    if (console_list) {
      fputc('\n', STDOUT);
    }
    // std::cout << std::endl;
    fputc('\n', LIS);
    LL++;
    // do_eoln(SRC);
    line.str().at(LL) = ' ';
  }
  CC++;
  CH = line.str().at(CC);
label37:;
}

void ALTNEXTCH() {
  if (CC2 == LL2) {
    // if ((*INSRC).eof())
    if (feof(INSRC) != 0) {
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
      CH = fgetc(INSRC);
      if (CH == '\x09') {
        CH = ' ';
      }
      line2.str().at(LL2) = CH;
    }
    LL2++;
    //(*INSRC).ignore();
    line2.str().at(LL2) = ' ';
  }
  CC2++;
  CH = line2.str().at(CC2);
}

void NEXTCH() {
  if (inCLUDEFLAG()) {
    ALTNEXTCH();
  } else {
    MAINNEXTCH();
  }
}

void ADJUSTSCALE(struct InsymbolLocal *nl) {
  int S = 0;
  float D = NAN;
  float T = NAN;
  if (nl->K + nl->E > EMAX) {
    error(21);
  } else if (nl->K + nl->E < EMIN) {
    RNUM = 0.0;
  } else {
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
    if (nl->E >= 0) {
      RNUM = RNUM * T;
    } else {
      RNUM = RNUM / T;
    }
  }
}

void READSCALE(struct InsymbolLocal *nl) {
  int S = 0;
  int SIGN = 0;
  NEXTCH();
  SIGN = 1;
  S = 0;
  if (CH == '+') {
    NEXTCH();
  } else if (CH == '-') {
    NEXTCH();
    SIGN = -1;
  }
  while (isdigit(CH) != 0) {
    S = 10 * S + (CH - '0');
    NEXTCH();
  }
  nl->E = S * SIGN + nl->E;
}

void FRACTION(struct InsymbolLocal *symbol_local) {
  symbol = Symbol::REALCON;
  RNUM = INUM;
  symbol_local->E = 0;
  while (isdigit(CH) != 0) {
    symbol_local->E -= 1;
    RNUM = RNUM * 10.0 + (CH - '0');
    NEXTCH();
  }
  if (CH == 'E') {
    READSCALE(symbol_local);
  }
  if (symbol_local->E != 0) {
    ADJUSTSCALE(symbol_local);
  }
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

static auto isoctal(char ltr) -> bool { return ltr >= '0' && ltr <= '7'; }

void ESCAPECHAR2() {
  NEXTCH();
  if (isoctal(CH)) {
    INUM = 0;
    while (isoctal(CH)) {
      INUM = INUM * 8 + (CH - '0');
      NEXTCH();
    }
    if (INUM < 128) {
      STAB[SX] = static_cast<char>(INUM);
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
  struct InsymbolLocal symbol_local = {0, 0, 0, 0, false};
label1:
  while (CH == ' ') {
    NEXTCH();
  }

  if ((isalpha(CH) != 0) || CH == '#') {
    symbol_local.K = -1;
    strcpy(ID.data(), "              ");
    do {
      if (symbol_local.K < ALNG - 1) {
        symbol_local.K++;
        if (islower(CH) != 0) {
          CH = toupper(CH);
        }
        ID[symbol_local.K] = CH;
      } else {
        // fprintf(STDOUT, "long symbol %s\n", ID);
      }
      NEXTCH();
    } while (isalnum(CH) || CH == '_');
    symbol_local.I = 1;
    symbol_local.J = NKW;
    do {
      symbol_local.K = (symbol_local.I + symbol_local.J) / 2;
      if (strcmp(ID.data(), cs::key[symbol_local.K]) <= 0) {
        symbol_local.J = symbol_local.K - 1;
      }
      if (strcmp(ID.data(), cs::key[symbol_local.K]) >= 0) {
        symbol_local.I = symbol_local.K + 1;
      }
    } while (symbol_local.I <= symbol_local.J);
    if (symbol_local.I - 1 > symbol_local.J) {
      symbol = ksy[symbol_local.K];
    } else {
      symbol = Symbol::IDENT;
    }
    if (mpi_mode && (non_mpi_set.test(static_cast<int>(symbol)))) {
      error(145);
    }
  } else if (isdigit(CH) != 0) {
    symbol_local.K = 0;
    INUM = 0;
    symbol_local.ISDEC = true;
    symbol = Symbol::INTCON;
    if (CH == '0') {
      symbol_local.ISDEC = false;
      NEXTCH();
      if (CH == 'X' || CH == 'x') {
        NEXTCH();
        while (isxdigit(CH) != 0) {
          if (symbol_local.K <= KMAX) {
            INUM = INUM * 16;
            if (isdigit(CH) != 0) {
              INUM += CH - '0';
            } else if (isupper(CH) != 0) {
              INUM += CH - 'A' + 10;
            } else if (islower(CH) != 0) {
              INUM += CH - 'a' + 10;
            }
          }
          symbol_local.K++;
          NEXTCH();
        }
      } else {
        while (isdigit(CH) != 0) {
          if (symbol_local.K <= KMAX) {
            INUM = INUM * 8 + CH - '0';
          }
          symbol_local.K++;
          NEXTCH();
        }
      }
    } else {
      do {
        if (symbol_local.K <= KMAX) {
          INUM = INUM * 10 + CH - '0';
        }
        symbol_local.K++;
        NEXTCH();
      } while (isdigit(CH) != 0);
    }
    if (symbol_local.K > KMAX || abs(INUM) > NMAX) {
      error(21);
      INUM = 0;
      symbol_local.K = 0;
    }
    if (symbol_local.ISDEC || INUM == 0) {
      if (CH == '.') {
        NEXTCH();
        if (CH == '.') {
          CH = ':';
        } else {
          FRACTION(&symbol_local);
        }
      } else if (CH == 'E') {
        symbol = Symbol::REALCON;
        RNUM = INUM;
        symbol_local.E = 0;
        READSCALE(&symbol_local);
        if (symbol_local.E != 0) {
          ADJUSTSCALE(&symbol_local);
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
      symbol = Symbol::ANDSY;
      NEXTCH();
    } else if (CH == ' ') {
      symbol = Symbol::BITANDSY;
    } else {
      symbol = Symbol::ADDRSY;
    }
  } else if (CH == '|') {
    NEXTCH();
    if (CH == '|') {
      symbol = Symbol::ORSY;
      NEXTCH();
    } else {
      symbol = Symbol::BITINCLSY;
    }
  } else if (CH == '>') {
    NEXTCH();
    if (CH == '=') {
      symbol = Symbol::GEQ;
      NEXTCH();
    } else if (CH == '>') {
      symbol = Symbol::INSTR;
      NEXTCH();
    } else if (CH == ';') {
      symbol = Symbol::GRSEMI;
      NEXTCH();
    } else {
      symbol = Symbol::GTR;
    }
  } else if (CH == '+') {
    NEXTCH();
    if (CH == '+') {
      symbol = Symbol::INCREMENT;
      NEXTCH();
    } else if (CH == '=') {
      symbol = Symbol::PLUSEQ;
      NEXTCH();
    } else {
      symbol = Symbol::PLUS;
    }
  } else if (CH == '-') {
    NEXTCH();
    if (CH == '-') {
      symbol = Symbol::DECREMENT;
      NEXTCH();
    } else if (CH == '>') {
      symbol = Symbol::RARROW;
      NEXTCH();
    } else if (CH == '=') {
      symbol = Symbol::MINUSEQ;
      NEXTCH();
    } else {
      symbol = Symbol::MINUS;
    }
  } else if (CH == '.') {
    NEXTCH();
    if (CH == '.') {
      symbol = Symbol::COLON;
      NEXTCH();
    } else if (isdigit(CH) != 0) {
      INUM = 0;
      symbol_local.K = 0;
      FRACTION(&symbol_local);
    } else {
      symbol = Symbol::PERIOD;
    }
  } else if (CH == '\'') {
    NEXTCH();
    if (CH == '\'') {
      error(38);
      symbol = Symbol::CHARCON;
      INUM = 0;
    } else {
      if (SX + 1 == SMAX) {
        fatal(3);
      }
      if (CH == '\\') {
        ESCAPECHAR2();
      } else {
        STAB[SX] = CH;
        symbol = Symbol::CHARCON;
        INUM = static_cast<unsigned char>(STAB[SX]);
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
    symbol_local.K = 0;
  label4:
    NEXTCH();
    if (CH == '"') {
      NEXTCH();
      if (CH != '"') {
        goto label5;
      }
    }
    if (SX + symbol_local.K == SMAX) {
      fatal(3);
    }
    if (CH == '\\') {
      NEXTCH();
      ESCAPECHAR();
    }
    STAB[SX + symbol_local.K] = CH;
    symbol_local.K++;
    if (CC == 1) {
      symbol_local.K = 0;
    } else {
      goto label4;
    }
  label5:
    if (symbol_local.K == 0) {
      error(38);
      symbol = Symbol::CHARCON;
      INUM = 0;
    } else {
      symbol = Symbol::STRNG;
      INUM = SX;
      SLENG = symbol_local.K;
      SX += symbol_local.K;
    }
  } else if (CH == '/') {
    NEXTCH();
    if (CH == '*') {
      NEXTCH();
      do {
        while (CH != '*' && CH != static_cast<char>(28)) {
          NEXTCH();
        }
        NEXTCH();
      } while (CH != '/' && CH != static_cast<char>(28));
      if (CH == static_cast<char>(28)) {
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
      symbol = Symbol::RDIVEQ;
      NEXTCH();
    } else {
      symbol = Symbol::RDIV;
    }
  } else if (CH == '?') {
    NEXTCH();
    if (CH == '?') {
      symbol = Symbol::DBLQUEST;
      NEXTCH();
    } else {
      symbol = Symbol::QUEST;
    }
  } else if (CH == '*') {
    NEXTCH();
    if (CH == '=') {
      symbol = Symbol::TIMESEQ;
      NEXTCH();
    } else {
      symbol = Symbol::TIMES;
    }
  } else if (CH == '%') {
    NEXTCH();
    if (CH == '=') {
      symbol = Symbol::IMODEQ;
      NEXTCH();
    } else {
      symbol = Symbol::IMOD;
    }
  } else if (CH == '(' || CH == ')' || CH == ':' || CH == ',' || CH == '[' ||
             CH == ']' || CH == ';' || CH == '@' || CH == '{' || CH == '}' ||
             CH == '~' || CH == '^') {
    symbol = SPS[CH];
    NEXTCH();
  } else if (CH == static_cast<char>(28)) {
    symbol = Symbol::EOFSY;
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
  if (execution_set.test(static_cast<int>(symbol))) {
    execution_count++;
  }
  if (symbol == Symbol::UNIONSY) {
    error(154);
  }
  if (symbol == Symbol::ENUMSY) {
    error(155);
  }
  if (symbol == Symbol::BITCOMPSY || symbol == Symbol::BITANDSY ||
      symbol == Symbol::BITXORSY || symbol == Symbol::BITINCLSY) {
    error(156);
  }
  if (symbol == Symbol::QUEST) {
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
