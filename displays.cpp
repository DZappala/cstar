//
// Created by Dan Evans on 2/6/24.
//
#include "cs_compile.h"
#include "cs_global.h"
#include "cs_interpret.h"

#include <print>
#include <string_view>

namespace cs {
  using std::array;
  using std::print;
  using std::println;
  using std::string;
  using std::string_view;

  auto lookupSym(int /*lev*/, int /*adr*/) -> string;
  auto arrayName(int /*ref*/) -> string;
  array<string, 115> opcodes{
    "pushstkloc",
    // 0
    "pushstklocri",
    "",
    "movdspbas",
    "clrchnoseq",
    "unhookprcr",
    "noop",
    "jmpseqon",
    "pushsys",
    "mpiinit",
    "jmp",
    "popjmpfalse",
    "",
    "getstkfrm1",
    "pushfm[T]",
    "addint",
    "",
    "",
    "getstkfrm2",
    "callblk",
    "swap",
    "pushaelstkloc",
    "pushpid",
    "cpystk[T]>[T-1]",
    "pushimm",
    "",
    "cnvi2r",
    "cin",
    "outstr",
    "outint",
    "setwidth/prec",
    "end",
    "returnv",
    "return",
    "rplindfmstk",
    "not",
    "negate",
    "outreal",
    "stindstk",
    "",
    "",
    "",
    "",
    "",
    "",
    "cmpeq",
    "cmpne",
    "cmplt",
    "cmple",
    "cmpgt",
    "cmpge",
    "or",
    "add",
    "sub",
    "",
    "",
    "and",
    "times",
    "intdiv",
    "intmod",
    "",
    "",
    "",
    "outendl",
    "recvchvar",
    "recvch[var]",
    "send",
    "fork",
    "",
    "procend --fork",
    "procend --child",
    "recvch[T]",
    "",
    "pushprcr",
    "forall",
    "forgrpT-2gtT-1jmp",
    "incrT-2ltT-1jmp",
    "",
    "wakepar",
    "findprctb",
    "",
    "",
    "initarr",
    "zeroarr",
    "dup",
    "join",
    "testvar",
    "pushrconfmtbl",
    "",
    "noswitchon",
    "noswitchoff",
    "",
    "sendri",
    "",
    "tststrm",
    "tststrmind",
    "tststrmstkind",
    "",
    "copymsg",
    "",
    "",
    "lock[T]",
    "unlock[T]",
    "",
    "ltincr[T-1]jmp",
    "",
    "seqoff",
    "seqon",
    "incr[T]imm",
    "decr[T]imm",
    "add3",
    "pop",
    "condrel[T]",
    "cpymsg",
    ""

  };
  static array<string, 12> types{
    "NOTYP",
    "REALS",
    "INTS",
    "BOOLS",
    "CHARS",
    "ARRAY",
    "CHANS",
    "RECS",
    "PNTS",
    "FORWD",
    "LOCKS",
    "VOIDS"
  };

  static array<string, 6> states{
    "READY",
    "RUNNING",
    "BLOCKED",
    "DELAYED",
    "TERMINATED",
    "SPINNING"
  };

  static array<string, 3> read_status{"NONE", "ATCHANNEL", "HASTICKET"};
  static array<string, 2> priority{"LOW", "HIGH"};
  static array<string, 7> objects{
    "KONSTANT",
    "VARIABLE",
    "TYPE1",
    "PROZEDURE",
    "FUNKTION",
    "COMPONENT",
    "STRUCTAG"
  };

  static string nosym = "nosym";
  static array<string, 2> prepost{"pre", "post"};

  static array<string, 24> sysname{
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "int()",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "self",
    "clock",
    "seqtime",
    "myid",
    "10"
  };

  static array<string, 4> status{
    "NEVERUSED",
    "EMPTY    ",
    "RESERVED ",
    "FULL     "
  };

  struct CurExec {
    int functionSymtabIndex;
    int blockTableIndex;
    int functionVarFirst;
    int functionVarLast;
  };

  static CurExec curExec = {0, 0, 0, 0};

  auto name_state(State st) -> string {
    return states.at(static_cast<int>(st));
  }

  auto name_read_status(ReadStatus st) -> string {
    return read_status.at(static_cast<int>(st));
  }

  auto processor_status(Status st) -> string {
    return status.at(static_cast<int>(st));
  }

  void dumpInst(int i) {
    int F = 0;
    int X = 0;
    int Y = 0;

    F = code.at(i).F;
    X = code.at(i).X;
    Y = code.at(i).Y;
    const string &parse_op = opcodes.at(F);
    string op = parse_op.empty() ? "..." : parse_op;

    switch (F) {
      case 0:
      case 1: print(
          string_view{},
          "{{%4d}}: {{%3d}} {{%d}},{{%d}} {{%s}} {{%s}}\n",
          i,
          F,
          X,
          Y,
          op,
          lookupSym(X, Y)
        );
        break;
      case 8: print(
          string_view{},
          "%4d: %3d %d,%d %s %s\n",
          i,
          F,
          X,
          Y,
          op,
          sysname.at(Y)
        );
        break;
      case 7:
      case 10: print(
          string_view{},
          "%4d: %3d %d,%d %s %d\n",
          i,
          F,
          X,
          Y,
          op,
          Y
        );
        break;
      case 18: print(
          string_view{},
          "%4d: %3d %d,%d %s %s\n",
          i,
          F,
          X,
          Y,
          op,
          TAB[Y].name
        );
        curExec.functionSymtabIndex = Y;
        curExec.blockTableIndex = TAB[Y].reference;
        curExec.functionVarFirst = BTAB[curExec.blockTableIndex].LAST;
        curExec.functionVarLast = BTAB[curExec.blockTableIndex].LASTPAR;
        break;
      case 19: print(
          string_view{},
          "%4d: %3d %d,%d %s %s\n",
          i,
          F,
          X,
          Y,
          op,
          TAB[X].name
        );
        break;
      case 23:
      case 110: print(
          string_view{},
          "%4d: %3d %d,%d %s len=%d\n",
          i,
          F,
          X,
          Y,
          op,
          X
        );
        break;

      case 21: print(
          string_view{},
          "%4d: %3d %d,%d %s %s\n",
          i,
          F,
          X,
          Y,
          op,
          arrayName(Y)
        );
        break;
      case 24:
      case 75:
      case 76:
      case 112: print(
          string_view{},
          "%4d: %3d %d,%d %s %d\n",
          i,
          F,
          X,
          Y,
          op,
          Y
        );
        break;
      case 64: print(
          string_view{},
          "%4d: %3d %d,%d %s %s\n",
          i,
          F,
          X,
          Y,
          op,
          lookupSym(X, Y)
        );
        break;
      case 104: print(
          string_view{},
          "%4d: %3d %d,%d %s %d%s\n",
          i,
          F,
          X,
          Y,
          op,
          X,
          Y == 1 ? ",grprep" : ""
        );
        break;
      case 108:
      case 109: print(
          string_view{},
          "%4d: %3d %d,%d %s %s %d\n",
          i,
          F,
          X,
          Y,
          op,
          prepost.at(Y),
          X
        );
        break;
      default: print(string_view{}, "%4d: %3d %d,%d %s\n", i, F, X, Y, op);
        break;
    }
    print("{}", string_view{});
  }

  void dumpLInst(int idx, int* line) {
    while (LOCATION[*line] <= idx) {
      println("Line {:4}", *line);
      ++*line;
    }
    dumpInst(idx);
  }

  void dumpCode() {
    int line = 1;
    for (int i = 0; i < line_count; ++i) {
      dumpLInst(i, &line);
    }
  }

  void dumpSymbols() {
    //        const char *np;
    //
    //        ALFA name;
    println("Symbol Table");
    println(
      "Indx Name           Link     Object  Type Ref Nm Lev   Addr "
      "  Size Fref FLev PP"
    );
    for (int i = 0; i <= tab_index; ++i) {
      // strcpy(name, TAB[ix].NAME);
      print(
        "{{:4}} {{:.14}} {{:4}} {{:>10}} {{:>5}} {{:3}} {{:2d}} {{:3}} "
        "{{:6}} {{:6}} {{:4}} {{:4}} {{:2d}}",
        i,
        TAB[i].name,
        TAB[i].link,
        objects.at(static_cast<int>(TAB[i].object)),
        types.at(static_cast<int>(TAB[i].types)),
        TAB[i].reference,
        static_cast<int>(TAB[i].normal),
        TAB[i].LEV,
        TAB[i].address,
        TAB[i].size,
        TAB[i].FREF,
        TAB[i].FORLEV,
        TAB[i].PNTPARAM
      );
    }
  }

  void dumpArrays() {
    println("Array Table");
    println("Indx IxTyp ElTyp ERef Low High ElSz Siz");
    for (int i = 0; i <= atab_index; ++i) {
      println(
        "{:4} {:>5} {:>5} {:4} {:3} {:4} {:4} {:3}",
        i,
        types.at(static_cast<int>(ATAB[i].INXTYP)),
        types.at(static_cast<int>(ATAB[i].ELTYP)),
        ATAB[i].ELREF,
        ATAB[i].LOW,
        ATAB[i].HIGH,
        ATAB[i].ELSIZE,
        ATAB[i].SIZE
      );
    }
  }

  void dumpBlocks() {
    int idx = 0;
    int lst = 0;
    int cnt = 0;
    println("Block Table");
    println("  Display");
    idx = 1;
    while (idx <= LMAX && DISPLAY.at(idx) > 0) {
      std::println("  D{} {}", idx, DISPLAY[idx]);
      idx += 1;
    }
    std::println("Indx  Last LstPr PSize VSize PrCnt");
    for (idx = 0; idx <= btab_index; ++idx) {
      std::println(
        "{:4} {:5} {:5} {:5} {:5} {:5}",
        idx,
        BTAB[idx].LAST,
        BTAB[idx].LASTPAR,
        BTAB[idx].PSIZE,
        BTAB[idx].VSIZE,
        BTAB[idx].PARCNT
      );
    }
    for (idx = btab_index; idx >= 1; --idx) {
      lst = BTAB[idx].LAST;
      std::println("block {} declares", idx);
      cnt = 0;
      while (lst != 0) {
        std::print(" {}", TAB[lst].name);
        if (++cnt % 8 == 0) {
          std::println("");
        }
        lst = TAB[lst].link;
      }
      if (cnt % 8 != 0) {
        println("");
      }
    }
  }

  void dumpReals() {
    println("Real Constants");
    println("Indx     RConst  IConval    RConval");
    for (int i = 0; i <= CPNT; ++i) {
      println(
        "{:4} {:12.6f} {:6} {:12.6f}",
        i,
        CONTABLE.at(i),
        INITABLE[i].IVAL,
        INITABLE[i].RVAL
      );
    }
  }

  void dumpPDES(PROCPNT pd) {
    int idx = 0;
    int ppid = 0;
    //        int T;  // process's stack top index
    //        int B;
    //        int PC;  // process's program counter (index into ORDER type CODE
    //        array) int STACKSIZE;  // process's stack size int DISPLAY[LMAX +
    //        1]; PROCPNT PARENT; int NUMCHILDREN; int BASE; float MAXCHILDTIME;
    //        float TIME;
    //        int FORLEVEL;
    //        int FORINDEX;
    //        int PROCESSOR;
    //        int ALTPROC;
    //        float WAKETIME;
    //        enum STATE
    //        {
    //            READY, RUNNING, BLOCKED, DELAYED, TERMINATED, SPINNING
    //        } STATE;
    //        int PID;
    //        float VIRTUALTIME;
    //        enum READSTATUS
    //        {
    //            NONE, ATCHANNEL, HASTICKET
    //        } READSTATUS;
    //        int FORKCOUNT;
    //        int JOINSEM;
    //        float MAXFORKTIME;
    //        enum PRIORITY
    //        {
    //            LOW, HIGH
    //        } PRIORITY;
    //        bool SEQON;
    //        bool GROUPREP;
    println("**current stack top: {}", pd->T);
    println("  block table index: {}", pd->B);
    println("         current PC: {}", pd->PC);
    println("          stacksize: {}", pd->STACKSIZE);
    print("            display: {}", pd->DISPLAY[0]);
    for (idx = 1; idx <= LMAX; ++idx) {
      print(", {}", pd->DISPLAY[idx]);
    }
    println("");
    ppid = pd->PARENT == nullptr ? -1 : pd->PARENT->PID;
    println("             parent: {}", ppid);
    println(" number of children: {}", pd->NUMCHILDREN);
    println("         stack base: {}", pd->BASE);
    println("     max child time: {:f}", pd->MAXCHILDTIME);
    println("               time: {:f}", pd->TIME);
    println("          for level: {}", pd->FORLEVEL);
    println("          for index: {}", pd->FORINDEX);
    println("          processor: {}", pd->PROCESSOR);
    println("      alt processor: {}", pd->ALTPROC);
    println("          wake time: {:f}", pd->WAKETIME);
    println(
      "              state: {}",
      states.at(static_cast<int>(pd->STATE))
    );
    println("         process ID: {}", pd->PID);
    println("       virtual time: {:f}", pd->VIRTUALTIME);
    println(
      "         read staus: {}",
      read_status.at(static_cast<int>(pd->READSTATUS))
    );
    println("         fork count: {}", pd->FORKCOUNT);
    println("     join semaphore: {}", pd->JOINSEM);
    println("      max fork time: {:f}", pd->MAXFORKTIME);
    println(
      "           priority: {}",
      priority.at(static_cast<int>(pd->PRIORITY))
    );
    println("        sequence on: {}", pd->SEQON ? "true" : "false");
    println(
      "          group rep: {}",
      pd->GROUPREP ? "true" : "false"
    );
  }

  void snapPDES(
    Interpreter* interpreter,
    ProcessDescriptor* process_descriptor
  ) {
    ProcessTable* process_table = nullptr;
    process_table = &interpreter->PROCTAB.at(process_descriptor->PROCESSOR);
    println("process ID: {}", process_descriptor->PID);
    println(
      "  state: {}",
      states.at(static_cast<int>(process_descriptor->STATE))
    );
    println("  time: {:7.1f}", process_descriptor->TIME);
    println("  wake time: {:7.1f}", process_descriptor->WAKETIME);
    println(
      "  processor: {}{}",
      process_descriptor->PROCESSOR,
      process_table->RUNPROC == process_descriptor ? " runproc" : ""
    );
    println(
      "  stack S[T] S[S[T]] {} {}",
      interpreter->S[process_descriptor->T],
      interpreter->S[interpreter->S[process_descriptor->T]]
    );
    println(
      "  rdstatus: {}",
      name_read_status(process_descriptor->READSTATUS)
    );
    println(
      "  priority: {}",
      process_descriptor->PRIORITY == PRIORITY::LOW
        ? "LOW"
        : "HIGH"
    );
    if (process_descriptor->STATE == State::RUNNING &&
        process_descriptor->TIME < interpreter->CLOCK)
      println("    can dispatch");
    else if (process_descriptor->STATE == State::READY) {
      println(
        "    ptab RUNPROC {}null",
        process_table->RUNPROC != nullptr ? "non-" : ""
      );
      if (process_table->RUNPROC != nullptr) {
        println("        RUNPROC PID {}", process_table->RUNPROC->PID);
        println(
          "        RUNPROC STATE {}",
          states.at(static_cast<int>(process_table->RUNPROC->STATE))
        );
      }
      println("    ptab VIRTIME {:7.1f}", process_table->VIRTIME);
      println("    ptab STARTTIME {:7.1f}", process_table->STARTTIME);
    }
  }

  auto dumpDeadlock(Interpreter* interpreter) -> void {
    const ActiveProcess* active = nullptr;
    const ActiveProcess* head = nullptr;
    ProcessDescriptor* descriptor = nullptr;
    head = interpreter->ACPHEAD;
    active = head;
    do {
      descriptor = active->PDES;
      snapPDES(interpreter, descriptor);
      active = active->NEXT;
    } while (active != nullptr && active != head);
  }

  auto dumpChans(Interpreter* interpreter) -> void {
    //         struct Channel {
    //            int HEAD;
    //            int SEM;
    //            ACTPNT WAIT;
    //            float READTIME;
    //            bool MOVED;
    //            int READER;
    //        } CHAN[OPCHMAX+1];
    int idx = 0;
    int pid = 0;
    println("Channels");
    println("Indx  Head   Sem  WPID   RTime Moved Reader");
    for (idx = 0; idx <= OPCHMAX; ++idx) {
      if (interpreter->CHAN.at(idx).HEAD != -1) {
        pid = interpreter->CHAN.at(idx).WAIT == nullptr
                ? -1
                : interpreter->CHAN.at(idx).WAIT->PDES->PID;
        println(
          "{:4} {:5} {:5} {:5} {:7.1f} {:5d} {:6}",
          idx,
          interpreter->CHAN.at(idx).HEAD,
          interpreter->CHAN.at(idx).SEM,
          pid,
          interpreter->CHAN.at(idx).READTIME,
          interpreter->CHAN.at(idx).MOVED,
          interpreter->CHAN.at(idx).READER
        );
      }
    }
  }

  auto dumpStkfrms(const Interpreter* interpreter) -> void {
    BLKPNT free = nullptr;
    // int top = 0;
    int first = 0;
    int idx = 0;
    int val = 0;
    std::println("STACK USE");
    first = 0;
    free = interpreter->STHEAD;

    while (free != nullptr) {
      if (first < free->START) {
        println(
          "used {} - {} len {} processor {}",
          first,
          free->START - 1,
          free->START - first,
          interpreter->SLOCATION[first]
        );
      }
      println(
        "free {} - {} len {} SLOC {}",
        free->START,
        free->START + free->SIZE - 1,
        free->SIZE,
        interpreter->SLOCATION[free->START]
      );
      // top = free->START;
      first = free->START + free->SIZE;
      free = free->NEXT;
    }

    println("SLOCATION");
    val = interpreter->SLOCATION[0];
    first = idx = 0;

    while (idx < STMAX) {
      if (interpreter->SLOCATION[idx] != val) {
        println("first {} last {} val {}", first, idx - 1, val);
        first = idx;
        val = interpreter->SLOCATION[idx];
      }
      idx += 1;
    }
    println("first {} last {} val {}", first, idx - 1, val);
  }

  void dumpProctab(Interpreter* interp_local) {
    //         struct PROCTAB {
    //            enum STATUS {NEVERUSED, EMPTY, RESERVED, FULL} STATUS;
    //            float VIRTIME;
    //            float BRKTIME;
    //            float PROTIME;
    //            PROCPNT RUNPROC;
    //            int NUMPROC;
    //            float STARTTIME;
    //            BUSYPNT BUSYLIST;
    //            float SPEED;
    //        } PROCTAB[PMAX+1];
    int idx = 0;
    int pid = 0;
    std::println("Processor Table");
    std::println(
      "Indx Status    Virtime Brktime Protime  Rpid  Num Strtime   Speed"
    );
    idx = 0;
    while (interp_local->PROCTAB.at(idx).STATUS != Status::NEVERUSED) {
      pid = interp_local->PROCTAB.at(idx).RUNPROC == nullptr
              ? -1
              : interp_local->PROCTAB.at(idx).RUNPROC->PID;
      println(
        "{:4} {:.9} {:7.1f} {:7.1f} {:7.1f} {:5} {:4} {:7.1f} {:7.1f}",
        idx,
        status.at(static_cast<int>(interp_local->PROCTAB.at(idx).STATUS)),
        interp_local->PROCTAB.at(idx).VIRTIME,
        interp_local->PROCTAB.at(idx).BRKTIME,
        interp_local->PROCTAB.at(idx).PROTIME,
        pid,
        interp_local->PROCTAB.at(idx).NUMPROC,
        interp_local->PROCTAB.at(idx).STARTTIME,
        interp_local->PROCTAB.at(idx).SPEED
      );
      idx += 1;
    }
  }

  void dumpActive(const Interpreter* interp_local) {
    ActiveProcess* act = nullptr;
    ActiveProcess* acphead = nullptr;
    acphead = interp_local->ACPHEAD;
    act = acphead;
    do {
      dumpPDES(act->PDES);
      act = act->NEXT;
    } while (act != nullptr && act != acphead);
  }

  auto lookupSym(const int lev, const int adr) -> string {
    int idx = 0;
    int blev = 0;
    //        for (ix = Tx; ix >= 0; --ix)
    //        {
    //            if (TAB[ix].ADR == adr && TAB[ix].LEV == lev)
    //                return TAB[ix].NAME;
    //        }
    //        for (ix = curExec.functionVarFirst; ix >= curExec.functionVarLast;
    //        --ix)
    //        {
    //            if (TAB[ix].ADR == adr && TAB[ix].LEV == lev)
    //                return TAB[ix].NAME;
    //        }
    idx = BTAB[lev].LAST;
    while (idx != 0) {
      if (TAB.at(idx).address == adr)
        return TAB.at(idx).name;
      idx = TAB.at(idx).link;
    }
    return nosym;
  }

  auto arrayName(const int ref) -> string {
    for (int idx = tab_index; idx >= 0; --idx) {
      if (TAB.at(idx).types == Types::ARRAYS && TAB[idx].reference == ref) {
        return TAB.at(idx).name;
      }
    }
    return nosym;
  }
} // namespace cs
