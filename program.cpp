//
// Created by Dan Evans on 1/27/24.
//
#include <cstdio>
#include <iostream>
#include <unistd.h>
#define EXPORT_CS_GLOBAL
#include "cs_errors.h"
#include "cs_global.h"
#define EXPORT_CS_COMPILE
#include "cs_PreBuffer.h"
#include "cs_compile.h"

namespace Cstar {
  extern void INTERPRET();
  extern void showCodeList(bool);
  extern void showBlockList(bool);
  extern void showSymbolList(bool);
  extern void showConsoleList(bool);
  extern void showArrayList(bool);
  extern void showRealList(bool);
  extern void showInstTrace(bool);
  extern PreBuffer* prebuf;
  bool interactive = false;

  static void ENTER(const char* X0, OBJECTS X1, TYPES X2, int X3) {
    Tx++;
    if (Tx == TMAX) {
      FATAL(1);
    } else {
      strcpy(TAB[Tx].NAME, X0);
      TAB[Tx].LINK = Tx - 1;
      TAB[Tx].OBJ = X1;
      TAB[Tx].TYP = X2;
      TAB[Tx].REF = 0;
      TAB[Tx].NORMAL = true;
      TAB[Tx].LEV = 0;
      TAB[Tx].ADR = X3;
      TAB[Tx].FORLEV = 0;
      TAB[Tx].PNTPARAM = false;
    }
  }

  void LIBFINIT(int index, const char* name, int idnum) {
    strcpy(LIBFUNC[index].NAME, name);
    LIBFUNC[index].IDNUM = idnum;
  }

  void INITCOMPILER() {
    KSY[0] = DEFINESY;
    KSY[1] = DEFINESY;
    KSY[2] = INCLUDESY;
    KSY[3] = BREAKSY;
    KSY[4] = CASESY;
    KSY[5] = CINSY;
    KSY[6] = CONSTSY;
    KSY[7] = CONTSY;
    KSY[8] = COUTSY;
    KSY[9] = DEFAULTSY;
    KSY[10] = DOSY;
    KSY[11] = ELSESY;
    KSY[12] = ENDLSY;
    KSY[13] = ENUMSY;
    KSY[14] = FORSY;
    KSY[15] = FORALLSY;
    KSY[16] = FORKSY;
    KSY[17] = FUNCTIONSY;
    KSY[18] = GROUPINGSY;
    KSY[19] = IFSY;
    KSY[20] = JOINSY;
    KSY[21] = LONGSY;
    KSY[22] = MOVESY;
    KSY[23] = NEWSY;
    KSY[24] = RETURNSY;
    KSY[25] = SHORTSY;
    KSY[26] = CHANSY;
    KSY[27] = STRUCTSY;
    KSY[28] = SWITCHSY;
    KSY[29] = THENSY;
    KSY[30] = TOSY;
    KSY[31] = TYPESY;
    KSY[32] = UNIONSY;
    KSY[33] = UNSIGNEDSY;
    KSY[34] = VALUESY;
    KSY[35] = WHILESY;
    SPS['('] = LPARENT;
    SPS[')'] = RPARENT;
    SPS[','] = COMMA;
    SPS['['] = LBRACK;
    SPS[']'] = RBRACK;
    SPS[';'] = SEMICOLON;
    SPS['~'] = BITCOMPSY;
    SPS['{'] = LSETBRACK;
    SPS['^'] = BITXORSY;
    SPS['@'] = ATSY;
    SPS['}'] = RSETBRACK;
    SPS[28] = EOFSY;
    SPS[':'] = COLON;
    // CONSTBEGSYS = {PLUS, MINUS, INTCON, CHARCON, REALCON, IDENT};
    CONSTBEGSYS[PLUS] = true;
    CONSTBEGSYS[MINUS] = true;
    CONSTBEGSYS[INTCON] = true;
    CONSTBEGSYS[CHARCON] = true;
    CONSTBEGSYS[REALCON] = true;
    CONSTBEGSYS[IDENT] = true;
    // TYPEBEGSYS  = {IDENT, STRUCTSY, CONSTSY, SHORTSY, LONGSY, UNSIGNEDSY};
    TYPEBEGSYS[IDENT] = true;
    TYPEBEGSYS[STRUCTSY] = true;
    TYPEBEGSYS[CONSTSY] = true;
    TYPEBEGSYS[SHORTSY] = true;
    TYPEBEGSYS[LONGSY] = true;
    TYPEBEGSYS[UNSIGNEDSY] = true;
    // DECLBEGSYS  = {IDENT, STRUCTSY, CONSTSY, SHORTSY, LONGSY, UNSIGNEDSY,
    // TYPESY, DEFINESY, INCLUDESY}; //+ TYPEBEGSYS;
    DECLBEGSYS = TYPEBEGSYS;
    DECLBEGSYS[TYPESY] = true;
    DECLBEGSYS[DEFINESY] = true;
    DECLBEGSYS[INCLUDESY] = true;
    // BLOCKBEGSYS = {IDENT, STRUCTSY, CONSTSY, SHORTSY, LONGSY, UNSIGNEDSY,
    // TYPESY, DEFINESY}; // + TYPEBEGSYS;
    BLOCKBEGSYS = DECLBEGSYS;
    BLOCKBEGSYS[INCLUDESY] = false;
    // FACBEGSYS = {INTCON, CHARCON, REALCON, IDENT, LPARENT, NOTSY, DECREMENT,
    // INCREMENT, PLUS, MINUS, TIMES, STRNG};
    FACBEGSYS[INTCON] = true;
    FACBEGSYS[CHARCON] = true;
    FACBEGSYS[REALCON] = true;
    FACBEGSYS[IDENT] = true;
    FACBEGSYS[LPARENT] = true;
    FACBEGSYS[NOTSY] = true;
    FACBEGSYS[DECREMENT] = true;
    FACBEGSYS[INCREMENT] = true;
    FACBEGSYS[PLUS] = true;
    FACBEGSYS[MINUS] = true;
    FACBEGSYS[TIMES] = true;
    FACBEGSYS[STRNG] = true;
    //        STATBEGSYS = {IFSY, WHILESY, DOSY, FORSY, FORALLSY, SEMICOLON,
    //        FORKSY, JOINSY, SWITCHSY, LSETBRACK, RETURNSY,
    //                      BREAKSY, CONTSY, MOVESY, CINSY, COUTSY};
    STATBEGSYS[IFSY] = true;
    STATBEGSYS[WHILESY] = true;
    STATBEGSYS[DOSY] = true;
    STATBEGSYS[FORSY] = true;
    STATBEGSYS[FORALLSY] = true;
    STATBEGSYS[SEMICOLON] = true;
    STATBEGSYS[FORKSY] = true;
    STATBEGSYS[JOINSY] = true;
    STATBEGSYS[SWITCHSY] = true;
    STATBEGSYS[LSETBRACK] = true;
    STATBEGSYS[RETURNSY] = true;
    STATBEGSYS[BREAKSY] = true;
    STATBEGSYS[CONTSY] = true;
    STATBEGSYS[MOVESY] = true;
    STATBEGSYS[CINSY] = true;
    STATBEGSYS[COUTSY] = true;
    // ASSIGNBEGSYS = {IDENT, LPARENT, DECREMENT, INCREMENT, TIMES};
    ASSIGNBEGSYS[IDENT] = true;
    ASSIGNBEGSYS[LPARENT] = true;
    ASSIGNBEGSYS[DECREMENT] = true;
    ASSIGNBEGSYS[INCREMENT] = true;
    ASSIGNBEGSYS[TIMES] = true;
    // COMPASGNSYS = {PLUSEQ, MINUSEQ, TIMESEQ, RDIVEQ, IMODEQ};
    COMPASGNSYS[PLUSEQ] = true;
    COMPASGNSYS[MINUSEQ] = true;
    COMPASGNSYS[TIMESEQ] = true;
    COMPASGNSYS[RDIVEQ] = true;
    COMPASGNSYS[IMODEQ] = true;
    // SELECTSYS = {LBRACK, PERIOD, RARROW};
    SELECTSYS[LBRACK] = true;
    SELECTSYS[PERIOD] = true;
    SELECTSYS[RARROW] = true;
    // EXECSYS = {IFSY, WHILESY, DOSY, FORSY, FORALLSY, SEMICOLON, FORKSY, JOINSY,
    // SWITCHSY, LSETBRACK, RETURNSY,
    //            BREAKSY, CONTSY, MOVESY, CINSY, COUTSY, ELSESY, TOSY, DOSY,
    //            CASESY, DEFAULTSY};
    EXECSYS[IFSY] = true;
    EXECSYS[WHILESY] = true;
    EXECSYS[DOSY] = true;
    EXECSYS[FORSY] = true;
    EXECSYS[FORALLSY] = true;
    EXECSYS[SEMICOLON] = true;
    EXECSYS[FORKSY] = true;
    EXECSYS[JOINSY] = true;
    EXECSYS[SWITCHSY] = true;
    EXECSYS[LSETBRACK] = true;
    EXECSYS[RETURNSY] = true;
    EXECSYS[BREAKSY] = true;
    EXECSYS[CONTSY] = true;
    EXECSYS[MOVESY] = true;
    EXECSYS[CINSY] = true;
    EXECSYS[COUTSY] = true;
    EXECSYS[ELSESY] = true;
    EXECSYS[TOSY] = true;
    EXECSYS[CASESY] = true;
    EXECSYS[DEFAULTSY] = true;
    // NONMPISYS = {CHANSY, FORKSY, JOINSY, FORALLSY};
    NONMPISYS[CHANSY] = true;
    NONMPISYS[FORKSY] = true;
    NONMPISYS[JOINSY] = true;
    NONMPISYS[FORALLSY] = true;
    // STANTYPS = {NOTYP, REALS, INTS, BOOLS, CHARS};
    STANTYPS[NOTYP] = true;
    STANTYPS[REALS] = true;
    STANTYPS[INTS] = true;
    STANTYPS[BOOLS] = true;
    STANTYPS[CHARS] = true;
    LC = 0;
    LL = 0;
    LL2 = 0;
    CC = 0;
    CC2 = 0;
    CH = ' ';
    CPNT = 1;
    LNUM = 0;
    ERRPOS = 0;
    ERRS = 0;
    INSYMBOL();
    EXECNT = 0;
    EOFCOUNT = 0;
    Tx = -1;
    A = 0;
    B = 1; // index to BTAB
    SX = 0;
    C2 = 0;
    C = 0;
    ERRORCOUNT = 0;
    OKBREAK = false;
    BREAKPNT = 0;
    ITPNT = 1;
    DISPLAY[0] = 1;
    strcpy(DUMMYNAME, "************00");
    INCLUDEFLAG = false;
    FATALERROR = false;
    PROTOINDEX = -1;
    ENTER("              ", VARIABLE, NOTYP, 0);
    ENTER("FALSE         ", KONSTANT, BOOLS, 0);
    ENTER("TRUE          ", KONSTANT, BOOLS, 1);
    ENTER("NULL          ", KONSTANT, PNTS, 0);
    ENTER("CHAR          ", TYPE1, CHARS, 1);
    ENTER("BOOLEAN       ", TYPE1, BOOLS, 1);
    ENTER("INT           ", TYPE1, INTS, 1);
    ENTER("FLOAT         ", TYPE1, REALS, 1);
    ENTER("DOUBLE        ", TYPE1, REALS, 1);
    ENTER("VOID          ", TYPE1, VOIDS, 1);
    ENTER("SPINLOCK      ", TYPE1, LOCKS, 1);
    ENTER("SELF          ", FUNKTION, INTS, 19);
    ENTER("CLOCK         ", FUNKTION, REALS, 20);
    ENTER("SEQTIME       ", FUNKTION, REALS, 21);
    ENTER("MYID          ", FUNKTION, INTS, 22);
    ENTER("SIZEOF        ", FUNKTION, INTS, 24);
    ENTER("SEND          ", PROZEDURE, NOTYP, 3);
    ENTER("RECV          ", PROZEDURE, NOTYP, 4);
    ENTER("NEW           ", PROZEDURE, NOTYP, 5);
    ENTER("DELETE        ", PROZEDURE, NOTYP, 6);
    ENTER("LOCK          ", PROZEDURE, NOTYP, 7);
    ENTER("UNLOCK        ", PROZEDURE, NOTYP, 8);
    ENTER("DURATION      ", PROZEDURE, NOTYP, 9);
    ENTER("SEQOFF        ", PROZEDURE, NOTYP, 10);
    ENTER("SEQON         ", PROZEDURE, NOTYP, 11);
    ENTER("              ", PROZEDURE, NOTYP, 0);
    BTAB[1].LAST = Tx;
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

  void PROGRAM() {
    if (interactive) {
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
      std::cout <<
        "    *RUN - Initialize and run your program from the beginning"
        << std::endl;
      std::cout <<
        "    *CLOSE - Close your program source file to allow editing"
        << std::endl;
      std::cout << "    *EXIT - Terminate this C* System" << std::endl;
      std::cout << "    *HELP - Show a complete list of commands" << std::endl;
      std::cout << std::endl;
    }
    INTERPRET();
    std::cout << std::endl;
    if (FATALERROR && interactive) {
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
static const char cfile[] = {
  '.',
  'C',
  's',
  't',
  'a',
  'r',
  '.',
  'c',
  'm',
  'd',
  '\0'
};
static bool mpi = false;

static void usage(const char* pgm) {
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

static void doOption(const char* opt, const char* pgm) {
  int ix = 0;
  switch (*opt) {
    case 'X': ix = 1;
      while (opt[ix] != '\0') {
        switch (opt[ix]) {
          case 'a': Cstar::showArrayList(true);
            break;
          case 'b': Cstar::showBlockList(true);
            break;
          case 'c': Cstar::showCodeList(true);
            break;
          case 'r': Cstar::showRealList(true);
            break;
          case 's': Cstar::showSymbolList(true);
            break;
          case 't': Cstar::showInstTrace(true);
            break;
          default: break;
        }
        ix += 1;
      }
      break;
    case 'i': Cstar::interactive = true;
      break;
    case 'l': Cstar::showConsoleList(true);
      break;
    case 'm': mpi = true;
      break;
    default: usage(pgm);
      break;
  }
}

static void cs_error(const char* p, int ern = 0, const char* p2 = "") {
  // std::cerr << p << " " << p2 << "\n";
  fprintf(stderr, "cs%03d %s %s\n", ern, p, p2);
  std::exit(1);
}

int main(int argc, const char* argv[]) {
  FILE *from = nullptr, *to = nullptr, *cmds = nullptr;
  const char* from_file = nullptr;
  char* tbuf;
  int ix, jx;
  const int PREOVHD = 25; // overhead for longest prebuf command sequences
  int return_code = 0;
#if defined(__APPLE__)
  __sFILE *stdin_save = nullptr;
  Cstar::STDOUT = __stdoutp;
#elif defined(__linux__) || defined(_WIN32)
  FILE* stdin_save = nullptr;
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
  Cstar::prebuf = new Cstar::PreBuffer(Cstar::STDIN);
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
      ix = (int)std::strlen(from_file) + PREOVHD;
      tbuf = (char*)malloc(ix);
      jx = std::snprintf(
        tbuf,
        ix,
        "%sOPEN %s\n",
        (mpi) ? "MPI ON\n" : "",
        from_file
      );
      if (jx > ix)
        cs_error("command buffer length issue");
      Cstar::prebuf->setBuffer(tbuf, jx);
      free(tbuf);
    }
  } else {
    // compile and execute
    //        cmds = fopen(cfile, "w");
    //        if (!cmds)
    //            error("cannot open command file", cfile);
    ix = (int)std::strlen(from_file) + PREOVHD;
    tbuf = (char*)malloc(ix);
    // fprintf(cmds, "OPEN %s\nRUN\nEXIT\n", from_file);
    jx = std::snprintf(
      tbuf,
      ix,
      "%sOPEN %s\nRUN\nEXIT\n",
      (mpi) ? "MPI ON\n" : "",
      from_file
    );
    if (jx > ix)
      cs_error("command buffer length issue");
    Cstar::prebuf->setBuffer(tbuf, jx);
    free(tbuf);
    //        fclose(cmds);
    //        cmds = fopen(cfile, "r");
    //        if (!cmds)
    //            error("cannot reopen command file", cfile);
    //        stdin_save = __stdinp;
    //        Cstar::STDIN = cmds;
  }
  try {
    Cstar::PROGRAM();
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
    delete Cstar::prebuf;
  return return_code;
}
