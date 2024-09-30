// #include <iostream>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <iostream>
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
  using std::cout;
  using std::cin;

  //    void fatal(int);
  //    void error(int);
  //    void errorMSG();
  //    void errorEXIT();
  //    extern bool fatalerror;

  extern void INTERPRET();
  extern void dumpInst(int);

  // extern bool eoln(FILE *);
  //  double RNUM; // source local ?
  //  int STRING_LENGTH;  // source local ?
  //  int eof_count = 0;  // source local ?

  void next_char();
  fs::path file_name;

  struct IncludeStack {
    std::ifstream inc;
    fs::path file_name;
  };

  struct InSymbolLocal {
    int I, J, K, E;
    bool ISDEC;
  };

  static constexpr int istack_max = 10;
  static std::array<IncludeStack, istack_max> include_stack;
  static int istack_index = -1;

  auto HasIncludeFlag() -> bool { return istack_index >= 0; }

  static int look_ahead = -2;
  static bool console_list = false;
  static bool code_list = false;

  void showConsoleList(bool flag) { console_list = flag; }

  static void showCodeList(bool flag) { code_list = flag; }

  auto is_eoln(FILE* inf) -> bool {
    return look_ahead == '\r' || look_ahead == '\n';
  }

  void do_eoln(FILE* inf) {
    if (look_ahead == '\r')
      look_ahead = fgetc(inf);
    if (look_ahead == '\n')
      look_ahead = fgetc(inf);
  }

  static auto is_eof(FILE* inf) -> bool {
    if (look_ahead == -2)
      look_ahead = fgetc(inf);

    return look_ahead < 0;
  }

  static auto readc(FILE* inf) -> char {
    char ltr = 0;
    ltr = static_cast<char>(look_ahead);
    look_ahead = fgetc(inf);
    return ltr;
  }

  static auto eoln(FILE* inf) -> bool {
    int ltr = 0;
    bool eln = false;
    // //        if ((*SOURCE).eof())
    // //            return true;
    //        ch = (*SOURCE).peek();
    ltr = fgetc(inf);
    if (ltr >= 0) {
      if (ltr == '\r') {
        // (*SOURCE).ignore();
        ltr = fgetc(inf);
        eln = true;
      }
      if (ltr == '\n') {
        // (*SOURCE).ignore(2);
        ltr = fgetc(inf);
        eln = true;
      }
      if (ltr >= 0)
        auto unget = ungetc(ltr, inf);
      else
        eln = true;
    } else
      eln = true;
    return eln;
  }

  void emit(const int FCT, const int lA, const int lB) {
    if (line_count == CMAX) fatal(6);
    else {
      auto [F, X, Y] = CODE.at(line_count);
      F = FCT;
      X = lA;
      Y = lB;
      if (code_list) dumpInst(line_count);
      line_count++;
    }
  }

  void include_directive() {
    // int CHVAL;
    int len = 0;
    in_symbol();
    if (symbol == Symbol::LSS) {
      file_name = "";
      len = 0;
      // while (((CH >= 'a' && CH <= 'z') || (CH >= 'A' && CH <= 'Z') || (CH >=
      // '0' && CH <= '9') ||
      //     (CH == '\\' || CH == '.' || CH == '_' || CH == ':')) && (LENG <=
      //     FILMAX))
      while ((std::isalnum(CH) != 0 || CH == '/' || CH == '\\' || CH == '.' ||
              CH == '_' || CH == ':') &&
             len <= FILMAX) {
        // if (CH >= 'a' && CH <= 'z') CH = CH - 32;  // DE
        file_name += CH;
        len++;
        next_char();
      }
      in_symbol();
      if (symbol == Symbol::GTR || symbol == Symbol::GRSEMI) {
        // #I-
        // needs fixup
        // ASSIGN(INPUT_SOURCE, file_name.c_str());
        // RESET(INPUT_SOURCE);
        // #I-
        if (++istack_index == istack_max)
          error(149);
        //                if (INPUT_SOURCE != nullptr) // || IOResult() != 0)
        //                    error(149);
        else {
          INPUT_SOURCE = std::ifstream(file_name.c_str());
          if (!INPUT_SOURCE)
            error(150);
          else {
            include_stack.at(istack_index).file_name = file_name;
            include_stack.at(istack_index).inc.swap(INPUT_SOURCE);
            INCLUDE_FLAG = true;
            if (istack_index == 0) {
              save_symbol_count = symbol_count;
              save_execution_count = execution_count;
              // SAVELC = LC;  // DE
            }
            in_symbol();
          }
        }
      } else error(6);
    } else error(6);
  }

  void main_next_char() {
    if (CC == LL) {
      // end of the buffered line
      // read and buffer another line
      // if ((*SOURCE).eof())
      if (SOURCE.eof()) {
        CH = static_cast<char>(28);
        return;
      }

      if (error_position != 0) {
        if (console_list) std::cout << '\n';
        LISTFILE.put('\n');
        error_position = 0;
      }

      if (LINE_NUMBER > 0)
        if (LOCATION[LINE_NUMBER] == line_count && execution_count > 0) emit(6);

      LINE_NUMBER++;
      symbol_count = 0;
      execution_count = 0;
      if (console_list) print("{:5} ", LINE_NUMBER);

      print(LISTFILE, "{:5} ", LINE_NUMBER);
      LOCATION.at(LINE_NUMBER) = line_count;
      BREAK_ALLOW.at(LINE_NUMBER) = OK_BREAK;
      LL = 0;
      CC = 0;

      while (SOURCE.peek() != '\n' && !SOURCE.eof()) {
        LL++;
        CH = static_cast<char>(SOURCE.get());
        if (console_list) print("{}", CH);
        LISTFILE.put(CH);
        if (CH == 9) CH = ' ';
        line.at(LL) = CH;
      }

      if (console_list) print("\n");
      LISTFILE.put('\n');
      LL++;
      line.at(LL) = ' ';
    }

    CC++;
    CH = line.at(CC);
  }

  auto alt_next_char() -> void {
    if (CC2 == LL2) {
      if (INPUT_SOURCE.eof()) {
        INPUT_SOURCE.close();

        if (--istack_index < 0) {
          INCLUDE_FLAG = false;
          INPUT_SOURCE.close();
          execution_count = save_execution_count;
          symbol_count = save_symbol_count;
          // line_count = SAVELC;  // DE
          main_next_char();
          return;
        }

        INPUT_SOURCE.swap(include_stack.at(istack_index).inc);
        file_name = include_stack.at(istack_index).file_name;
      }
      LL2 = 0;
      CC2 = 0;
      while (INPUT_SOURCE.peek() != '\n' && !INPUT_SOURCE.eof()) {
        LL2++;
        CH = INPUT_SOURCE.get();
        if (CH == '\x09')
          CH = ' ';
        line2.at(LL2) = CH;
      }
      LL2++;
      //(*INPUT_SOURCE).ignore();
      line2.at(LL2) = ' ';
    }
    CC2++;
    CH = line2.at(CC2);
  }

  void next_char() {
    if (HasIncludeFlag()) alt_next_char();
    else main_next_char();
  }

  void adjust_skill(InSymbolLocal* sym) {
    int S = 0;
    float D = NAN;
    float T = NAN;
    if (sym->K + sym->E > EMAX) error(21);
    else if (sym->K + sym->E < EMIN) RNUM = 0.0;
    else {
      S = abs(sym->E);
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
      RNUM = sym->E >= 0 ? RNUM * T : RNUM / T;
    }
  }

  void read_scale(InSymbolLocal* sym) {
    int S = 0;
    int sign = 0;
    next_char();
    sign = 1;
    S = 0;
    if (CH == '+') next_char();
    else if (CH == '-') {
      next_char();
      sign = -1;
    }

    while (isdigit(CH) != 0) {
      S = 10 * S + (CH - '0');
      next_char();
    }
    sym->E = S * sign + sym->E;
  }

  void fraction(InSymbolLocal* sym) {
    symbol = Symbol::REALCON;
    RNUM = INUM;
    sym->E = 0;
    while (isdigit(CH) != 0) {
      sym->E -= 1;
      RNUM = RNUM * 10.0 + (CH - '0');
      next_char();
    }
    if (CH == 'E') read_scale(sym);
    if (sym->E != 0) adjust_skill(sym);
  }

  void escape_char() {
    switch (CH) {
      case 'a': CH = 7;
        break;
      case 'b': CH = 8;
        break;
      case 'f': CH = 12;
        break;
      case 'n': CH = 10;
        break;
      case 'r': CH = 13;
        break;
      case 't': CH = 9;
        break;
      case 'v': CH = 11;
        break;
      case '\\':
      case '?':
      case '\'':
      case '"': break;
      default: error(132);
        break;
    }
  }

  static auto is_octal(const char ltr) -> bool {
    return ltr >= '0' && ltr <= '7';
  }

  void escape_char2() {
    next_char();
    if (is_octal(CH)) {
      INUM = 0;
      while (is_octal(CH)) {
        INUM = INUM * 8 + (CH - '0');
        next_char();
      }
      if (INUM < 128) {
        STAB[SX] = static_cast<char>(INUM);
        SX += 1;
        if (CH == '\'') next_char();
        else error(130);
      } else {
        error(24);
        if (CH == '\'') next_char();
      }
    } else {
      escape_char();
      STAB[SX] = CH;
      INUM = CH;
      SX += 1;
      next_char();
      if (CH == '\'') next_char();
      else error(130);
    }
    symbol = Symbol::CHARCON;
  }

  void in_symbol() {
    // int I, J, K, E;
    // bool ISDEC;
    InSymbolLocal sym = {0, 0, 0, 0, false};
  label1:
    while (CH == ' ') next_char();

    if (isalpha(CH) != 0 || CH == '#') {
      sym.K = -1;
      strcpy(ID.data(), "              ");
      do {
        if (sym.K < ALNG - 1) {
          sym.K++;
          if (islower(CH) != 0) CH = toupper(CH);
          ID[sym.K] = CH;
        } else {
          // fprintf(STANDARD_OUTPUT, "long symbol %s\n", ID);
        }
        next_char();
      } while (isalnum(CH) || CH == '_');
      sym.I = 1;
      sym.J = NKW;
      do {
        sym.K = (sym.I + sym.J) / 2;
        if (strcmp(ID.data(), key[sym.K]) <= 0)
          sym.J = sym.K - 1;
        if (strcmp(ID.data(), key[sym.K]) >= 0)
          sym.I = sym.K + 1;
      } while (sym.I <= sym.J);
      if (sym.I - 1 > sym.J) symbol = ksy[sym.K];
      else symbol = Symbol::IDENT;
      if (mpi_mode && non_mpi_set.test(static_cast<int>(symbol))) error(145);
    } else if (isdigit(CH) != 0) {
      sym.K = 0;
      INUM = 0;
      sym.ISDEC = true;
      symbol = Symbol::INTCON;
      if (CH == '0') {
        sym.ISDEC = false;
        next_char();
        if (CH == 'X' || CH == 'x') {
          next_char();
          while (isxdigit(CH) != 0) {
            if (sym.K <= KMAX) {
              INUM = INUM * 16;
              if (isdigit(CH) != 0)
                INUM += CH - '0';
              else if (isupper(CH) != 0)
                INUM += CH - 'A' + 10;
              else if (islower(CH) != 0)
                INUM += CH - 'a' + 10;
            }
            sym.K++;
            next_char();
          }
        } else {
          while (isdigit(CH) != 0) {
            if (sym.K <= KMAX) INUM = INUM * 8 + CH - '0';
            sym.K++;
            next_char();
          }
        }
      } else {
        do {
          if (sym.K <= KMAX) INUM = INUM * 10 + CH - '0';
          sym.K++;
          next_char();
        } while (isdigit(CH) != 0);
      }
      if (sym.K > KMAX || abs(INUM) > NMAX) {
        error(21);
        INUM = 0;
        sym.K = 0;
      }
      if (sym.ISDEC || INUM == 0) {
        if (CH == '.') {
          next_char();
          if (CH == '.') CH = ':';
          else fraction(&sym);
        } else if (CH == 'E') {
          symbol = Symbol::REALCON;
          RNUM = INUM;
          sym.E = 0;
          read_scale(&sym);
          if (sym.E != 0) adjust_skill(&sym);
        }
      }
    } else if (CH == '<') {
      next_char();
      if (CH == '=') {
        symbol = Symbol::LEQ;
        next_char();
      } else if (CH == '>') {
        symbol = Symbol::NEQ;
        next_char();
      } else if (CH == '<') {
        symbol = Symbol::OUTSTR;
        next_char();
      } else
        symbol = Symbol::LSS;
    } else if (CH == '=') {
      next_char();
      if (CH == '=') {
        symbol = Symbol::EQL;
        next_char();
      } else
        symbol = Symbol::BECOMES;
    } else if (CH == '!') {
      next_char();
      if (CH == '=') {
        symbol = Symbol::NEQ;
        next_char();
      } else
        symbol = Symbol::NOTSY;
    } else if (CH == '&') {
      next_char();
      if (CH == '&') {
        symbol = Symbol::ANDSY;
        next_char();
      } else if (CH == ' ')
        symbol = Symbol::BITANDSY;
      else
        symbol = Symbol::ADDRSY;
    } else if (CH == '|') {
      next_char();
      if (CH == '|') {
        symbol = Symbol::ORSY;
        next_char();
      } else
        symbol = Symbol::BITINCLSY;
    } else if (CH == '>') {
      next_char();
      if (CH == '=') {
        symbol = Symbol::GEQ;
        next_char();
      } else if (CH == '>') {
        symbol = Symbol::INSTR;
        next_char();
      } else if (CH == ';') {
        symbol = Symbol::GRSEMI;
        next_char();
      } else
        symbol = Symbol::GTR;
    } else if (CH == '+') {
      next_char();
      if (CH == '+') {
        symbol = Symbol::INCREMENT;
        next_char();
      } else if (CH == '=') {
        symbol = Symbol::PLUSEQ;
        next_char();
      } else
        symbol = Symbol::PLUS;
    } else if (CH == '-') {
      next_char();
      if (CH == '-') {
        symbol = Symbol::DECREMENT;
        next_char();
      } else if (CH == '>') {
        symbol = Symbol::RARROW;
        next_char();
      } else if (CH == '=') {
        symbol = Symbol::MINUSEQ;
        next_char();
      } else
        symbol = Symbol::MINUS;
    } else if (CH == '.') {
      next_char();
      if (CH == '.') {
        symbol = Symbol::COLON;
        next_char();
      } else if (isdigit(CH) != 0) {
        INUM = 0;
        sym.K = 0;
        fraction(&sym);
      } else
        symbol = Symbol::PERIOD;
    } else if (CH == '\'') {
      next_char();
      if (CH == '\'') {
        error(38);
        symbol = Symbol::CHARCON;
        INUM = 0;
      } else {
        if (SX + 1 == SMAX)
          fatal(3);
        if (CH == '\\')
          escape_char2();
        else {
          STAB[SX] = CH;
          symbol = Symbol::CHARCON;
          INUM = static_cast<unsigned char>(STAB[SX]);
          SX++;
          next_char();
          if (CH == '\'') next_char();
          else error(130);
        }
      }
    } else if (CH == '"') {
      sym.K = 0;
    label4:
      next_char();
      if (CH == '"') {
        next_char();
        if (CH != '"')
          goto label5;
      }
      if (SX + sym.K == SMAX) fatal(3);
      if (CH == '\\') {
        next_char();
        escape_char();
      }
      STAB[SX + sym.K] = CH;
      sym.K++;
      if (CC == 1) sym.K = 0;
      else goto label4;
    label5:
      if (sym.K == 0) {
        error(38);
        symbol = Symbol::CHARCON;
        INUM = 0;
      } else {
        symbol = Symbol::STRNG;
        INUM = SX;
        STRING_LENGTH = sym.K;
        SX += sym.K;
      }
    } else if (CH == '/') {
      next_char();
      if (CH == '*') {
        next_char();
        do {
          while (CH != '*' && CH != static_cast<char>(28))
            next_char();
          next_char();
        } while (CH != '/' && CH != static_cast<char>(28));
        if (CH == static_cast<char>(28)) {
          //                    std::cout << std::endl;
          //                    std::cout << std::endl;
          //                    std::cout << " FAILURE TO END A COMMENT" <<
          //                    std::endl;
          if (console_list) println("\n\n FAILURE TO END A COMMENT");
          println(LISTFILE, "\n\n FAILURE TO END A COMMENT");
          error_message();
          fatal_error = true;
          error_exit();
        }
        next_char();
        goto label1;
      }

      if (CH == '=') {
        symbol = Symbol::RDIVEQ;
        next_char();
      } else symbol = Symbol::RDIV;
    } else if (CH == '?') {
      next_char();
      if (CH == '?') {
        symbol = Symbol::DBLQUEST;
        next_char();
      } else symbol = Symbol::QUEST;
    } else if (CH == '*') {
      next_char();
      if (CH == '=') {
        symbol = Symbol::TIMESEQ;
        next_char();
      } else
        symbol = Symbol::TIMES;
    } else if (CH == '%') {
      next_char();
      if (CH == '=') {
        symbol = Symbol::IMODEQ;
        next_char();
      } else symbol = Symbol::IMOD;
    } else if (CH == '(' || CH == ')' || CH == ':' || CH == ',' || CH == '[' ||
               CH == ']' || CH == ';' || CH == '@' || CH == '{' || CH == '}' ||
               CH == '~' || CH == '^') {
      symbol = SPS[CH];
      next_char();
    } else if (CH == static_cast<char>(28)) {
      symbol = Symbol::EOFSY;
      eof_count++;
      if (eof_count > 5) {
        //                std::cout << std::endl;
        //                std::cout << std::endl;
        //                std::cout << " PROGRAM INCOMPLETE" << std::endl;
        if (console_list) println("\n\n PROGRAM INCOMPLETE");
        println(LISTFILE, "\n\n PROGRAM INCOMPLETE");
        error_message();
        fatal_error = true;
        error_exit();
      }
    } else {
      error(24);
      next_char();
      goto label1;
    }
    symbol_count++;
    if (execution_set.test(static_cast<int>(symbol))) execution_count++;
    if (symbol == Symbol::UNIONSY) error(154);
    if (symbol == Symbol::ENUMSY) error(155);
    if (symbol == Symbol::BITCOMPSY || symbol == Symbol::BITANDSY ||
        symbol == Symbol::BITXORSY || symbol == Symbol::BITINCLSY)
      error(156);
    if (symbol == Symbol::QUEST) error(157);
  }

  void enter_array(Types t, int l, int h) {
    if (abs(l) > XMAX || abs(h) > XMAX) {
      error(27);
      l = 0;
      h = 0;
    }
    if (atab_index == AMAX) fatal(4);
    else {
      atab_index++;
      ATAB.at(atab_index).INXTYP = t;
      ATAB.at(atab_index).LOW = l;
      ATAB.at(atab_index).HIGH = h;
    }
  }

  void enter_channel() {
    if (ctab_index == CMAX) fatal(9);
    else ctab_index++;
  }
} // namespace Cstar
