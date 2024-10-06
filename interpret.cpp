// #include <iostream>
#include "interpret.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <print>
#include <string>

#include "cs_PreBuffer.h"
#include "cs_compile.h"
#include "cs_errors.h"
#include "cs_global.h"
#include "cs_interpret.h"

#include "displays.hpp"

namespace cs {
using std::cout;
using std::getc;
using std::print;
using std::println;

extern void EXECUTE(Interpreter *);

Commander::Commander()
    : displays(std::make_unique<Displays>()),
      commands(
        {"RUN           ", "CONTINUE      ", "STEP          ", "EXIT          ",
         "BREAK         ", "TRACE         ", "LIST          ", "CLEAR         ",
         "WRITE         ", "DISPLAY       ", "HELP          ", "PROFILE       ",
         "ALARM         ", "CODE          ", "STACK         ", "STATUS        ",
         "TIME          ", "UTILIZATION   ", "DELAY         ", "CONGESTION    ",
         "VARIATION     ", "OPEN          ", "CLOSE         ", "INPUT         ",
         "OUTPUT        ", "VIEW          ", "SHORTCUTS     ", "RESET         ",
         "MPI           ", "VERSION       "}),
      abbreviations(
        {"R             ", "C             ", "S             ", "E             ",
         "B             ", "T             ", "NONE          ", "NONE          ",
         "W             ", "D             ", "H             ", "P             ",
         "A             ", "NONE          ", "NONE          ", "NONE          ",
         "NONE          ", "U             ", "NONE          ", "NONE          ",
         "NONE          ", "O             ", "NONE          ", "NONE          ",
         "NONE          ", "V             ", "NONE          ", "NONE          ",
         "NONE          ", "NONE          "}),
      command_jump(
        {COMTYP::RUNP,    COMTYP::CONT,     COMTYP::STEP,   COMTYP::EXIT2,
         COMTYP::BREAKP,  COMTYP::TRACE,    COMTYP::LIST,   COMTYP::CLEAR,
         COMTYP::WRVAR,   COMTYP::DISPLAYP, COMTYP::HELP,   COMTYP::PROFILE,
         COMTYP::ALARM,   COMTYP::WRCODE,   COMTYP::STACK,  COMTYP::PSTATUS,
         COMTYP::PTIME,   COMTYP::UTIL,     COMTYP::CDELAY, COMTYP::CONGEST,
         COMTYP::VARY,    COMTYP::OPENF,    COMTYP::CLOSEF, COMTYP::INPUTF,
         COMTYP::OUTPUTF, COMTYP::VIEW,     COMTYP::SHORT,  COMTYP::RESETP,
         COMTYP::MPI,     COMTYP::VERSION}),
      pre_buf(std::make_unique<PreBuffer>(PreBuffer())) {}

void Commander::show_code_list(const bool flag) { code_list = flag; }
void Commander::show_block_list(const bool flag) { block_list = flag; }
void Commander::show_symbol_list(const bool flag) { symbol_list = flag; }
void Commander::show_array_list(const bool flag) { array_list = flag; }
void Commander::show_real_list(const bool flag) { real_list = flag; }
// void Commander::showConsoleList(const bool flag) { console_list = flag; }

auto Commander::handle_comma() -> void {
  next_char();
  interp->INX = num();
  if (interp->INX == -1 || interp->TYP != Types::ARRAYS) {
    std::cout << "Error in Array Index" << '\n';
    interp->ERR = true;
    return;
  }
  if (ATAB[interp->REF].INXTYP != Types::INTS) {
    std::cout << "Only Integer Indices Allowed" << '\n';
    interp->ERR = true;
    return;
  }
  const int j = ATAB[interp->REF].LOW;
  if (interp->INX < j) {
    std::cout << "Index Too Low" << '\n';
    interp->ERR = true;
    return;
  }
  interp->ADR = interp->ADR + (interp->INX - j) * ATAB[interp->REF].ELSIZE;
  interp->TYP = ATAB[interp->REF].ELTYP;
  interp->REF = ATAB[interp->REF].ELREF;
  remove_block();
}

auto Commander::qualify() -> void {
  while (CH == '[' || CH == '-' || CH == '.') {
    remove_block();
    if (CH == '[') {
      if (interp->TYP != Types::ARRAYS) {
        std::cout << "INDEX OF NONARRAY TYPE" << '\n';
        interp->ERR = true;
        continue;
      }
      while (CH == ',') {
        handle_comma();
        return qualify();
      }
      if (CH == ']')
        next_char();
    } else if (CH == '.') {
      next_char();
      if (interp->TYP != Types::RECS || !in_word()) {
        std::cout << "Error in Record Component" << '\n';
        interp->ERR = true;
        continue;
      }
      atab_index = interp->REF;
      strcpy(TAB[0].name.data(), ID.data());
      while (TAB[atab_index].name != ID)
        atab_index = TAB[atab_index].link;

      if (atab_index == 0) {
        std::cout << "Struct Component Not Found" << '\n';
        interp->ERR = true;
        continue;
      }

      interp->ADR = interp->ADR + TAB[atab_index].address;
      interp->TYP = TAB[atab_index].types;
      interp->REF = TAB[atab_index].reference;
    } else if (CH == '-') {
      next_char();
      if (CH != '>') {
        std::cout << "Invalid Character in Command" << '\n';
        interp->ERR = true;
      } else if (interp->TYP != Types::PNTS) {
        std::cout << "Nonpointer Variable Cannot Have ->" << '\n';
        interp->ERR = true;
      } else if (interp->S[interp->ADR] == 0) {
        std::cout << "Pointer is NULL" << '\n';
        interp->ERR = true;
      } else {
        interp->TYP = CTAB[interp->REF].ELTYP;
        interp->REF = CTAB[interp->REF].ELREF;
        interp->ADR = interp->S[interp->ADR];
        CH = '.';
      }
    }
  }
  std::cout << '\n';
}

auto Commander::increment_line() -> bool {
  bool rtn = false;
  interp->LINECNT = interp->LINECNT + 1;
  if (interp->LINECNT > 200) {
    cout << '\n';
    cout << "  Do you want to see more MPI Buffer Data? (Y or N): ";
    char input = 0;
    input = static_cast<char>(getc(stdin));
    if (in_word()) {
      if (input == 'Y')
        interp->LINECNT = 0;
      else
        rtn = true;
    } else
      rtn = true;
  }
  return rtn;
}

void Commander::array_write(Partwrite *pl) {
  int64_t stadr = pl->padr;
  std::cout << "ARRAY" << '\n';
  interp->LINECNT = interp->LINECNT + 1;
  for (int i = ATAB[pl->pref].LOW; i <= ATAB[pl->pref].HIGH; i++) {
    if (!pl->done) {
      for (int j = 1; j <= pl->start - 1; j++)
        std::cout << " ";
      std::cout << i << "-->";
      partwrite(
        pl->start + 9,
        ATAB[pl->pref].ELTYP,
        ATAB[pl->pref].ELREF,
        stadr,
        pl->done);
      stadr = stadr + ATAB[pl->pref].ELSIZE;
    }
  }
}

void Commander::record_write(Partwrite *pl) {
  std::vector<int> save_ref{};
  int num_ref = 2;
  int j = pl->pref;
  save_ref[1] = pl->pref;
  while (TAB[j].link != 0) {
    j = TAB[j].link;
    save_ref.at(num_ref) = j;
    num_ref = num_ref + 1;
    if (num_ref > 20) {
      std::println("Struct Has Too Many Components\n");
      return;
    }
  }
  int64_t stadr = pl->padr;
  std::println("  STRUCT");
  interp->LINECNT = interp->LINECNT + 1;
  for (int i = num_ref - 1; i >= 1; i--) {
    for (j = 1; j <= pl->start - 5; j++)
      std::print(" ");
    std::string tname = TAB[save_ref.at(i)].name;
    for (int k = 0; k <= ALNG - 1; k++) {
      if (tname[ALNG] == ' ') {
        for (int l = ALNG; l >= 2; l--)
          tname[l] = tname[l - 1];
        tname[1] = ' ';
      }
    }
    std::cout << tname << ": ";
    partwrite(pl->start + 12, interp->TYP, interp->REF, stadr, pl->done);
    if (pl->done)
      break;
    stadr = stadr + TAB[save_ref.at(i)].size;
  }
  std::cout << '\n';
}

auto Commander::channel_write(Partwrite *pl) -> void {
  const int cnum = interp->S[pl->padr];
  if (bool empty = cnum == 0) {
    std::cout << "STREAM IS EMPTY" << '\n';
    pl->done = increment_line();
  } else {
    if (!empty)
      empty = interp->CHAN.at(cnum).SEM == 0;
    if (empty) {
      std::cout << "STREAM IS EMPTY" << '\n';
      pl->done = increment_line();
    } else {
      int tsadr = 0;
      int pnt = interp->CHAN.at(cnum).HEAD;
      std::cout << "STREAM" << '\n';
      interp->LINECNT = interp->LINECNT + 1;
      while (pnt != 0 && !pl->done) {
        for (int I = 1; I <= pl->start - 1; I++)
          std::cout << " ";
        if (
          interp->DATE[pnt] <= interp->CLOCK + TIMESTEP ||
          topology == Symbol::SHAREDSY)
          std::cout << "     >";
        else
          std::cout << "   **>";
        switch (ATAB[pl->pref].ELTYP) {
          case Types::RECS:
          case Types::ARRAYS: {
            partwrite(
              pl->start + 7,
              ATAB[pl->pref].ELTYP,
              ATAB[pl->pref].ELREF,
              interp->VALUE[pnt],
              pl->done);
            break;
          }
          case Types::REALS: {
            std::cout << interp->RVALUE[pnt] << '\n';
            break;
          }
          case Types::INTS: {
            std::cout << interp->VALUE[pnt] << '\n';
            break;
          }
          case Types::BOOLS: {
            std::println("{}", interp->VALUE[pnt] == 0 ? "FALSE" : "TRUE");
            break;
          }
          case Types::CHARS: {
            std::cout << interp->VALUE[pnt] << '\n';
            break;
          }
          case Types::PNTS:
            tsadr = find_frame(1);
            interp->S[tsadr] = interp->VALUE[pnt];
            partwrite(
              pl->start + 7,
              ATAB[pl->pref].ELTYP,
              ATAB[pl->pref].ELREF,
              tsadr,
              pl->done);
            break;
          default: break;
        }
        pnt = interp->LINK[pnt];
        if (const Types typs = ATAB[pl->pref].ELTYP;
            typs == Types::REALS || typs == Types::INTS ||
            typs == Types::BOOLS || typs == Types::CHARS)
          pl->done = increment_line();
      }
    }
  }
}

void Commander::scalar_write(Partwrite *pl) {
  switch (pl->ptyp) {
    case Types::INTS: {
      std::cout << interp->S[pl->padr] << '\n';
      break;
    }
    case Types::REALS: {
      if (interp->S[pl->padr] == 0)
        std::cout << 0 << '\n';
      else
        std::cout << interp->RS[pl->padr] << '\n';
      break;
    }
    case Types::BOOLS: {
      if (interp->S[pl->padr] == 0)
        std::cout << "FALSE" << '\n';
      else
        std::cout << "TRUE" << '\n';
      break;
    }
    case Types::CHARS:
      std::cout << static_cast<char>(interp->S[pl->padr]) << '\n';
      break;
    case Types::LOCKS: {
      if (interp->S[pl->padr] == 0)
        std::cout << "UNLOCKED" << '\n';
      else
        std::cout << "LOCKED" << '\n';
      break;
    }
    default: break;
  }
  pl->done = increment_line();
}

auto Commander::ptr_write(Partwrite *pl) -> void {
  if (interp->S[pl->padr] == 0)
    std::cout << "NULL POINTER" << '\n';
  else {
    std::cout << "POINTER" << '\n';
    interp->LINECNT = interp->LINECNT + 1;
    for (int I = 1; I <= pl->start; I++)
      std::cout << " ";
    std::cout << "  TO   ";
    partwrite(
      pl->start + 8,
      CTAB[pl->pref].ELTYP,
      CTAB[pl->pref].ELREF,
      interp->S[pl->padr],
      pl->done);
  }
}

auto Commander::partwrite(
  int start, Types ptyp, Index pref, int64_t padr, bool &done) -> void {
  Partwrite pl{};
  pl.start = start;
  pl.ptyp = ptyp;
  pl.padr = padr;
  pl.pref = pref;
  pl.done = done;

  if (!done) {
    if (start > 70) {
      print("\nData Has Too Many Levels of Structure\n");
      return;
    }
    switch (ptyp) {
      case Types::ARRAYS: {
        array_write(&pl);
        break;
      }
      case Types::RECS: {
        record_write(&pl);
        break;
      }
      case Types::CHANS: {
        channel_write(&pl);
        break;
      }
      case Types::PNTS: {
        ptr_write(&pl);
        break;
      }
      case Types::INTS:
      case Types::REALS:
      case Types::BOOLS:
      case Types::CHARS:
      case Types::LOCKS: {
        scalar_write(&pl);
        break;
      }
      default: break;
    }
  }
}

auto Commander::wrmpibuf(int pnum) -> void {
  if (interp->CHAN.at(pnum).SEM == 0)
    std::cout << "MPI BUFFER IS EMPTY" << '\n';
  else {
    int pnt = interp->CHAN.at(pnum).HEAD;
    interp->LINECNT = 0;
    while (pnt != 0) {
      std::cout << '\n';
      if (increment_line())
        goto L49;
      if (interp->DATE[pnt] <= interp->CLOCK + TIMESTEP)
        std::cout << "   >";
      else
        std::cout << " **>";
      const int BADR = interp->VALUE[pnt];
      const int count = interp->S[BADR];
      const int data_type = interp->S[BADR + 1];
      std::cout << "  Source: " << interp->S[BADR + 2]
                << "  Tag: " << interp->S[BADR + 3] << "  Datatype: ";
      switch (data_type) {
        case 1: std::cout << "int"; break;
        case 2: std::cout << "float"; break;
        case 3: std::cout << "char"; break;
        default: break;
      }
      if (increment_line())
        goto L49;
      if (increment_line())
        goto L49;
      std::cout << "         DATA: ";
      if (count == 1) {
        switch (data_type) {
          case 1: {
            std::cout << interp->S[BADR + 4] << '\n';
            break;
          }
          case 2: {
            std::cout << interp->RS[BADR + 4] << '\n';
            break;
          }
          case 3: {
            std::cout << static_cast<char>(interp->S[BADR + 4]) << '\n';
            break;
          }
          default: break;
        }
      } else {
        std::cout << '\n';
        if (increment_line())
          goto L49;
        for (int i = BADR + 4; i <= BADR + 3 + count; i++) {
          std::cout << "             " << i - BADR - 3 << "--> ";
          switch (data_type) {
            case 1: {
              std::cout << interp->S[i] << '\n';
              break;
            }
            case 2: {
              std::cout << interp->RS[i] << '\n';
              break;
            }
            case 3: {
              std::cout << static_cast<char>(interp->S[i]) << '\n';
              break;
            }
            default: break;
          }
          if (increment_line())
            goto L49;
        }
      }
      pnt = interp->LINK[pnt];
    }
  }
L49:;
}

auto init_commands() -> void {
  //        strcpy(COMTAB[0], "RUN           ");
  //        strcpy(ABBREVTAB[0], "R             ");
  //        COMJMP[0] = RUNP;
  //        strcpy(COMTAB[1], "CONTINUE      ");
  //        strcpy(ABBREVTAB[1], "C             ");
  //        COMJMP[1] = CONT;
  //        strcpy(COMTAB[2], "STEP          ");
  //        strcpy(ABBREVTAB[2], "S             ");
  //        COMJMP[2] = STEP;
  //        strcpy(COMTAB[3], "EXIT          ");
  //        strcpy(ABBREVTAB[3], "E             ");
  //        COMJMP[3] = EXIT2;
  //        strcpy(COMTAB[4], "BREAK         ");
  //        strcpy(ABBREVTAB[4], "B             ");
  //        COMJMP[4] = BREAKP;
  //        strcpy(COMTAB[5], "TRACE         ");
  //        strcpy(ABBREVTAB[5], "T             ");
  //        COMJMP[5] = TRACE;
  //        strcpy(COMTAB[6], "LIST          ");
  //        strcpy(ABBREVTAB[6], "NONE          ");
  //        COMJMP[6] = LIST;
  //        strcpy(COMTAB[7], "CLEAR         ");
  //        strcpy(ABBREVTAB[7], "NONE          ");
  //        COMJMP[7] = CLEAR;
  //        strcpy(COMTAB[8], "WRITE         ");
  //        strcpy(ABBREVTAB[8], "W             ");
  //        COMJMP[8] = WRVAR;
  //        strcpy(COMTAB[9], "DISPLAY       ");
  //        strcpy(ABBREVTAB[9], "D             ");
  //        COMJMP[9] = DISPLAY;
  //        strcpy(COMTAB[10], "HELP          ");
  //        strcpy(ABBREVTAB[10], "H             ");
  //        COMJMP[10] = HELP;
  //        strcpy(COMTAB[11], "PROFILE       ");
  //        strcpy(ABBREVTAB[11], "P             ");
  //        COMJMP[11] = PROFILE;
  //        strcpy(COMTAB[12], "ALARM         ");
  //        strcpy(ABBREVTAB[12], "A             ");
  //        COMJMP[12] = ALARM;
  //        strcpy(COMTAB[13], "CODE          ");
  //        strcpy(ABBREVTAB[13], "NONE          ");
  //        COMJMP[13] = WRCODE;
  //        strcpy(COMTAB[14], "STACK         ");
  //        strcpy(ABBREVTAB[14], "NONE          ");
  //        COMJMP[14] = STACK;
  //        strcpy(COMTAB[15], "STATUS        ");
  //        strcpy(ABBREVTAB[15], "NONE          ");
  //        COMJMP[15] = PSTATUS;
  //        strcpy(COMTAB[16], "TIME          ");
  //        strcpy(ABBREVTAB[16], "NONE          ");
  //        COMJMP[16] = PTIME;
  //        strcpy(COMTAB[17], "UTILIZATION   ");
  //        strcpy(ABBREVTAB[17], "U             ");
  //        COMJMP[17] = UTIL;
  //        strcpy(COMTAB[18], "DELAY         ");
  //        strcpy(ABBREVTAB[18], "NONE          ");
  //        COMJMP[18] = CDELAY;
  //        strcpy(COMTAB[19], "CONGESTION    ");
  //        strcpy(ABBREVTAB[19], "NONE          ");
  //        COMJMP[19] = CONGEST;
  //        strcpy(COMTAB[20], "VARIATION     ");
  //        strcpy(ABBREVTAB[20], "NONE          ");
  //        COMJMP[20] = VARY;
  //        strcpy(COMTAB[21], "OPEN          ");
  //        strcpy(ABBREVTAB[21], "O             ");
  //        COMJMP[21] = OPENF;
  //        strcpy(COMTAB[22], "CLOSE         ");
  //        strcpy(ABBREVTAB[22], "NONE          ");
  //        COMJMP[22] = CLOSEF;
  //        strcpy(COMTAB[23], "INPUT         ");
  //        strcpy(ABBREVTAB[23], "NONE          ");
  //        COMJMP[23] = INPUTF;
  //        strcpy(COMTAB[24], "OUTPUT        ");
  //        strcpy(ABBREVTAB[24], "NONE          ");
  //        COMJMP[24] = OUTPUTF;
  //        strcpy(COMTAB[25], "VIEW          ");
  //        strcpy(ABBREVTAB[25], "V             ");
  //        COMJMP[25] = VIEW;
  //        strcpy(COMTAB[26], "SHORTCUTS     ");
  //        strcpy(ABBREVTAB[26], "NONE          ");
  //        COMJMP[26] = SHORT;
  //        strcpy(COMTAB[27], "RESET         ");
  //        strcpy(ABBREVTAB[27], "NONE          ");
  //        COMJMP[27] = RESETP;
  //        strcpy(COMTAB[28], "MPI           ");
  //        strcpy(ABBREVTAB[28], "NONE          ");
  //        COMJMP[28] = MPI;
  //        strcpy(COMTAB[29], "VERSION       ");
  //        strcpy(ABBREVTAB[29], "NONE          ");
  //        COMJMP[29] = VERSION;
}

//    bool eoln(std::ifstream *inp)
//    {
//        int ch;
////        if ((*SOURCE).eof())
////            return true;
//        ch = (*inp).peek();
//        if (ch < 0)
//            return true;
//        if (ch == '\n')
//        {
//            (*inp).ignore();
//            return true;
//        }
//        if (ch == '\r')
//        {
//            (*inp).ignore(2);
//            return true;
//        }
//        return false;
//    }

auto eoln(std::ifstream &inf) -> bool {
  bool eln = false;
  if (int ltr = inf.get(); ltr >= 0) {
    if (ltr == '\r') {
      ltr = inf.get();
      eln = true;
    }
    if (ltr != '\n') {
      if (ltr >= 0)
        inf.putback(static_cast<char>(ltr));
      else
        eln = true;
    } else
      eln = true;
  } else
    eln = true;
  return eln;
}

void Commander::init_interpreter() {
  interp->INITFLAG = false;
  interp->PROFILEON = false;
  interp->ALARMON = false;
  interp->ALARMENABLED = false;
  interp->CONGESTION = false;
  interp->VARIATION = false;
  interp->TOPDELAY = 10;
  for (int i = 1; i <= BRKMAX; i++) {
    interp->BRKTAB.at(i) = -1;
    interp->BRKLINE.at(i) = -1;
  }
  interp->NUMBRK = 0;
  interp->NUMTRACE = 0;
  for (int I = 1; I <= VARMAX; I++)
    interp->TRCTAB[I].MEMLOC = -1;
  interp->LISTDEFNAME = "LISTFILE.TXT";
  // LISTFILE = fopen(LISTDEFNAME, "w");   Pascal ASSIGN
  interp->LISTFNAME = interp->LISTDEFNAME;
  LISTDEF_FLAG = true;
  INPUTFILE_FLAG = false;
  OUTPUTFILE_FLAG = false;

  interp->INPUTFNAME.clear();
  interp->OUTPUTFNAME.clear();

  INPUTOPEN_FLAG = false;
  OUTPUTOPEN_FLAG = false;
  MUST_RUN_FLAG = false;
}

void Commander::get_var() {
  PROCPNT KDES;
  int i = 0;
  int j = 0;
  interp->ERR = false;
  strcpy(interp->PROMPT.data(), "Process:  ");
  interp->K = num();
  if (interp->K < 0) {
    std::cout << "Invalid Process Number" << '\n';
    interp->ERR = true;
    goto L70;
  }
  if (!interp->INITFLAG) {
    std::cout << "Must Run Program First" << '\n';
    interp->ERR = true;
    goto L70;
  }
  if (interp->K == 0) {
    KDES = interp->ACPHEAD->PDES;
    interp->TEMP = true;
  } else {
    interp->PTEMP = interp->ACPHEAD;
    interp->TEMP = false;
    do {
      if (
        interp->PTEMP->PDES->PID == interp->K &&
        interp->PTEMP->PDES->STATE != State::TERMINATED) {
        interp->TEMP = true;
        KDES = interp->PTEMP->PDES;
      } else
        interp->PTEMP = interp->PTEMP->NEXT;
    } while (!interp->TEMP && interp->PTEMP != interp->ACPHEAD);
  }
  if (!interp->TEMP) {
    std::cout << "Invalid Process Number" << '\n';
    std::cout << "Type Status Command to Get List of Processes" << '\n';
    interp->ERR = true;
    goto L70;
  }
  remove_block();
  while (END_FLAG) {
    std::cout << "Variable: ";
    //        std::string input;
    //        getline(std::cin, input);
    read_line();
    remove_block();
  }
  for (j = 1; j <= NAMELEN; j++) {
    i = CC + j - 1;
    if (i <= LL)
      interp->VARNAME[j] = LINE.at(i);
    else
      interp->VARNAME[j] = ' ';
  }
  if (!in_word()) {
    std::cout << "Error in Variable Name" << '\n';
    interp->ERR = true;
    goto L70;
  }
  interp->LEVEL = LMAX;
  while (KDES->DISPLAY[interp->LEVEL] == -1)
    interp->LEVEL = interp->LEVEL - 1;
  i = interp->LEVEL;
  strcpy(TAB[0].name.data(), &ID[1]);
  do {
    if (i > 1) {
      interp->VAL = KDES->DISPLAY[i];
      interp->INX = interp->S[interp->VAL + 4];
      j = BTAB[TAB[interp->INX].reference].LAST;
    } else
      j = BTAB[2].LAST;
    while (strcmp(TAB[j].name.data(), &ID[1]) != 0)
      j = TAB[j].link;
    i = i - 1;
  } while (i >= 1 && j == 0);
  if (j == 0) {
    if (strcmp(&ID[1], "MPI_BUFFER    ") == 0 && mpi_mode)
      goto L70;
    std::cout << "Name Not Found" << '\n';
    interp->ERR = true;
    goto L70;
  }
  interp->LEVEL = i + 1;
  interp->TYP = TAB[j].types;
  interp->REF = TAB[j].reference;
  interp->OBJ = TAB[j].object;
  if (interp->OBJ != Objects::Constant && interp->OBJ != Objects::Variable) {
    std::cout << "Name Not Found" << '\n';
    interp->ERR = true;
    goto L70;
  }
  if (interp->OBJ != Objects::Constant) {
    interp->ADR = TAB[j].address + KDES->DISPLAY[interp->LEVEL];
    if (!TAB[j].normal)
      interp->ADR = interp->S[interp->ADR];
    if (
      interp->TYP == Types::INTS || interp->TYP == Types::REALS ||
      interp->TYP == Types::BOOLS || interp->TYP == Types::CHARS) {
      for (i = KDES->BASE; i <= KDES->BASE + KDES->FORLEVEL - 1; i++) {
        if (
          interp->SLOCATION[i] == interp->ADR &&
          KDES->PC >= std::lround(interp->RS[i]))
          interp->ADR = i;
      }
    }
  } else
    interp->ADR = TAB[j].address;
  qualify();
  if (interp->ERR)
    goto L70;
  remove_block();
  if (!END_FLAG) {
    std::cout << "Error in Variable Name" << '\n';
    interp->ERR = true;
    goto L70;
  }
L70:;
  std::cout << '\n';
}

auto Commander::read_line(std::ifstream input) -> void {
  // Pascal reads optional values, but reads through any of CRLF, LF, CR, end
  // styles handled in eoln()
  //        size_t reclen = FILMAX + 1;
  //        char *buffer = FNAME;
  //        getline(&buffer, &reclen, input);
  while (!eoln(input))
    char ch = input.get();
}

auto Commander::rand_gen() -> double {
  const double lB = 100001.0;
  const double lC = 1615;
  xr = xr * lC;
  xr = xr - std::floor(xr / lB) * lB;
  return xr / lB;
}

void Commander::convert() {
  if (CH >= 'a' && CH <= 'z')
    CH = static_cast<char>(CH - ' ');
}

void Commander::read_line() {
  LL = 0;
  while (!eoln(INPUT) && LL <= LLNG) {
    CH = static_cast<char>(INPUT.get());
    if (CH == '\x08') {
      if (LL > 0)
        LL = LL - 1;
    } else {
      convert();
      LL = LL + 1;
      LINE.at(LL) = CH;
    }
  }
  // READLN(STANDARD_INPUT);
  if (LL > 0) {
    END_FLAG = false;
    CC = 1;
    CH = LINE.at(CC);
  } else {
    END_FLAG = true;
    CH = ' ';
    CC = 0;
  }
}

void Commander::fread_line() {
  LL = 0;
  while (!eoln(INPUT) && LL <= LLNG) {
    // CH = (*INPUT).get();
    CH = static_cast<char>(INPUT.get());
    if (CH == '\x08') {
      if (LL > 0)
        LL = LL - 1;
    } else {
      LL = LL + 1;
      LINE.at(LL) = CH;
    }
  }
  // READLN(STANDARD_INPUT);
  if (LL > 0) {
    END_FLAG = false;
    CC = 1;
    CH = LINE.at(CC);
  } else {
    END_FLAG = true;
    CH = ' ';
    CC = 0;
  }
}

auto Commander::fread_line(PreBuffer &pre_buf) -> void {
  int ch = 0;
  LL = 0;
  ch = pre_buf.sgetc();
  while (ch >= 0 && ch != '\n' && ch != '\r') {
    LINE.at(++LL) = static_cast<char>(ch);
    ch = pre_buf.sgetc();
  }
  if (ch == '\r')
    ch = pre_buf.sgetc();
  if (LL > 0) {
    END_FLAG = false;
    CC = 1;
    CH = LINE.at(CC);
  } else {
    END_FLAG = true;
    CH = ' ';
    CC = 0;
  }
}

auto Commander::next_char() -> void {
  if (CC == LL) {
    END_FLAG = true;
    CH = ' ';
  } else {
    CC = CC + 1;
    CH = LINE.at(CC);
  }
}

auto Commander::remove_block() -> void {
  while (!END_FLAG && CH == ' ')
    next_char();
}

auto Commander::in_word() -> bool {
  remove_block();
  int k = 0;
  if (END_FLAG)
    return false;
  convert();

  if (CH < 'A' || CH > 'Z')
    return false;
  while ((CH >= 'A' && CH <= 'Z') || CH == '_' || (CH >= '0' && CH <= '9')) {
    if (k < ALNG) {
      k = k + 1;
      ID[k] = CH;
    }
    next_char();
    convert();
  }
  while (k < ALNG) {
    k = k + 1;
    ID[k] = ' ';
  }
  return true;
}

auto Commander::in_word_2() -> bool {
  remove_block();
  if (END_FLAG)
    return false;
  // CONVERT();
  if (isupper(CH) == 0)
    return false;
  int k = 0;
  CH = static_cast<char>(toupper(CH));
  while (isalpha(CH) != 0 || isdigit(CH) != 0 || CH == '_') {
    if (k < ALNG) {
      k = k + 1;
      ID[k] = CH;
    }
    next_char();
    CH = static_cast<char>(toupper(CH));
  }
  while (k < ALNG) {
    k = k + 1;
    ID[k] = ' ';
  }
  return true;
}

auto Commander::get_num() -> bool {
  remove_block();
  int val = 0;
  int i = 0;
  int sign = 1;
  if (CH == '-') {
    sign = -1;
    next_char();
  }
  while (CH >= '0' && CH <= '9') {
    if (i < KMAX)
      val = val * 10 + CH - '0';
    i = i + 1;
    next_char();
  }
  if (i > KMAX || val > LONGNMAX || i == 0)
    return false;
  INUM = val * sign;
  return true;
}

auto Commander::in_filename() -> void {
  remove_block();
  // FNAME[0] = '\0';
  int i = -1;
  while (i < FILMAX && !END_FLAG) {
    i = i + 1;
    filename.string().at(i) = CH;
    next_char();
  }
  i = i + 1;
  filename.string().at(i) = '\0';
}

auto Commander::eq_in_filename() -> void {
  remove_block();
  if (END_FLAG)
    filename.string().at(0) = '\0';
  else {
    if (CH == '=')
      next_char();
    in_filename();
  }
}

auto Commander::get_range(int &first, int &last, bool &err) -> void {
  err = false;
  remove_block();
  if (!END_FLAG) {
    if (get_num())
      first = INUM;
    else
      err = true;
    remove_block();
    if (CH == ':') {
      next_char();
      if (get_num())
        last = INUM;
      else
        err = true;
    } else if (END_FLAG)
      last = first;
    else
      err = true;
  }
}

auto get_lnum(int pc) -> int {
  int i = 1;
  while (LOCATION[i] <= pc && i <= LINE_NUMBER)
    i++;
  return i - 1;
}

auto Commander::print_stats() -> void {
  std::cout << '\n';
  if (interp->PS != PS::FIN && interp->PS != PS::DEAD) {
    std::cout << "*** HALT AT " << get_lnum(interp->CURPR->PC - 1)
              << " IN PROCESS " << interp->CURPR->PID << " BECAUSE OF ";
    // pascal:       print(stdout, '*** HALT AT ',GETLNUM(PC-1):1,' IN PROCESS
    // ', CURPR^.PID:1); pascal:       print(stdout,  ' BECAUSE OF ');
    switch (interp->PS) {
      case PS::DIVCHK: {
        println("DIVISION BY 0");
        break;
      }
      case PS::INXCHK: {
        println("INVALID INDEX");
        break;
      }
      case PS::STKCHK: {
        // std::cout << "EXPRESSION EVALUATION OVERFLOW" << '\n';
        println("STACK OVERFLOW DURING EXPRESSION EVALUATION");
        break;
      }
      case PS::INTCHK: {
        println("INTEGER VALUE TOO LARGE");
        break;
      }
      case PS::LINCHK: {
        println("TOO MUCH OUTPUT");
        break;
      }
      case PS::LNGCHK: {
        println("LINE TOO LONG");
        break;
      }
      case PS::REDCHK: {
        println("READING PAST END OF FILE");
        break;
      }
      case PS::DATACHK: {
        println("IMPROPER DATA DURING READ");
        break;
      }
      case PS::CHANCHK: {
        println("TOO MANY STREAMS OPEN");
        break;
      }
      case PS::BUFCHK: {
        println("STREAM BUFFER OVERFLOW");
        break;
      }
      case PS::PROCCHK: {
        println("TOO MANY PROCESSES");
        break;
      }
      case PS::CPUCHK: {
        println("INVALID PROCESSOR NUMBER");
        break;
      }
      case PS::REFCHK: {
        println("POINTER ERROR");
        break;
      }
      case PS::CASCHK: {
        println("UNDEFINED CASE");
        break;
      }
      case PS::CHRCHK: {
        println("INVALID CHARACTER");
        break;
      }
      case PS::STORCHK: {
        println("STORAGE OVERFLOW");
        break;
      }
      case PS::GRPCHK: {
        println("ILLEGAL GROUP SIZE");
        break;
      }
      case PS::REMCHK: {
        println("REFERENCE TO A NONLOCAL VARIABLE");
        break;
      }
      case PS::LOCKCHK: {
        println("USE OF SPINLOCK IN MULTICOMPUTER");
        break;
      }
      case PS::MPIINITCHK: {
        println("FAILURE TO CALL MPI_Init()");
        break;
      }
      case PS::MPIFINCHK: {
        println("FAILURE TO CALL MPI_Finalize()");
        break;
      }
      case PS::MPIPARCHK: {
        println("INVALID VALUE OF MPI FUNCTION PARAMETER");
        break;
      }
      case PS::MPITYPECHK: {
        println("TYPE MISMATCH IN MPI FUNCTION CALL");
        break;
      }
      case PS::MPICNTCHK: {
        println("TOO MANY VALUES IN THE MESSAGE");
        break;
      }
      case PS::MPIGRPCHK: {
        println("MISMATCH IN MPI GROUP COMMUNICATION CALLS");
        break;
      }
      case PS::FUNCCHK: {
        println("FUNCTION NOT FULLY DEFINED");
        break;
      }
      case PS::CARTOVR: {
        println("TOO MANY CARTESIAN TOPOLOGY");
        break;
      }
      case PS::STRCHK: {
        println("INVALID STRING");
        break;
      }
      case PS::USERSTOP: {
        println("PROGRAM REQUESTED ABNORMAL TERMINATION");
        break;
      }
      case PS::OVRCHK: {
        println("FLOATING POINT OVERFLOW");
        break;
      }
      default: break;
    }
    print("\n");
  }
  if (!OUTPUTFILE_FLAG) {
    println("SEQUENTIAL EXECUTION TIME: {}", static_cast<int>(interp->SEQTIME));
    println(
      "PARALLEL EXECUTION TIME: {}", static_cast<int>(interp->MAINPROC->TIME));
    if (interp->MAINPROC->TIME != 0) {
      interp->SPEED = interp->SEQTIME / interp->MAINPROC->TIME;
      println("SPEEDUP:  %{}", interp->SPEED);
    }
    println("NUMBER OF PROCESSORS USED: {}", interp->USEDPROCS);
  } else {
    // print(STANDARD_OUTPUT, OUTPUT);
    println(
      OUTPUT,
      "SEQUENTIAL EXECUTION TIME: {}",
      static_cast<int>(interp->SEQTIME));
    println(
      OUTPUT,
      "PARALLEL EXECUTION TIME: {}",
      static_cast<int>(interp->MAINPROC->TIME));
    if (interp->MAINPROC->TIME != 0) {
      interp->SPEED = interp->SEQTIME / interp->MAINPROC->TIME;
      println(OUTPUT, "SPEEDUP:  {}", interp->SPEED);
    }
    println(OUTPUT, "NUMBER OF PROCESSORS USED: {}", interp->USEDPROCS);
    // (*OUTPUT).close();
    OUTPUT.close();
    OUTPUTOPEN_FLAG = false;
    if (INPUTOPEN_FLAG) {
      // (*INPUT).close();
      INPUT.close();
      INPUTOPEN_FLAG = false;
    }
  }
}

auto Commander::find() -> int {
  int i = 1;
  while (interp->CHAN[i].HEAD != -1 && i < OPCHMAX)
    i++;
  if (i == OPCHMAX)
    interp->PS = PS::CHANCHK;
  interp->CHAN[i].HEAD = 0;
  interp->CHAN[i].WAIT = nullptr;
  interp->CHAN[i].SEM = 0;
  interp->CHAN[i].READTIME = 0;
  interp->CHAN[i].MOVED = false;
  interp->CHAN[i].READER = -1;
  return i;
}

static int rel_alloc = 0;
static int rel_free = 0;
static int rel_fail = 0;

auto Commander::find_frame(int len) -> int {
  // a frame is an unused chunk of stack parallels, S, RS, SLOCATION, STARTMEM
  // of the passed LENGTH
  // the BLOCKR chain is managed by this code
  BLKPNT PREV = nullptr, REF = interp->STHEAD;
  int STARTLOC;
  int rtn;
  bool FOUND = false;
  while (!FOUND && REF != nullptr) {
    if (REF->SIZE >= len) {
      FOUND = true;
      rtn = REF->START;
      STARTLOC = REF->START;
      REF->START = REF->START + len;
      REF->SIZE = REF->SIZE - len;
      if (REF->SIZE == 0) {
        if (REF == interp->STHEAD)
          interp->STHEAD = REF->NEXT;
        else {
          // PREV has been set if REF moves beyond STHEAD
          PREV->NEXT = REF->NEXT;
          free(REF);
          ++rel_free;
        }
        // free(REF);  // ??
        REF = nullptr;
      }
    } else {
      PREV = REF;
      REF = REF->NEXT;
    }
  }
  if (!FOUND) {
    interp->PS = PS::STORCHK;
    rtn = -1;
  } else {
    for (int I = STARTLOC; I <= STARTLOC + len - 1; I += 1)
      interp->STARTMEM[I] = STARTLOC;
  }
  return rtn;
}

auto Commander::release(int base, int len) -> void {
  STYPE *smp = interp->STARTMEM;
  for (int I = base; I < base + len; I++)
    smp[I] = 0;
  BLKPNT PREV;
  BLKPNT REF = interp->STHEAD;
  bool LOCATED = false;
  while (!LOCATED && REF != nullptr) {
    if (base <= REF->START)
      LOCATED = true;
    else {
      PREV = REF;
      REF = REF->NEXT;
    }
  }
  if (!LOCATED)
    ++rel_fail;
  BLKPNT NEWREF = (BLKPNT)malloc(sizeof(BLOCKR));
  ++rel_alloc;
  NEWREF->START = base;
  NEWREF->SIZE = len;
  NEWREF->NEXT = REF;
  if (REF == interp->STHEAD)
    interp->STHEAD = NEWREF;
  else {
    if (PREV->START + PREV->SIZE != base)
      PREV->NEXT = NEWREF;
    else {
      PREV->SIZE = PREV->SIZE + len;
      free(NEWREF);
      ++rel_free;
      NEWREF = PREV;
    }
  }
  if (REF != nullptr) {
    if (NEWREF->START + NEWREF->SIZE == REF->START) {
      NEWREF->SIZE = NEWREF->SIZE + REF->SIZE;
      NEWREF->NEXT = REF->NEXT;
      ++rel_free;
      free(REF);
    }
  }
}

auto Commander::unreleased() -> void {
  std::println(
    "rel_fail %d, rel_alloc %d, rel_free %d", rel_fail, rel_alloc, rel_free);
  BLKPNT seq = interp->STHEAD;
  while (seq != nullptr) {
    std::println("unreleased %d len %d", seq->START, seq->SIZE);
    seq = seq->NEXT;
  }
}

auto Commander::num() -> int {
  bool found = get_num();
  bool err = false;
  int cnt = 0;
  while (!found && !err && cnt < 3) {
    remove_block();
    if (!END_FLAG)
      err = true;
    else {
      std::cout << interp->PROMPT;
      read_line();
      found = get_num();
      if (!found)
        cnt++;
    }
  }
  return err || cnt >= 3 ? -1 : INUM;
}

auto Commander::initialize() -> void {
  if (interp->INITFLAG) {
    interp->PTEMP = interp->ACPHEAD;
    do {
      free(interp->PTEMP->PDES);
      interp->RTEMP = interp->PTEMP;
      interp->PTEMP = interp->PTEMP->NEXT;
      free(interp->RTEMP);
    } while (interp->PTEMP != interp->ACPHEAD);

    BLKPNT tbp = interp->STHEAD;
    while (tbp != nullptr) {
      BLKPNT sbp = tbp;
      tbp = tbp->NEXT;
      free(sbp);
    }

    if (topology != Symbol::SHAREDSY) {
      for (int i = 0; i <= PMAX; i++) {
        BUSYPNT tcp = interp->PROCTAB.at(i).BUSYLIST;
        while (tcp != nullptr) {
          const BUSYPNT scp = tcp;
          tcp = tcp->NEXT;
          free(scp);
        }
      }
    }
  }

  int *ip = interp->LINK;
  for (int i = 1; i <= BUFMAX - 1; i++)
    ip[i] = i + 1;

  interp->FREE = 1;
  interp->LINK[BUFMAX] = 0;
  for (int i = 0; i <= OPCHMAX; i++)
    interp->CHAN.at(i).HEAD = -1;

  interp->STHEAD = static_cast<BLKPNT>(calloc(1, sizeof(BLOCKR)));
  ++rel_alloc;

  // WORKSIZE=30
  interp->STKMAIN = BTAB[2].VSIZE + WORKSIZE;

  // first STKMAIN elements of S, RS, SLOCATION, STARTMEN, initialized
  for (int i = 0; i <= interp->STKMAIN - 1; i++) {
    interp->S[i] = 0;
    interp->SLOCATION[i] = 0;
    interp->STARTMEM[i] = 1;
    interp->RS[i] = 0.0;
  }

  // first BLOCKR (il->STKHEAD) saves first free stack index, free size of
  // stack, next BLOCKR (null)
  interp->STHEAD->START = interp->STKMAIN;
  interp->STHEAD->SIZE = STMAX - interp->STKMAIN;
  interp->STHEAD->NEXT = nullptr;
  interp->NOSWITCH = false;
  interp->S[1] = 0;
  interp->S[2] = 0;
  interp->S[3] = -1;
  interp->S[4] = BTAB[1].LAST; // first invalid stack bottom index
  interp->S[5] = 1;
  interp->S[6] = interp->STKMAIN; // first stack beyond reserved WORKSIZE

  // BASESIZE is 8, STARTMEM 8 elements negated
  for (int I = 0; I <= BASESIZE - 1; I++)
    interp->STARTMEM[I] = -1;

  interp->FLD[1] = 20;
  interp->FLD[2] = 4;
  interp->FLD[3] = 5;
  interp->FLD[4] = 1;
  interp->SEQTIME = 0;
  if (interp->VARIATION) {
    std::cout << "Random Number Seed for Variation: ";
    std::cin >> xr;
    xr = xr + 52379.0;
  }

  // init all PROCTAB elements
  for (int I = 0; I <= PMAX; I++) {
    interp->PROCTAB.at(I).STATUS = Status::NEVERUSED;
    interp->PROCTAB.at(I).VIRTIME = 0;
    interp->PROCTAB.at(I).BRKTIME = 0;
    interp->PROCTAB.at(I).PROTIME = 0;
    interp->PROCTAB.at(I).RUNPROC = nullptr;
    interp->PROCTAB.at(I).NUMPROC = 0;
    interp->PROCTAB.at(I).STARTTIME = 0;
    interp->PROCTAB.at(I).BUSYLIST = nullptr;
    if (interp->VARIATION)
      interp->PROCTAB.at(I).SPEED = static_cast<float>(rand_gen() + 0.001);
    else
      interp->PROCTAB.at(I).SPEED = 1;
  }

  interp->PROLINECNT = 0;
  interp->CLOCK = 0;
  interp->ACPHEAD = static_cast<ACTPNT>(calloc(1, sizeof(ActiveProcess)));
  interp->MAINPROC = static_cast<PROCPNT>(calloc(1, sizeof(ProcessDescriptor)));
  interp->ACPHEAD->PDES = interp->MAINPROC;
  interp->ACPHEAD->NEXT = interp->ACPHEAD;
  interp->ACPTAIL = interp->ACPHEAD;
  interp->ACPCUR = interp->ACPHEAD;
  interp->MAINPROC->BASE = 0;
  interp->MAINPROC->B = 0;
  interp->MAINPROC->DISPLAY[0] = -2; // unused, 1-origin
  interp->MAINPROC->DISPLAY[1] = 0;

  for (int I = 2; I <= LMAX; I++)
    interp->MAINPROC->DISPLAY.at(I) = -1;

  // last in prefix for initial increment
  interp->MAINPROC->T = BTAB[2].VSIZE - 1;
  interp->MAINPROC->PC = 0;
  interp->MAINPROC->TIME = 0;
  interp->MAINPROC->VIRTUALTIME = 0;
  interp->MAINPROC->STATE = State::RUNNING;
  interp->MAINPROC->STACKSIZE = interp->STKMAIN - 2;
  interp->MAINPROC->FORLEVEL = 0;
  interp->MAINPROC->PROCESSOR = 0;
  interp->MAINPROC->ALTPROC = -1;
  interp->MAINPROC->PID = 0;
  interp->MAINPROC->READSTATUS = ReadStatus::NONE;
  interp->MAINPROC->FORKCOUNT = 1;
  interp->MAINPROC->MAXFORKTIME = 0;
  interp->MAINPROC->JOINSEM = 0;
  interp->MAINPROC->PRIORITY = PRIORITY::LOW;
  interp->MAINPROC->SEQON = true;
  interp->PROCTAB[0].STATUS = Status::FULL;
  interp->PROCTAB[0].RUNPROC = interp->MAINPROC;
  interp->PROCTAB[0].NUMPROC = 1;
  interp->CURPR = interp->MAINPROC;
  interp->USEDPROCS = 1;
  interp->NEXTID = 1;
  interp->PS = PS::RUN;
  interp->LNCNT = 0;
  interp->CHRCNT = 0;
  interp->COUTWIDTH = -1;
  interp->COUTPREC = -1;
  interp->INITFLAG = true;
  interp->OLDSEQTIME = 0;
  interp->OLDTIME = 0;
  MUST_RUN_FLAG = false;

  if (interp->ALARMON)
    interp->ALARMENABLED = true;

  if (mpi_mode) {
    for (int I = 0; I <= PMAX; I++) {
      interp->MPIINIT[I] = 0;
      interp->MPIFIN[I] = 0;
      interp->MPIPNT.at(I) = -1;
    }

    interp->MPICODE = -1;
    interp->MPITIME = 0;
    interp->MPISEM = 0;
    interp->MPIROOT = -1;
    interp->MPITYPE = 0;
    interp->MPICNT = 0;
    interp->MPIOP = 0;

    for (int I = 1; I <= CARTMAX; I++)
      interp->MPICART.at(I).at(0) = -1;
  }
}

auto Commander::test_end() -> bool {
  remove_block();
  if (END_FLAG)
    return true;
  std::cout << "INVALID COMMAND" << '\n';
  return false;
}

//    void EXECUTE()
//    {
//        int I, J, K, H1, H2, H3, H4, TGAP;
//        float RH1;
//        ORDER IR;
//        bool B1;
//        PROCPNT NEWPROC;
//        Label699:;
//        Label999:;
//    }

extern void BLOCK(
  Interpreter *interp_local,
  SymbolSet FSYS,
  bool is_function,
  int level,
  int PRT);

auto Commander::interpret() -> void {
  int i = 0;
  int j = 0;
  int k = 0;
  //        STYPE *S;
  //        STYPE *SLOCATION;
  //        STYPE *STARTMEM;
  //        RSTYPE *RS;
  //        BUFINTTYPE *VALUE;
  //        BUFREALTYPE *RVALUE;
  //        BUFREALTYPE *DATE;
  //        BUFINTTYPE *LINK;

  init_interpreter();
  // NEW(S); NEW(SLOCATION); NEW(STARTMEM); NEW(RS);
  // NEW(VALUE); NEW(RVALUE); NEW(DATE); NEW(LINK);

  interp->S = (STYPE *)calloc(STMAX + 1, sizeof(STYPE));
  interp->SLOCATION = (STYPE *)calloc(STMAX + 1, sizeof(STYPE));
  interp->STARTMEM = (STYPE *)calloc(STMAX + 1, sizeof(STYPE));
  interp->RS = (RSTYPE *)calloc(STMAX + 1, sizeof(RSTYPE));
  interp->LINK = (BUFINTTYPE *)calloc(BUFMAX + 1, sizeof(BUFINTTYPE));
  interp->VALUE = (BUFINTTYPE *)calloc(BUFMAX + 1, sizeof(BUFINTTYPE));
  interp->RVALUE = (BUFREALTYPE *)calloc(BUFMAX + 1, sizeof(BUFREALTYPE));
  interp->DATE = (BUFREALTYPE *)calloc(BUFMAX + 1, sizeof(BUFREALTYPE));

  init_commands();
  mpi_mode = false;
  do {
    int cct = 0;
    do {
      // print(stdout, "\n");
      // std::cout << '\n';
      std::print("\n");
      // print(stdout, '*');
      // std::cout << "*";
      std::print("*");
      //                FREADLINE();
      fread_line(*pre_buf);
      //                print(STANDARD_OUTPUT, "%s\n", &LINE[1]);
      if (++cct > 3) {
        println("prompt loop may be infinite");
        exit(1);
      }
    } while (END_FLAG);
    interp->TEMP = in_word();
    interp->COMMLABEL = COMTYP::ERRC;
    for (i = 0; i < 30; i++) {
      if (
        commands[i] != &ID[1] ||
        abbreviations[i] != &ID[1] && abbreviations[i] == "NONE")
        interp->COMMLABEL = command_jump[i];
    }
    switch (interp->COMMLABEL) {
      case COMTYP::RUNP:
        if (!SOURCEOPEN_FLAG) {
          std::println("Must Open a Program Source File First");
          break;
        }
        if (test_end()) {
          if (!INPUTOPEN_FLAG) {
            // size_t ll = il->INPUTFNAME.string().length() - 1;
            // while (ll > 0 && il->INPUTFNAME[ll] == ' ') ll -= 1;
            // if (ll++ > 0) il->INPUTFNAME[ll] = '\0';
            INPUT.open(interp->INPUTFNAME);
            // il->INPUTFNAME[ll] = ' ';
          }

          // RESET(INPUT);
          if (!INPUT) {
            print("\n");
            print("Error In Data Input File {}\n", interp->INPUTFNAME.string());
          } else
            INPUTOPEN_FLAG = true;

          if (!OUTPUTOPEN_FLAG) {
            // size_t ll = il->INPUTFNAME.length() - 1;
            // while (ll > 0 && il->INPUTFNAME[ll] == ' ') ll -= 1;
            // if (ll++ > 0) il->INPUTFNAME[ll] = '\0';
            OUTPUT.open(interp->OUTPUTFNAME);
            // il->INPUTFNAME[ll] = ' ';
          }

          // REprint(stdout, OUTPUT);
          if (!OUTPUT) {
            std::print("\n");
            std::print(
              "Error In Data Output File {}\n", interp->OUTPUTFNAME.string());
          } else
            OUTPUTOPEN_FLAG = true;

          initialize();
          interp->MAXSTEPS = -1;
          interp->RESTART = false;
          if (interp->PROFILEON)
            interp->PROTIME = interp->PROSTEP;
          //                                    dumpCode();
          EXECUTE(interp);
          //                                    dumpSymbols();
          //                                    dumpArrays();
          //                                    dumpBlocks();
          //                                    dumpReals();
          //                                    dumpProctab(il);
          //                                    dumpStkfrms(il);
          //                                    dumpChans(il);
          //                                    dumpActive(il);
          //                                    dumpDeadlock(il);
          if (interp->PS != PS::BREAK)
            print_stats();
        }
        break;
      case COMTYP::CONT:
        if (test_end()) {
          if (interp->INITFLAG && !MUST_RUN_FLAG) {
            if (interp->PS == PS::BREAK) {
              interp->PS = PS::RUN;
              interp->RESTART = true;
              interp->MAXSTEPS = -1;
              interp->OLDSEQTIME = interp->SEQTIME;
              interp->OLDTIME = interp->CLOCK;
              for (i = 0; i <= PMAX; i++) {
                interp->PROCTAB.at(i).BRKTIME = 0;
                interp->PROCTAB.at(i).PROTIME = 0;
              }
              if (interp->PROFILEON) {
                interp->PROLINECNT = 0;
                interp->PROTIME = interp->OLDTIME + interp->PROSTEP;
              }
              EXECUTE(interp);
              if (interp->PS != PS::BREAK)
                print_stats();
            } else
              print(stdout, "Can Only Continue After a Breakpoint");
          } else
            print(stdout, "Must Run Program Before Continuing");
        }
        break;
      case COMTYP::STEP:
        if (interp->INITFLAG && !MUST_RUN_FLAG) {
          if (interp->PS == PS::BREAK) {
            if (in_word()) {
              if (strcmp(&ID[1], "PROCESS       ") == 0) {
                strcpy(interp->PROMPT.data(), "Number:   ");
                k = num();
                interp->PTEMP = interp->ACPHEAD;
                interp->TEMP = false;
                do {
                  if (interp->PTEMP->PDES->PID == k) {
                    interp->TEMP = true;
                    interp->STEPROC = interp->PTEMP->PDES;
                  } else
                    interp->PTEMP = interp->PTEMP->NEXT;
                } while (!interp->TEMP && interp->PTEMP != interp->ACPHEAD);
                if (!interp->TEMP) {
                  print(stdout, "Invalid Process Number");
                  goto L200;
                }
                print(stdout, "\n");
              } else {
                print(stdout, "Error in Command");
                goto L200;
              }
            } else {
              strcpy(interp->PROMPT.data(), "Number:   ");
              k = num();
              if (k < 0)
                print(stdout, "Error in Number");
              else {
                interp->MAXSTEPS = k;
                interp->OLDSEQTIME = interp->SEQTIME;
                interp->OLDTIME = interp->CLOCK;
                interp->STEPTIME = interp->STEPROC->TIME;
                interp->VIRSTEPTIME = interp->STEPROC->VIRTUALTIME;
                for (i = 0; i <= PMAX; i++) {
                  interp->PROCTAB.at(i).BRKTIME = 0;
                  interp->PROCTAB.at(i).PROTIME = 0;
                }
                if (interp->PROFILEON)
                  interp->PROTIME = interp->OLDTIME + interp->PROSTEP;
                interp->STARTLOC = LOCATION[interp->BLINE];
                interp->ENDLOC = LOCATION[interp->BLINE + 1] - 1;
                interp->PS = PS::RUN;
                EXECUTE(interp);
                if (interp->PS != PS::BREAK)
                  print_stats();
              }
            }
          } else
            print(stdout, "Cannot Step Because Program is Terminated");
        } else
          print(stdout, "Must Begin with Run Command");
        break;
      case COMTYP::BREAKP:
        if (!SOURCEOPEN_FLAG) {
          print(stdout, "Must Open a Program Source File First");
          goto L200;
        }
        j = 0;
        for (i = 1; i <= BRKMAX; i++) {
          if (interp->BRKTAB.at(i) == -1)
            j = i;
        }
        if (j == 0)
          print(stdout, "Too Many Breakpoints");
        else {
          strcpy(interp->PROMPT.data(), "At Line:  ");
          k = num();
          if (k < 0 || k > LINE_NUMBER)
            print(stdout, "Out of Bounds");
          else if (LOCATION[k + 1] == LOCATION[k] || !BREAK_ALLOW[k])
            print(stdout, "Breakpoints Must Be Set at Executable Lines");
          else {
            interp->BRKLINE.at(j) = k;
            interp->BRKTAB.at(j) = LOCATION[k];
            interp->NUMBRK++;
            print(stdout, "\n");
          }
        }
        break;
      case COMTYP::EXIT2: print(stdout, "TERMINATE C* SYSTEM\n"); break;
      case COMTYP::VIEW:
        if (!SOURCEOPEN_FLAG) {
          print(stdout, "Must Open a Program Source File First");
          goto L200;
        }
        interp->FIRST = 1;
        interp->LAST = LINE_NUMBER;
        get_range(interp->FIRST, interp->LAST, interp->ERR);
        if (interp->ERR) {
          print(stdout, "Error in Range");
          goto L200;
        }
        interp->LAST = std::min(interp->LAST, LINE_NUMBER);
        interp->FIRST = std::min(interp->FIRST, LINE_NUMBER);
        if (SOURCEOPEN_FLAG) {
          // RESET(SRC);
          SOURCE.seekg(0, std::ios::beg);
          for (k = 1; k < interp->FIRST; k++) {
            std::string line;
            std::getline(SOURCE, line);
            LINE = std::vector(line.begin(), line.end());
          }
          for (k = interp->FIRST; k <= interp->LAST; k++) {
            print("{}", k);
            print(" ");
            while (!eoln(SOURCE)) {
              CH = static_cast<char>(SOURCE.get());
              print("{}", CH);
            }
            // READLN(SOURCE);
            print("\n");
          }
        } else {
          print("There is no Program Source File open");
          print("\n");
        }
        break;
      case COMTYP::CLEAR:
        if (!SOURCEOPEN_FLAG) {
          print("Must Open a Program Source File First");
          goto L200;
        }
        remove_block();
        if (END_FLAG) {
          print("Break or Trace? ");
          read_line();
          if (END_FLAG)
            goto L200;
        }
        if (!in_word()) {
          print("Error in Command");
          goto L200;
        }
        if (
          strcmp(&ID[1], "BREAK         ") == 0 ||
          strcmp(&ID[1], "B             ") == 0) {
          strcpy(interp->PROMPT.data(), "From Line: ");
          k = num();
          if (k < 0 || k > LINE_NUMBER)
            print("Not Valid Line Number");
          else {
            for (i = 1; i <= BRKMAX; i++) {
              if (interp->BRKLINE.at(i) == k) {
                interp->BRKTAB.at(i) = -1;
                interp->BRKLINE.at(i) = -1;
                interp->NUMBRK--;
              }
            }
            print("\n");
          }
        } else if (
          strcmp(&ID[1], "TRACE         ") == 0 ||
          strcmp(&ID[1], "T             ") == 0) {
          strcpy(interp->PROMPT.data(), "Memory Lc: ");
          k = num();
          if (k < 0 || k > STMAX - 1)
            print(stdout, "Not Valid Memory Location");
          else {
            for (i = 1; i <= VARMAX; i++) {
              if (interp->TRCTAB.at(i).MEMLOC == k) {
                interp->TRCTAB.at(i).MEMLOC = -1;
                interp->NUMTRACE--;
              }
            }
            print("\n");
          }
        } else {
          print("Error in Command");
          goto L200;
        }
        break;
      case COMTYP::DISPLAYP: {
        if (!SOURCEOPEN_FLAG) {
          print("Must Open a Program Source File First");
          goto L200;
        }
        if (test_end()) {
          if (interp->NUMBRK > 0) {
            for (k = 1; k <= BRKMAX - 1; k++) {
              for (i = 1; i <= BRKMAX - k; i++) {
                if (interp->BRKLINE.at(i) > interp->BRKLINE.at(i + 1)) {
                  j = interp->BRKTAB.at(i);
                  interp->BRKTAB.at(i) = interp->BRKTAB.at(i + 1);
                  interp->BRKTAB.at(i + 1) = j;
                  j = interp->BRKLINE.at(i);
                  interp->BRKLINE.at(i) = interp->BRKLINE.at(i + 1);
                  interp->BRKLINE.at(i + 1) = j;
                }
              }
            }
            print("Breakpoints at Following Lines:\n");
            for (i = 1; i <= BRKMAX; i++) {
              if (interp->BRKLINE[i] >= 0)
                print("{}\n", interp->BRKLINE[i]);
            }
          }
          if (interp->NUMTRACE > 0) {
            print("\n");
            print("List of Trace Variables:\n");
            print("Variable Name\t\tMemory Location\n");
            for (i = 1; i <= VARMAX; i++) {
              if (interp->TRCTAB[i].MEMLOC != -1) {
                print(
                  "\t{}{}\n", interp->TRCTAB[i].NAME, interp->TRCTAB[i].MEMLOC);
              }
            }
          }
          if (interp->ALARMON) {
            print("\n");
            print("Alarm is Set at Time {}\n", interp->ALARMTIME);
          }
          if (topology != Symbol::SHAREDSY) {
            print("\n");
            print("Communication Link Delay: {}\n", interp->TOPDELAY);
            if (interp->CONGESTION)
              print("Congestion is On\n");
            else
              print("Congestion is Off\n");
          }
          if (mpi_mode)
            print("MPI Mode is On\n");
          if (interp->INPUTFNAME.empty()) {
            print(
              "File for Program Data Input: {}\n", interp->INPUTFNAME.string());
          }
          if (interp->OUTPUTFNAME.empty()) {
            print(
              "File for Program Data Output: {}\n",
              interp->OUTPUTFNAME.string());
          }
          std::println("Listing File for your Source Program: ");
          std::println(
            "{}",
            LISTDEF_FLAG ? interp->LISTDEFNAME.string()
                         : interp->LISTFNAME.string());
        }
        break;
      }
      case COMTYP::PROFILE: {
        if (!SOURCEOPEN_FLAG) {
          print("Must Open a Program Source File First");
          goto L200;
        }
        if (in_word()) {
          if (strcmp(&ID[1], "OFF           ") == 0)
            interp->PROFILEON = false;
          else {
            print("Error in Command");
            goto L200;
          }
        } else {
          interp->FIRSTPROC = -1;
          interp->PROFILEON = false;
          do {
            remove_block();
            if (END_FLAG) {
              print("Process Range: ");
              read_line();
            }
            get_range(interp->FIRSTPROC, interp->LASTPROC, interp->ERR);
          } while (interp->ERR || interp->FIRSTPROC == -1);
          if (interp->ERR) {
            print("Error in Range");
            goto L200;
          }
          if (interp->LASTPROC - interp->FIRSTPROC >= 40) {
            print("Maximum of 40 Processes Allowed in Profile");
            goto L200;
          }
          strcpy(interp->PROMPT.data(), "Time Step:");
          interp->PROSTEP = num();
          if (interp->PROSTEP < 0) {
            print("Not Valid Time Step");
            goto L200;
          }
          if (interp->PROSTEP < 10) {
            print("Minimum Time Step is 10");
            goto L200;
          }
          if (test_end())
            interp->PROFILEON = true;
        }
        print("\n");
        break;
      }
      case COMTYP::ALARM: {
        if (!SOURCEOPEN_FLAG) {
          print("Must Open a Program Source File First");
          goto L200;
        }
        if (in_word()) {
          if (strcmp(&ID[1], "OFF           ") == 0)
            interp->ALARMON = false;
          else {
            print("Error in Command");
            goto L200;
          }
        } else {
          strcpy(interp->PROMPT.data(), "  Time:   ");
          interp->ALARMTIME = num();
          if (interp->ALARMTIME < 0) {
            print("Not a Valid Alarm Time");
            goto L200;
          }
          interp->ALARMON = true;
          interp->ALARMENABLED = interp->ALARMTIME > interp->CLOCK;
        }
        print("\n");
        break;
      }
      case COMTYP::TRACE: {
        if (!SOURCEOPEN_FLAG) {
          print("Must Open a Program Source File First");
          goto L200;
        }
        get_var();
        if (interp->ERR)
          goto L200;
        if (interp->OBJ != Objects::Variable) {
          print("Can Only Trace on Variables");
          goto L200;
        }
        if (interp->TYP == Types::ARRAYS || interp->TYP == Types::RECS) {
          print("Cannot Trace a Whole Array or Structure");
          goto L200;
        }
        j = 0;
        for (i = 1; i <= VARMAX; i++) {
          if (interp->TRCTAB.at(i).MEMLOC == -1)
            j = i;
        }
        if (j == 0) {
          print("Too Many Trace Variables");
          goto L200;
        }
        strcpy(interp->TRCTAB.at(j).NAME.data(), &interp->VARNAME[1]);
        interp->TRCTAB.at(j).MEMLOC = interp->ADR;
        interp->NUMTRACE++;
        print("\n");
        break;
      }
      case COMTYP::WRVAR: {
        if (!SOURCEOPEN_FLAG) {
          print("Must Open a Program Source File First");
          goto L200;
        }
        get_var();
        if (interp->ERR)
          goto L200;
        if (strcmp(&ID[1], "MPI_BUFFER    ") == 0) {
          wrmpibuf(k);
          goto L200;
        }
        if (interp->OBJ == Objects::Constant) {
          switch (interp->TYP) {
            case Types::REALS: {
              print("{}\n", CONTABLE.at(interp->ADR));
              break;
            }
            case Types::INTS: {
              print("{}\n", interp->ADR);
              break;
            }
            case Types::BOOLS: {
              print("{}", interp->ADR == 0 ? "false" : "true");
              break;
            }
            case Types::CHARS: {
              print("{}\n", interp->ADR);
              break;
            }
            default: break;
          }
          goto L200;
        }
        interp->TEMP = false;
        interp->LINECNT = 0;
        if (interp->TYP == Types::ARRAYS) {
          print("Index Range: ");
          read_line();
          if (END_FLAG)
            goto L200;
          interp->FIRST = ATAB[interp->REF].LOW;
          interp->LAST = ATAB[interp->REF].HIGH;
          get_range(interp->FIRST, interp->LAST, interp->ERR);
          if (interp->ERR) {
            print("Error in Range");
            goto L200;
          }
          if (
            interp->FIRST < ATAB[interp->REF].LOW ||
            interp->LAST > ATAB[interp->REF].HIGH) {
            print("Array Bounds Exceeded");
            goto L200;
          }
          interp->ADR = interp->ADR + (interp->FIRST - ATAB[interp->REF].LOW) *
                                        ATAB[interp->REF].ELSIZE;
          for (i = interp->FIRST; i <= interp->LAST; i++) {
            if (!interp->TEMP) {
              print("{}", i);
              print("--> ");
              partwrite(
                10,
                ATAB[interp->REF].ELTYP,
                ATAB[interp->REF].ELREF,
                interp->ADR,
                interp->TEMP);
              interp->ADR = interp->ADR + ATAB[interp->REF].ELSIZE;
            }
          }
        } else
          partwrite(1, interp->TYP, interp->REF, interp->ADR, interp->TEMP);
        break;
      }
      case COMTYP::PSTATUS: {
        if (!interp->INITFLAG) {
          std::cout << "Must Run Program First" << '\n';
          goto L200;
        }
        i = 0;
        j = interp->NEXTID - 1;
        get_range(i, j, interp->TEMP);
        interp->TEMP = false;
        if (interp->TEMP) {
          std::cout << "Error in Range" << '\n';
          goto L200;
        }
        if (test_end()) {
          print("\tPROCESS #\tFUNCTION]t\tLINE "
                "NUMBER\tSTATUS\tPROCESSOR\n");
          interp->PTEMP = interp->ACPHEAD;
          do {
            if (
              interp->PTEMP->PDES->PID >= i && interp->PTEMP->PDES->PID <= j &&
              interp->PTEMP->PDES->STATE != State::TERMINATED) {
              if (interp->PTEMP->PDES->PID < 10)
                std::cout << "       " << interp->PTEMP->PDES->PID;
              else if (interp->PTEMP->PDES->PID < 100)
                std::cout << "      " << interp->PTEMP->PDES->PID;
              else if (interp->PTEMP->PDES->PID < 1000)
                std::cout << "     " << interp->PTEMP->PDES->PID;
              else if (interp->PTEMP->PDES->PID < 10000)
                std::cout << "    " << interp->PTEMP->PDES->PID;
              else if (interp->PTEMP->PDES->PID < 100000)
                std::cout << "   " << interp->PTEMP->PDES->PID;
              else
                std::cout << "  " << interp->PTEMP->PDES->PID;
              interp->LEVEL = LMAX;
              while (interp->PTEMP->PDES->DISPLAY[interp->LEVEL] == -1)
                interp->LEVEL = interp->LEVEL - 1;
              interp->H1 = interp->PTEMP->PDES->DISPLAY[interp->LEVEL];
              interp->INX = interp->S[interp->H1 + 4];
              while (TAB[interp->INX].name[1] == '*') {
                interp->H1 = interp->S[interp->H1 + 3];
                interp->INX = interp->S[interp->H1 + 4];
              }
              if (interp->LEVEL == 1)
                std::cout << "        " << "MAIN          ";
              else
                std::cout << "        " << TAB[interp->INX].name;
              if (interp->PTEMP->PDES->PID == interp->CURPR->PID)
                std::cout << std::setw(7) << get_lnum(interp->PTEMP->PDES->PC);
              else
                std::cout << std::setw(7)
                          << get_lnum(interp->PTEMP->PDES->PC - 1);
              std::cout << "      ";
              switch (interp->PTEMP->PDES->STATE) {
                case State::READY: std::cout << "READY   "; break;
                case State::RUNNING: std::cout << "RUNNING "; break;
                case State::BLOCKED: std::cout << "BLOCKED "; break;
                case State::DELAYED: std::cout << "DELAYED "; break;
                case State::SPINNING: std::cout << "SPINNING"; break;
                default: break;
              }
              std::cout << std::setw(6);
              std::println("{}", interp->PTEMP->PDES->PROCESSOR);
            }
            interp->PTEMP = interp->PTEMP->NEXT;
          } while (interp->PTEMP != interp->ACPHEAD);
        }
        break;
      }
      case COMTYP::PTIME: {
        if (!SOURCEOPEN_FLAG) {
          std::cout << "Must Open a Program Source File First" << '\n';
          goto L200;
        }
        remove_block();
        if (test_end()) {
          std::cout << "Since Start of Program:" << '\n';
          std::cout << "     Elapsed Time:  " << interp->CLOCK << '\n';
          std::cout << "     Number of Processors Used:  " << interp->USEDPROCS
                    << '\n';
          if (interp->CLOCK > 0) {
            interp->SPEED = interp->SEQTIME / interp->CLOCK;
            //                        std::cout << "     Speedup:  "
            //                        << interp->SPEED
            //                        << '\n'; std::cout << '\n';
            std::println("\tSpeedup:  {}\n", interp->SPEED);
          }
          if (interp->OLDTIME != 0) {
            interp->R1 = interp->CLOCK - interp->OLDTIME;
            interp->R2 = interp->SEQTIME - interp->OLDSEQTIME;
            std::cout << "Since Last Breakpoint:" << '\n';
            std::cout << "     Elapsed Time:  " << interp->R1 << '\n';
            if (interp->R1 > 0) {
              interp->SPEED = interp->R2 / interp->R1;
              //                            std::cout << " Speedup:  "
              //                            << interp->SPEED << '\n';
              std::println("     Speedup:  {}", interp->SPEED);
            }
          }
        }
        break;
      }
      case COMTYP::UTIL: {
        if (!SOURCEOPEN_FLAG) {
          std::cout << "Must Open a Program Source File First" << '\n';
          goto L200;
        }
        interp->FIRST = 0;
        interp->LAST = PMAX;
        get_range(interp->FIRST, interp->LAST, interp->ERR);
        if (interp->ERR) {
          std::cout << "Error in Processor Range" << '\n';
          goto L200;
        }
        if (test_end()) {
          interp->R1 = interp->CLOCK - interp->OLDTIME;
          std::cout << "                  PERCENT UTILIZATION" << '\n';
          std::cout << "PROCESSOR    SINCE START    SINCE LAST BREAK" << '\n';
          for (i = interp->FIRST; i <= interp->LAST; i++) {
            if (
              interp->PROCTAB.at(i).STATUS != Status::NEVERUSED ||
              interp->LAST < PMAX) {
              std::cout << std::setw(6) << i << "    " << std::setw(9)
                        << std::floor(
                             interp->PROCTAB.at(i).VIRTIME / interp->CLOCK *
                             100)
                        << "     ";
              if (interp->R1 > 0)
                interp->H1 =
                  std::floor(interp->PROCTAB.at(i).BRKTIME / interp->R1 * 100);
              else
                interp->H1 = 100;
              if (interp->H1 > 100)
                interp->H1 = 100;
              std::cout << std::setw(12) << interp->H1 << '\n';
            }
          }
        }
        break;
      }
      case COMTYP::CDELAY: {
        if (!SOURCEOPEN_FLAG) {
          std::cout << "Must Open a Program Source File First" << '\n';
          goto L200;
        }
        if (topology == Symbol::SHAREDSY) {
          std::cout << "Not allowed in shared memory multiprocessor" << '\n';
        } else {
          strcpy(interp->PROMPT.data(), "   Size:  ");
          k = num();
          if (k < 0)
            std::cout << "Invalid Communication Delay" << '\n';
          else
            interp->TOPDELAY = k;
        }
        break;
      }
      case COMTYP::CONGEST: {
        if (!SOURCEOPEN_FLAG) {
          std::cout << "Must Open a Program Source File First" << '\n';
          goto L200;
        }
        if (topology == Symbol::SHAREDSY) {
          std::cout << "Not allowed in shared memory multiprocessor" << '\n';
        } else {
          if (in_word()) {
            if (strcmp(&ID[1], "ON            ") == 0)
              interp->CONGESTION = true;
            else if (strcmp(&ID[1], "OFF           ") == 0)
              interp->CONGESTION = false;
            else
              std::cout << R"("ON" or "OFF" Missing)" << '\n';
          } else
            std::cout << R"("ON" or "OFF" Missing)" << '\n';
        }
        break;
      }
      case COMTYP::VARY: {
        if (in_word()) {
          if (strcmp(&ID[1], "ON            ") == 0)
            interp->VARIATION = true;
          else if (strcmp(&ID[1], "OFF           ") == 0)
            interp->VARIATION = false;
          else
            std::cout << R"("ON" or "OFF" Missing)" << '\n';
        } else
          std::cout << R"("ON" or "OFF" Missing)" << '\n';
        break;
      }
      case COMTYP::HELP: {
        std::cout << "The following commands are available:" << '\n';
        std::cout << '\n';
        std::cout << "*OPEN filename - Open and Compile your program "
                     "source file"
                  << '\n';
        std::cout << "*RUN - Initialize and run the program from the "
                     "beginning"
                  << '\n';
        std::cout << "*CLOSE - Close your program source file to "
                     "allow editing"
                  << '\n';
        std::cout << "*EXIT - Terminate the C* system" << '\n';
        std::cout << '\n';
        std::cout << "*LIST = filename - Change the source program "
                     "listing file "
                     "to \"filename\""
                  << '\n';
        std::cout << "*INPUT = filename - Use \"filename\" for "
                     "source program input"
                  << '\n';
        std::cout << "*OUTPUT = filename - Use \"filename\" for "
                     "source program output"
                  << '\n';
        std::cout << "*VIEW n:m - List program source lines n through m"
                  << '\n';
        std::cout << '\n';
        std::cout << "*BREAK n - Set a breakpoint at program line n" << '\n';
        std::cout << "*CLEAR BREAK n - Clear the breakpoint from line n"
                  << '\n';
        std::cout << "*CONTINUE - Continue program execution after a "
                     "breakpoint"
                  << '\n';
        std::cout << "*STATUS p:q - Display status of processes p through q"
                  << '\n';
        std::cout << "*STEP n - Continue execution for n lines in "
                     "current STEP process"
                  << '\n';
        std::cout << "*STEP PROCESS p - Set current STEP process number to p"
                  << '\n';
        std::cout << "*WRITE p name - Write out value of variable "
                     "\"name\" in process p"
                  << '\n';
        std::cout << "*TRACE p name - Make variable \"name\" a trace variable"
                  << '\n';
        std::cout << "*CLEAR TRACE m - Clear the trace from memory location m"
                  << '\n';
        std::cout << "*DISPLAY - Display list of breakpoints, trace "
                     "variables, "
                     "and alarm"
                  << '\n';
        std::cout << '\n';
        std::cout << "*TIME - Show elapsed time since start of "
                     "program and since "
                     "last break"
                  << '\n';
        std::cout << "*UTILIZATION p:q - Give utilization percentage for "
                     "processors p to q"
                  << '\n';
        std::cout << "*PROFILE p:q t - Set utilization profile to be "
                     "generated for"
                  << '\n';
        std::cout << "                    processes p to q every t time units"
                  << '\n';
        std::cout << "*PROFILE OFF - Turn off profile" << '\n';
        std::cout << "*ALARM t - Set alarm to go off at time t" << '\n';
        std::cout << "*ALARM OFF - Disable alarm from going off in future"
                  << '\n';
        std::cout << '\n';
        std::cout << "*VARIATION ON (OFF) - Turn variation option on (off)"
                  << '\n';
        std::cout << "*CONGESTION ON (OFF) - Turn congestion option on (off)"
                  << '\n';
        std::cout << "*DELAY d - Set the basic communication delay "
                     "to d time units"
                  << '\n';
        std::cout << "*RESET - Reset all debugger settings to "
                     "original default values"
                  << '\n';
        std::cout << "*MPI ON (OFF) - Turn Message-Passing Interface "
                     "mode on (off)"
                  << '\n';
        std::cout << "*VERSION - Display information about this version of C*"
                  << '\n';
        std::cout << "*SHORTCUTS - Show a list of shortcuts for "
                     "frequently used "
                     "commands"
                  << '\n';
        std::cout << "*HELP - Show a complete list of commands" << '\n';
        break;
      }
      case COMTYP::WRCODE: {
        if (!SOURCEOPEN_FLAG) {
          std::cout << "Must Open a Program Source File First" << '\n';
          goto L200;
        }
        interp->FIRST = 1;
        interp->LAST = LINE_NUMBER;
        get_range(interp->FIRST, interp->LAST, interp->ERR);
        if (interp->ERR) {
          std::cout << "Error in Range" << '\n';
          goto L200;
        }
        if (interp->LAST > LINE_NUMBER)
          interp->LAST = LINE_NUMBER;
        //                    std::cout << "LINE NUMBER" << "  OPCODE"
        //                    << "    X " << "  Y  " << '\n';
        std::print("LINE NUMBER  OPCODE    X    Y  \n");
        for (i = interp->FIRST; i <= interp->LAST; i++) {
          // std::cout << I << "    ";
          std::print("{}    ", i);
          if (LOCATION[i] == LOCATION[i + 1]) {
            // std::cout << '\n';
            std::print("\n");
          } else {
            j = LOCATION[i];
            std::print("{}{}{}\n", CODE.at(j).F, CODE.at(j).X, CODE.at(j).Y);
            for (j = LOCATION[i] + 1; j <= LOCATION[i + 1] - 1; j++) {
              // std::cout << CODE[J].F << " " << CODE[J].X << " " <<
              // CODE[J].Y
              // <<
              // '\n';
              std::print(
                "           {}{}{}\n",
                CODE.at(j).F,
                CODE.at(j).X,
                CODE.at(j).Y);
            }
          }
        }
        // std::cout << '\n';
        std::print("\n");
        break;
      }
      case COMTYP::STACK: {
        if (!SOURCEOPEN_FLAG) {
          std::cout << "Must Open a Program Source File First" << '\n';
          goto L200;
        }
        interp->FIRST = -1;
        interp->LAST = -1;
        get_range(interp->FIRST, interp->LAST, interp->ERR);
        if (interp->ERR || interp->FIRST < 0) {
          std::cout << "Error in Range" << '\n';
          goto L200;
        }
        std::cout << "STACK ADDRESS" << "   VALUE" << '\n';
        for (i = interp->FIRST; i <= interp->LAST; i++)
          std::cout << i << "   " << interp->S[i] << '\n';
        std::cout << '\n';
        break;
      }
      case COMTYP::OPENF: {
        if (SOURCEOPEN_FLAG) {
          std::println("A file is already open. "
                       "You must close the open file before continuing.\n");
          goto L200;
        }
        in_filename();
        SOURCE.open(filename);
        if (!SOURCE) {
          std::println(
            "Couldn't open the program source file: {}\n", filename.string());
          goto L200;
        }
        LISTFILE.open(interp->LISTFNAME);
        if (!LISTFILE) {
          std::println(
            "Couldn't open the program listing file: {}",
            interp->LISTFNAME.string());
          SOURCE.close();
          goto L200;
        }
        // fclose(LISTFILE);
        if (INPUTFILE_FLAG) {
          std::ifstream inp{interp->INPUTFNAME};
          if (!inp) {
            // std::cout << "Cannot Open the Input File " <<
            // INPUTFNAME <<
            // '\n';
            std::print(
              "Couldn't open the input file: {}\n",
              interp->INPUTFNAME.string());
            SOURCE.close();
            goto L200;
          }
          inp.close();
        }
        if (OUTPUTFILE_FLAG) {
          std::ofstream outp{interp->OUTPUTFNAME};
          if (!outp) {
            std::println(
              "Couldn't open the output file: {}",
              interp->OUTPUTFNAME.string());

            SOURCE.close();
            std::cout << '\n';
            goto L200;
          }
          outp.close();
        }
        //                    showCodeList(true);
        //                    showConsoleList(true);
        initialize_compiler();
        strcpy(program_name.data(), "PROGRAM       ");
        main_func = -1;

        BLOCK(interp, declaration_set, false, 1, tab_index);
        if (symbol != Symbol::EOFSY)
          error(22);
        if (BTAB[2].VSIZE + WORKSIZE > STMAX)
          error(49);
        if (main_func < 0)
          error(100);
        else {
          if (mpi_mode) {
            emit(9);
            emit(18, main_func);
            emit(19, BTAB[TAB[main_func].reference].PSIZE - 1);
            emit(69);
          }
          emit(18, main_func);
          emit(19, BTAB[TAB[main_func].reference].PSIZE - 1);
        }
        emit(31);
        interp->ENDLOC = line_count;
        LOCATION[LINE_NUMBER + 1] = line_count;
        std::cout << '\n';
        //                    dumpCode();
        if (errors.none()) {
          // std::cout << '\n';
          // std::cout << "Program Successfully Compiled" << '\n';
          std::print("\n{} Successfully Compiled\n", filename.string());
          SOURCEOPEN_FLAG = true;
          if (code_list)
            displays->dump_code();
          if (block_list)
            displays->dump_blocks();
          if (symbol_list)
            displays->dump_symbols();
          if (array_list)
            displays->dump_arrays();
          if (real_list)
            displays->dump_reals();
        } else {
          error_message();
          print("\n");
          SOURCE.close();
          println("PROGRAM SOURCE FILE IS NOW CLOSED TO ALLOW EDITING");
        }
        print("\n");
        println(
          "To view a complete program listing,"
          "please refer to the listing file at: {}",
          interp->LISTFNAME.string());
        LISTFILE.close();
        break;
      }
      case COMTYP::CLOSEF: {
        if (SOURCEOPEN_FLAG) {
          SOURCEOPEN_FLAG = false;
          // std::cout << '\n';
          // std::cout << "You can now modify your Program Source
          // File" <<
          // '\n';
          print("\nYou can now modify your Program Source File\n");
          for (i = 1; i <= BRKMAX; ++i) {
            interp->BRKTAB.at(i) = -1;
            interp->BRKLINE.at(i) = -1;
          }
          interp->NUMBRK = 0;
          interp->NUMTRACE = 0;
          for (i = 1; i <= VARMAX; ++i)
            interp->TRCTAB.at(i).MEMLOC = -1;
          if (OUTPUTOPEN_FLAG) {
            OUTPUT.close();
            OUTPUTOPEN_FLAG = false;
          }
          if (INPUTOPEN_FLAG) {
            INPUT.close();
            INPUTOPEN_FLAG = false;
          }
        } else
          std::cout << "No Program Source File is open" << '\n';
        std::cout << '\n';
        break;
      }
      case COMTYP::INPUTF: {
        eq_in_filename();
        if (INPUTOPEN_FLAG)
          INPUT.close();
        if (filename.empty()) {
          INPUTFILE_FLAG = false;
          INPUTOPEN_FLAG = false;
        } else {
          if (std::ifstream inp{filename}; !inp)
            std::println("Can't open input file: {}\n", filename.string());
          else {
            inp.close();
            INPUTFILE_FLAG = true;
            interp->INPUTFNAME = filename;
            INPUTOPEN_FLAG = false;
            MUST_RUN_FLAG = true;
          }
        }
        break;
      }
      case COMTYP::OUTPUTF: {
        eq_in_filename();
        if (OUTPUTOPEN_FLAG)
          OUTPUT.close();
        if (filename.empty()) {
          OUTPUTFILE_FLAG = false;
          OUTPUTOPEN_FLAG = false;
        } else {
          if (std::ofstream outp{filename}; !outp)
            std::println("Can't open output file: {}\n", filename.string());
          else {
            outp.close();
            OUTPUTFILE_FLAG = true;
            interp->OUTPUTFNAME = filename;
            OUTPUTOPEN_FLAG = false;
            MUST_RUN_FLAG = true;
          }
        }
        break;
      }
      case COMTYP::LIST: {
        eq_in_filename();
        if (filename.empty()) {
          LISTDEF_FLAG = true;
          if (std::ofstream lis{interp->LISTDEFNAME}; !lis) {
            std::println(
              "Can't open default listing file: {}\n", filename.string());
          } else
            lis.close();
        } else {
          if (std::ofstream lis{filename}; !lis)
            std::println("Can't open listing file: {}\n", filename.string());
          else {
            lis.close();
            LISTDEF_FLAG = false;
            interp->LISTFNAME = filename;
          }
        }
        break;
      }
      case COMTYP::RESETP: {
        init_interpreter();
        if (!SOURCEOPEN_FLAG)
          mpi_mode = false;
        std::println("Debugger settings reset to default values.");
        break;
      }
      case COMTYP::ERRC: {
        std::println("INVALID COMMAND : {} :", &ID[1]);
        break;
      }
      case COMTYP::SHORT: {
        std::print("The following Command Keywords may be abbreviated using"
                   "only the first letter:"
                   "\n"
                   "\n"
                   "\t[aA]LARM\n"
                   "\t[bB]REAK\n"
                   "\t[cC]ONTINUE\n"
                   "\t[dD]ISPLAY\n"
                   "\t[eE]XIT\n"
                   "\t[hH]ELP\n"
                   "\t[oO]PEN\n"
                   "\t[pP]ROFILE\n"
                   "\t[rR]UN\n"
                   "\t[sS]TEP\n"
                   "\t[tT]RACE\n"
                   "\t[uU]TILIZATION\n"
                   "\t[vV]IEW\n"
                   "\t[wW]RITE\n"
                   "\n");
        break;
      }
      case COMTYP::MPI: {
        if (SOURCEOPEN_FLAG) {
          std::cout << "You Must Close Your Source Program before "
                       "changing MPI Mode"
                    << '\n';
        } else {
          if (in_word()) {
            if (strcmp(&ID[1], "ON            ") == 0)
              mpi_mode = true;
            else if (strcmp(&ID[1], "OFF           ") == 0)
              mpi_mode = false;
            else
              std::cout << R"("ON" or "OFF" Missing)" << '\n';
          } else
            std::cout << R"("ON" or "OFF" Missing)" << '\n';
        }
        break;
      }
      case COMTYP::VERSION: {
        std::cout << '\n';
        std::cout << "  C* COMPILER AND PARALLEL COMPUTER SIMULATION SYSTEM"
                  << '\n';
        std::cout << "                      VERSION 2.2c++" << '\n';
        std::cout << '\n';
        std::cout << "The C* programming language was created from "
                     "the C programming"
                  << '\n';
        std::cout << "language by adding a few basic parallel "
                     "programming primitives."
                  << '\n';
        std::cout << "However, in this version 2.2 of the C* "
                     "compiler, the following"
                  << '\n';
        std::cout << "features of standard C are not implemented:" << '\n';
        std::cout << '\n';
        std::cout << "1. Union Data Types" << '\n';
        std::cout << "2. Enumeration Types" << '\n';
        std::cout << "3. Bitwise Operators:" << '\n';
        std::cout << "      left shift, right shift, " << '\n';
        std::cout << "      bitwise complement, bitwise and, bitwise or"
                  << '\n';
        std::cout << "4. Conditional Operator (?)" << '\n';
        std::cout << "      Example:  i >= 0 ? i : 0" << '\n';
        //                std::cout << "5. Compound Assignment
        //                Operators:" <<
        //                '\n'; std::cout << "      +=  -=  *=  /= %="
        //                <<
        //                '\n';
        std::cout << '\n';
        std::cout << '\n';
        break;
      }
      default: break;
    }
  L200:;
  } while (interp->COMMLABEL != COMTYP::EXIT2);
  free(interp->S);
  free(interp->SLOCATION);
  free(interp->STARTMEM);
  free(interp->RS);
  free(interp->LINK);
  free(interp->VALUE);
  free(interp->RVALUE);
  free(interp->DATE);
}
} // namespace cs
