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

#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>

#define EXPORT_CS_GLOBAL
#include "cs_errors.h"
#include "cs_global.h"
#define EXPORT_CS_COMPILE

#include "cs_compile.h"
#include "cs_PreBuffer.h"

using cs::PreBuffer, cs::STANDARD_INPUT;
using std::exit;
using std::make_unique, std::snprintf, std::string, std::cout, std::expected,
  std::unexpected;
using std::println;

auto cstar_button(
  const char* label,
  const std::function<void()> &callback
) -> ftxui::Component {
  return Button(
    label,
    callback,
    ftxui::ButtonOption::Animated(
      ftxui::Color::GrayDark,
      ftxui::Color::White,
      ftxui::Color::White,
      ftxui::Color::GrayDark
    )
  );
}

namespace cs {
  extern void INTERPRET();
  extern void showCodeList(bool);
  extern void showBlockList(bool);
  extern void showSymbolList(bool);
  extern void showConsoleList(bool);
  extern void showArrayList(bool);
  extern void showRealList(bool);
  extern void showInstTrace(bool);

  namespace {
    auto PRE_BUF = std::make_unique<
      std::streambuf>(std::streambuf<char>());
  } // buffers

  bool interactive = false;

  static void enter(const char* X0, Objects X1, Types X2, int X3) {
    tab_index++;
    if (tab_index == TMAX)
      fatal(1);
    else {
      strcpy(TAB[tab_index].name.data(), X0);
      TAB[tab_index].link = tab_index - 1;
      TAB[tab_index].object = X1;
      TAB[tab_index].types = X2;
      TAB[tab_index].reference = 0;
      TAB[tab_index].normal = true;
      TAB[tab_index].LEV = 0;
      TAB[tab_index].address = X3;
      TAB[tab_index].FORLEV = 0;
      TAB[tab_index].PNTPARAM = false;
    }
  }

  void LIBFINIT(int index, const char* name, int idnum) {
    strcpy(LIBFUNC[index].NAME.data(), name);
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
    base_set.set(static_cast<int>(Symbol::PLUS), true);
    base_set.set(static_cast<int>(Symbol::MINUS), true);
    base_set.set(static_cast<int>(Symbol::INTCON), true);
    base_set.set(static_cast<int>(Symbol::CHARCON), true);
    base_set.set(static_cast<int>(Symbol::REALCON), true);
    base_set.set(static_cast<int>(Symbol::IDENT), true);
    // TYPEBEGSYS  = {IDENT, STRUCTSY, CONSTSY, SHORTSY, LONGSY, UNSIGNEDSY};
    type_set.set(static_cast<int>(Symbol::IDENT), true);
    type_set.set(static_cast<int>(Symbol::STRUCTSY), true);
    type_set.set(static_cast<int>(Symbol::CONSTSY), true);
    type_set.set(static_cast<int>(Symbol::SHORTSY), true);
    type_set.set(static_cast<int>(Symbol::LONGSY), true);
    type_set.set(static_cast<int>(Symbol::UNSIGNEDSY), true);
    // DECLBEGSYS  = {IDENT, STRUCTSY, CONSTSY, SHORTSY, LONGSY, UNSIGNEDSY,
    // TYPESY, DEFINESY, INCLUDESY}; //+ TYPEBEGSYS;
    declaration_set = type_set;
    declaration_set.set(static_cast<int>(Symbol::TYPESY), true);
    declaration_set.set(static_cast<int>(Symbol::DEFINESY), true);
    declaration_set.set(static_cast<int>(Symbol::INCLUDESY), true);
    // BLOCKBEGSYS = {IDENT, STRUCTSY, CONSTSY, SHORTSY, LONGSY, UNSIGNEDSY,
    // TYPESY, DEFINESY}; // + TYPEBEGSYS;
    block_set = declaration_set;
    block_set.set(static_cast<int>(Symbol::INCLUDESY), false);
    // FACBEGSYS = {INTCON, CHARCON, REALCON, IDENT, LPARENT, NOTSY, DECREMENT,
    // INCREMENT, PLUS, MINUS, TIMES, STRNG};
    FACBEGSYS.set(static_cast<int>(Symbol::INTCON), true);
    FACBEGSYS.set(static_cast<int>(Symbol::CHARCON), true);
    FACBEGSYS.set(static_cast<int>(Symbol::REALCON), true);
    FACBEGSYS.set(static_cast<int>(Symbol::IDENT), true);
    FACBEGSYS.set(static_cast<int>(Symbol::LPARENT), true);
    FACBEGSYS.set(static_cast<int>(Symbol::NOTSY), true);
    FACBEGSYS.set(static_cast<int>(Symbol::DECREMENT), true);
    FACBEGSYS.set(static_cast<int>(Symbol::INCREMENT), true);
    FACBEGSYS.set(static_cast<int>(Symbol::PLUS), true);
    FACBEGSYS.set(static_cast<int>(Symbol::MINUS), true);
    FACBEGSYS.set(static_cast<int>(Symbol::TIMES), true);
    FACBEGSYS.set(static_cast<int>(Symbol::STRNG), true);
    //        STATBEGSYS = {IFSY, WHILESY, DOSY, FORSY, FORALLSY, SEMICOLON,
    //        FORKSY, JOINSY, SWITCHSY, LSETBRACK, RETURNSY,
    //                      BREAKSY, CONTSY, MOVESY, CINSY, COUTSY};
    keyword_set.set(static_cast<int>(Symbol::IFSY), true);
    keyword_set.set(static_cast<int>(Symbol::WHILESY), true);
    keyword_set.set(static_cast<int>(Symbol::DOSY), true);
    keyword_set.set(static_cast<int>(Symbol::FORSY), true);
    keyword_set.set(static_cast<int>(Symbol::FORALLSY), true);
    keyword_set.set(static_cast<int>(Symbol::SEMICOLON), true);
    keyword_set.set(static_cast<int>(Symbol::FORKSY), true);
    keyword_set.set(static_cast<int>(Symbol::JOINSY), true);
    keyword_set.set(static_cast<int>(Symbol::SWITCHSY), true);
    keyword_set.set(static_cast<int>(Symbol::LSETBRACK), true);
    keyword_set.set(static_cast<int>(Symbol::RETURNSY), true);
    keyword_set.set(static_cast<int>(Symbol::BREAKSY), true);
    keyword_set.set(static_cast<int>(Symbol::CONTSY), true);
    keyword_set.set(static_cast<int>(Symbol::MOVESY), true);
    keyword_set.set(static_cast<int>(Symbol::CINSY), true);
    keyword_set.set(static_cast<int>(Symbol::COUTSY), true);
    // ASSIGNBEGSYS = {IDENT, LPARENT, DECREMENT, INCREMENT, TIMES};
    assigners_set.set(static_cast<int>(Symbol::IDENT), true);
    assigners_set.set(static_cast<int>(Symbol::LPARENT), true);
    assigners_set.set(static_cast<int>(Symbol::DECREMENT), true);
    assigners_set.set(static_cast<int>(Symbol::INCREMENT), true);
    assigners_set.set(static_cast<int>(Symbol::TIMES), true);
    // COMPASGNSYS = {PLUSEQ, MINUSEQ, TIMESEQ, RDIVEQ, IMODEQ};
    comparators_set.set(static_cast<int>(Symbol::PLUSEQ), true);
    comparators_set.set(static_cast<int>(Symbol::MINUSEQ), true);
    comparators_set.set(static_cast<int>(Symbol::TIMESEQ), true);
    comparators_set.set(static_cast<int>(Symbol::RDIVEQ), true);
    comparators_set.set(static_cast<int>(Symbol::IMODEQ), true);
    // SELECTSYS = {LBRACK, PERIOD, RARROW};
    selection_set.set(static_cast<int>(Symbol::LBRACK), true);
    selection_set.set(static_cast<int>(Symbol::PERIOD), true);
    selection_set.set(static_cast<int>(Symbol::RARROW), true);
    // EXECSYS = {IFSY, WHILESY, DOSY, FORSY, FORALLSY, SEMICOLON, FORKSY, JOINSY,
    // SWITCHSY, LSETBRACK, RETURNSY,
    //            BREAKSY, CONTSY, MOVESY, CINSY, COUTSY, ELSESY, TOSY, DOSY,
    //            CASESY, DEFAULTSY};
    execution_set.set(static_cast<int>(Symbol::IFSY), true);
    execution_set.set(static_cast<int>(Symbol::WHILESY), true);
    execution_set.set(static_cast<int>(Symbol::DOSY), true);
    execution_set.set(static_cast<int>(Symbol::FORSY), true);
    execution_set.set(static_cast<int>(Symbol::FORALLSY), true);
    execution_set.set(static_cast<int>(Symbol::SEMICOLON), true);
    execution_set.set(static_cast<int>(Symbol::FORKSY), true);
    execution_set.set(static_cast<int>(Symbol::JOINSY), true);
    execution_set.set(static_cast<int>(Symbol::SWITCHSY), true);
    execution_set.set(static_cast<int>(Symbol::LSETBRACK), true);
    execution_set.set(static_cast<int>(Symbol::RETURNSY), true);
    execution_set.set(static_cast<int>(Symbol::BREAKSY), true);
    execution_set.set(static_cast<int>(Symbol::CONTSY), true);
    execution_set.set(static_cast<int>(Symbol::MOVESY), true);
    execution_set.set(static_cast<int>(Symbol::CINSY), true);
    execution_set.set(static_cast<int>(Symbol::COUTSY), true);
    execution_set.set(static_cast<int>(Symbol::ELSESY), true);
    execution_set.set(static_cast<int>(Symbol::TOSY), true);
    execution_set.set(static_cast<int>(Symbol::CASESY), true);
    execution_set.set(static_cast<int>(Symbol::DEFAULTSY), true);
    // NONMPISYS = {CHANSY, FORKSY, JOINSY, FORALLSY};
    non_mpi_set.set(static_cast<int>(Symbol::CHANSY), true);
    non_mpi_set.set(static_cast<int>(Symbol::FORKSY), true);
    non_mpi_set.set(static_cast<int>(Symbol::JOINSY), true);
    non_mpi_set.set(static_cast<int>(Symbol::FORALLSY), true);
    // STANTYPS = {NOTYP, REALS, INTS, BOOLS, CHARS};
    standard_set.set(static_cast<int>(Types::NOTYP), true);
    standard_set.set(static_cast<int>(Types::REALS), true);
    standard_set.set(static_cast<int>(Types::INTS), true);
    standard_set.set(static_cast<int>(Types::BOOLS), true);
    standard_set.set(static_cast<int>(Types::CHARS), true);
    line_count = 0;
    LL = 0;
    LL2 = 0;
    CC = 0;
    CC2 = 0;
    CH = ' ';
    CPNT = 1;
    LINE_NUMBER = 0;
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
    OK_BREAK = false;
    BREAK_POINT = 0;
    ITPNT = 1;
    DISPLAY[0] = 1;
    strcpy(dummy_name.data(), "************00");
    INCLUDE_FLAG = false;
    fatal_error = false;
    proto_index = -1;
    enter("              ", Objects::Variable, Types::NOTYP, 0);
    enter("FALSE         ", Objects::Constant, Types::BOOLS, 0);
    enter("TRUE          ", Objects::Constant, Types::BOOLS, 1);
    enter("NULL          ", Objects::Constant, Types::PNTS, 0);
    enter("CHAR          ", Objects::Type1, Types::CHARS, 1);
    enter("BOOLEAN       ", Objects::Type1, Types::BOOLS, 1);
    enter("INT           ", Objects::Type1, Types::INTS, 1);
    enter("FLOAT         ", Objects::Type1, Types::REALS, 1);
    enter("DOUBLE        ", Objects::Type1, Types::REALS, 1);
    enter("VOID          ", Objects::Type1, Types::VOIDS, 1);
    enter("SPINLOCK      ", Objects::Type1, Types::LOCKS, 1);
    enter("SELF          ", Objects::Function, Types::INTS, 19);
    enter("CLOCK         ", Objects::Function, Types::REALS, 20);
    enter("SEQTIME       ", Objects::Function, Types::REALS, 21);
    enter("MYID          ", Objects::Function, Types::INTS, 22);
    enter("SIZEOF        ", Objects::Function, Types::INTS, 24);
    enter("SEND          ", Objects::Procedure, Types::NOTYP, 3);
    enter("RECV          ", Objects::Procedure, Types::NOTYP, 4);
    enter("NEW           ", Objects::Procedure, Types::NOTYP, 5);
    enter("DELETE        ", Objects::Procedure, Types::NOTYP, 6);
    enter("LOCK          ", Objects::Procedure, Types::NOTYP, 7);
    enter("UNLOCK        ", Objects::Procedure, Types::NOTYP, 8);
    enter("DURATION      ", Objects::Procedure, Types::NOTYP, 9);
    enter("SEQOFF        ", Objects::Procedure, Types::NOTYP, 10);
    enter("SEQON         ", Objects::Procedure, Types::NOTYP, 11);
    enter("              ", Objects::Procedure, Types::NOTYP, 0);
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

  using ErrorBase = uint8_t;

  enum class ProgramError : ErrorBase { GeneralError };

  auto to_string(const ProgramError err) -> const char* {
    switch (err) {
      case ProgramError::GeneralError: return "GeneralError";
      default: return "unknown";
    }
  }

  enum class IOError : ErrorBase {
    FileOpen,
    FileRead,
    FileWrite,
    FileClose,
    InvalidPath,
    EmptyPath,
    BufferOverflow,
    BufferUnderflow
  };

  auto to_string(const IOError err) -> const char* {
    switch (err) {
      case IOError::FileOpen: return "There was an error opening the file";
      case IOError::FileRead: return "There was an error reading the file";
      case IOError::FileWrite: return "There was an error writing the file";
      case IOError::FileClose: return "There was an error closing the file";
      case IOError::InvalidPath: return "The path provided is invalid";
      case IOError::EmptyPath: return "No File path was provided";
      case IOError::BufferOverflow: return
          "command buffer could not fit OPEN command.\n
          "The `prebuffer_overhead` variable in the cstar source code
          "to increase the buffer size";
      case IOError::BufferUnderflow: return "BufferUnderflow";
      default: return "unknown";
    }
  }

  auto program() -> std::expected<void, ProgramError> {
    if (interactive) {
      auto screen = ftxui::ScreenInteractive::Fullscreen();
      int shift = 0;

      const ftxui::Element header_paragraph =
        ftxui::text("C* Compiler \nand parallel computer simulation system")
        | ftxui::hcenter | ftxui::bold;
      const ftxui::Element subheading =
        ftxui::text("(Ver. 3.0c++)") | ftxui::hcenter | ftxui::dim;
      const ftxui::Element copyright =
        ftxui::text(
          "(C) Copyright 2007 by Bruce P. Lester, All Rights Reserved"
        )
        | ftxui::hcenter | ftxui::dim;

      const ftxui::Elements header_content = {
        header_paragraph,
        subheading,
        copyright
      };

      const ftxui::Component header_renderer = ftxui::Renderer(
        [&] {
          return ftxui::vbox(
            {vbox(header_content) | ftxui::flex | ftxui::center}
          );
        }
      );

      const ftxui::Element open = ftxui::text(
        "*(O)PEN filename - Open and Compile your program source file"
      );
      const ftxui::Element run = ftxui::text(
        "*(R)UN - Initialize and run your program from the beginning"
      );
      const ftxui::Element close = ftxui::text(
        "*(C)LOSE - Close your program source file to allow editing"
      );
      const ftxui::Element exit = ftxui::text(
        "*(E)XIT - Terminate this C* System"
      );
      const ftxui::Element help = ftxui::text(
        "*(H)ELP - Show a complete list of commands"
      );

      const ftxui::Elements tutorial_content = {open, run, close, exit, help,};
      const ftxui::Component tutorial_renderer = ftxui::Renderer(
        [&] {
          return ftxui::vbox(
            {
              window(
                ftxui::text("Basic Commands"),
                vbox(tutorial_content) | ftxui::flex
              )
            }
          );
        }
      );

      const ftxui::Component exit_button =
        cstar_button("Exit", [&] { screen.Exit(); }) | ftxui::hcenter;

      const ftxui::Component main_container = ftxui::Container::Vertical(
        {
          ftxui::Container::Horizontal(
            {
              /* button greup */
              exit_button
            }
          ),
          header_renderer,
          tutorial_renderer
        }
      );

      ftxui::Component main_renderer = Renderer(
        main_container,
        [&] {
          return ftxui::vbox(
            {
              header_renderer->Render() | ftxui::flex,
              tutorial_renderer->Render() | ftxui::flex,
              exit_button->Render() | size(
                ftxui::WIDTH,
                ftxui::EQUAL,
                10
              ) | ftxui::hcenter
            }
          );
        }
      );

      main_renderer = main_renderer | ftxui::borderEmpty;

      std::atomic refresh_ui_continue = true;
      std::thread refresh_ui(
        [&] {
          while (refresh_ui_continue) {
            using namespace std::chrono_literals;
            std::this_thread::sleep_for(.05s);
            screen.Post([&] { shift++; });
            screen.Post(ftxui::Event::Custom);
          }
        }
      );

      screen.Loop(main_renderer);
      refresh_ui_continue = false;
      refresh_ui.join();
    }

    INTERPRET();
    std::cout << '\n';
    if (fatal_error && interactive) {
      fclose(SOURCE);

      // std::cout << "PROGRAM SOURCE FILE IS NOW CLOSED TO ALLOW EDITING" <<
      // std::endl;
      fclose(LISTFILE);
      std::cout << '\n';
      std::cout
        << "To continue, press ENTER key, then Restart the C* Software System"
        << '\n';
      fgetc(stdin);
      std::exit(1);
    }

    if (OUTPUTFILE_FLAG)
      fclose(OUTPUT);
    if (INPUTFILE_FLAG)
      fclose(INPUT);
    if (SOURCEOPEN_FLAG)
      fclose(SOURCE);
    return {};

    // auto usage_paragraph_rendeerer = ftxui::Renderer(
    //   [&] {
    //     string usage = "Usage: cstar [-i] [-h] [-l] [-m] [-Xabcrst] [file]";
    //     string no_operands = "no operands implies -i";
    //     string i =
    //       "i  interactive - if file is specified, it will be OPEN'ed";
    //     string h = "h  display this help and exit";
    //     string l = "l  display listing on the console";
    //     string m = "m  set MPI ON";
    //     string file = "file  compile and execute a C* file (no -i switch)";
    //     string X =
    //       "X  execution options, combined in any order after X (no spaces)";
    //     string a = "a  display the array table";
    //     string b = "b  display the block table";
    //     string c = "c  display the generated interpreter code";
    //     string r = "r  display the real constant table";
    //     string s = "s  display the symbol table";
    //     string t = "t  trace interpreter instruction execution";
    //     return ftxui::vbox({});
    //   }
    // );
  }
} // namespace Cstar

using cs::program;
using std::exception;

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

static void usage(string pgm) {
  println("usage: {} [-i] [-h] [-l] [-m] [-Xabcrst] [file]", pgm);
  println("     no operands implies -i");
  println("  i  interactive - if file is specified, it will be OPEN'ed");
  println("  h  display this help and exit");
  println("  l  display listing on the console");
  println("  m  set MPI ON");
  println("  file  compile and execute a C* file (no -i switch)");
  println("  X  execution options, combined in any order after X (no spaces)");
  println("     a  display the array table");
  println("     b  display the block table");
  println("     c  display the generated interpreter code");
  println("     r  display the real constant table");
  println("     s  display the symbol table");
  println("     t  trace interpreter instruction execution");
  exit(1);
}

static void doOption(const char* opt, const char* pgm) {
  int ix;
  switch (*opt) {
    case 'X': ix = 1;
      while (opt[ix] != '\0') {
        switch (opt[ix]) {
          case 'a': cs::showArrayList(true);
            break;
          case 'b': cs::showBlockList(true);
            break;
          case 'c': cs::showCodeList(true);
            break;
          case 'r': cs::showRealList(true);
            break;
          case 's': cs::showSymbolList(true);
            break;
          case 't': cs::showInstTrace(true);
            break;
          default: break;
        }
        ix += 1;
      }
      break;
    case 'i': cs::interactive = true;
      break;
    case 'l': cs::showConsoleList(true);
      break;
    case 'm': mpi = true;
      break;
    default: usage(pgm);
      break;
  }
}

namespace {
  template<typename T> requires std::is_same_v<T, std::string> ||
                                std::is_same_v<T, const char*>
  void cs_error(T err_msg, int err_num = 0, T context = "") {
    println("cs{:03} {} {}", err_num, err_msg, context);
    exit(1);
  }

  template<typename T> requires
    std::is_same_v<std::underlying_type_t<T>, cs::ErrorBase>
  void cs_error(T err, int err_num = 0, T context = "") {
    println("cs{:03} {} {}", err_num, to_string(err), context);
    exit(1);
  }
} // namespace

namespace {
  auto make_prebuf(
    const fs::path &from_file,
    string &temp_buf,
    const int prebuffer_overhead
  ) -> std::expected<void, cs::IOError> {
    temp_buf =
      string(mpi ? "MPI ON\n" : "")
      + (cs::interactive
           ? std::format("OPEN {}\n", from_file.string())
           : std::format("OPEN {}\n", from_file.string()) + "RUN\nEXIT\n");

    if (temp_buf.length() > prebuffer_overhead)
      return std::unexpected(cs::IOError::BufferOverflow);

    cs::PRE_BUF->pubsetbuf(temp_buf.data(), temp_buf.size());
    return {};
  }

  auto check_file(
    const fs::path &from_file_path,
    std::ifstream &from_file
  ) -> std::expected<bool, cs::IOError> {
    if (from_file_path.empty()) return std::unexpected(cs::IOError::EmptyPath);
    from_file = std::ifstream(from_file_path);
    if (!from_file) return std::unexpected(cs::IOError::FileOpen);
    return true;
  }
} // namespace

auto main(int argc, const char* argv[]) -> int {
  using cs::ProgramError;

  fs::path from_file_path;
  auto to_file = std::ofstream();
  auto commands = std::fstream();
  auto from_file = std::ifstream();
  string temp_buf;
  int ix = 0;

  // overhead for longest prebuf command sequences
  int prebuffer_overhead = 25;
  int return_code = 0;
  cs::interactive = argc == 1;

  for (ix = 1; ix < argc; ++ix) {
    if (argv[ix][0] == '-') doOption(&argv[ix][1], argv[0]);
    else if (from_file_path.empty()) from_file_path = argv[ix];
    else usage(argv[0]);
  }

  check_file(from_file_path, from_file).and_then(
    make_prebuf(from_file_path, temp_buf, prebuffer_overhead)
  ).or_else(
    [](const cs::IOError &err)-> std::expected<void, cs::IOError> {
      cs_error<cs::IOError>(err);
      return {};
    }
  );

  program().or_else(
    [](const ProgramError &err)-> std::expected<void, ProgramError> {
      cs_error<ProgramError>(err);
      return {};
    }
  );

  if (to_file.is_open()) to_file.close();
  if (commands.is_open()) {
    commands.close();
    unlink(cfile);
  }

  return return_code;
}


