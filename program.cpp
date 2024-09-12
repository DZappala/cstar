//
// Created by Dan Evans on 1/27/24.
//
#include <cstdio>
#include <expected>
#include <iostream>
#include <memory>
#include <print>
#include <string>
#include <unistd.h>

#include "ftxui/components/screen_interactive.hpp"

#define EXPORT_CS_GLOBAL
#include "cs_errors.h"
#include "cs_global.h"
#define EXPORT_CS_COMPILE
#include "cs_PreBuffer.h"
#include "cs_compile.h"

using Cstar::PreBuffer, Cstar::STDIN;
using std::make_unique, std::snprintf, std::string, std::cout, std::expected,
    std::unexpected;

namespace Cstar {
extern void INTERPRET();
extern void showCodeList(bool);
extern void showBlockList(bool);
extern void showSymbolList(bool);
extern void showConsoleList(bool);
extern void showArrayList(bool);
extern void showRealList(bool);
extern void showInstTrace(bool);
extern std::unique_ptr<PreBuffer> pre_buf;
bool interactive = false;

static void enter(const char *X0, OBJECTS X1, Types X2, int X3) {
  tab_index++;
  if (tab_index == TMAX) {
    fatal(1);
  } else {
    strcpy(TAB[tab_index].NAME, X0);
    TAB[tab_index].LINK = tab_index - 1;
    TAB[tab_index].OBJ = X1;
    TAB[tab_index].TYP = X2;
    TAB[tab_index].REF = 0;
    TAB[tab_index].NORMAL = true;
    TAB[tab_index].LEV = 0;
    TAB[tab_index].ADR = X3;
    TAB[tab_index].FORLEV = 0;
    TAB[tab_index].PNTPARAM = false;
  }
}
void LIBFINIT(int index, const char *name, int idnum) {
  strcpy(LIBFUNC[index].NAME, name);
  LIBFUNC[index].IDNUM = idnum;
}
void initialize_compiler() {
  ksy[0] = Symbol::DEFINESY;
  ksy[1] = Symbol::DEFINESY;
  ksy[2] = Symbol::INCLUDESY;
  ksy[3] = Symbol::BREAKSY;
  ksy[4] = Symbol::CASESY;
  ksy[5] = Symbol::CINSY;
  ksy[6] = Symbol::CONSTSY;
  ksy[7] = Symbol::CONTSY;
  ksy[8] = Symbol::COUTSY;
  ksy[9] = Symbol::DEFAULTSY;
  ksy[10] = Symbol::DOSY;
  ksy[11] = Symbol::ELSESY;
  ksy[12] = Symbol::ENDLSY;
  ksy[13] = Symbol::ENUMSY;
  ksy[14] = Symbol::FORSY;
  ksy[15] = Symbol::FORALLSY;
  ksy[16] = Symbol::FORKSY;
  ksy[17] = Symbol::FUNCTIONSY;
  ksy[18] = Symbol::GROUPINGSY;
  ksy[19] = Symbol::IFSY;
  ksy[20] = Symbol::JOINSY;
  ksy[21] = Symbol::LONGSY;
  ksy[22] = Symbol::MOVESY;
  ksy[23] = Symbol::NEWSY;
  ksy[24] = Symbol::RETURNSY;
  ksy[25] = Symbol::SHORTSY;
  ksy[26] = Symbol::CHANSY;
  ksy[27] = Symbol::STRUCTSY;
  ksy[28] = Symbol::SWITCHSY;
  ksy[29] = Symbol::THENSY;
  ksy[30] = Symbol::TOSY;
  ksy[31] = Symbol::TYPESY;
  ksy[32] = Symbol::UNIONSY;
  ksy[33] = Symbol::UNSIGNEDSY;
  ksy[34] = Symbol::VALUESY;
  ksy[35] = Symbol::WHILESY;
  SPS['('] = Symbol::LPARENT;
  SPS[')'] = Symbol::RPARENT;
  SPS[','] = Symbol::COMMA;
  SPS['['] = Symbol::LBRACK;
  SPS[']'] = Symbol::RBRACK;
  SPS[';'] = Symbol::SEMICOLON;
  SPS['~'] = Symbol::BITCOMPSY;
  SPS['{'] = Symbol::LSETBRACK;
  SPS['^'] = Symbol::BITXORSY;
  SPS['@'] = Symbol::ATSY;
  SPS['}'] = Symbol::RSETBRACK;
  SPS[28] = Symbol::EOFSY;
  SPS[':'] = Symbol::COLON;
  // CONSTBEGSYS = {PLUS, MINUS, INTCON, CHARCON, REALCON, IDENT};
  CONSTBEGSYS[Symbol::PLUS] = true;
  CONSTBEGSYS[Symbol::MINUS] = true;
  CONSTBEGSYS[Symbol::INTCON] = true;
  CONSTBEGSYS[Symbol::CHARCON] = true;
  CONSTBEGSYS[Symbol::REALCON] = true;
  CONSTBEGSYS[Symbol::IDENT] = true;
  // TYPEBEGSYS  = {IDENT, STRUCTSY, CONSTSY, SHORTSY, LONGSY, UNSIGNEDSY};
  TYPEBEGSYS[Symbol::IDENT] = true;
  TYPEBEGSYS[Symbol::STRUCTSY] = true;
  TYPEBEGSYS[Symbol::CONSTSY] = true;
  TYPEBEGSYS[Symbol::SHORTSY] = true;
  TYPEBEGSYS[Symbol::LONGSY] = true;
  TYPEBEGSYS[Symbol::UNSIGNEDSY] = true;
  // DECLBEGSYS  = {IDENT, STRUCTSY, CONSTSY, SHORTSY, LONGSY, UNSIGNEDSY,
  // TYPESY, DEFINESY, INCLUDESY}; //+ TYPEBEGSYS;
  DECLBEGSYS = TYPEBEGSYS;
  DECLBEGSYS[Symbol::TYPESY] = true;
  DECLBEGSYS[Symbol::DEFINESY] = true;
  DECLBEGSYS[Symbol::INCLUDESY] = true;
  // BLOCKBEGSYS = {IDENT, STRUCTSY, CONSTSY, SHORTSY, LONGSY, UNSIGNEDSY,
  // TYPESY, DEFINESY}; // + TYPEBEGSYS;
  BLOCKBEGSYS = DECLBEGSYS;
  BLOCKBEGSYS[Symbol::INCLUDESY] = false;
  // FACBEGSYS = {INTCON, CHARCON, REALCON, IDENT, LPARENT, NOTSY, DECREMENT,
  // INCREMENT, PLUS, MINUS, TIMES, STRNG};
  FACBEGSYS[Symbol::INTCON] = true;
  FACBEGSYS[Symbol::CHARCON] = true;
  FACBEGSYS[Symbol::REALCON] = true;
  FACBEGSYS[Symbol::IDENT] = true;
  FACBEGSYS[Symbol::LPARENT] = true;
  FACBEGSYS[Symbol::NOTSY] = true;
  FACBEGSYS[Symbol::DECREMENT] = true;
  FACBEGSYS[Symbol::INCREMENT] = true;
  FACBEGSYS[Symbol::PLUS] = true;
  FACBEGSYS[Symbol::MINUS] = true;
  FACBEGSYS[Symbol::TIMES] = true;
  FACBEGSYS[Symbol::STRNG] = true;
  //        STATBEGSYS = {IFSY, WHILESY, DOSY, FORSY, FORALLSY, SEMICOLON,
  //        FORKSY, JOINSY, SWITCHSY, LSETBRACK, RETURNSY,
  //                      BREAKSY, CONTSY, MOVESY, CINSY, COUTSY};
  STATBEGSYS[Symbol::IFSY] = true;
  STATBEGSYS[Symbol::WHILESY] = true;
  STATBEGSYS[Symbol::DOSY] = true;
  STATBEGSYS[Symbol::FORSY] = true;
  STATBEGSYS[Symbol::FORALLSY] = true;
  STATBEGSYS[Symbol::SEMICOLON] = true;
  STATBEGSYS[Symbol::FORKSY] = true;
  STATBEGSYS[Symbol::JOINSY] = true;
  STATBEGSYS[Symbol::SWITCHSY] = true;
  STATBEGSYS[Symbol::LSETBRACK] = true;
  STATBEGSYS[Symbol::RETURNSY] = true;
  STATBEGSYS[Symbol::BREAKSY] = true;
  STATBEGSYS[Symbol::CONTSY] = true;
  STATBEGSYS[Symbol::MOVESY] = true;
  STATBEGSYS[Symbol::CINSY] = true;
  STATBEGSYS[Symbol::COUTSY] = true;
  // ASSIGNBEGSYS = {IDENT, LPARENT, DECREMENT, INCREMENT, TIMES};
  ASSIGNBEGSYS[Symbol::IDENT] = true;
  ASSIGNBEGSYS[Symbol::LPARENT] = true;
  ASSIGNBEGSYS[Symbol::DECREMENT] = true;
  ASSIGNBEGSYS[Symbol::INCREMENT] = true;
  ASSIGNBEGSYS[Symbol::TIMES] = true;
  // COMPASGNSYS = {PLUSEQ, MINUSEQ, TIMESEQ, RDIVEQ, IMODEQ};
  COMPASGNSYS[Symbol::PLUSEQ] = true;
  COMPASGNSYS[Symbol::MINUSEQ] = true;
  COMPASGNSYS[Symbol::TIMESEQ] = true;
  COMPASGNSYS[Symbol::RDIVEQ] = true;
  COMPASGNSYS[Symbol::IMODEQ] = true;
  // SELECTSYS = {LBRACK, PERIOD, RARROW};
  SELECTSYS[Symbol::LBRACK] = true;
  SELECTSYS[Symbol::PERIOD] = true;
  SELECTSYS[Symbol::RARROW] = true;
  // EXECSYS = {IFSY, WHILESY, DOSY, FORSY, FORALLSY, SEMICOLON, FORKSY, JOINSY,
  // SWITCHSY, LSETBRACK, RETURNSY,
  //            BREAKSY, CONTSY, MOVESY, CINSY, COUTSY, ELSESY, TOSY, DOSY,
  //            CASESY, DEFAULTSY};
  EXECSYS[Symbol::IFSY] = true;
  EXECSYS[Symbol::WHILESY] = true;
  EXECSYS[Symbol::DOSY] = true;
  EXECSYS[Symbol::FORSY] = true;
  EXECSYS[Symbol::FORALLSY] = true;
  EXECSYS[Symbol::SEMICOLON] = true;
  EXECSYS[Symbol::FORKSY] = true;
  EXECSYS[Symbol::JOINSY] = true;
  EXECSYS[Symbol::SWITCHSY] = true;
  EXECSYS[Symbol::LSETBRACK] = true;
  EXECSYS[Symbol::RETURNSY] = true;
  EXECSYS[Symbol::BREAKSY] = true;
  EXECSYS[Symbol::CONTSY] = true;
  EXECSYS[Symbol::MOVESY] = true;
  EXECSYS[Symbol::CINSY] = true;
  EXECSYS[Symbol::COUTSY] = true;
  EXECSYS[Symbol::ELSESY] = true;
  EXECSYS[Symbol::TOSY] = true;
  EXECSYS[Symbol::CASESY] = true;
  EXECSYS[Symbol::DEFAULTSY] = true;
  // NONMPISYS = {CHANSY, FORKSY, JOINSY, FORALLSY};
  NONMPISYS[Symbol::CHANSY] = true;
  NONMPISYS[Symbol::FORKSY] = true;
  NONMPISYS[Symbol::JOINSY] = true;
  NONMPISYS[Symbol::FORALLSY] = true;
  // STANTYPS = {NOTYP, REALS, INTS, BOOLS, CHARS};
  STANTYPS[Types::NOTYP] = true;
  STANTYPS[Types::REALS] = true;
  STANTYPS[Types::INTS] = true;
  STANTYPS[Types::BOOLS] = true;
  STANTYPS[Types::CHARS] = true;
  line_count = 0;
  LL = 0;
  LL2 = 0;
  CC = 0;
  CC2 = 0;
  CH = ' ';
  CPNT = 1;
  LNUM = 0;
  error_position = 0;
  errors = 0;
  INSYMBOL();
  execution_count = 0;
  eof_count = 0;
  tab_index = -1;
  atab_index = 0;
  btab_index = 1; // index to BTAB
  SX = 0;
  C2 = 0;
  ctab_index = 0;
  error_count = 0;
  OKBREAK = false;
  BREAKPNT = 0;
  ITPNT = 1;
  DISPLAY[0] = 1;
  strcpy(DUMMYNAME, "************00");
  INCLUDEFLAG = false;
  fatal_error = false;
  PROTOINDEX = -1;
  enter("              ", OBJECTS::VARIABLE, Types::NOTYP, 0);
  enter("FALSE         ", OBJECTS::KONSTANT, Types::BOOLS, 0);
  enter("TRUE          ", OBJECTS::KONSTANT, Types::BOOLS, 1);
  enter("NULL          ", OBJECTS::KONSTANT, Types::PNTS, 0);
  enter("CHAR          ", OBJECTS::TYPE1, Types::CHARS, 1);
  enter("BOOLEAN       ", OBJECTS::TYPE1, Types::BOOLS, 1);
  enter("INT           ", OBJECTS::TYPE1, Types::INTS, 1);
  enter("FLOAT         ", OBJECTS::TYPE1, Types::REALS, 1);
  enter("DOUBLE        ", OBJECTS::TYPE1, Types::REALS, 1);
  enter("VOID          ", OBJECTS::TYPE1, Types::VOIDS, 1);
  enter("SPINLOCK      ", OBJECTS::TYPE1, Types::LOCKS, 1);
  enter("SELF          ", OBJECTS::FUNKTION, Types::INTS, 19);
  enter("CLOCK         ", OBJECTS::FUNKTION, Types::REALS, 20);
  enter("SEQTIME       ", OBJECTS::FUNKTION, Types::REALS, 21);
  enter("MYID          ", OBJECTS::FUNKTION, Types::INTS, 22);
  enter("SIZEOF        ", OBJECTS::FUNKTION, Types::INTS, 24);
  enter("SEND          ", OBJECTS::PROZEDURE, Types::NOTYP, 3);
  enter("RECV          ", OBJECTS::PROZEDURE, Types::NOTYP, 4);
  enter("NEW           ", OBJECTS::PROZEDURE, Types::NOTYP, 5);
  enter("DELETE        ", OBJECTS::PROZEDURE, Types::NOTYP, 6);
  enter("LOCK          ", OBJECTS::PROZEDURE, Types::NOTYP, 7);
  enter("UNLOCK        ", OBJECTS::PROZEDURE, Types::NOTYP, 8);
  enter("DURATION      ", OBJECTS::PROZEDURE, Types::NOTYP, 9);
  enter("SEQOFF        ", OBJECTS::PROZEDURE, Types::NOTYP, 10);
  enter("SEQON         ", OBJECTS::PROZEDURE, Types::NOTYP, 11);
  enter("              ", OBJECTS::PROZEDURE, Types::NOTYP, 0);
  BTAB[1].LAST = tab_index;
  BTAB[1].LASTPAR = 1;
  BTAB[1].PSIZE = 0;
  BTAB[1].VSIZE = 0;
  LIBFINIT(1, "ABS           ", 1);
  LIBFINIT(2, "FABS          ", 2);
  LIBFINIT(3, "CEIL          ", 9);
  LIBFINIT(4, "FLOOR         ", 10);
  LIBFINIT(5, "SIN           ", 11);
  LIBFINIT(6, "COS           ", 12);
  LIBFINIT(7, "EXP           ", 13);
  LIBFINIT(8, "LOG           ", 14);
  LIBFINIT(9, "SQRT          ", 15);
  LIBFINIT(10, "ATAN          ", 16);
  LIBFINIT(11, "MPI_INIT      ", 50);
  LIBFINIT(12, "MPI_FINALIZE  ", 51);
  LIBFINIT(13, "MPI_COMM_RANK ", 52);
  LIBFINIT(14, "MPI_COMM_SIZE ", 53);
  LIBFINIT(15, "MPI_SEND      ", 54);
  LIBFINIT(16, "MPI_RECV      ", 55);
  LIBFINIT(17, "MPI_GET_COUNT ", 56);
  LIBFINIT(18, "MPI_BARRIER   ", 57);
  LIBFINIT(19, "MPI_BCAST     ", 58);
  LIBFINIT(20, "MPI_GATHER    ", 59);
  LIBFINIT(21, "MPI_SCATTER   ", 60);
  LIBFINIT(22, "MPI_REDUCE    ", 61);
  LIBFINIT(23, "MPI_ALLREDUCE ", 62);
  LIBFINIT(24, "MPI_IPROBE    ", 63);
  LIBFINIT(25, "MPI_CART_CREAT", 64);
  LIBFINIT(26, "MPI_CARTDIM_GE", 65);
  LIBFINIT(27, "MPI_CART_GET  ", 66);
  LIBFINIT(28, "MPI_CART_RANK ", 67);
  LIBFINIT(29, "MPI_CART_COORD", 68);
  LIBFINIT(30, "MPI_CART_SHIFT", 69);
  LIBFINIT(31, "MPI_COMM_FREE ", 70);
  LIBFINIT(32, "MPI_WTIME     ", 71);
  LIBFINIT(33, "MALLOC        ", 17);
  LIBFINIT(34, "CALLOC        ", 18);
  LIBFINIT(35, "REALLOC       ", 19);
  LIBFINIT(36, "FREE          ", 20);
  LIBFINIT(37, "STRCAT        ", 21);
  LIBFINIT(38, "STRCHR        ", 22);
  LIBFINIT(39, "STRCMP        ", 23);
  LIBFINIT(40, "STRCPY        ", 24);
  LIBFINIT(41, "STRLEN        ", 25);
  LIBFINIT(42, "STRSTR        ", 26);
  LIBFINIT(43, "ISALNUM       ", 27);
  LIBFINIT(44, "ISALPHA       ", 28);
  LIBFINIT(45, "ISCNTRL       ", 29);
  LIBFINIT(46, "ISDIGIT       ", 30);
  LIBFINIT(47, "ISGRAPH       ", 31);
  LIBFINIT(48, "ISLOWER       ", 32);
  LIBFINIT(49, "ISPRINT       ", 33);
  LIBFINIT(50, "ISPUNCT       ", 34);
  LIBFINIT(51, "ISSPACE       ", 35);
  LIBFINIT(52, "ISUPPER       ", 36);
  LIBFINIT(53, "ISXDIGIT      ", 37);
  LIBFINIT(54, "TOLOWER       ", 38);
  LIBFINIT(55, "TOUPPER       ", 39);
  LIBFINIT(56, "ABORT         ", 40);
  LIBFINIT(57, "EXIT          ", 41);
  LIBFINIT(58, "DIV           ", 42);
  LIBFINIT(59, "RAND          ", 43);
}

enum class ProgramError { GeneralError };

auto program() -> std::expected<void, ProgramError> {
  if (interactive) {
    string welcome{

    };

    std::cout << std::endl;
    std::cout << "                          C* COMPILER AND" << std::endl;
    std::cout << "                 PARALLEL COMPUTER SIMULATION SYSTEM "
              << std::endl;
    std::cout << "                             (VER. 2.2c++)" << std::endl;
    //            std::cout << "                             (VER. 2.1)" <<
    //            std::endl;
    std::cout << std::endl;
    std::cout
        << "     (C) Copyright 2007 by Bruce P. Lester, All Rights Reserved"
        << std::endl;
    //            std::cout << "     (C) Copyright 2006 by Bruce P. Lester, All
    //            Rights Reserved" << std::endl;
    std::cout << std::endl;
    std::cout << std::endl;
    std::cout << "  Basic Commands:" << std::endl;
    std::cout
        << "    *OPEN filename - Open and Compile your program source file"
        << std::endl;
    std::cout << "    *RUN - Initialize and run your program from the beginning"
              << std::endl;
    std::cout << "    *CLOSE - Close your program source file to allow editing"
              << std::endl;
    std::cout << "    *EXIT - Terminate this C* System" << std::endl;
    std::cout << "    *HELP - Show a complete list of commands" << std::endl;
    std::cout << std::endl;
  }
  INTERPRET();
  std::cout << std::endl;
  if (fatal_error && interactive) {
    fclose(SRC);
    // std::cout << "PROGRAM SOURCE FILE IS NOW CLOSED TO ALLOW EDITING" <<
    // std::endl;
    fclose(LIS);
    std::cout << std::endl;
    std::cout
        << "To continue, press ENTER key, then Restart the C* Software System"
        << std::endl;
    fgetc(stdin);
    std::exit(1);
  } else {
    if (OUTPUTFILE)
      fclose(OUTP);
    if (INPUTFILE)
      fclose(INP);
    if (SRCOPEN)
      fclose(SRC);
  }
}
} // namespace Cstar
static const char cfile[] = {'.', 'C', 's', 't', 'a', 'r',
                             '.', 'c', 'm', 'd', '\0'};
static bool mpi = false;
static void usage(string pgm) {
  printf("usage: %s [-i] [-h] [-l] [-m] [-Xabcrst] [file]\n", pgm);
  printf("     no operands implies -i\n");
  printf("  i  interactive - if file is specified, it will be OPEN'ed\n");
  printf("  h  display this help and exit\n");
  printf("  l  display listing on the console\n");
  printf("  m  set MPI ON\n");
  printf("  file  compile and execute a C* file (no -i switch)\n");
  printf("  X  execution options, combined in any order after X (no spaces)\n");
  printf("     a  display the array table\n");
  printf("     b  display the block table\n");
  printf("     c  display the generated interpreter code\n");
  printf("     r  display the real constant table\n");
  printf("     s  display the symbol table\n");
  printf("     t  trace interpreter instruction execution\n");
  std::exit(1);
}
static void doOption(const char *opt, const char *pgm) {
  int ix;
  switch (*opt) {
  case 'X':
    ix = 1;
    while (opt[ix] != '\0') {
      switch (opt[ix]) {
      case 'a':
        Cstar::showArrayList(true);
        break;
      case 'b':
        Cstar::showBlockList(true);
        break;
      case 'c':
        Cstar::showCodeList(true);
        break;
      case 'r':
        Cstar::showRealList(true);
        break;
      case 's':
        Cstar::showSymbolList(true);
        break;
      case 't':
        Cstar::showInstTrace(true);
        break;
      default:
        break;
      }
      ix += 1;
    }
    break;
  case 'i':
    Cstar::interactive = true;
    break;
  case 'l':
    Cstar::showConsoleList(true);
    break;
  case 'm':
    mpi = true;
    break;
  default:
    usage(pgm);
    break;
  }
}

static void cs_error(const char *p, int ern = 0, const char *p2 = "") {
  // std::cerr << p << " " << p2 << "\n";
  fprintf(stderr, "cs%03d %s %s\n", ern, p, p2);
  std::exit(1);
}

int main(int argc, const char *argv[]) {
  auto screen = ScreenInteractive::Fullscreen();
  FILE *from = nullptr, *to = nullptr, *cmds = nullptr;
  const char *from_file = nullptr;
  char *tbuf;
  int ix, jx;

  // overhead for longest prebuf command sequences
  const int prebuffer_overhead = 25;
  int return_code = 0;

#if defined(__APPLE__)
  __sFILE *stdin_save = nullptr;
  Cstar::STDOUT = __stdoutp;
#elif defined(__linux__) || defined(_WIN32)
  FILE *stdin_save = nullptr;
  Cstar::STDOUT = stdout;
#endif

  if (argc == 1)
    Cstar::interactive = true;

  for (ix = 1; ix < argc; ++ix) {
    if (argv[ix][0] == '-') {
      doOption(&argv[ix][1], argv[0]);
    } else if (from_file == nullptr) {
      from_file = argv[ix];
    } else
      usage(argv[0]);
  }

#if defined(__APPLE__)
  Cstar::STDIN = __stdinp;
#elif defined(__linux__) || defined(_WIN32)
  Cstar::STDIN = stdin;
#endif

  Cstar::pre_buf = make_unique<PreBuffer>(PreBuffer(STDIN));
  if (from_file != nullptr) {
    // if file present, open it to see if it exists, error out otherwise
    // close it because the interpreter will open it again
    from = fopen(from_file, "r");
    if (!from)
      cs_error("cannot open input file", 1, from_file);
    fclose(from);
  }
  if (Cstar::interactive) {
    // interactive - enter an OPEN command if a file is present
    if (from_file != nullptr) {
      // fprintf(Cstar::STDIN, "OPEN %s\n", from_file);
      ix = static_cast<int>(strlen(from_file)) + prebuffer_overhead;
      tbuf = static_cast<char *>(malloc(ix));
      jx =
          snprintf(tbuf, ix, "%sOPEN %s\n", (mpi) ? "MPI ON\n" : "", from_file);
      if (jx > ix)
        cs_error("command buffer length issue");
      Cstar::pre_buf->setBuffer(tbuf, jx);
      free(tbuf);
    }
  } else {
    // compile and execute
    //        cmds = fopen(cfile, "w");
    //        if (!cmds)
    //            error("cannot open command file", cfile);
    ix = static_cast<int>(std::strlen(from_file)) + prebuffer_overhead;
    tbuf = (char *)malloc(ix);
    // fprintf(cmds, "OPEN %s\nRUN\nEXIT\n", from_file);
    jx = std::snprintf(tbuf, ix, "%sOPEN %s\nRUN\nEXIT\n",
                       (mpi) ? "MPI ON\n" : "", from_file);
    if (jx > ix)
      cs_error("command buffer length issue");
    Cstar::pre_buf->setBuffer(tbuf, jx);
    free(tbuf);
    //        fclose(cmds);
    //        cmds = fopen(cfile, "r");
    //        if (!cmds)
    //            error("cannot reopen command file", cfile);
    //        stdin_save = __stdinp;
    //        Cstar::STDIN = cmds;
  }
  try {
    Cstar::program();
  } catch (std::exception &exc) {
    cs_error(exc.what());
    return_code = 2;
  }
  if (to != nullptr)
    fclose(to);
  if (cmds != nullptr) {
    fclose(cmds);
    unlink(cfile);
  }
  if (stdin_save != nullptr)

#if defined(__APPLE__)
    __stdinp = stdin_save;
#elif defined(__linux__) || defined(_WIN32)
  // stdin = stdin_save;
#endif

  return return_code;
}
