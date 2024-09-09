//
// Created by Dan Evans on 2/6/24.
//
#include "cs_block.h"
#include "cs_compile.h"
#include "cs_global.h"
#include "cs_interpret.h"
#include <cstdio>
#include <cstring>
namespace Cstar {
const char *lookupSym(int, int);
const char *arrayName(int);
const char *opcodes[115] = {"pushstkloc", // 0
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
static const char *types[] = {"NOTYP", "REALS", "INTS",  "BOOLS",
                              "CHARS", "ARRAY", "CHANS", "RECS",
                              "PNTS",  "FORWD", "LOCKS", "VOIDS"};
static const char *states[] = {"READY",   "RUNNING",    "BLOCKED",
                               "DELAYED", "TERMINATED", "SPINNING"};
static const char *readstatus[] = {"NONE", "ATCHANNEL", "HASTICKET"};
static const char *priority[] = {"LOW", "HIGH"};
static const char *objects[] = {"KONSTANT", "VARIABLE",  "TYPE1",   "PROZEDURE",
                                "FUNKTION", "COMPONENT", "STRUCTAG"};
static const char *nosym = "nosym";
static const char *prepost[] = {"pre", "post"};
static const char *sysname[] = {
    "", "", "", "", "", "", "", "",     "",      "",        "int()", "",
    "", "", "", "", "", "", "", "self", "clock", "seqtime", "myid",  "10"};
static const char *status[] = {"NEVERUSED", "EMPTY    ", "RESERVED ",
                               "FULL     "};

static struct CurExec {
  int functionSymtabIndex;
  int blockTableIndex;
  int functionVarFirst;
  int functionVarLast;
} curExec = {0, 0, 0, 0};
const char *nameState(enum PROCESSDESCRIPTOR::STATE st) { return states[st]; }
const char *nameRdstatus(enum PROCESSDESCRIPTOR::READSTATUS st) {
  return readstatus[st];
}
const char *prcsrStatus(enum InterpLocal::PROCTAB::STATUS st) {
  return status[st];
}
void dumpInst(int ix) {
  int F, X, Y;
  const char *op;
  char ibuf[80];

  F = CODE[ix].F;
  X = CODE[ix].X;
  Y = CODE[ix].Y;
  op = (opcodes[F] == nullptr) ? "..." : opcodes[F];
  switch (F) {
  case 0:
  case 1:
    snprintf(ibuf, sizeof ibuf, "%4d: %3d %d,%d %s %s\n", ix, F, X, Y, op,
             lookupSym(X, Y));
    break;
  case 8:
    snprintf(ibuf, sizeof ibuf, "%4d: %3d %d,%d %s %s\n", ix, F, X, Y, op,
             sysname[Y]);
    break;
  case 7:
  case 10:
    snprintf(ibuf, sizeof ibuf, "%4d: %3d %d,%d %s %d\n", ix, F, X, Y, op, Y);
    break;
  case 18:
    snprintf(ibuf, sizeof ibuf, "%4d: %3d %d,%d %s %s\n", ix, F, X, Y, op,
             TAB[Y].NAME);
    curExec.functionSymtabIndex = Y;
    curExec.blockTableIndex = TAB[Y].REF;
    curExec.functionVarFirst = BTAB[curExec.blockTableIndex].LAST;
    curExec.functionVarLast = BTAB[curExec.blockTableIndex].LASTPAR;
    break;
  case 19:
    snprintf(ibuf, sizeof ibuf, "%4d: %3d %d,%d %s %s\n", ix, F, X, Y, op,
             TAB[X].NAME);
    break;
  case 23:
  case 110:
    snprintf(ibuf, sizeof ibuf, "%4d: %3d %d,%d %s len=%d\n", ix, F, X, Y, op,
             X);
    break;

  case 21:
    snprintf(ibuf, sizeof ibuf, "%4d: %3d %d,%d %s %s\n", ix, F, X, Y, op,
             arrayName(Y));
    break;
  case 24:
  case 75:
  case 76:
  case 112:
    snprintf(ibuf, sizeof ibuf, "%4d: %3d %d,%d %s %d\n", ix, F, X, Y, op, Y);
    break;
  case 64:
    snprintf(ibuf, sizeof ibuf, "%4d: %3d %d,%d %s %s\n", ix, F, X, Y, op,
             lookupSym(X, Y));
    break;
  case 104:
    snprintf(ibuf, sizeof ibuf, "%4d: %3d %d,%d %s %d%s\n", ix, F, X, Y, op, X,
             (Y == 1) ? ",grprep" : "");
    break;
  case 108:
  case 109:
    snprintf(ibuf, sizeof ibuf, "%4d: %3d %d,%d %s %s %d\n", ix, F, X, Y, op,
             prepost[Y], X);
    break;
  default:
    snprintf(ibuf, sizeof ibuf, "%4d: %3d %d,%d %s\n", ix, F, X, Y, op);
    break;
  }
  fprintf(STDOUT, "%s", ibuf);
}
void dumpLInst(int ix, int *line) {
  while (LOCATION[*line] <= ix) {
    fprintf(STDOUT, "Line %4d\n", *line);
    ++*line;
  }
  dumpInst(ix);
}

void dumpCode() {
  int ix;
  int line = 1;
  for (ix = 0; ix < LC; ++ix) {
    dumpLInst(ix, &line);
  }
}
void dumpSymbols() {
  int ix;
  //        const char *np;
  //        ALFA name;
  fprintf(STDOUT, "Symbol Table\n");
  fprintf(STDOUT, "Indx Name           Link     Object  Type Ref Nm Lev   Addr "
                  "  Size Fref FLev PP\n");
  for (ix = 0; ix <= Tx; ++ix) {
    // strcpy(name, TAB[ix].NAME);
    fprintf(STDOUT, "%4d %.14s %4d %10s %5s %3d %2d %3d %6d %6ld %4d %4d %2d\n",
            ix, TAB[ix].NAME, TAB[ix].LINK, objects[TAB[ix].OBJ],
            types[TAB[ix].TYP], TAB[ix].REF, TAB[ix].NORMAL, TAB[ix].LEV,
            TAB[ix].ADR, TAB[ix].SIZE, TAB[ix].FREF, TAB[ix].FORLEV,
            TAB[ix].PNTPARAM);
  }
}
void dumpArrays() {
  int ix;
  fprintf(STDOUT, "Array Table\n");
  fprintf(STDOUT, "Indx IxTyp ElTyp ERef Low High ElSz Siz\n");
  for (ix = 0; ix <= A; ++ix) {
    fprintf(STDOUT, "%4d %5s %5s %4d %3d %4d %4d %3d\n", ix,
            types[ATAB[ix].INXTYP], types[ATAB[ix].ELTYP], ATAB[ix].ELREF,
            ATAB[ix].LOW, ATAB[ix].HIGH, ATAB[ix].ELSIZE, ATAB[ix].SIZE);
  }
}
void dumpBlocks() {
  int ix, lst, ct;
  fprintf(STDOUT, "Block Table\n");
  fprintf(STDOUT, "  Display\n");
  ix = 1;
  while (ix <= LMAX && DISPLAY[ix] > 0) {
    fprintf(STDOUT, "  D%d %d\n", ix, DISPLAY[ix]);
    ix += 1;
  }
  fprintf(STDOUT, "Indx  Last LstPr PSize VSize PrCnt\n");
  for (ix = 0; ix <= B; ++ix) {
    fprintf(STDOUT, "%4d %5d %5d %5d %5d %5d\n", ix, BTAB[ix].LAST,
            BTAB[ix].LASTPAR, BTAB[ix].PSIZE, BTAB[ix].VSIZE, BTAB[ix].PARCNT);
  }
  for (ix = B; ix >= 1; --ix) {
    lst = BTAB[ix].LAST;
    fprintf(STDOUT, "block %d declares\n", ix);
    ct = 0;
    while (lst != 0) {
      fprintf(STDOUT, " %s", TAB[lst].NAME);
      if (++ct % 8 == 0) {
        fprintf(STDOUT, "\n");
      }
      lst = TAB[lst].LINK;
    }
    if (ct % 8 != 0)
      fprintf(STDOUT, "\n");
  }
}
void dumpReals() {
  int ix;
  fprintf(STDOUT, "Real Constants\n");
  fprintf(STDOUT, "Indx     RConst  IConval    RConval\n");
  for (ix = 0; ix <= CPNT; ++ix) {
    fprintf(STDOUT, "%4d %12.6f %6d %12.6f\n", ix, CONTABLE[ix],
            INITABLE[ix].IVAL, INITABLE[ix].RVAL);
  }
}
void dumpPDES(PROCPNT pd) {
  int ix, ppid;
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
  fprintf(STDOUT, "**current stack top: %d\n", pd->T);
  fprintf(STDOUT, "  block table index: %d\n", pd->B);
  fprintf(STDOUT, "         current PC: %d\n", pd->PC);
  fprintf(STDOUT, "          stacksize: %d\n", pd->STACKSIZE);
  fprintf(STDOUT, "            display: %d", pd->DISPLAY[0]);
  for (ix = 1; ix <= LMAX; ++ix) {
    fprintf(STDOUT, ", %d", pd->DISPLAY[ix]);
  }
  fprintf(STDOUT, "\n");
  ppid = (pd->PARENT == nullptr) ? -1 : pd->PARENT->PID;
  fprintf(STDOUT, "             parent: %d\n", ppid);
  fprintf(STDOUT, " number of children: %d\n", pd->NUMCHILDREN);
  fprintf(STDOUT, "         stack base: %d\n", pd->BASE);
  fprintf(STDOUT, "     max child time: %f\n", pd->MAXCHILDTIME);
  fprintf(STDOUT, "               time: %f\n", pd->TIME);
  fprintf(STDOUT, "          for level: %d\n", pd->FORLEVEL);
  fprintf(STDOUT, "          for index: %d\n", pd->FORINDEX);
  fprintf(STDOUT, "          processor: %d\n", pd->PROCESSOR);
  fprintf(STDOUT, "      alt processor: %d\n", pd->ALTPROC);
  fprintf(STDOUT, "          wake time: %f\n", pd->WAKETIME);
  fprintf(STDOUT, "              state: %s\n", states[pd->STATE]);
  fprintf(STDOUT, "         process ID: %d\n", pd->PID);
  fprintf(STDOUT, "       virtual time: %f\n", pd->VIRTUALTIME);
  fprintf(STDOUT, "         read staus: %s\n", readstatus[pd->READSTATUS]);
  fprintf(STDOUT, "         fork count: %d\n", pd->FORKCOUNT);
  fprintf(STDOUT, "     join semaphore: %d\n", pd->JOINSEM);
  fprintf(STDOUT, "      max fork time: %f\n", pd->MAXFORKTIME);
  fprintf(STDOUT, "           priority: %s\n", priority[pd->PRIORITY]);
  fprintf(STDOUT, "        sequence on: %s\n", (pd->SEQON) ? "true" : "false");
  fprintf(STDOUT, "          group rep: %s\n",
          (pd->GROUPREP) ? "true" : "false");
}
void snapPDES(InterpLocal *il, PROCESSDESCRIPTOR *pd) {
  struct InterpLocal::PROCTAB *ptab;
  ptab = &il->PROCTAB[pd->PROCESSOR];
  fprintf(STDOUT, "process ID: %d\n", pd->PID);
  fprintf(STDOUT, "  state: %s\n", states[pd->STATE]);
  fprintf(STDOUT, "  time: %7.1f\n", pd->TIME);
  fprintf(STDOUT, "  wake time: %7.1f\n", pd->WAKETIME);
  fprintf(STDOUT, "  processor: %d%s\n", pd->PROCESSOR,
          (ptab->RUNPROC == pd) ? " runproc" : "");
  fprintf(STDOUT, "  stack S[T] S[S[T]] %d %d\n", il->S[pd->T],
          il->S[il->S[pd->T]]);
  fprintf(STDOUT, "  rdstatus: %s\n", nameRdstatus(pd->READSTATUS));
  fprintf(STDOUT, "  priority: %s\n",
          (pd->PRIORITY == PROCESSDESCRIPTOR::PRIORITY::LOW) ? "LOW" : "HIGH");
  if (pd->STATE == PROCESSDESCRIPTOR::STATE::RUNNING && pd->TIME < il->CLOCK) {
    fprintf(STDOUT, "    can dispatch\n");
  } else if (pd->STATE == PROCESSDESCRIPTOR::STATE::READY) {
    fprintf(STDOUT, "    ptab RUNPROC %snull\n",
            (ptab->RUNPROC != nullptr) ? "non-" : "");
    if (ptab->RUNPROC != nullptr) {
      fprintf(STDOUT, "        RUNPROC PID %d\n", ptab->RUNPROC->PID);
      fprintf(STDOUT, "        RUNPROC STATE %s\n",
              states[ptab->RUNPROC->STATE]);
    }
    fprintf(STDOUT, "    ptab VIRTIME %7.1f\n", ptab->VIRTIME);
    fprintf(STDOUT, "    ptab STARTTIME %7.1f\n", ptab->STARTTIME);
  }
}
void dumpDeadlock(InterpLocal *il) {
  ACTIVEPROCESS *act, *acphead;
  PROCESSDESCRIPTOR *pd;
  struct InterpLocal::PROCTAB *ptab;
  acphead = il->ACPHEAD;
  act = acphead;
  do {
    pd = act->PDES;
    snapPDES(il, pd);
    act = act->NEXT;
  } while (act != nullptr && act != acphead);
}
void dumpChans(InterpLocal *il) {
  //         struct Channel {
  //            int HEAD;
  //            int SEM;
  //            ACTPNT WAIT;
  //            float READTIME;
  //            bool MOVED;
  //            int READER;
  //        } CHAN[OPCHMAX+1];
  int ix, pid;
  PROCESSDESCRIPTOR *pdes;
  fprintf(STDOUT, "Channels\n");
  fprintf(STDOUT, "Indx  Head   Sem  WPID   RTime Moved Reader\n");
  for (ix = 0; ix <= OPCHMAX; ++ix) {
    if (il->CHAN[ix].HEAD != -1) {
      pid = (il->CHAN[ix].WAIT == nullptr) ? -1 : il->CHAN[ix].WAIT->PDES->PID;
      fprintf(STDOUT, "%4d %5d %5d %5d %7.1f %5d %6d\n", ix, il->CHAN[ix].HEAD,
              il->CHAN[ix].SEM, pid, il->CHAN[ix].READTIME, il->CHAN[ix].MOVED,
              il->CHAN[ix].READER);
    }
  }
}
void dumpStkfrms(InterpLocal *il) {
  BLKPNT free;
  int top, first, ix, val;
  fprintf(STDOUT, "STACK USE\n");
  first = 0;
  free = il->STHEAD;
  while (free != nullptr) {
    if (first < free->START)
      fprintf(STDOUT, "used %d - %d len %d processor %d\n", first,
              free->START - 1, free->START - first, il->SLOCATION[first]);
    fprintf(STDOUT, "free %d - %d len %d SLOC %d\n", free->START,
            free->START + free->SIZE - 1, free->SIZE,
            il->SLOCATION[free->START]);
    top = free->START;
    first = free->START + free->SIZE;
    free = free->NEXT;
  }
  fprintf(STDOUT, "SLOCATION\n");
  val = il->SLOCATION[0];
  first = ix = 0;
  while (ix < STMAX) {
    if (il->SLOCATION[ix] != val) {
      fprintf(STDOUT, "first %d last %d val %d\n", first, ix - 1, val);
      first = ix;
      val = il->SLOCATION[ix];
    }
    ix += 1;
  }
  fprintf(STDOUT, "first %d last %d val %d\n", first, ix - 1, val);
}
void dumpProctab(InterpLocal *il) {
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
  int ix, pid;
  fprintf(STDOUT, "Processor Table\n");
  fprintf(
      STDOUT,
      "Indx Status    Virtime Brktime Protime  Rpid  Num Strtime   Speed\n");
  ix = 0;
  while (il->PROCTAB[ix].STATUS != InterpLocal::PROCTAB::STATUS::NEVERUSED) {
    pid = (il->PROCTAB[ix].RUNPROC == nullptr) ? -1
                                               : il->PROCTAB[ix].RUNPROC->PID;
    fprintf(STDOUT, "%4d %.9s %7.1f %7.1f %7.1f %5d %4d %7.1f %7.1f\n", ix,
            status[il->PROCTAB[ix].STATUS], il->PROCTAB[ix].VIRTIME,
            il->PROCTAB[ix].BRKTIME, il->PROCTAB[ix].PROTIME, pid,
            il->PROCTAB[ix].NUMPROC, il->PROCTAB[ix].STARTTIME,
            il->PROCTAB[ix].SPEED);
    ix += 1;
  }
}
void dumpActive(InterpLocal *il) {
  ACTIVEPROCESS *act, *acphead;
  acphead = il->ACPHEAD;
  act = acphead;
  do {
    dumpPDES(act->PDES);
    act = act->NEXT;
  } while (act != nullptr && act != acphead);
}
const char *lookupSym(int lev, int adr) {
  int ix, blev;
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
  ix = BTAB[lev].LAST;
  while (ix != 0) {
    if (TAB[ix].ADR == adr)
      return TAB[ix].NAME;
    ix = TAB[ix].LINK;
  }
  return nosym;
}
const char *arrayName(int ref) {
  int ix;
  for (ix = Tx; ix >= 0; --ix) {
    if (TAB[ix].TYP == ARRAYS && TAB[ix].REF == ref)
      return TAB[ix].NAME;
  }
  return nosym;
}
} // namespace Cstar
