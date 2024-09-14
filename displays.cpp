//
// Created by Dan Evans on 2/6/24.
//
#include "cs_block.h"
#include "cs_compile.h"
#include "cs_global.h"
#include "cs_interpret.h"

#include <cstring>
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
array<string, 115> opcodes{"pushstkloc", // 0
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
static array<string, 12> types{"NOTYP", "REALS", "INTS",  "BOOLS",
                               "CHARS", "ARRAY", "CHANS", "RECS",
                               "PNTS",  "FORWD", "LOCKS", "VOIDS"};

static array<string, 6> states{"READY",   "RUNNING",    "BLOCKED",
                               "DELAYED", "TERMINATED", "SPINNING"};

static array<string, 3> readstatus{"NONE", "ATCHANNEL", "HASTICKET"};
static array<string, 2> priority{"LOW", "HIGH"};
static array<string, 7> objects{"KONSTANT", "VARIABLE",  "TYPE1",   "PROZEDURE",
                                "FUNKTION", "COMPONENT", "STRUCTAG"};

static string nosym = "nosym";
static array<string, 2> prepost{"pre", "post"};

static array<string, 24> sysname{
    "", "", "", "", "", "", "", "",     "",      "",        "int()", "",
    "", "", "", "", "", "", "", "self", "clock", "seqtime", "myid",  "10"};

static array<string, 4> status{"NEVERUSED", "EMPTY    ", "RESERVED ",
                               "FULL     "};

static struct CurExec {
  int functionSymtabIndex;
  int blockTableIndex;
  int functionVarFirst;
  int functionVarLast;
} __attribute__((aligned(16))) curExec = {0, 0, 0, 0};

auto nameState(enum PROCESSDESCRIPTOR::STATE st) -> string {
  return states.at(static_cast<int>(st));
}
auto nameRdstatus(enum PROCESSDESCRIPTOR::READSTATUS st) -> string {
  return readstatus.at(static_cast<int>(st));
}

auto prcsrStatus(enum InterpLocal::PROCTAB::STATUS st) -> string {
  return status.at(static_cast<int>(st));
}
void dumpInst(int i) {
  int F = 0;
  int X = 0;
  int Y = 0;
  string op;

  F = code.at(i).F;
  X = code.at(i).X;
  Y = code.at(i).Y;
  string parse_op = opcodes.at(F);
  op = (parse_op.empty()) ? "..." : parse_op;

  switch (F) {
  case 0:
  case 1:
    print(string_view{}, "{{%4d}}: {{%3d}} {{%d}},{{%d}} {{%s}} {{%s}}\n", i, F,
          X, Y, op, lookupSym(X, Y));
    break;
  case 8:
    print(string_view{}, "%4d: %3d %d,%d %s %s\n", i, F, X, Y, op,
          sysname.at(Y));
    break;
  case 7:
  case 10:
    print(string_view{}, "%4d: %3d %d,%d %s %d\n", i, F, X, Y, op, Y);
    break;
  case 18:
    print(string_view{}, "%4d: %3d %d,%d %s %s\n", i, F, X, Y, op, TAB[Y].name);
    curExec.functionSymtabIndex = Y;
    curExec.blockTableIndex = TAB[Y].reference;
    curExec.functionVarFirst = BTAB[curExec.blockTableIndex].LAST;
    curExec.functionVarLast = BTAB[curExec.blockTableIndex].LASTPAR;
    break;
  case 19:
    print(string_view{}, "%4d: %3d %d,%d %s %s\n", i, F, X, Y, op, TAB[X].name);
    break;
  case 23:
  case 110:
    print(string_view{}, "%4d: %3d %d,%d %s len=%d\n", i, F, X, Y, op, X);
    break;

  case 21:
    print(string_view{}, "%4d: %3d %d,%d %s %s\n", i, F, X, Y, op,
          arrayName(Y));
    break;
  case 24:
  case 75:
  case 76:
  case 112:
    print(string_view{}, "%4d: %3d %d,%d %s %d\n", i, F, X, Y, op, Y);
    break;
  case 64:
    print(string_view{}, "%4d: %3d %d,%d %s %s\n", i, F, X, Y, op,
          lookupSym(X, Y));
    break;
  case 104:
    print(string_view{}, "%4d: %3d %d,%d %s %d%s\n", i, F, X, Y, op, X,
          (Y == 1) ? ",grprep" : "");
    break;
  case 108:
  case 109:
    print(string_view{}, "%4d: %3d %d,%d %s %s %d\n", i, F, X, Y, op,
          prepost.at(Y), X);
    break;
  default:
    print(string_view{}, "%4d: %3d %d,%d %s\n", i, F, X, Y, op);
    break;
  }
  print(STANDARD_OUTPUT, "{}", string_view{});
}
void dumpLInst(int idx, int *line) {
  while (LOCATION[*line] <= idx) {
    println(STANDARD_OUTPUT, "Line {:4}", *line);
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
  println(STANDARD_OUTPUT, "Symbol Table");
  println(STANDARD_OUTPUT, "Indx Name           Link     Object  Type Ref Nm Lev   Addr "
                  "  Size Fref FLev PP");
  for (int i = 0; i <= tab_index; ++i) {
    // strcpy(name, TAB[ix].NAME);
    print(STANDARD_OUTPUT,
          "{{:4}} {{:.14}} {{:4}} {{:>10}} {{:>5}} {{:3}} {{:2d}} {{:3}} "
          "{{:6}} {{:6}} {{:4}} {{:4}} {{:2d}}",
          i, TAB[i].name, TAB[i].link,
          objects.at(static_cast<int>(TAB[i].object)),
          types.at(static_cast<int>(TAB[i].types)), TAB[i].reference,
          static_cast<int>(TAB[i].normal), TAB[i].LEV, TAB[i].address,
          TAB[i].size, TAB[i].FREF, TAB[i].FORLEV, TAB[i].PNTPARAM);
  }
}
void dumpArrays() {
  println(STANDARD_OUTPUT, "Array Table");
  println(STANDARD_OUTPUT, "Indx IxTyp ElTyp ERef Low High ElSz Siz");
  for (int i = 0; i <= atab_index; ++i) {
    println(STANDARD_OUTPUT, "{:4} {:>5} {:>5} {:4} {:3} {:4} {:4} {:3}", i,
            types.at(static_cast<int>(ATAB[i].INXTYP)),
            types.at(static_cast<int>(ATAB[i].ELTYP)), ATAB[i].ELREF,
            ATAB[i].LOW, ATAB[i].HIGH, ATAB[i].ELSIZE, ATAB[i].SIZE);
  }
}
void dumpBlocks() {
  int idx = 0;
  int lst = 0;
  int cnt = 0;
  println(STANDARD_OUTPUT, "Block Table");
  println(STANDARD_OUTPUT, "  Display");
  idx = 1;
  while (idx <= LMAX && DISPLAY.at(idx) > 0) {
    std::println(STANDARD_OUTPUT, "  D{} {}", idx, DISPLAY[idx]);
    idx += 1;
  }
  std::println(STANDARD_OUTPUT, "Indx  Last LstPr PSize VSize PrCnt");
  for (idx = 0; idx <= btab_index; ++idx) {
    std::println(STANDARD_OUTPUT, "{:4} {:5} {:5} {:5} {:5} {:5}", idx, BTAB[idx].LAST,
                 BTAB[idx].LASTPAR, BTAB[idx].PSIZE, BTAB[idx].VSIZE,
                 BTAB[idx].PARCNT);
  }
  for (idx = btab_index; idx >= 1; --idx) {
    lst = BTAB[idx].LAST;
    std::println(STANDARD_OUTPUT, "block {} declares", idx);
    cnt = 0;
    while (lst != 0) {
      std::print(STANDARD_OUTPUT, " {}", TAB[lst].name);
      if (++cnt % 8 == 0) {
        std::println(STANDARD_OUTPUT, "");
      }
      lst = TAB[lst].link;
    }
    if (cnt % 8 != 0) {
      println(STANDARD_OUTPUT, "");
    }
  }
}
void dumpReals() {
  println(STANDARD_OUTPUT, "Real Constants");
  println(STANDARD_OUTPUT, "Indx     RConst  IConval    RConval");
  for (int i = 0; i <= CPNT; ++i) {
    println(STANDARD_OUTPUT, "{:4} {:12.6f} {:6} {:12.6f}", i, CONTABLE.at(i),
            INITABLE[i].IVAL, INITABLE[i].RVAL);
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
  println(STANDARD_OUTPUT, "**current stack top: {}", pd->T);
  println(STANDARD_OUTPUT, "  block table index: {}", pd->B);
  println(STANDARD_OUTPUT, "         current PC: {}", pd->PC);
  println(STANDARD_OUTPUT, "          stacksize: {}", pd->STACKSIZE);
  print(STANDARD_OUTPUT, "            display: {}", pd->DISPLAY[0]);
  for (idx = 1; idx <= LMAX; ++idx) {
    print(STANDARD_OUTPUT, ", {}", pd->DISPLAY[idx]);
  }
  println(STANDARD_OUTPUT, "");
  ppid = (pd->PARENT == nullptr) ? -1 : pd->PARENT->PID;
  println(STANDARD_OUTPUT, "             parent: {}", ppid);
  println(STANDARD_OUTPUT, " number of children: {}", pd->NUMCHILDREN);
  println(STANDARD_OUTPUT, "         stack base: {}", pd->BASE);
  println(STANDARD_OUTPUT, "     max child time: {:f}", pd->MAXCHILDTIME);
  println(STANDARD_OUTPUT, "               time: {:f}", pd->TIME);
  println(STANDARD_OUTPUT, "          for level: {}", pd->FORLEVEL);
  println(STANDARD_OUTPUT, "          for index: {}", pd->FORINDEX);
  println(STANDARD_OUTPUT, "          processor: {}", pd->PROCESSOR);
  println(STANDARD_OUTPUT, "      alt processor: {}", pd->ALTPROC);
  println(STANDARD_OUTPUT, "          wake time: {:f}", pd->WAKETIME);
  println(STANDARD_OUTPUT, "              state: {}",
          states.at(static_cast<int>(pd->STATE)));
  println(STANDARD_OUTPUT, "         process ID: {}", pd->PID);
  println(STDOUT, "       virtual time: {:f}", pd->VIRTUALTIME);
  println(STDOUT, "         read staus: {}",
          readstatus.at(static_cast<int>(pd->READSTATUS)));
  println(STDOUT, "         fork count: {}", pd->FORKCOUNT);
  println(STDOUT, "     join semaphore: {}", pd->JOINSEM);
  println(STDOUT, "      max fork time: {:f}", pd->MAXFORKTIME);
  println(STDOUT, "           priority: {}",
          priority.at(static_cast<int>(pd->PRIORITY)));
  println(STDOUT, "        sequence on: {}", (pd->SEQON) ? "true" : "false");
  println(STDOUT, "          group rep: {}", (pd->GROUPREP) ? "true" : "false");
}

void snapPDES(InterpLocal *interp_local,
              PROCESSDESCRIPTOR *process_descriptor) {
  struct InterpLocal::PROCTAB *ptab = nullptr;
  ptab = &interp_local->PROCTAB[process_descriptor->PROCESSOR];
  println(STDOUT, "process ID: {}", process_descriptor->PID);
  println(STDOUT, "  state: {}",
          states.at(static_cast<int>(process_descriptor->STATE)));
  println(STDOUT, "  time: {:7.1f}", process_descriptor->TIME);
  println(STDOUT, "  wake time: {:7.1f}", process_descriptor->WAKETIME);
  println(STDOUT, "  processor: {}{}", process_descriptor->PROCESSOR,
          (ptab->RUNPROC == process_descriptor) ? " runproc" : "");
  println(STDOUT, "  stack S[T] S[S[T]] {} {}",
          interp_local->S[process_descriptor->T],
          interp_local->S[interp_local->S[process_descriptor->T]]);
  println(STDOUT, "  rdstatus: {}",
          nameRdstatus(process_descriptor->READSTATUS));
  println(STDOUT, "  priority: {}",
          (process_descriptor->PRIORITY == PROCESSDESCRIPTOR::PRIORITY::LOW)
              ? "LOW"
              : "HIGH");
  if (process_descriptor->STATE == PROCESSDESCRIPTOR::STATE::RUNNING &&
      process_descriptor->TIME < interp_local->CLOCK) {
    println(STDOUT, "    can dispatch");
  } else if (process_descriptor->STATE == PROCESSDESCRIPTOR::STATE::READY) {
    println(STDOUT, "    ptab RUNPROC {}null",
            (ptab->RUNPROC != nullptr) ? "non-" : "");
    if (ptab->RUNPROC != nullptr) {
      println(STDOUT, "        RUNPROC PID {}", ptab->RUNPROC->PID);
      println(STDOUT, "        RUNPROC STATE {}",
              states.at(static_cast<int>(ptab->RUNPROC->STATE)));
    }
    println(STDOUT, "    ptab VIRTIME {:7.1f}", ptab->VIRTIME);
    println(STDOUT, "    ptab STARTTIME {:7.1f}", ptab->STARTTIME);
  }
}
void dumpDeadlock(InterpLocal *interp_local) {
  ACTIVEPROCESS *act = nullptr;
  ACTIVEPROCESS *acphead = nullptr;
  PROCESSDESCRIPTOR *pd = nullptr;
  struct InterpLocal::PROCTAB *ptab = nullptr;
  acphead = interp_local->ACPHEAD;
  act = acphead;
  do {
    pd = act->PDES;
    snapPDES(interp_local, pd);
    act = act->NEXT;
  } while (act != nullptr && act != acphead);
}

void dumpChans(InterpLocal *interp_local) {
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
  PROCESSDESCRIPTOR *pdes = nullptr;
  println(STDOUT, "Channels");
  println(STDOUT, "Indx  Head   Sem  WPID   RTime Moved Reader");
  for (idx = 0; idx <= OPCHMAX; ++idx) {
    if (interp_local->CHAN[idx].HEAD != -1) {
      pid = (interp_local->CHAN[idx].WAIT == nullptr)
                ? -1
                : interp_local->CHAN[idx].WAIT->PDES->PID;
      println(STDOUT, "{:4} {:5} {:5} {:5} {:7.1f} {:5d} {:6}", idx,
              interp_local->CHAN[idx].HEAD, interp_local->CHAN[idx].SEM, pid,
              interp_local->CHAN[idx].READTIME, interp_local->CHAN[idx].MOVED,
              interp_local->CHAN[idx].READER);
    }
  }
}
void dumpStkfrms(InterpLocal *interp_local) {
  BLKPNT free = nullptr;
  int top = 0;
  int first = 0;
  int idx = 0;
  int val = 0;
  std::println(STDOUT, "STACK USE");
  first = 0;
  free = interp_local->STHEAD;

  while (free != nullptr) {
    if (first < free->START) {
      println(STDOUT, "used {} - {} len {} processor {}", first,
              free->START - 1, free->START - first,
              interp_local->SLOCATION[first]);
    }
    println(STDOUT, "free {} - {} len {} SLOC {}", free->START,
            free->START + free->SIZE - 1, free->SIZE,
            interp_local->SLOCATION[free->START]);
    top = free->START;
    first = free->START + free->SIZE;
    free = free->NEXT;
  }

  println(STDOUT, "SLOCATION");
  val = interp_local->SLOCATION[0];
  first = idx = 0;

  while (idx < STMAX) {
    if (interp_local->SLOCATION[idx] != val) {
      println(STDOUT, "first {} last {} val {}", first, idx - 1, val);
      first = idx;
      val = interp_local->SLOCATION[idx];
    }
    idx += 1;
  }
  println(STDOUT, "first {} last {} val {}", first, idx - 1, val);
}

void dumpProctab(InterpLocal *interp_local) {
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
  std::println(STDOUT, "Processor Table");
  std::println(
      STDOUT,
      "Indx Status    Virtime Brktime Protime  Rpid  Num Strtime   Speed");
  idx = 0;
  while (interp_local->PROCTAB[idx].STATUS !=
         InterpLocal::PROCTAB::STATUS::NEVERUSED) {
    pid = (interp_local->PROCTAB[idx].RUNPROC == nullptr)
              ? -1
              : interp_local->PROCTAB[idx].RUNPROC->PID;
    println(
        STDOUT, "{:4} {:.9} {:7.1f} {:7.1f} {:7.1f} {:5} {:4} {:7.1f} {:7.1f}",
        idx, status.at(static_cast<int>(interp_local->PROCTAB[idx].STATUS)),
        interp_local->PROCTAB[idx].VIRTIME, interp_local->PROCTAB[idx].BRKTIME,
        interp_local->PROCTAB[idx].PROTIME, pid,
        interp_local->PROCTAB[idx].NUMPROC,
        interp_local->PROCTAB[idx].STARTTIME, interp_local->PROCTAB[idx].SPEED);
    idx += 1;
  }
}
void dumpActive(InterpLocal *interp_local) {
  ACTIVEPROCESS *act = nullptr;
  ACTIVEPROCESS *acphead = nullptr;
  acphead = interp_local->ACPHEAD;
  act = acphead;
  do {
    dumpPDES(act->PDES);
    act = act->NEXT;
  } while (act != nullptr && act != acphead);
}
auto lookupSym(int lev, int adr) -> string {
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
    if (TAB.at(idx).address == adr) {
      return TAB.at(idx).name;
    }
    idx = TAB.at(idx).link;
  }
  return nosym;
}
auto arrayName(int ref) -> string {
  for (int idx = tab_index; idx >= 0; --idx) {
    if (TAB.at(idx).types == Types::ARRAYS && TAB[idx].reference == ref) {
      return TAB.at(idx).name;
    }
  }
  return nosym;
}
} // namespace Cstar
