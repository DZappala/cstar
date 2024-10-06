//
// Created by Dan Evans on 2/6/24.
//
#include "displays.hpp"

#include "cs_compile.h"
#include "cs_global.h"
#include "cs_interpret.h"

#include <print>
#include <string_view>

namespace cs {
using std::print;
using std::println;
using std::string;
using std::string_view;

Displays::Displays()
  : curExec(
      {
        .functionSymtabIndex = 0,
        .blockTableIndex = 0,
        .functionVarFirst = 0,
        .functionVarLast = 0
      }
    ),
    opcodes(
      {
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
      }
    ),
    types(
      {
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
      }
    ),
    states(
      {
        "READY",
        "RUNNING",
        "BLOCKED",
        "DELAYED",
        "TERMINATED",
        "SPINNING"
      }
    ),
    read_status(
      {
        "NONE",
        "ATCHANNEL",
        "HASTICKET"
      }
    ),
    priority({"LOW", "HIGH"}),
    objects(
      {
        "CONSTANT",
        "VARIABLE",
        "TYPE1",
        "PROCEDURE",
        "FUNCTION",
        "COMPONENT",
        "STRUCTAG"
      }
    ),
    prepost({"pre", "post"}),
    sysname(
      {
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
      }
    ),
    status(
      {
        "NEVERUSED",
        "EMPTY    ",
        "RESERVED ",
        "FULL     "
      }
    ),
    nosym("nosym") {}

auto Displays::name_state(State s) -> string {
  return states.at(static_cast<int>(s));
}

auto Displays::name_read_status(ReadStatus s) -> string {
  return read_status.at(static_cast<int>(s));
}

auto Displays::processor_status(Status s) -> string {
  return status.at(static_cast<int>(s));
}

auto Displays::dump_inst(int idx) -> void {
  int f = 0;
  int x = 0;
  int y = 0;

  f = CODE.at(idx).F;
  x = CODE.at(idx).X;
  y = CODE.at(idx).Y;
  const string &parse_op = opcodes.at(f);
  string op = parse_op.empty() ? "..." : parse_op;

  switch (f) {
    case 0:
    case 1: {
      print(
        string_view{},
        "{}: {} {},{} {} {}\n",
        idx,
        f,
        x,
        y,
        op,
        lookup_sym(x, y)
      );
      break;
    }
    case 8: {
      print(
        string_view{},
        "{}: {} {},{} {} {}\n",
        idx,
        f,
        x,
        y,
        op,
        sysname.at(y)
      );
      break;
    }
    case 7:
    case 10: {
      print(string_view{}, "{}: {} {},{} {} {}\n", idx, f, x, y, op, y);
      break;
    }
    case 18: {
      print(
        string_view{},
        "{}: {} {},{} {} {}\n",
        idx,
        f,
        x,
        y,
        op,
        TAB[y].name
      );
      curExec.functionSymtabIndex = y;
      curExec.blockTableIndex = TAB[y].reference;
      curExec.functionVarFirst = BTAB[curExec.blockTableIndex].LAST;
      curExec.functionVarLast = BTAB[curExec.blockTableIndex].LASTPAR;
      break;
    }
    case 19: {
      print(
        string_view{},
        "{}: {} {},{} {} {}\n",
        idx,
        f,
        x,
        y,
        op,
        TAB[x].name
      );
      break;
    }
    case 23:
    case 110: {
      print(string_view{}, "{}: {} {},{} {} len={}\n", idx, f, x, y, op, x);
      break;
    }
    case 21: {
      print(
        string_view{},
        "{}: {} {},{} {} {}\n",
        idx,
        f,
        x,
        y,
        op,
        array_name(y)
      );
      break;
    }
    case 24:
    case 75:
    case 76:
    case 112: {
      print(string_view{}, "{}: {} {},{} {} {}\n", idx, f, x, y, op, y);
      break;
    }
    case 64: {
      print(
        string_view{},
        "{}: {} {},{} {} {}\n",
        idx,
        f,
        x,
        y,
        op,
        lookup_sym(x, y)
      );
      break;
    }
    case 104: {
      print(
        string_view{},
        "{}: {} {},{} {} {}{}\n",
        idx,
        f,
        x,
        y,
        op,
        x,
        y == 1 ? ",grprep" : ""
      );
      break;
    }
    case 108:
    case 109: {
      print(
        string_view{},
        "{}: {} {},{} {} {} {}\n",
        idx,
        f,
        x,
        y,
        op,
        prepost.at(y),
        x
      );
      break;
    }
    default: {
      print(string_view{}, "{}: {} {},{} {}\n", idx, f, x, y, op);
      break;
    }
  }
  print("{}", string_view{});
}

auto Displays::dump_l_inst(const int idx, int* line) -> void {
  while (LOCATION[*line] <= idx) {
    println("Line {}", *line);
    ++*line;
  }
  dump_inst(idx);
}

auto Displays::dump_code() -> void {
  int line = 1;
  for (int i = 0; i < line_count; ++i)
    dump_l_inst(i, &line);
}

auto Displays::dump_symbols() -> void {
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
      "{{}} {{:.14}} {{}} {{:>10}} {{:>5}} {{}} {{:2d}} {{}} "
      "{{}} {{}} {{}} {{}} {{:2d}}",
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

auto Displays::dump_arrays() -> void {
  println("Array Table");
  println("Indx IxTyp ElTyp ERef Low High ElSz Siz");
  for (int i = 0; i <= atab_index; ++i) {
    println(
      "{} {} {} {} {} {} {} {}",
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

auto Displays::dump_blocks() -> void {
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
      "{} {} {} {} {} {}",
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
      if (++cnt % 8 == 0)
        std::println("");
      lst = TAB[lst].link;
    }
    if (cnt % 8 != 0)
      println("");
  }
}

auto Displays::dump_reals() -> void {
  println("Real Constants");
  println("Indx     RConst  IConval    RConval");
  for (int i = 0; i <= CPNT; ++i) {
    println(
      "{} {:12.6f} {} {:12.6f}",
      i,
      CONTABLE.at(i),
      INITABLE[i].IVAL,
      INITABLE[i].RVAL
    );
  }
}

auto Displays::dump_pdes(const PROCPNT pd) -> void {
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
  for (idx = 1; idx <= LMAX; ++idx)
    print(", {}", pd->DISPLAY[idx]);
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

auto Displays::snap_pdes(
  Interpreter* interp,
  ProcessDescriptor* desc
) -> void {
  ProcessTable* process_table = nullptr;
  process_table = &interp->PROCTAB.at(desc->PROCESSOR);
  println("process ID: {}", desc->PID);
  println(
    "  state: {}",
    states.at(static_cast<int>(desc->STATE))
  );
  println("  time: {}", desc->TIME);
  println("  wake time: {}", desc->WAKETIME);
  println(
    "  processor: {}{}",
    desc->PROCESSOR,
    process_table->RUNPROC == desc ? " runproc" : ""
  );
  println(
    "  stack S[T] S[S[T]] {} {}",
    interp->S[desc->T],
    interp->S[interp->S[desc->T]]
  );
  println(
    "  rdstatus: {}",
    name_read_status(desc->READSTATUS)
  );
  println(
    "  priority: {}",
    desc->PRIORITY == PRIORITY::LOW
      ? "LOW"
      : "HIGH"
  );
  if (desc->STATE == State::RUNNING &&
      desc->TIME < interp->CLOCK)
    println("    can dispatch");
  else if (desc->STATE == State::READY) {
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
    println("    ptab VIRTIME {}", process_table->VIRTIME);
    println("    ptab STARTTIME {}", process_table->STARTTIME);
  }
}

auto Displays::dump_deadlock(Interpreter* interp) -> void {
  const ActiveProcess* active = nullptr;
  const ActiveProcess* head = nullptr;
  ProcessDescriptor* descriptor = nullptr;
  head = interp->ACPHEAD;
  active = head;
  do {
    descriptor = active->PDES;
    snap_pdes(interp, descriptor);
    active = active->NEXT;
  } while (active != nullptr && active != head);
}

auto Displays::dump_chans(Interpreter* interp) -> void {
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
    if (interp->CHAN.at(idx).HEAD != -1) {
      pid = interp->CHAN.at(idx).WAIT == nullptr
              ? -1
              : interp->CHAN.at(idx).WAIT->PDES->PID;
      println(
        "{} {} {} {} {} {:5d} {}",
        idx,
        interp->CHAN.at(idx).HEAD,
        interp->CHAN.at(idx).SEM,
        pid,
        interp->CHAN.at(idx).READTIME,
        interp->CHAN.at(idx).MOVED,
        interp->CHAN.at(idx).READER
      );
    }
  }
}

auto Displays::dump_stkfrms(const Interpreter* interp) -> void {
  BLKPNT free = nullptr;
  // int top = 0;
  int first = 0;
  int idx = 0;
  int val = 0;
  std::println("STACK USE");
  first = 0;
  free = interp->STHEAD;

  while (free != nullptr) {
    if (first < free->START) {
      println(
        "used {} - {} len {} processor {}",
        first,
        free->START - 1,
        free->START - first,
        interp->SLOCATION[first]
      );
    }
    println(
      "free {} - {} len {} SLOC {}",
      free->START,
      free->START + free->SIZE - 1,
      free->SIZE,
      interp->SLOCATION[free->START]
    );
    // top = free->START;
    first = free->START + free->SIZE;
    free = free->NEXT;
  }

  println("SLOCATION");
  val = interp->SLOCATION[0];
  first = idx = 0;

  while (idx < STMAX) {
    if (interp->SLOCATION[idx] != val) {
      println("first {} last {} val {}", first, idx - 1, val);
      first = idx;
      val = interp->SLOCATION[idx];
    }
    idx += 1;
  }
  println("first {} last {} val {}", first, idx - 1, val);
}

auto Displays::dump_proctab(Interpreter* interp) -> void {
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
  while (interp->PROCTAB.at(idx).STATUS != Status::NEVERUSED) {
    pid = interp->PROCTAB.at(idx).RUNPROC == nullptr
            ? -1
            : interp->PROCTAB.at(idx).RUNPROC->PID;
    println(
      "{} {} {} {} {} {} {} {} {}",
      idx,
      status.at(static_cast<int>(interp->PROCTAB.at(idx).STATUS)),
      interp->PROCTAB.at(idx).VIRTIME,
      interp->PROCTAB.at(idx).BRKTIME,
      interp->PROCTAB.at(idx).PROTIME,
      pid,
      interp->PROCTAB.at(idx).NUMPROC,
      interp->PROCTAB.at(idx).STARTTIME,
      interp->PROCTAB.at(idx).SPEED
    );
    idx += 1;
  }
}

auto Displays::dump_active(const Interpreter* interp) -> void {
  const ActiveProcess* act = nullptr;
  const ActiveProcess* acphead = nullptr;
  acphead = interp->ACPHEAD;
  act = acphead;
  do {
    dump_pdes(act->PDES);
    act = act->NEXT;
  } while (act != nullptr && act != acphead);
}

auto Displays::lookup_sym(const int lev, const int adr) -> string {
  int idx = 0;
  [[maybe_unused]] constexpr int blev = 0;
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
    if (TAB.at(idx).address == adr) return TAB.at(idx).name;
    idx = TAB.at(idx).link;
  }
  return nosym;
}

auto Displays::array_name(const int ref) const -> string {
  for (int idx = tab_index; idx >= 0; --idx) {
    if (TAB.at(idx).types == Types::ARRAYS && TAB[idx].reference == ref)
      return TAB.at(idx).name;
  }
  return nosym;
}
} // namespace cs
