// #include <iostream>
#include "cs_compile.h"
#include "cs_errors.h"
#include "cs_global.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iomanip>
#include <iostream>
#define EXPORT_CS_INTERPRET
#include "cs_PreBuffer.h"
#include "cs_interpret.h"

#define COMMLINKTIME 10
#define TRU 1
#define FALS 0
#define COMMAX 30

namespace Cstar {
using std::print;

extern void dumpCode();
extern void dumpSymbols();
extern void dumpArrays();
extern void dumpBlocks();
extern void dumpReals();
extern void dumpChans(InterpLocal *);
extern void dumpStkfrms(InterpLocal *);
extern void dumpProctab(InterpLocal *);
extern void dumpDeadlock(InterpLocal *);
extern void dumpActive(InterpLocal *);
extern void showCodeList(bool flg);
extern void showConsoleList(bool flg);
extern void EXECUTE(InterpLocal *il);
extern void ENTERBLOCK();
extern bool INCLUDEDIRECTIVE();
extern void BLOCK(InterpLocal *, SymbolSet, bool, int, int);
extern void NEXTCHAR();
extern void WRMPIBUF(InterpLocal *, int PNUM);
void QUALIFY(InterpLocal *);
void PARTWRITE(InterpLocal *, int START, Types PTYP, Index PREF, int PADR,
               bool &DONE);
static void READLINE();
int NUM(InterpLocal *);
void REMOVEBL();
bool INWORD();
int FINDFRAME(InterpLocal *, int);
int MAINFUNC;
ALFA PROGNAME;
char FNAME[FILMAX + 1];
double XR;
static bool code_list = false;
static bool block_list = false;
static bool symbol_list = false;
static bool array_list = false;
static bool real_list = false;

void showCodeList(bool flg) { code_list = flg; }
void showBlockList(bool flg) { block_list = flg; }
void showSymbolList(bool flg) { symbol_list = flg; }
void showArrayList(bool flg) { array_list = flg; }
void showRealList(bool flg) { real_list = flg; }
// #define BRKMAX 100
// #define VARMAX 100
// #define STMAX 1000

ALFA COMTAB[COMMAX + 1] = {
    "RUN           ", "CONTINUE      ", "STEP          ", "EXIT          ",
    "BREAK         ", "TRACE         ", "LIST          ", "CLEAR         ",
    "WRITE         ", "DISPLAY       ", "HELP          ", "PROFILE       ",
    "ALARM         ", "CODE          ", "STACK         ", "STATUS        ",
    "TIME          ", "UTILIZATION   ", "DELAY         ", "CONGESTION    ",
    "VARIATION     ", "OPEN          ", "CLOSE         ", "INPUT         ",
    "OUTPUT        ", "VIEW          ", "SHORTCUTS     ", "RESET         ",
    "MPI           ", "VERSION       "};

ALFA ABBREVTAB[COMMAX + 1] = {
    "R             ", "C             ", "S             ", "E             ",
    "B             ", "T             ", "NONE          ", "NONE          ",
    "W             ", "D             ", "H             ", "P             ",
    "A             ", "NONE          ", "NONE          ", "NONE          ",
    "NONE          ", "U             ", "NONE          ", "NONE          ",
    "NONE          ", "O             ", "NONE          ", "NONE          ",
    "NONE          ", "V             ", "NONE          ", "NONE          ",
    "NONE          ", "NONE          "};

COMTYP COMJMP[COMMAX + 1] = {
    COMTYP::RUNP,    COMTYP::CONT,     COMTYP::STEP,   COMTYP::EXIT2,
    COMTYP::BREAKP,  COMTYP::TRACE,    COMTYP::LIST,   COMTYP::CLEAR,
    COMTYP::WRVAR,   COMTYP::DISPLAYP, COMTYP::HELP,   COMTYP::PROFILE,
    COMTYP::ALARM,   COMTYP::WRCODE,   COMTYP::STACK,  COMTYP::PSTATUS,
    COMTYP::PTIME,   COMTYP::UTIL,     COMTYP::CDELAY, COMTYP::CONGEST,
    COMTYP::VARY,    COMTYP::OPENF,    COMTYP::CLOSEF, COMTYP::INPUTF,
    COMTYP::OUTPUTF, COMTYP::VIEW,     COMTYP::SHORT,  COMTYP::RESETP,
    COMTYP::MPI,     COMTYP::VERSION};

PreBuffer *prebuf;
void QUALIFY(InterpLocal *il) {
  int J;
L50:;
  while (CH == '[' || CH == '-' || CH == '.') {
    REMOVEBL();
    if (CH == '[') {
      if (il->TYP != Types::ARRAYS) {
        std::cout << "INDEX OF NONARRAY TYPE" << std::endl;
        il->ERR = true;
        goto L50;
      } else {
        do {
          NEXTCHAR();
          il->INX = NUM(il);
          if (il->INX == -1 || il->TYP != Types::ARRAYS) {
            std::cout << "Error in Array Index" << std::endl;
            il->ERR = true;
            goto L50;
          }
          if (ATAB[il->REF].INXTYP != Types::INTS) {
            std::cout << "Only Integer Indices Allowed" << std::endl;
            il->ERR = true;
            goto L50;
          }
          J = ATAB[il->REF].LOW;
          if (il->INX < J) {
            std::cout << "Index Too Low" << std::endl;
            il->ERR = true;
            goto L50;
          }
          il->ADR = il->ADR + (il->INX - J) * ATAB[il->REF].ELSIZE;
          il->TYP = ATAB[il->REF].ELTYP;
          il->REF = ATAB[il->REF].ELREF;
          REMOVEBL();
        } while (CH == ',');
        if (CH == ']') {
          NEXTCHAR();
        }
      }
    } else if (CH == '.') {
      NEXTCHAR();
      if (il->TYP != Types::RECS || !INWORD()) {
        std::cout << "Error in Record Component" << std::endl;
        il->ERR = true;
        goto L50;
      } else {
        atab_index = il->REF;
        strcpy(TAB[0].name, ID);
        while (TAB[atab_index].name != ID) {
          atab_index = TAB[atab_index].link;
        }
        if (atab_index == 0) {
          std::cout << "Struct Component Not Found" << std::endl;
          il->ERR = true;
          goto L50;
        } else {
          il->ADR = il->ADR + TAB[atab_index].address;
          il->TYP = TAB[atab_index].types;
          il->REF = TAB[atab_index].reference;
        }
      }
    } else if (CH == '-') {
      NEXTCHAR();
      if (CH != '>') {
        std::cout << "Invalid Character in Command" << std::endl;
        il->ERR = true;
      } else if (il->TYP != Types::PNTS) {
        std::cout << "Nonpointer Variable Cannot Have ->" << std::endl;
        il->ERR = true;
      } else if (il->S[il->ADR] == 0) {
        std::cout << "Pointer is NULL" << std::endl;
        il->ERR = true;
        goto L50;
      } else {
        il->TYP = CTAB[il->REF].ELTYP;
        il->REF = CTAB[il->REF].ELREF;
        il->ADR = il->S[il->ADR];
        CH = '.';
      }
    }
  }
  std::cout << std::endl;
}

bool INCRLINE(InterpLocal *il) {
  bool rtn;
  rtn = false;
  il->LINECNT = il->LINECNT + 1;
  if (il->LINECNT > 200) {
    std::cout << std::endl;
    std::cout << "  Do you want to see more MPI Buffer Data? (Y or N): ";
    char input;
    input = (char)std::getc(stdin);
    if (!INWORD()) {
      rtn = true;
    } else if (input == 'Y') {
      il->LINECNT = 0;
    } else {
      rtn = true;
    }
  }
  return rtn;
}
struct PartwriteLocal {
  InterpLocal *il;
  int START;
  Types PTYP;
  Index PREF;
  int PADR;
  bool DONE;
};

void ARRAYWRITE(struct PartwriteLocal *pl) {
  int I, J, STADR;
  InterpLocal *il;
  il = pl->il;
  STADR = pl->PADR;
  std::cout << "ARRAY" << std::endl;
  il->LINECNT = il->LINECNT + 1;
  for (I = ATAB[pl->PREF].LOW; I <= ATAB[pl->PREF].HIGH; I++) {
    if (!pl->DONE) {
      for (J = 1; J <= pl->START - 1; J++) {
        std::cout << " ";
      }
      std::cout << I << "-->";
      PARTWRITE(il, pl->START + 9, ATAB[pl->PREF].ELTYP, ATAB[pl->PREF].ELREF,
                STADR, pl->DONE);
      STADR = STADR + ATAB[pl->PREF].ELSIZE;
    }
  }
}

void RECORDWRITE(PartwriteLocal *pl) {
  int SAVEREF[20 + 1];
  int NUMREF, I, J, K, L, STADR;
  ALFA TNAME;
  InterpLocal *il;
  il = pl->il;
  NUMREF = 2;
  J = pl->PREF;
  SAVEREF[1] = pl->PREF;
  while (TAB[J].link != 0) {
    J = TAB[J].link;
    SAVEREF[NUMREF] = J;
    NUMREF = NUMREF + 1;
    if (NUMREF > 20) {
      std::cout << "Struct Has Too Many Components" << std::endl;
      goto L90;
    }
  }
  STADR = pl->PADR;
  std::cout << "  STRUCT" << std::endl;
  il->LINECNT = il->LINECNT + 1;
  for (I = NUMREF - 1; I >= 1; I--) {
    for (J = 1; J <= pl->START - 5; J++) {
      std::cout << " ";
    }
    strcpy(TNAME, TAB[SAVEREF[I]].name);
    for (K = 0; K <= ALNG - 1; K++) {
      if (TNAME[ALNG] == ' ') {
        for (L = ALNG; L >= 2; L--) {
          TNAME[L] = TNAME[L - 1];
        }
        TNAME[1] = ' ';
      }
    }
    std::cout << TNAME << ": ";
    PARTWRITE(il, pl->START + 12, il->TYP, il->REF, STADR, pl->DONE);
    if (pl->DONE) {
      goto L90;
    }
    STADR = STADR + TAB[SAVEREF[I]].size;
  }
L90:
  std::cout << std::endl;
}

void CHANNELWRITE(PartwriteLocal *pl) {
  int CNUM, PNT, I, TSADR;
  bool EMPTY;
  InterpLocal *il = pl->il;
  CNUM = il->S[pl->PADR];
  EMPTY = CNUM == 0;
  if (EMPTY) {
    std::cout << "STREAM IS EMPTY" << std::endl;
    pl->DONE = INCRLINE(il);
  } else {
    if (!EMPTY) {
      EMPTY = il->CHAN[CNUM].SEM == 0;
    }
    if (EMPTY) {
      std::cout << "STREAM IS EMPTY" << std::endl;
      pl->DONE = INCRLINE(il);
    } else {
      PNT = il->CHAN[CNUM].HEAD;
      std::cout << "STREAM" << std::endl;
      il->LINECNT = il->LINECNT + 1;
      while (PNT != 0 && !pl->DONE) {
        for (I = 1; I <= pl->START - 1; I++) {
          std::cout << " ";
        }
        if (il->DATE[PNT] <= il->CLOCK + TIMESTEP ||
            Topology == Symbol::SHAREDSY) {
          std::cout << "     >";
        } else {
          std::cout << "   **>";
        }
        switch (ATAB[pl->PREF].ELTYP) {
        case Types::RECS:
        case Types::ARRAYS:
          PARTWRITE(il, pl->START + 7, ATAB[pl->PREF].ELTYP,
                    ATAB[pl->PREF].ELREF, il->VALUE[PNT], pl->DONE);
          break;
        case Types::REALS:
          std::cout << il->RVALUE[PNT] << std::endl;
          break;
        case Types::INTS:
          std::cout << il->VALUE[PNT] << std::endl;
          break;
        case Types::BOOLS:
          if (il->VALUE[PNT] == 0) {
            std::cout << "FALSE" << std::endl;
          } else {
            std::cout << "TRUE" << std::endl;
          }
          break;
        case Types::CHARS:
          std::cout << il->VALUE[PNT] << std::endl;
          break;
        case Types::PNTS:
          TSADR = FINDFRAME(il, 1);
          il->S[TSADR] = il->VALUE[PNT];
          PARTWRITE(il, pl->START + 7, ATAB[pl->PREF].ELTYP,
                    ATAB[pl->PREF].ELREF, TSADR, pl->DONE);
          break;
        default:
          break;
        }
        PNT = il->LINK[PNT];
        Types typs = ATAB[pl->PREF].ELTYP;
        if (typs == Types::REALS || typs == Types::INTS ||
            typs == Types::BOOLS || typs == Types::CHARS) {
          pl->DONE = INCRLINE(il);
        }
      }
    }
  }
}

void SCALARWRITE(PartwriteLocal *pl) {
  InterpLocal *il = pl->il;
  switch (pl->PTYP) {
  case Types::INTS:
    std::cout << il->S[pl->PADR] << std::endl;
    break;
  case Types::REALS:
    if (il->S[pl->PADR] == 0) {
      std::cout << 0 << std::endl;
    } else {
      std::cout << il->RS[pl->PADR] << std::endl;
    }
    break;
  case Types::BOOLS:
    if (il->S[pl->PADR] == 0) {
      std::cout << "FALSE" << std::endl;
    } else {
      std::cout << "TRUE" << std::endl;
    }
    break;
  case Types::CHARS:
    std::cout << (char)il->S[pl->PADR] << std::endl;
    break;
  case Types::LOCKS:
    if (il->S[pl->PADR] == 0) {
      std::cout << "UNLOCKED" << std::endl;
    } else {
      std::cout << "LOCKED" << std::endl;
    }
    break;
  default:
    break;
  }
  pl->DONE = INCRLINE(il);
}

void PNTWRITE(PartwriteLocal *pl) {
  int I;
  InterpLocal *il = pl->il;
  if (il->S[pl->PADR] == 0) {
    std::cout << "NULL POINTER" << std::endl;
  } else {
    std::cout << "POINTER" << std::endl;
    il->LINECNT = il->LINECNT + 1;
    for (I = 1; I <= pl->START; I++) {
      std::cout << " ";
    }
    std::cout << "  TO   ";
    PARTWRITE(il, pl->START + 8, CTAB[pl->PREF].ELTYP, CTAB[pl->PREF].ELREF,
              il->S[pl->PADR], pl->DONE);
  }
}

void PARTWRITE(InterpLocal *il, int START, Types PTYP, Index PREF, int PADR,
               bool &DONE) {
  struct PartwriteLocal pl;
  pl.il = il;
  pl.START = START;
  pl.PTYP = PTYP;
  pl.PADR = PADR;
  pl.PREF = PREF;
  pl.DONE = DONE;
  if (!DONE) {
    if (START > 70) {
      print(STDOUT, "\nData Has Too Many Levels of Structure\n");
      return;
    }
    switch (PTYP) {
    case Types::ARRAYS:
      ARRAYWRITE(&pl);
      break;
    case Types::RECS:
      RECORDWRITE(&pl);
      break;
    case Types::CHANS:
      CHANNELWRITE(&pl);
      break;
    case Types::PNTS:
      PNTWRITE(&pl);
      break;
    case Types::INTS:
    case Types::REALS:
    case Types::BOOLS:
    case Types::CHARS:
    case Types::LOCKS:
      SCALARWRITE(&pl);
      break;
    default:
      break;
    }
  }
}

void WRMPIBUF(InterpLocal *il, int PNUM) {
  int PNT, I, BADR, COUNT, DATATYPE;
  bool EMPTY;
  EMPTY = il->CHAN[PNUM].SEM == 0;
  if (EMPTY) {
    std::cout << "MPI BUFFER IS EMPTY" << std::endl;
  } else {
    PNT = il->CHAN[PNUM].HEAD;
    il->LINECNT = 0;
    while (PNT != 0) {
      std::cout << std::endl;
      if (INCRLINE(il)) {
        goto L49;
      }
      if (il->DATE[PNT] <= il->CLOCK + TIMESTEP) {
        std::cout << "   >";
      } else {
        std::cout << " **>";
      }
      BADR = il->VALUE[PNT];
      COUNT = il->S[BADR];
      DATATYPE = il->S[BADR + 1];
      std::cout << "  Source: " << il->S[BADR + 2]
                << "  Tag: " << il->S[BADR + 3] << "  Datatype: ";
      switch (DATATYPE) {
      case 1:
        std::cout << "int";
        break;
      case 2:
        std::cout << "float";
        break;
      case 3:
        std::cout << "char";
        break;
      }
      if (INCRLINE(il)) {
        goto L49;
      }
      if (INCRLINE(il)) {
        goto L49;
      }
      std::cout << "         DATA: ";
      if (COUNT == 1) {
        switch (DATATYPE) {
        case 1:
          std::cout << il->S[BADR + 4] << std::endl;
          break;
        case 2:
          std::cout << il->RS[BADR + 4] << std::endl;
          break;
        case 3:
          std::cout << (char)il->S[BADR + 4] << std::endl;
          break;
        }
      } else {
        std::cout << std::endl;
        if (INCRLINE(il)) {
          goto L49;
        }
        for (I = BADR + 4; I <= BADR + 3 + COUNT; I++) {
          std::cout << "             " << I - BADR - 3 << "--> ";
          switch (DATATYPE) {
          case 1:
            std::cout << il->S[I] << std::endl;
            break;
          case 2:
            std::cout << il->RS[I] << std::endl;
            break;
          case 3:
            std::cout << (char)il->S[I] << std::endl;
            break;
          }
          if (INCRLINE(il)) {
            goto L49;
          }
        }
      }
      PNT = il->LINK[PNT];
    }
  }
L49:;
}
InterpLocal InterpretLocal;

void INITCOMMANDS() {
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
////        if ((*SRC).eof())
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
bool eoln(FILE *inf) {
  int ch;
  bool eln = false;
  ch = fgetc(inf);
  if (ch >= 0) {
    if (ch == '\r') {
      ch = fgetc(inf);
      eln = true;
    }
    if (ch == '\n') {
      // ch = fgetc(inf);
      eln = true;
    } else if (ch >= 0)
      ungetc(ch, inf);
    else
      eln = true;
  } else
    eln = true;
  return eln;
}
void INITINTERP(InterpLocal *il) {
  il->INITFLAG = false;
  il->PROFILEON = false;
  il->ALARMON = false;
  il->ALARMENABLED = false;
  il->CONGESTION = false;
  il->VARIATION = false;
  il->TOPDELAY = COMMLINKTIME;
  for (int I = 1; I <= BRKMAX; I++) {
    il->BRKTAB[I] = -1;
    il->BRKLINE[I] = -1;
  }
  il->NUMBRK = 0;
  il->NUMTRACE = 0;
  for (int I = 1; I <= VARMAX; I++) {
    il->TRCTAB[I].MEMLOC = -1;
  }
  strcpy(il->LISTDEFNAME, "LISTFILE.TXT");
  // LIS = fopen(LISTDEFNAME, "w");   Pascal ASSIGN
  strcpy(il->LISTFNAME, il->LISTDEFNAME);
  LISTDEF = true;
  INPUTFILE = false;
  OUTPUTFILE = false;
  il->INPUTFNAME[0] = '\0';
  il->OUTPUTFNAME[0] = '\0';
  INPUTOPEN = false;
  OUTPUTOPEN = false;
  MUSTRUN = false;
}
void GETVAR(InterpLocal *il) {
  PROCPNT KDES;
  int I, J;
  il->ERR = false;
  strcpy(il->PROMPT, "Process:  ");
  il->K = NUM(il);
  if (il->K < 0) {
    std::cout << "Invalid Process Number" << std::endl;
    il->ERR = true;
    goto L70;
  }
  if (!il->INITFLAG) {
    std::cout << "Must Run Program First" << std::endl;
    il->ERR = true;
    goto L70;
  }
  if (il->K == 0) {
    KDES = il->ACPHEAD->PDES;
    il->TEMP = true;
  } else {
    il->PTEMP = il->ACPHEAD;
    il->TEMP = false;
    do {
      if (il->PTEMP->PDES->PID == il->K &&
          il->PTEMP->PDES->STATE != PROCESSDESCRIPTOR::STATE::TERMINATED) {
        il->TEMP = true;
        KDES = il->PTEMP->PDES;
      } else {
        il->PTEMP = il->PTEMP->NEXT;
      }
    } while (!il->TEMP && il->PTEMP != il->ACPHEAD);
  }
  if (!il->TEMP) {
    std::cout << "Invalid Process Number" << std::endl;
    std::cout << "Type Status Command to Get List of Processes" << std::endl;
    il->ERR = true;
    goto L70;
  }
  REMOVEBL();
  while (ENDFLAG) {
    std::cout << "Variable: ";
    //        std::string input;
    //        getline(std::cin, input);
    READLINE();
    REMOVEBL();
  }
  for (J = 1; J <= NAMELEN; J++) {
    I = CC + J - 1;
    if (I <= LL) {
      il->VARNAME[J] = line[I];
    } else {
      il->VARNAME[J] = ' ';
    }
  }
  if (!INWORD()) {
    std::cout << "Error in Variable Name" << std::endl;
    il->ERR = true;
    goto L70;
  }
  il->LEVEL = LMAX;
  while (KDES->DISPLAY[il->LEVEL] == -1) {
    il->LEVEL = il->LEVEL - 1;
  }
  I = il->LEVEL;
  strcpy(TAB[0].name, &ID[1]);
  do {
    if (I > 1) {
      il->VAL = KDES->DISPLAY[I];
      il->INX = il->S[il->VAL + 4];
      J = BTAB[TAB[il->INX].reference].LAST;
    } else {
      J = BTAB[2].LAST;
    }
    while (strcmp(TAB[J].name, &ID[1]) != 0) {
      J = TAB[J].link;
    }
    I = I - 1;
  } while (I >= 1 && J == 0);
  if (J == 0) {
    if (strcmp(&ID[1], "MPI_BUFFER    ") == 0 && mpi_mode) {
      goto L70;
    } else {
      std::cout << "Name Not Found" << std::endl;
      il->ERR = true;
      goto L70;
    }
  }
  il->LEVEL = I + 1;
  il->TYP = TAB[J].types;
  il->REF = TAB[J].reference;
  il->OBJ = TAB[J].object;
  if (il->OBJ != OBJECTS::KONSTANT && il->OBJ != OBJECTS::VARIABLE) {
    std::cout << "Name Not Found" << std::endl;
    il->ERR = true;
    goto L70;
  }
  if (il->OBJ != OBJECTS::KONSTANT) {
    il->ADR = TAB[J].address + KDES->DISPLAY[il->LEVEL];
    if (!TAB[J].normal) {
      il->ADR = il->S[il->ADR];
    }
    if (il->TYP == Types::INTS || il->TYP == Types::REALS ||
        il->TYP == Types::BOOLS || il->TYP == Types::CHARS) {
      for (I = KDES->BASE; I <= KDES->BASE + KDES->FORLEVEL - 1; I++) {
        if (il->SLOCATION[I] == il->ADR && KDES->PC >= std::lround(il->RS[I])) {
          il->ADR = I;
        }
      }
    }
  } else {
    il->ADR = TAB[J].address;
  }
  QUALIFY(il);
  if (il->ERR) {
    goto L70;
  }
  REMOVEBL();
  if (!ENDFLAG) {
    std::cout << "Error in Variable Name" << std::endl;
    il->ERR = true;
    goto L70;
  }
L70:;
  std::cout << std::endl;
}
void READLN(FILE *input) {
  int ch;
  // Pascal reads optional values, but reads through any of CRLF, LF, CR, end
  // styles handled in eoln()
  //        size_t reclen = FILMAX + 1;
  //        char *buffer = FNAME;
  //        getline(&buffer, &reclen, input);
  while (!eoln(input)) {
    ch = fgetc(input);
  }
}
double RANDGEN() {
  const double lB = 100001.0;
  const double lC = 1615;
  XR = XR * lC;
  XR = XR - std::floor(XR / lB) * lB;
  return XR / lB;
}
void CONVERT() {
  if (CH >= 'a' && CH <= 'z') {
    CH = (char)(CH - ' ');
  }
}

static void READLINE() {
  LL = 0;
  while (!eoln(STDIN) && (LL <= LLNG)) {
    // CH = (*INP).get();
    CH = (char)fgetc(STDIN);
    if (CH == '\x08') {
      if (LL > 0) {
        LL = LL - 1;
      }
    } else {
      CONVERT();
      LL = LL + 1;
      line[LL] = CH;
    }
  }
  // READLN(STDIN);
  if (LL > 0) {
    ENDFLAG = false;
    CC = 1;
    CH = line[CC];
  } else {
    ENDFLAG = true;
    CH = ' ';
    CC = 0;
  }
}

static void FREADLINE() {
  LL = 0;
  while (!eoln(STDIN) && (LL <= LLNG)) {
    // CH = (*INP).get();
    CH = (char)fgetc(STDIN);
    if (CH == '\x08') {
      if (LL > 0) {
        LL = LL - 1;
      }
    } else {
      LL = LL + 1;
      line[LL] = CH;
    }
  }
  // READLN(STDIN);
  if (LL > 0) {
    ENDFLAG = false;
    CC = 1;
    CH = line[CC];
  } else {
    ENDFLAG = true;
    CH = ' ';
    CC = 0;
  }
}

void Freadline(PreBuffer *pbp) {
  int ch = 0;
  LL = 0;
  ch = pbp->getc();
  while (ch >= 0 && ch != '\n' && ch != '\r') {
    line[++LL] = (char)ch;
    ch = pbp->getc();
  }
  if (ch == '\r')
    ch = pbp->getc();
  if (LL > 0) {
    ENDFLAG = false;
    CC = 1;
    CH = line[CC];
  } else {
    ENDFLAG = true;
    CH = ' ';
    CC = 0;
  }
}
void NEXTCHAR() {
  if (CC == LL) {
    ENDFLAG = true;
    CH = ' ';
  } else {
    CC = CC + 1;
    CH = line[CC];
  }
}

void REMOVEBL() {
  while (!ENDFLAG && CH == ' ') {
    NEXTCHAR();
  }
}

bool INWORD() {
  REMOVEBL();
  int K = 0;
  if (ENDFLAG) {
    return false;
  } else {
    CONVERT();
    if (!(CH >= 'A' && CH <= 'Z')) {
      return false;
    } else {
      while ((CH >= 'A' && CH <= 'Z') || (CH == '_') ||
             (CH >= '0' && CH <= '9')) {
        if (K < ALNG) {
          K = K + 1;
          ID[K] = CH;
        }
        NEXTCHAR();
        CONVERT();
      }
      while (K < ALNG) {
        K = K + 1;
        ID[K] = ' ';
      }
      return true;
    }
  }
}
bool Inword() {
  int K;
  REMOVEBL();
  if (ENDFLAG) {
    return false;
  }
  // CONVERT();
  if (!isupper(CH))
    return false;
  K = 0;
  CH = (char)toupper(CH);
  while (isalpha(CH) || isdigit(CH) || CH == '_') {
    if (K < ALNG) {
      K = K + 1;
      ID[K] = CH;
    }
    NEXTCHAR();
    CH = (char)toupper(CH);
  }
  while (K < ALNG) {
    K = K + 1;
    ID[K] = ' ';
  }
  return true;
}

bool GETNUM() {
  REMOVEBL();
  int VAL = 0;
  int I = 0;
  int SIGN = 1;
  if (CH == '-') {
    SIGN = -1;
    NEXTCHAR();
  }
  while (CH >= '0' && CH <= '9') {
    if (I < KMAX) {
      VAL = VAL * 10 + CH - '0';
    }
    I = I + 1;
    NEXTCHAR();
  }
  if ((I > KMAX) || (VAL > LONGNMAX) || (I == 0)) {
    return false;
  } else {
    INUM = VAL * SIGN;
    return true;
  }
}

void INFNAME() {
  REMOVEBL();
  // FNAME[0] = '\0';
  int I = -1;
  while ((I < FILMAX) && (!ENDFLAG)) {
    I = I + 1;
    FNAME[I] = CH;
    NEXTCHAR();
  }
  I = I + 1;
  FNAME[I] = '\0';
}

void EQINFNAME() {
  REMOVEBL();
  if (ENDFLAG) {
    FNAME[0] = '\0';
  } else {
    if (CH == '=') {
      NEXTCHAR();
    }
    INFNAME();
  }
}

void GETRANGE(int &FIRST, int &LAST, bool &ERR) {
  ERR = false;
  REMOVEBL();
  if (!ENDFLAG) {
    if (GETNUM()) {
      FIRST = INUM;
    } else {
      ERR = true;
    }
    REMOVEBL();
    if (CH == ':') {
      NEXTCHAR();
      if (GETNUM()) {
        LAST = INUM;
      } else {
        ERR = true;
      }
    } else if (ENDFLAG) {
      LAST = FIRST;
    } else {
      ERR = true;
    }
  }
}

int GETLNUM(int PC) {
  int I = 1;
  while (LOCATION[I] <= PC && I <= LNUM) {
    I++;
  }
  return I - 1;
}
void PRINTSTATS(InterpLocal *il) {
  std::cout << std::endl;
  if (il->PS != InterpLocal::PS::FIN && il->PS != InterpLocal::PS::DEAD) {
    std::cout << "*** HALT AT " << GETLNUM(il->CURPR->PC - 1) << " IN PROCESS "
              << il->CURPR->PID << " BECAUSE OF ";
    // pascal:       print(stdout, '*** HALT AT ',GETLNUM(PC-1):1,' IN PROCESS
    // ', CURPR^.PID:1); pascal:       print(stdout,  ' BECAUSE OF ');
    switch (il->PS) {
    case InterpLocal::PS::DIVCHK:
      std::cout << "DIVISION BY 0" << std::endl;
      break;
    case InterpLocal::PS::INXCHK:
      std::cout << "INVALID INDEX" << std::endl;
      break;
    case InterpLocal::PS::STKCHK:
      // std::cout << "EXPRESSION EVALUATION OVERFLOW" << std::endl;
      std::cout << "STACK OVERFLOW DURING EXPRESSION EVALUATION" << std::endl;
      break;
    case InterpLocal::PS::INTCHK:
      std::cout << "INTEGER VALUE TOO LARGE" << std::endl;
      break;
    case InterpLocal::PS::LINCHK:
      std::cout << "TOO MUCH OUTPUT" << std::endl;
      break;
    case InterpLocal::PS::LNGCHK:
      std::cout << "LINE TOO LONG" << std::endl;
      break;
    case InterpLocal::PS::REDCHK:
      std::cout << "READING PAST END OF FILE" << std::endl;
      break;
    case InterpLocal::PS::DATACHK:
      std::cout << "IMPROPER DATA DURING READ" << std::endl;
      break;
    case InterpLocal::PS::CHANCHK:
      std::cout << "TOO MANY STREAMS OPEN" << std::endl;
      break;
    case InterpLocal::PS::BUFCHK:
      std::cout << "STREAM BUFFER OVERFLOW" << std::endl;
      break;
    case InterpLocal::PS::PROCCHK:
      std::cout << "TOO MANY PROCESSES" << std::endl;
      break;
    case InterpLocal::PS::CPUCHK:
      std::cout << "INVALID PROCESSOR NUMBER" << std::endl;
      break;
    case InterpLocal::PS::REFCHK:
      std::cout << "POINTER ERROR" << std::endl;
      break;
    case InterpLocal::PS::CASCHK:
      std::cout << "UNDEFINED CASE" << std::endl;
      break;
    case InterpLocal::PS::CHRCHK:
      std::cout << "INVALID CHARACTER" << std::endl;
      break;
    case InterpLocal::PS::STORCHK:
      std::cout << "STORAGE OVERFLOW" << std::endl;
      break;
    case InterpLocal::PS::GRPCHK:
      std::cout << "ILLEGAL GROUP SIZE" << std::endl;
      break;
    case InterpLocal::PS::REMCHK:
      std::cout << "REFERENCE TO A NONLOCAL VARIABLE" << std::endl;
      break;
    case InterpLocal::PS::LOCKCHK:
      std::cout << "USE OF SPINLOCK IN MULTICOMPUTER" << std::endl;
      break;
    case InterpLocal::PS::MPIINITCHK:
      std::cout << "FAILURE TO CALL MPI_Init()" << std::endl;
      break;
    case InterpLocal::PS::MPIFINCHK:
      std::cout << "FAILURE TO CALL MPI_Finalize()" << std::endl;
      break;
    case InterpLocal::PS::MPIPARCHK:
      std::cout << "INVALID VALUE OF MPI FUNCTION PARAMETER" << std::endl;
      break;
    case InterpLocal::PS::MPITYPECHK:
      std::cout << "TYPE MISMATCH IN MPI FUNCTION CALL" << std::endl;
      break;
    case InterpLocal::PS::MPICNTCHK:
      std::cout << "TOO MANY VALUES IN THE MESSAGE" << std::endl;
      break;
    case InterpLocal::PS::MPIGRPCHK:
      std::cout << "MISMATCH IN MPI GROUP COMMUNICATION CALLS" << std::endl;
      break;
    case InterpLocal::PS::FUNCCHK:
      std::cout << "FUNCTION NOT FULLY DEFINED" << std::endl;
      break;
    case InterpLocal::PS::CARTOVR:
      std::cout << "TOO MANY CARTESIAN TOPOLOGY" << std::endl;
      break;
    case InterpLocal::PS::STRCHK:
      std::cout << "INVALID STRING" << std::endl;
      break;
    case InterpLocal::PS::USERSTOP:
      std::cout << "PROGRAM REQUESTED ABNORMAL TERMINATION" << std::endl;
      break;
    case InterpLocal::PS::OVRCHK:
      std::cout << "FLOATING POINT OVERFLOW" << std::endl;
      break;
    default:
      break;
    }
    print(STDOUT, "\n");
  }
  if (!OUTPUTFILE) {
    print(STDOUT, "SEQUENTIAL EXECUTION TIME: %d\n", (int)il->SEQTIME);
    print(STDOUT, "PARALLEL EXECUTION TIME: %d\n", (int)il->MAINPROC->TIME);
    if (il->MAINPROC->TIME != 0) {
      il->SPEED = il->SEQTIME / il->MAINPROC->TIME;
      print(STDOUT, "SPEEDUP:  %6.2f\n", il->SPEED);
    }
    print(STDOUT, "NUMBER OF PROCESSORS USED: %d\n", il->USEDPROCS);
  } else {
    // print(STDOUT, OUTP);
    print(OUTP, "SEQUENTIAL EXECUTION TIME: %d\n", (int)il->SEQTIME);
    print(OUTP, "PARALLEL EXECUTION TIME: %d\n", (int)il->MAINPROC->TIME);
    if (il->MAINPROC->TIME != 0) {
      il->SPEED = il->SEQTIME / il->MAINPROC->TIME;
      print(OUTP, "SPEEDUP:  %6.2f\n", il->SPEED);
    }
    print(OUTP, "NUMBER OF PROCESSORS USED: %d\n", il->USEDPROCS);
    // (*OUTP).close();
    fclose(OUTP);
    OUTPUTOPEN = false;
    if (INPUTOPEN) {
      // (*INP).close();
      fclose(INP);
      INPUTOPEN = false;
    }
  }
}
int FIND(InterpLocal *il) {
  int I = 1;
  while (il->CHAN[I].HEAD != -1 && I < OPCHMAX) {
    I++;
  }
  if (I == OPCHMAX) {
    il->PS = InterpLocal::PS::CHANCHK;
  }
  il->CHAN[I].HEAD = 0;
  il->CHAN[I].WAIT = nullptr;
  il->CHAN[I].SEM = 0;
  il->CHAN[I].READTIME = 0;
  il->CHAN[I].MOVED = false;
  il->CHAN[I].READER = -1;
  return I;
}

static int rel_alloc = 0, rel_free = 0, rel_fail = 0;
int FINDFRAME(InterpLocal *il, int LENGTH) {
  // a frame is an unused chunk of stack parallels, S, RS, SLOCATION, STARTMEM
  // of the passed LENGTH
  // the BLOCKR chain is managed by this code
  BLKPNT PREV = nullptr, REF = il->STHEAD;
  int I, STARTLOC;
  int rtn;
  bool FOUND = false;
  while (!FOUND && REF != nullptr) {
    if (REF->SIZE >= LENGTH) {
      FOUND = true;
      rtn = REF->START;
      STARTLOC = REF->START;
      REF->START = REF->START + LENGTH;
      REF->SIZE = REF->SIZE - LENGTH;
      if (REF->SIZE == 0) {
        if (REF == il->STHEAD) {
          il->STHEAD = REF->NEXT;
        } else {
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
    il->PS = InterpLocal::PS::STORCHK;
    rtn = -1;
  } else {
    for (I = STARTLOC; I <= STARTLOC + LENGTH - 1; I += 1) {
      il->STARTMEM[I] = STARTLOC;
    }
  }
  return rtn;
}
void RELEASE(InterpLocal *il, int BASE, int LENGTH) {
  STYPE *smp = il->STARTMEM;
  for (int I = BASE; I < BASE + LENGTH; I++) {
    smp[I] = 0;
  }
  BLKPNT NEWREF;
  BLKPNT PREV;
  BLKPNT REF = il->STHEAD;
  bool LOCATED = false;
  while (!LOCATED && REF != nullptr) {
    if (BASE <= REF->START) {
      LOCATED = true;
    } else {
      PREV = REF;
      REF = REF->NEXT;
    }
  }
  if (!LOCATED)
    ++rel_fail;
  NEWREF = (BLKPNT)malloc(sizeof(BLOCKR));
  ++rel_alloc;
  NEWREF->START = BASE;
  NEWREF->SIZE = LENGTH;
  NEWREF->NEXT = REF;
  if (REF == il->STHEAD) {
    il->STHEAD = NEWREF;
  } else {
    if (PREV->START + PREV->SIZE != BASE) {
      PREV->NEXT = NEWREF;
    } else {
      PREV->SIZE = PREV->SIZE + LENGTH;
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
void unreleased(InterpLocal *il) {
  BLKPNT seq;
  print(STDOUT, "rel_fail %d, rel_alloc %d, rel_free %d\n", rel_fail, rel_alloc,
        rel_free);
  seq = il->STHEAD;
  while (seq != nullptr) {
    print(STDOUT, "unreleased %d len %d\n", seq->START, seq->SIZE);
    seq = seq->NEXT;
  }
}

int ITOB(int I) {
  if (I == TRU) {
    return true;
  } else {
    return false;
  }
}

int BTOI(bool B) {
  if (B) {
    return TRU;
  } else {
    return FALS;
  }
}

int NUM(InterpLocal *il) {
  bool FOUND = GETNUM();
  bool ERR = false;
  int CNT = 0;
  while (!FOUND && !ERR && CNT < 3) {
    REMOVEBL();
    if (!ENDFLAG) {
      ERR = true;
    } else {
      std::cout << il->PROMPT;
      READLINE();
      FOUND = GETNUM();
      if (!FOUND) {
        CNT++;
      }
    }
  }
  if (ERR || CNT >= 3) {
    return -1;
  } else {
    return INUM;
  }
}

void INITIALIZE(InterpLocal *il) {
  int *ip;
  if (il->INITFLAG) {
    il->PTEMP = il->ACPHEAD;
    do {
      free(il->PTEMP->PDES);
      il->RTEMP = il->PTEMP;
      il->PTEMP = il->PTEMP->NEXT;
      free(il->RTEMP);
    } while (il->PTEMP != il->ACPHEAD);
    BLKPNT TBP = il->STHEAD;
    while (TBP != nullptr) {
      BLKPNT SBP = TBP;
      TBP = TBP->NEXT;
      free(SBP);
    }
    if (Topology != Symbol::SHAREDSY) {
      for (int I = 0; I <= PMAX; I++) {
        BUSYPNT TCP = il->PROCTAB[I].BUSYLIST;
        while (TCP != nullptr) {
          BUSYPNT SCP = TCP;
          TCP = TCP->NEXT;
          free(SCP);
        }
      }
    }
  }
  ip = il->LINK;
  for (int I = 1; I <= BUFMAX - 1; I++) {
    ip[I] = I + 1;
  }
  il->FREE = 1;
  il->LINK[BUFMAX] = 0;
  for (int I = 0; I <= OPCHMAX; I++) {
    il->CHAN[I].HEAD = -1;
  }
  il->STHEAD = (BLKPNT)calloc(1, sizeof(BLOCKR));
  ++rel_alloc;
  il->STKMAIN = BTAB[2].VSIZE + WORKSIZE; // WORKSIZE=30
  // first STKMAIN elements of S, RS, SLOCATION, STARTMEN, initialized
  for (int I = 0; I <= il->STKMAIN - 1; I++) {
    il->S[I] = 0;
    il->SLOCATION[I] = 0;
    il->STARTMEM[I] = 1;
    il->RS[I] = 0.0;
  }
  // first BLOCKR (il->STKHEAD) saves first free stack index, free size of
  // stack, next BLOCKR (null)
  il->STHEAD->START = il->STKMAIN;
  il->STHEAD->SIZE = STMAX - il->STKMAIN;
  il->STHEAD->NEXT = nullptr;
  il->NOSWITCH = false;
  il->S[1] = 0;
  il->S[2] = 0;
  il->S[3] = -1;
  il->S[4] = BTAB[1].LAST; // first invalid stack bottom index
  il->S[5] = 1;
  il->S[6] = il->STKMAIN; // first stack beyond reserved WORKSIZE
  for (int I = 0; I <= BASESIZE - 1;
       I++) // BASESIZE is 8, STARTMEM 8 elements negated
  {
    il->STARTMEM[I] = -1;
  }
  il->FLD[1] = 20;
  il->FLD[2] = 4;
  il->FLD[3] = 5;
  il->FLD[4] = 1;
  il->SEQTIME = 0;
  if (il->VARIATION) {
    std::cout << "Random Number Seed for Variation: ";
    std::cin >> XR;
    XR = XR + 52379.0;
  }
  for (int I = 0; I <= PMAX; I++) // init all PROCTAB elements
  {
    il->PROCTAB[I].STATUS = InterpLocal::PROCTAB::STATUS::NEVERUSED;
    il->PROCTAB[I].VIRTIME = 0;
    il->PROCTAB[I].BRKTIME = 0;
    il->PROCTAB[I].PROTIME = 0;
    il->PROCTAB[I].RUNPROC = nullptr;
    il->PROCTAB[I].NUMPROC = 0;
    il->PROCTAB[I].STARTTIME = 0;
    il->PROCTAB[I].BUSYLIST = nullptr;
    if (il->VARIATION) {
      il->PROCTAB[I].SPEED = (float)(RANDGEN() + 0.001);
    } else {
      il->PROCTAB[I].SPEED = 1;
    }
  }
  il->PROLINECNT = 0;
  il->CLOCK = 0;
  il->ACPHEAD = (ACTPNT)calloc(1, sizeof(ACTIVEPROCESS));
  il->MAINPROC = (PROCPNT)calloc(1, sizeof(PROCESSDESCRIPTOR));
  il->ACPHEAD->PDES = il->MAINPROC;
  il->ACPHEAD->NEXT = il->ACPHEAD;
  il->ACPTAIL = il->ACPHEAD;
  il->ACPCUR = il->ACPHEAD;
  il->MAINPROC->BASE = 0;
  il->MAINPROC->B = 0;
  il->MAINPROC->DISPLAY[0] = -2; // unused, 1-origin
  il->MAINPROC->DISPLAY[1] = 0;
  for (int I = 2; I <= LMAX; I++) {
    il->MAINPROC->DISPLAY[I] = -1;
  }
  il->MAINPROC->T = BTAB[2].VSIZE - 1; // last in prefix for initial increment
  il->MAINPROC->PC = 0;
  il->MAINPROC->TIME = 0;
  il->MAINPROC->VIRTUALTIME = 0;
  il->MAINPROC->STATE = PROCESSDESCRIPTOR::STATE::RUNNING;
  il->MAINPROC->STACKSIZE = il->STKMAIN - 2;
  il->MAINPROC->FORLEVEL = 0;
  il->MAINPROC->PROCESSOR = 0;
  il->MAINPROC->ALTPROC = -1;
  il->MAINPROC->PID = 0;
  il->MAINPROC->READSTATUS = PROCESSDESCRIPTOR::READSTATUS::NONE;
  il->MAINPROC->FORKCOUNT = 1;
  il->MAINPROC->MAXFORKTIME = 0;
  il->MAINPROC->JOINSEM = 0;
  il->MAINPROC->PRIORITY = PROCESSDESCRIPTOR::PRIORITY::LOW;
  il->MAINPROC->SEQON = true;
  il->PROCTAB[0].STATUS = InterpLocal::PROCTAB::STATUS::FULL;
  il->PROCTAB[0].RUNPROC = il->MAINPROC;
  il->PROCTAB[0].NUMPROC = 1;
  il->CURPR = il->MAINPROC;
  il->USEDPROCS = 1;
  il->NEXTID = 1;
  il->PS = InterpLocal::PS::RUN;
  il->LNCNT = 0;
  il->CHRCNT = 0;
  il->COUTWIDTH = -1;
  il->COUTPREC = -1;
  il->INITFLAG = true;
  il->OLDSEQTIME = 0;
  il->OLDTIME = 0;
  MUSTRUN = false;
  if (il->ALARMON) {
    il->ALARMENABLED = true;
  }
  if (mpi_mode) {
    for (int I = 0; I <= PMAX; I++) {
      il->MPIINIT[I] = false;
      il->MPIFIN[I] = false;
      il->MPIPNT[I] = -1;
    }
    il->MPICODE = -1;
    il->MPITIME = 0;
    il->MPISEM = 0;
    il->MPIROOT = -1;
    il->MPITYPE = 0;
    il->MPICNT = 0;
    il->MPIOP = 0;
    for (int I = 1; I <= CARTMAX; I++) {
      il->MPICART[I][0] = -1;
    }
  }
}

bool TESTEND() {
  REMOVEBL();
  if (!ENDFLAG) {
    std::cout << "INVALID COMMAND" << std::endl;
    return false;
  } else {
    return true;
  }
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
void INTERPRET() {
  int I, J, K;
  //        STYPE *S;
  //        STYPE *SLOCATION;
  //        STYPE *STARTMEM;
  //        RSTYPE *RS;
  //        BUFINTTYPE *VALUE;
  //        BUFREALTYPE *RVALUE;
  //        BUFREALTYPE *DATE;
  //        BUFINTTYPE *LINK;
  struct InterpLocal *il = &InterpretLocal;
  INITINTERP(il);
  // NEW(S); NEW(SLOCATION); NEW(STARTMEM); NEW(RS);
  // NEW(VALUE); NEW(RVALUE); NEW(DATE); NEW(LINK);
  il->S = (STYPE *)calloc(STMAX + 1, sizeof(STYPE));
  il->SLOCATION = (STYPE *)calloc(STMAX + 1, sizeof(STYPE));
  il->STARTMEM = (STYPE *)calloc(STMAX + 1, sizeof(STYPE));
  il->RS = (RSTYPE *)calloc(STMAX + 1, sizeof(RSTYPE));
  il->LINK = (BUFINTTYPE *)calloc(BUFMAX + 1, sizeof(BUFINTTYPE));
  il->VALUE = (BUFINTTYPE *)calloc(BUFMAX + 1, sizeof(BUFINTTYPE));
  il->RVALUE = (BUFREALTYPE *)calloc(BUFMAX + 1, sizeof(BUFREALTYPE));
  il->DATE = (BUFREALTYPE *)calloc(BUFMAX + 1, sizeof(BUFREALTYPE));
  INITCOMMANDS();
  mpi_mode = false;
  do {
    int cct = 0;
    do {
      // print(stdout, "\n");
      // std::cout << std::endl;
      fputc('\n', STDOUT);
      // print(stdout, '*');
      // std::cout << "*";
      fputc('*', STDOUT);
      //                FREADLINE();
      Freadline(prebuf);
      //                print(STDOUT, "%s\n", &LINE[1]);
      if (++cct > 3) {
        printf("prompt loop may be infinite\n");
        exit(1);
      }
    } while (ENDFLAG);
    il->TEMP = INWORD();
    il->COMMLABEL = COMTYP::ERRC;
    for (I = 0; I < COMMAX; I++) {
      if (strcmp(COMTAB[I], &ID[1]) == 0 ||
          (strcmp(ABBREVTAB[I], &ID[1]) == 0 &&
           strcmp(ABBREVTAB[I], "NONE") != 0)) {
        il->COMMLABEL = COMJMP[I];
      }
    }
    switch (il->COMMLABEL) {
    case COMTYP::RUNP:
      if (!SRCOPEN) {
        print(stdout, "Must Open a Program Source File First\n");
        break;
      }
      if (TESTEND()) {
        if (INPUTFILE) {
          if (!INPUTOPEN) {
            size_t ll = il->INPUTFNAME.length() - 1;
            while (ll > 0 && il->INPUTFNAME[ll] == ' ')
              ll -= 1;
            if (ll++ > 0)
              il->INPUTFNAME[ll] = '\0';
            INP = fopen(il->INPUTFNAME, "r");
            il->INPUTFNAME[ll] = ' ';
          }
          // RESET(INP);
          if (INP == nullptr) {
            print(stdout, "\n");
            print(stdout, "Error In Data Input File %s\n", il->INPUTFNAME);
          } else
            INPUTOPEN = true;
        }
        if (OUTPUTFILE) {
          if (!OUTPUTOPEN) {
            size_t ll = il->INPUTFNAME.length() - 1;
            while (ll > 0 && il->INPUTFNAME[ll] == ' ')
              ll -= 1;
            if (ll++ > 0)
              il->INPUTFNAME[ll] = '\0';
            OUTP = fopen(il->OUTPUTFNAME, "w");
            il->INPUTFNAME[ll] = ' ';
          }
          // REprint(stdout, OUTP);
          if (OUTP == nullptr) {
            print(stdout, "\n");
            print(stdout, "Error In Data Output File %s\n", il->OUTPUTFNAME);
          } else
            OUTPUTOPEN = true;
        }
        INITIALIZE(il);
        il->MAXSTEPS = -1;
        il->RESTART = false;
        if (il->PROFILEON) {
          il->PROTIME = il->PROSTEP;
        }
        //                                    dumpCode();
        EXECUTE(il);
        //                                    dumpSymbols();
        //                                    dumpArrays();
        //                                    dumpBlocks();
        //                                    dumpReals();
        //                                    dumpProctab(il);
        //                                    dumpStkfrms(il);
        //                                    dumpChans(il);
        //                                    dumpActive(il);
        //                                    dumpDeadlock(il);
        if (il->PS != InterpLocal::PS::BREAK) {
          PRINTSTATS(il);
        }
      }
      break;
    case COMTYP::CONT:
      if (TESTEND()) {
        if (il->INITFLAG && !MUSTRUN) {
          if (il->PS == InterpLocal::PS::BREAK) {
            il->PS = InterpLocal::PS::RUN;
            il->RESTART = true;
            il->MAXSTEPS = -1;
            il->OLDSEQTIME = il->SEQTIME;
            il->OLDTIME = il->CLOCK;
            for (I = 0; I <= PMAX; I++) {
              il->PROCTAB[I].BRKTIME = 0;
              il->PROCTAB[I].PROTIME = 0;
            }
            if (il->PROFILEON) {
              il->PROLINECNT = 0;
              il->PROTIME = il->OLDTIME + il->PROSTEP;
            }
            EXECUTE(il);
            if (il->PS != InterpLocal::PS::BREAK) {
              PRINTSTATS(il);
            }
          } else {
            print(stdout, "Can Only Continue After a Breakpoint");
          }
        } else {
          print(stdout, "Must Run Program Before Continuing");
        }
      }
      break;
    case COMTYP::STEP:
      if (il->INITFLAG && !MUSTRUN) {
        if (il->PS == InterpLocal::PS::BREAK) {
          if (INWORD()) {
            if (strcmp(&ID[1], "PROCESS       ") == 0) {
              strcpy(il->PROMPT, "Number:   ");
              K = NUM(il);
              il->PTEMP = il->ACPHEAD;
              il->TEMP = false;
              do {
                if (il->PTEMP->PDES->PID == K) {
                  il->TEMP = true;
                  il->STEPROC = il->PTEMP->PDES;
                } else {
                  il->PTEMP = il->PTEMP->NEXT;
                }
              } while (!il->TEMP && il->PTEMP != il->ACPHEAD);
              if (!il->TEMP) {
                print(stdout, "Invalid Process Number");
                goto L200;
              }
              print(stdout, "\n");
            } else {
              print(stdout, "Error in Command");
              goto L200;
            }
          } else {
            strcpy(il->PROMPT, "Number:   ");
            K = NUM(il);
            if (K < 0) {
              print(stdout, "Error in Number");
            } else {
              il->MAXSTEPS = K;
              il->OLDSEQTIME = il->SEQTIME;
              il->OLDTIME = il->CLOCK;
              il->STEPTIME = il->STEPROC->TIME;
              il->VIRSTEPTIME = il->STEPROC->VIRTUALTIME;
              for (I = 0; I <= PMAX; I++) {
                il->PROCTAB[I].BRKTIME = 0;
                il->PROCTAB[I].PROTIME = 0;
              }
              if (il->PROFILEON) {
                il->PROTIME = il->OLDTIME + il->PROSTEP;
              }
              il->STARTLOC = LOCATION[il->BLINE];
              il->ENDLOC = LOCATION[il->BLINE + 1] - 1;
              il->PS = InterpLocal::PS::RUN;
              EXECUTE(il);
              if (il->PS != InterpLocal::PS::BREAK) {
                PRINTSTATS(il);
              }
            }
          }
        } else {
          print(stdout, "Cannot Step Because Program is Terminated");
        }
      } else {
        print(stdout, "Must Begin with Run Command");
      }
      break;
    case COMTYP::BREAKP:
      if (!SRCOPEN) {
        print(stdout, "Must Open a Program Source File First");
        goto L200;
      }
      J = 0;
      for (I = 1; I <= BRKMAX; I++) {
        if (il->BRKTAB[I] == -1) {
          J = I;
        }
      }
      if (J == 0) {
        print(stdout, "Too Many Breakpoints");
      } else {
        strcpy(il->PROMPT, "At Line:  ");
        K = NUM(il);
        if (K < 0 || K > LNUM) {
          print(stdout, "Out of Bounds");
        } else if (LOCATION[K + 1] == LOCATION[K] || !BREAKALLOW[K]) {
          print(stdout, "Breakpoints Must Be Set at Executable Lines");
        } else {
          il->BRKLINE[J] = K;
          il->BRKTAB[J] = LOCATION[K];
          il->NUMBRK++;
          print(stdout, "\n");
        }
      }
      break;
    case COMTYP::EXIT2:
      print(stdout, "TERMINATE C* SYSTEM\n");
      break;
    case COMTYP::VIEW:
      if (!SRCOPEN) {
        print(stdout, "Must Open a Program Source File First");
        goto L200;
      }
      il->FIRST = 1;
      il->LAST = LNUM;
      GETRANGE(il->FIRST, il->LAST, il->ERR);
      if (il->ERR) {
        print(stdout, "Error in Range");
        goto L200;
      }
      if (il->LAST > LNUM) {
        il->LAST = LNUM;
      }
      if (il->FIRST > LNUM) {
        il->FIRST = LNUM;
      }
      if (SRCOPEN) {
        // RESET(SRC);
        std::fseek(SRC, 0l, SEEK_SET);
        for (K = 1; K < il->FIRST; K++) {
          READLN(SRC);
        }
        for (K = il->FIRST; K <= il->LAST; K++) {
          print(stdout, "%4d", K);
          print(stdout, " ");
          while (!eoln(SRC)) {
            CH = (char)fgetc(SRC);
            fputc(CH, stdout);
          }
          // READLN(SRC);
          print(stdout, "\n");
        }
      } else {
        print(stdout, "There is no Program Source File open");
        print(stdout, "\n");
      }
      break;
    case COMTYP::CLEAR:
      if (!SRCOPEN) {
        print(stdout, "Must Open a Program Source File First");
        goto L200;
      }
      REMOVEBL();
      if (ENDFLAG) {
        print(stdout, "Break or Trace? ");
        READLINE();
        if (ENDFLAG) {
          goto L200;
        }
      }
      if (!INWORD()) {
        print(stdout, "Error in Command");
        goto L200;
      }
      if (strcmp(&ID[1], "BREAK         ") == 0 ||
          strcmp(&ID[1], "B             ") == 0) {
        strcpy(il->PROMPT, "From Line: ");
        K = NUM(il);
        if (K < 0 || K > LNUM) {
          print(stdout, "Not Valid Line Number");
        } else {
          for (I = 1; I <= BRKMAX; I++) {
            if (il->BRKLINE[I] == K) {
              il->BRKTAB[I] = -1;
              il->BRKLINE[I] = -1;
              il->NUMBRK--;
            }
          }
          print(stdout, "\n");
        }
      } else if (strcmp(&ID[1], "TRACE         ") == 0 ||
                 strcmp(&ID[1], "T             ") == 0) {
        strcpy(il->PROMPT, "Memory Lc: ");
        K = NUM(il);
        if (K < 0 || K > STMAX - 1) {
          print(stdout, "Not Valid Memory Location");
        } else {
          for (I = 1; I <= VARMAX; I++) {
            if (il->TRCTAB[I].MEMLOC == K) {
              il->TRCTAB[I].MEMLOC = -1;
              il->NUMTRACE--;
            }
          }
          print(stdout, "\n");
        }
      } else {
        print(stdout, "Error in Command");
        goto L200;
      }
      break;
    case COMTYP::DISPLAYP: {
      if (!SRCOPEN) {
        print(stdout, "Must Open a Program Source File First");
        goto L200;
      }
      if (TESTEND()) {
        if (il->NUMBRK > 0) {
          for (K = 1; K <= BRKMAX - 1; K++) {
            for (I = 1; I <= BRKMAX - K; I++) {
              if (il->BRKLINE[I] > il->BRKLINE[I + 1]) {
                J = il->BRKTAB[I];
                il->BRKTAB[I] = il->BRKTAB[I + 1];
                il->BRKTAB[I + 1] = J;
                J = il->BRKLINE[I];
                il->BRKLINE[I] = il->BRKLINE[I + 1];
                il->BRKLINE[I + 1] = J;
              }
            }
          }
          print(stdout, "Breakpoints at Following Lines:\n");
          for (I = 1; I <= BRKMAX; I++) {
            if (il->BRKLINE[I] >= 0) {
              print(stdout, "%d\n", il->BRKLINE[I]);
            }
          }
        }
        if (il->NUMTRACE > 0) {
          print(stdout, "\n");
          print(stdout, "List of Trace Variables:\n");
          print(stdout, "Variable Name             Memory Location\n");
          for (I = 1; I <= VARMAX; I++) {
            if (il->TRCTAB[I].MEMLOC != -1) {
              print(stdout, "   %s%11d\n", il->TRCTAB[I].NAME,
                    il->TRCTAB[I].MEMLOC);
            }
          }
        }
        if (il->ALARMON) {
          print(stdout, "\n");
          print(stdout, "Alarm is Set at Time %f\n", il->ALARMTIME);
        }
        if (Topology != Symbol::SHAREDSY) {
          print(stdout, "\n");
          print(stdout, "Communication Link Delay: %d\n", il->TOPDELAY);
          if (il->CONGESTION) {
            print(stdout, "Congestion is On\n");
          } else {
            print(stdout, "Congestion is Off\n");
          }
        }
        if (mpi_mode) {
          print(stdout, "MPI Mode is On\n");
        }
        if (il->INPUTFNAME.length() != 0) {
          print(stdout, "File for Program Data Input: %s\n", il->INPUTFNAME);
        }
        if (il->OUTPUTFNAME.length() != 0) {
          print(stdout, "File for Program Data Output: %s\n", il->OUTPUTFNAME);
        }
        print(stdout, "Listing File for your Source Program: ");
        if (LISTDEF) {
          print(stdout, "%s\n", il->LISTDEFNAME);
        } else {
          print(stdout, "%s\n", il->LISTFNAME);
        }
      }
      break;
    }
    case COMTYP::PROFILE: {
      if (!SRCOPEN) {
        print(stdout, "Must Open a Program Source File First");
        goto L200;
      }
      if (INWORD()) {
        if (strcmp(&ID[1], "OFF           ") == 0) {
          il->PROFILEON = false;
        } else {
          print(stdout, "Error in Command");
          goto L200;
        }
      } else {
        il->FIRSTPROC = -1;
        il->PROFILEON = false;
        do {
          REMOVEBL();
          if (ENDFLAG) {
            print(stdout, "Process Range: ");
            READLINE();
          }
          GETRANGE(il->FIRSTPROC, il->LASTPROC, il->ERR);
        } while (il->ERR || il->FIRSTPROC == -1);
        if (il->ERR) {
          print(stdout, "Error in Range");
          goto L200;
        }
        if (il->LASTPROC - il->FIRSTPROC >= 40) {
          print(stdout, "Maximum of 40 Processes Allowed in Profile");
          goto L200;
        }
        strcpy(il->PROMPT, "Time Step:");
        il->PROSTEP = NUM(il);
        if (il->PROSTEP < 0) {
          print(stdout, "Not Valid Time Step");
          goto L200;
        }
        if (il->PROSTEP < 10) {
          print(stdout, "Minimum Time Step is 10");
          goto L200;
        }
        if (TESTEND()) {
          il->PROFILEON = true;
        }
      }
      print(stdout, "\n");
      break;
    }
    case COMTYP::ALARM: {
      if (!SRCOPEN) {
        print(stdout, "Must Open a Program Source File First");
        goto L200;
      }
      if (INWORD()) {
        if (strcmp(&ID[1], "OFF           ") == 0) {
          il->ALARMON = false;
        } else {
          print(stdout, "Error in Command");
          goto L200;
        }
      } else {
        strcpy(il->PROMPT, "  Time:   ");
        il->ALARMTIME = NUM(il);
        if (il->ALARMTIME < 0) {
          print(stdout, "Not a Valid Alarm Time");
          goto L200;
        }
        il->ALARMON = true;
        if (il->ALARMTIME > il->CLOCK) {
          il->ALARMENABLED = true;
        } else {
          il->ALARMENABLED = false;
        }
      }
      print(stdout, "\n");
      break;
    }
    case COMTYP::TRACE: {
      if (!SRCOPEN) {
        print(stdout, "Must Open a Program Source File First");
        goto L200;
      }
      GETVAR(il);
      if (il->ERR) {
        goto L200;
      }
      if (il->OBJ != OBJECTS::VARIABLE) {
        print(stdout, "Can Only Trace on Variables");
        goto L200;
      }
      if (il->TYP == Types::ARRAYS || il->TYP == Types::RECS) {
        print(stdout, "Cannot Trace a Whole Array or Structure");
        goto L200;
      }
      J = 0;
      for (I = 1; I <= VARMAX; I++) {
        if (il->TRCTAB[I].MEMLOC == -1) {
          J = I;
        }
      }
      if (J == 0) {
        print(stdout, "Too Many Trace Variables");
        goto L200;
      }
      strcpy(il->TRCTAB[J].NAME, &il->VARNAME[1]);
      il->TRCTAB[J].MEMLOC = il->ADR;
      il->NUMTRACE++;
      print(stdout, "\n");
      break;
    }
    case COMTYP::WRVAR: {
      if (!SRCOPEN) {
        print(stdout, "Must Open a Program Source File First");
        goto L200;
      }
      GETVAR(il);
      if (il->ERR) {
        goto L200;
      }
      if (strcmp(&ID[1], "MPI_BUFFER    ") == 0) {
        WRMPIBUF(il, K);
        goto L200;
      }
      if (il->OBJ == OBJECTS::KONSTANT) {
        switch (il->TYP) {
        case Types::REALS:
          print(stdout, "%f\n", CONTABLE[il->ADR]);
          break;
        case Types::INTS:
          print(stdout, "%d\n", il->ADR);
          break;
        case Types::BOOLS:
          if (il->ADR == 0) {
            print(stdout, "false");
          } else {
            print(stdout, "true");
          }
          break;
        case Types::CHARS:
          print(stdout, "%c\n", il->ADR);
          break;
        default:
          break;
        }
        goto L200;
      }
      il->TEMP = false;
      il->LINECNT = 0;
      if (il->TYP == Types::ARRAYS) {
        print(stdout, "Index Range: ");
        READLINE();
        if (ENDFLAG) {
          goto L200;
        }
        il->FIRST = ATAB[il->REF].LOW;
        il->LAST = ATAB[il->REF].HIGH;
        GETRANGE(il->FIRST, il->LAST, il->ERR);
        if (il->ERR) {
          print(stdout, "Error in Range");
          goto L200;
        }
        if (il->FIRST < ATAB[il->REF].LOW || il->LAST > ATAB[il->REF].HIGH) {
          print(stdout, "Array Bounds Exceeded");
          goto L200;
        }
        il->ADR =
            il->ADR + (il->FIRST - ATAB[il->REF].LOW) * ATAB[il->REF].ELSIZE;
        for (I = il->FIRST; I <= il->LAST; I++) {
          if (!il->TEMP) {
            print(stdout, "%d", I);
            print(stdout, "--> ");
            PARTWRITE(il, 10, ATAB[il->REF].ELTYP, ATAB[il->REF].ELREF, il->ADR,
                      il->TEMP);
            il->ADR = il->ADR + ATAB[il->REF].ELSIZE;
          }
        }
      } else {
        PARTWRITE(il, 1, il->TYP, il->REF, il->ADR, il->TEMP);
      }
      break;
    }
    case COMTYP::PSTATUS: {
      if (!il->INITFLAG) {
        std::cout << "Must Run Program First" << std::endl;
        goto L200;
      }
      I = 0;
      J = il->NEXTID - 1;
      GETRANGE(I, J, il->TEMP);
      il->TEMP = false;
      if (il->TEMP) {
        std::cout << "Error in Range" << std::endl;
        goto L200;
      }
      if (TESTEND()) {
        std::cout << "   PROCESS #   " << "FUNCTION   " << "    LINE NUMBER "
                  << " STATUS" << "   PROCESSOR" << std::endl;
        il->PTEMP = il->ACPHEAD;
        do {
          if (il->PTEMP->PDES->PID >= I && il->PTEMP->PDES->PID <= J &&
              il->PTEMP->PDES->STATE != PROCESSDESCRIPTOR ::STATE::TERMINATED) {
            if (il->PTEMP->PDES->PID < 10) {
              std::cout << "       " << il->PTEMP->PDES->PID;
            } else if (il->PTEMP->PDES->PID < 100) {
              std::cout << "      " << il->PTEMP->PDES->PID;
            } else if (il->PTEMP->PDES->PID < 1000) {
              std::cout << "     " << il->PTEMP->PDES->PID;
            } else if (il->PTEMP->PDES->PID < 10000) {
              std::cout << "    " << il->PTEMP->PDES->PID;
            } else if (il->PTEMP->PDES->PID < 100000) {
              std::cout << "   " << il->PTEMP->PDES->PID;
            } else {
              std::cout << "  " << il->PTEMP->PDES->PID;
            }
            il->LEVEL = LMAX;
            while (il->PTEMP->PDES->DISPLAY[il->LEVEL] == -1) {
              il->LEVEL = il->LEVEL - 1;
            }
            il->H1 = il->PTEMP->PDES->DISPLAY[il->LEVEL];
            il->INX = il->S[il->H1 + 4];
            while (TAB[il->INX].name[1] == '*') {
              il->H1 = il->S[il->H1 + 3];
              il->INX = il->S[il->H1 + 4];
            }
            if (il->LEVEL == 1) {
              std::cout << "        " << "MAIN          ";
            } else {
              std::cout << "        " << TAB[il->INX].name;
            }
            if (il->PTEMP->PDES->PID == il->CURPR->PID) {
              std::cout << std::setw(7) << GETLNUM(il->PTEMP->PDES->PC);
            } else {
              std::cout << std::setw(7) << GETLNUM(il->PTEMP->PDES->PC - 1);
            }
            std::cout << "      ";
            switch (il->PTEMP->PDES->STATE) {
            case PROCESSDESCRIPTOR::STATE::READY:
              std::cout << "READY   ";
              break;
            case PROCESSDESCRIPTOR::STATE::RUNNING:
              std::cout << "RUNNING ";
              break;
            case PROCESSDESCRIPTOR::STATE::BLOCKED:
              std::cout << "BLOCKED ";
              break;
            case PROCESSDESCRIPTOR::STATE::DELAYED:
              std::cout << "DELAYED ";
              break;
            case PROCESSDESCRIPTOR::STATE::SPINNING:
              std::cout << "SPINNING";
              break;
            default:
              break;
            }
            std::cout << std::setw(6) << il->PTEMP->PDES->PROCESSOR
                      << std::endl;
          }
          il->PTEMP = il->PTEMP->NEXT;
        } while (il->PTEMP != il->ACPHEAD);
      }
      break;
    }
    case COMTYP::PTIME: {
      if (!SRCOPEN) {
        std::cout << "Must Open a Program Source File First" << std::endl;
        goto L200;
      }
      REMOVEBL();
      if (TESTEND()) {
        std::cout << "Since Start of Program:" << std::endl;
        std::cout << "     Elapsed Time:  " << il->CLOCK << std::endl;
        std::cout << "     Number of Processors Used:  " << il->USEDPROCS
                  << std::endl;
        if (il->CLOCK > 0) {
          il->SPEED = il->SEQTIME / il->CLOCK;
          //                        std::cout << "     Speedup:  " << il->SPEED
          //                        << std::endl; std::cout << std::endl;
          print(STDOUT, "     Speedup:  %6.2f\n\n", il->SPEED);
        }
        if (il->OLDTIME != 0) {
          il->R1 = il->CLOCK - il->OLDTIME;
          il->R2 = il->SEQTIME - il->OLDSEQTIME;
          std::cout << "Since Last Breakpoint:" << std::endl;
          std::cout << "     Elapsed Time:  " << il->R1 << std::endl;
          if (il->R1 > 0) {
            il->SPEED = il->R2 / il->R1;
            //                            std::cout << "     Speedup:  " <<
            //                            il->SPEED << std::endl;
            print(STDOUT, "     Speedup:  %6.2f\n", il->SPEED);
          }
        }
      }
      break;
    }
    case COMTYP::UTIL: {
      if (!SRCOPEN) {
        std::cout << "Must Open a Program Source File First" << std::endl;
        goto L200;
      }
      il->FIRST = 0;
      il->LAST = PMAX;
      GETRANGE(il->FIRST, il->LAST, il->ERR);
      if (il->ERR) {
        std::cout << "Error in Processor Range" << std::endl;
        goto L200;
      }
      if (TESTEND()) {
        il->R1 = il->CLOCK - il->OLDTIME;
        std::cout << "                  PERCENT UTILIZATION" << std::endl;
        std::cout << "PROCESSOR    SINCE START    SINCE LAST BREAK"
                  << std::endl;
        for (I = il->FIRST; I <= il->LAST; I++) {
          if (il->PROCTAB[I].STATUS !=
                  InterpLocal::PROCTAB::STATUS::NEVERUSED ||
              il->LAST < PMAX) {
            std::cout << std::setw(6) << I << "    " << std::setw(9)
                      << std::floor(il->PROCTAB[I].VIRTIME / il->CLOCK * 100)
                      << "     ";
            if (il->R1 > 0) {
              il->H1 = std::floor(il->PROCTAB[I].BRKTIME / il->R1 * 100);
            } else {
              il->H1 = 100;
            }
            if (il->H1 > 100) {
              il->H1 = 100;
            }
            std::cout << std::setw(12) << il->H1 << std::endl;
          }
        }
      }
      break;
    }
    case COMTYP::CDELAY: {
      if (!SRCOPEN) {
        std::cout << "Must Open a Program Source File First" << std::endl;
        goto L200;
      }
      if (Topology == Symbol::SHAREDSY) {
        std::cout << "Not allowed in shared memory multiprocessor" << std::endl;
      } else {
        strcpy(il->PROMPT, "   Size:  ");
        K = NUM(il);
        if (K < 0) {
          std::cout << "Invalid Communication Delay" << std::endl;
        } else {
          il->TOPDELAY = K;
        }
      }
      break;
    }
    case COMTYP::CONGEST: {
      if (!SRCOPEN) {
        std::cout << "Must Open a Program Source File First" << std::endl;
        goto L200;
      }
      if (Topology == Symbol::SHAREDSY) {
        std::cout << "Not allowed in shared memory multiprocessor" << std::endl;
      } else {
        if (INWORD()) {
          if (strcmp(&ID[1], "ON            ") == 0) {
            il->CONGESTION = true;
          } else if (strcmp(&ID[1], "OFF           ") == 0) {
            il->CONGESTION = false;
          } else {
            std::cout << R"("ON" or "OFF" Missing)" << std::endl;
          }
        } else {
          std::cout << R"("ON" or "OFF" Missing)" << std::endl;
        }
      }
      break;
    }
    case COMTYP::VARY: {
      if (INWORD()) {
        if (strcmp(&ID[1], "ON            ") == 0) {
          il->VARIATION = true;
        } else if (strcmp(&ID[1], "OFF           ") == 0) {
          il->VARIATION = false;
        } else {
          std::cout << R"("ON" or "OFF" Missing)" << std::endl;
        }
      } else {
        std::cout << R"("ON" or "OFF" Missing)" << std::endl;
      }
      break;
    }
    case COMTYP::HELP: {
      std::cout << "The following commands are available:" << std::endl;
      std::cout << std::endl;
      std::cout << "*OPEN filename - Open and Compile your program source file"
                << std::endl;
      std::cout << "*RUN - Initialize and run the program from the beginning"
                << std::endl;
      std::cout << "*CLOSE - Close your program source file to allow editing"
                << std::endl;
      std::cout << "*EXIT - Terminate the C* system" << std::endl;
      std::cout << std::endl;
      std::cout << "*LIST = filename - Change the source program listing file "
                   "to \"filename\""
                << std::endl;
      std::cout
          << "*INPUT = filename - Use \"filename\" for source program input"
          << std::endl;
      std::cout
          << "*OUTPUT = filename - Use \"filename\" for source program output"
          << std::endl;
      std::cout << "*VIEW n:m - List program source lines n through m"
                << std::endl;
      std::cout << std::endl;
      std::cout << "*BREAK n - Set a breakpoint at program line n" << std::endl;
      std::cout << "*CLEAR BREAK n - Clear the breakpoint from line n"
                << std::endl;
      std::cout << "*CONTINUE - Continue program execution after a breakpoint"
                << std::endl;
      std::cout << "*STATUS p:q - Display status of processes p through q"
                << std::endl;
      std::cout
          << "*STEP n - Continue execution for n lines in current STEP process"
          << std::endl;
      std::cout << "*STEP PROCESS p - Set current STEP process number to p"
                << std::endl;
      std::cout
          << "*WRITE p name - Write out value of variable \"name\" in process p"
          << std::endl;
      std::cout << "*TRACE p name - Make variable \"name\" a trace variable"
                << std::endl;
      std::cout << "*CLEAR TRACE m - Clear the trace from memory location m"
                << std::endl;
      std::cout << "*DISPLAY - Display list of breakpoints, trace variables, "
                   "and alarm"
                << std::endl;
      std::cout << std::endl;
      std::cout << "*TIME - Show elapsed time since start of program and since "
                   "last break"
                << std::endl;
      std::cout << "*UTILIZATION p:q - Give utilization percentage for "
                   "processors p to q"
                << std::endl;
      std::cout
          << "*PROFILE p:q t - Set utilization profile to be generated for"
          << std::endl;
      std::cout << "                    processes p to q every t time units"
                << std::endl;
      std::cout << "*PROFILE OFF - Turn off profile" << std::endl;
      std::cout << "*ALARM t - Set alarm to go off at time t" << std::endl;
      std::cout << "*ALARM OFF - Disable alarm from going off in future"
                << std::endl;
      std::cout << std::endl;
      std::cout << "*VARIATION ON (OFF) - Turn variation option on (off)"
                << std::endl;
      std::cout << "*CONGESTION ON (OFF) - Turn congestion option on (off)"
                << std::endl;
      std::cout
          << "*DELAY d - Set the basic communication delay to d time units"
          << std::endl;
      std::cout
          << "*RESET - Reset all debugger settings to original default values"
          << std::endl;
      std::cout
          << "*MPI ON (OFF) - Turn Message-Passing Interface mode on (off)"
          << std::endl;
      std::cout << "*VERSION - Display information about this version of C*"
                << std::endl;
      std::cout << "*SHORTCUTS - Show a list of shortcuts for frequently used "
                   "commands"
                << std::endl;
      std::cout << "*HELP - Show a complete list of commands" << std::endl;
      break;
    }
    case COMTYP::WRCODE: {
      if (!SRCOPEN) {
        std::cout << "Must Open a Program Source File First" << std::endl;
        goto L200;
      }
      il->FIRST = 1;
      il->LAST = LNUM;
      GETRANGE(il->FIRST, il->LAST, il->ERR);
      if (il->ERR) {
        std::cout << "Error in Range" << std::endl;
        goto L200;
      }
      if (il->LAST > LNUM) {
        il->LAST = LNUM;
      }
      //                    std::cout << "LINE NUMBER" << "  OPCODE" << "    X
      //                    " << "  Y  " << std::endl;
      print(STDOUT, "LINE NUMBER  OPCODE    X    Y  \n");
      for (I = il->FIRST; I <= il->LAST; I++) {
        // std::cout << I << "    ";
        print(STDOUT, "%7d    ", I);
        if (LOCATION[I] == LOCATION[I + 1]) {
          // std::cout << std::endl;
          fputc('\n', STDOUT);
        } else {
          J = LOCATION[I];
          print(STDOUT, "%6d%6d%6d\n", CODE[J].F, CODE[J].X, CODE[J].Y);
          for (J = LOCATION[I] + 1; J <= LOCATION[I + 1] - 1; J++) {
            // std::cout << CODE[J].F << " " << CODE[J].X << " " << CODE[J].Y <<
            // std::endl;
            print(STDOUT, "           %6d%6d%6d\n", CODE[J].F, CODE[J].X,
                  CODE[J].Y);
          }
        }
      }
      // std::cout << std::endl;
      fputc('\n', STDOUT);
      break;
    }
    case COMTYP::STACK: {
      if (!SRCOPEN) {
        std::cout << "Must Open a Program Source File First" << std::endl;
        goto L200;
      }
      il->FIRST = -1;
      il->LAST = -1;
      GETRANGE(il->FIRST, il->LAST, il->ERR);
      if (il->ERR || il->FIRST < 0) {
        std::cout << "Error in Range" << std::endl;
        goto L200;
      }
      std::cout << "STACK ADDRESS" << "   VALUE" << std::endl;
      for (I = il->FIRST; I <= il->LAST; I++) {
        std::cout << I << "   " << il->S[I] << std::endl;
      }
      std::cout << std::endl;
      break;
    }
    case COMTYP::OPENF: {
      if (SRCOPEN) {
        std::cout << "You Must Close Current Source Program First" << std::endl;
        std::cout << std::endl;
        goto L200;
      }
      INFNAME();
      SRC = fopen(FNAME, "r");
      if (SRC == nullptr) {
        std::cout << "Cannot Open Your Program Source File" << std::endl;
        std::cout << std::endl;
        goto L200;
      }
      LIS = fopen(il->LISTFNAME, "w");
      if (LIS == nullptr) {
        std::cout << "Cannot Open the Program Listing File " << il->LISTFNAME
                  << std::endl;
        fclose(SRC);
        std::cout << std::endl;
        goto L200;
      }
      // fclose(LIS);
      if (INPUTFILE) {
        FILE *inp = fopen(il->INPUTFNAME, "r");
        if (!inp) {
          // std::cout << "Cannot Open the Input File " << INPUTFNAME <<
          // std::endl;
          print(stdout, "Cannot Open the Input File %s\n", il->INPUTFNAME);
          fclose(SRC);
          std::cout << std::endl;
          goto L200;
        }
        fclose(inp);
      }
      if (OUTPUTFILE) {
        FILE *outp = fopen(il->OUTPUTFNAME, "w");
        if (outp == nullptr) {
          std::cout << "Cannot Open the Output File " << il->OUTPUTFNAME
                    << std::endl;
          fclose(SRC);
          std::cout << std::endl;
          goto L200;
        }
        fclose(outp);
      }
      //                    showCodeList(true);
      //                    showConsoleList(true);
      INITCOMPILER();
      strcpy(PROGNAME, "PROGRAM       ");
      MAINFUNC = -1;
      BLOCK(il, DECLBEGSYS, false, 1, Tx);
      if (SY != Symbol::EOFSY)
        ERROR(22);
      if (BTAB[2].VSIZE + WORKSIZE > STMAX)
        ERROR(49);
      if (MAINFUNC < 0)
        ERROR(100);
      else {
        if (MPIMODE) {
          EMIT(9);
          EMIT1(18, MAINFUNC);
          EMIT1(19, BTAB[TAB[MAINFUNC].reference].PSIZE - 1);
          EMIT(69);
        }
        EMIT1(18, MAINFUNC);
        EMIT1(19, BTAB[TAB[MAINFUNC].reference].PSIZE - 1);
      }
      EMIT(31);
      il->ENDLOC = LC;
      LOCATION[LNUM + 1] = LC;
      std::cout << std::endl;
      //                    dumpCode();
      if (ERRS.none()) {
        // std::cout << std::endl;
        // std::cout << "Program Successfully Compiled" << std::endl;
        print(STDOUT, "\n%s Successfully Compiled\n", FNAME);
        SRCOPEN = true;
        if (code_list)
          dumpCode();
        if (block_list)
          dumpBlocks();
        if (symbol_list)
          dumpSymbols();
        if (array_list)
          dumpArrays();
        if (real_list)
          dumpReals();
      } else {
        ERRORMSG();
        std::cout << std::endl;
        fclose(SRC);
        std::cout << "PROGRAM SOURCE FILE IS NOW CLOSED TO ALLOW EDITING"
                  << std::endl;
      }
      std::cout << std::endl;
      std::cout << "To View a Complete Program Listing, See File "
                << il->LISTFNAME << std::endl;
      fclose(LIS);
      break;
    }
    case COMTYP::CLOSEF: {
      if (SRCOPEN) {
        SRCOPEN = false;
        // std::cout << std::endl;
        // std::cout << "You can now modify your Program Source File" <<
        // std::endl;
        print(STDOUT, "\nYou can now modify your Program Source File\n");
        for (I = 1; I <= BRKMAX; ++I) {
          il->BRKTAB[I] = -1;
          il->BRKLINE[I] = -1;
        }
        il->NUMBRK = 0;
        il->NUMTRACE = 0;
        for (I = 1; I <= VARMAX; ++I) {
          il->TRCTAB[I].MEMLOC = -1;
        }
        if (OUTPUTOPEN) {
          fclose(OUTP);
          OUTPUTOPEN = false;
        }
        if (INPUTOPEN) {
          fclose(INP);
          INPUTOPEN = false;
        }
      } else {
        std::cout << "No Program Source File is open" << std::endl;
      }
      std::cout << std::endl;
      break;
    }
    case COMTYP::INPUTF: {
      EQINFNAME();
      if (INPUTOPEN) {
        fclose(INP);
      }
      if (0 == strlen(FNAME)) {
        INPUTFILE = false;
        INPUTOPEN = false;
      } else {
        FILE *inp = fopen(FNAME, "r");
        if (inp == nullptr) {
          std::cout << "Cannot Open This Input File" << std::endl;
          std::cout << std::endl;
        } else {
          fclose(inp);
          INPUTFILE = true;
          strcpy(il->INPUTFNAME, FNAME);
          INPUTOPEN = false;
          MUSTRUN = true;
        }
      }
      break;
    }
    case COMTYP::OUTPUTF: {
      EQINFNAME();
      if (OUTPUTOPEN) {
        fclose(OUTP);
      }
      if (0 == strlen(FNAME)) {
        OUTPUTFILE = false;
        OUTPUTOPEN = false;
      } else {
        FILE *outp = fopen(FNAME, "w");
        if (outp == nullptr) {
          std::cout << "Cannot Open This Output File" << std::endl;
          std::cout << std::endl;
        } else {
          fclose(outp);
          OUTPUTFILE = true;
          strcpy(il->OUTPUTFNAME, FNAME);
          OUTPUTOPEN = false;
          MUSTRUN = true;
        }
      }
      break;
    }
    case COMTYP::LIST: {
      EQINFNAME();
      if (0 == strlen(FNAME)) {
        LISTDEF = true;
        FILE *lis = fopen(il->LISTDEFNAME, "w");
        if (lis == nullptr) {
          std::cout << "Cannot Open the Default Program Listing File "
                    << il->LISTDEFNAME << std::endl;
          std::cout << std::endl;
        } else {
          fclose(lis);
        }
      } else {
        FILE *lis = fopen(FNAME, "w");
        if (lis == nullptr) {
          std::cout << "Cannot Open This Listing File" << std::endl;
          std::cout << std::endl;
        } else {
          fclose(lis);
          LISTDEF = false;
          strcpy(il->LISTFNAME, FNAME);
        }
      }
      break;
    }
    case COMTYP::RESETP: {
      INITINTERP(il);
      if (!SRCOPEN) {
        MPIMODE = false;
      }
      std::cout << "All Debugger Settings are now reset to Default values"
                << std::endl;
      break;
    }
    case COMTYP::ERRC: {
      std::cout << "INVALID COMMAND" << ":" << &ID[1] << ":" << std::endl;
      break;
    }
    case COMTYP::SHORT: {
      std::cout << std::endl;
      std::cout << "The following Command Keywords may be abbreviated using "
                   "only the first letter:"
                << std::endl;
      std::cout << std::endl;
      std::cout << "   ALARM" << std::endl;
      std::cout << "   BREAK" << std::endl;
      std::cout << "   CONTINUE" << std::endl;
      std::cout << "   DISPLAY" << std::endl;
      std::cout << "   EXIT" << std::endl;
      std::cout << "   HELP" << std::endl;
      std::cout << "   OPEN" << std::endl;
      std::cout << "   PROFILE" << std::endl;
      std::cout << "   RUN" << std::endl;
      std::cout << "   STEP" << std::endl;
      std::cout << "   TRACE" << std::endl;
      std::cout << "   UTILIZATION" << std::endl;
      std::cout << "   VIEW" << std::endl;
      std::cout << "   WRITE" << std::endl;
      std::cout << std::endl;
      break;
    }
    case COMTYP::MPI: {
      if (SRCOPEN) {
        std::cout
            << "You Must Close Your Source Program before changing MPI Mode"
            << std::endl;
      } else {
        if (INWORD()) {
          if (strcmp(&ID[1], "ON            ") == 0) {
            mpi_mode = true;
          } else if (strcmp(&ID[1], "OFF           ") == 0) {
            mpi_mode = false;
          } else {
            std::cout << R"("ON" or "OFF" Missing)" << std::endl;
          }
        } else {
          std::cout << R"("ON" or "OFF" Missing)" << std::endl;
        }
      }
      break;
    }
    case COMTYP::VERSION: {
      std::cout << std::endl;
      std::cout << "  C* COMPILER AND PARALLEL COMPUTER SIMULATION SYSTEM"
                << std::endl;
      std::cout << "                      VERSION 2.2c++" << std::endl;
      std::cout << std::endl;
      std::cout
          << "The C* programming language was created from the C programming"
          << std::endl;
      std::cout
          << "language by adding a few basic parallel programming primitives."
          << std::endl;
      std::cout
          << "However, in this version 2.2 of the C* compiler, the following"
          << std::endl;
      std::cout << "features of standard C are not implemented:" << std::endl;
      std::cout << std::endl;
      std::cout << "1. Union Data Types" << std::endl;
      std::cout << "2. Enumeration Types" << std::endl;
      std::cout << "3. Bitwise Operators:" << std::endl;
      std::cout << "      left shift, right shift, " << std::endl;
      std::cout << "      bitwise complement, bitwise and, bitwise or"
                << std::endl;
      std::cout << "4. Conditional Operator (?)" << std::endl;
      std::cout << "      Example:  i >= 0 ? i : 0" << std::endl;
      //                std::cout << "5. Compound Assignment Operators:" <<
      //                std::endl; std::cout << "      +=  -=  *=  /=  %=" <<
      //                std::endl;
      std::cout << std::endl;
      std::cout << std::endl;
      break;
    }
    default:
      break;
    }
  L200:;
  } while (il->COMMLABEL != COMTYP::EXIT2);
  free(il->S);
  free(il->SLOCATION);
  free(il->STARTMEM);
  free(il->RS);
  free(il->LINK);
  free(il->VALUE);
  free(il->RVALUE);
  free(il->DATE);
}
} // namespace Cstar
