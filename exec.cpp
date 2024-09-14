//
// Created by Dan Evans on 1/29/24.
//
#include "cs_compile.h"
#include "cs_exec.h"
#include "cs_global.h"
#include "cs_interpret.h"
#include <algorithm>
#include <cerrno>
#include <cmath>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <numbers>
#include <print>

namespace cs {
using std::cout;
using std::println;
using std::numbers::ln10;

#define MAXINT 32767
// enum PS {RUN, BREAK, FIN, DEAD, STKCHK};
struct ACTIVEPROCESS;
using PRD = struct PROCESSDESCRIPTOR;
using PROCPNT = struct PROCESSDESCRIPTOR *;
using ACTPNT = struct ACTIVEPROCESS *;
//    typedef struct ACTIVEPROCESS {
//        PROCPNT PDES;
//        ACTPNT NEXT;
//    } ACTIVEPROCESS;

struct CommdelayLocal {
  int T1, T2, T3, T4, I, DIST, NUMPACK, PATHLEN;
  int PATH[PMAX + 1];
  double PASTINTERVAL, NOWINTERVAL, FINALINTERVAL;
  InterpLocal *il;
};

extern ALFA PROGNAME;
extern const char *opcodes[115];
extern void snapPDES(InterpLocal *, PROCPNT);
extern void dumpPDES(PROCPNT);
extern void dumpInst(int);
extern void dumpLInst(int, int *);
extern void dumpDeadlock(InterpLocal *il);
extern const char *nameState(enum PROCESSDESCRIPTOR::STATE);
extern const char *nameRdstatus(enum PROCESSDESCRIPTOR::READSTATUS);
extern const char *prcsrStatus(enum InterpLocal::PROCTAB::STATUS st);
extern const char *lookupSym(int, int);
extern void showInstTrace(bool);
extern int GETLNUM(int);
extern int BTOI(bool);
extern int ITOB(int);
extern int FIND(InterpLocal *il);
extern void RELEASE(InterpLocal *il, int BASE, int LENGTH);
extern void unreleased(InterpLocal *);
extern int FINDFRAME(InterpLocal *il, int LENGTH);
extern void EXECLIB(InterpLocal *il, ExLocal *, PROCPNT CURPR, int ID);
enum DEBUG {
  DBGRECV = 1,
  DBGSEND = 2,
  DBGRELEASE = 4,
  DBGPROC = 8,
  DBGTIME = 16,
  DBGSQTME = 32,
  DBGINST = 64
};
static int debug = 0;
const int SWITCHTIME = 5;
const int CREATETIME = 5;
void showInstTrace(bool flg) {
  if (flg)
    debug |= DBGINST;
  else
    debug &= ~DBGINST;
}
void CHKVAR(InterpLocal *interp_local, int STKLOC) {
  // check if the variable at the passed stack location is being traced
  int i = 0;
  int j = 0;
  for (i = 1; i <= VARMAX; i++) {
    if (STKLOC == interp_local->TRCTAB[i].MEMLOC) {
      std::cout << "Reference to Trace Variable "
                << (interp_local->TRCTAB[i].NAME) << '\n';
      // CURPR* curpr = new CURPR;
      interp_local->BLINE = GETLNUM(interp_local->CURPR->PC - 1);
      std::print("Line Number{:5}  In Function ", interp_local->BLINE);
      j = interp_local->CURPR->B;
      interp_local->INX = interp_local->S[interp_local->CURPR->B + 4];
      while (TAB[interp_local->INX].name[0] == '*') {
        j = interp_local->S[j + 3];
        interp_local->INX = interp_local->S[j + 4];
      }
      if (j == 0) {
        std::cout << PROGNAME << '\n';
      } else {
        std::cout << TAB[interp_local->INX].name << '\n';
      }
      //                std::cout << "Process Number" << il->CURPR->PID <<
      //                std::endl; std::cout << std::endl;
      std::println("Process Number{:4}\n", interp_local->CURPR->PID);
      interp_local->STEPROC = interp_local->CURPR;
      interp_local->PS = InterpLocal::PS::BREAK;
    }
  }
}

void MOVE(CommdelayLocal *commdelay_local, int lSOURCE, int DES, int STEP) {
  int DIREC = 0;
  int I = 0;
  if (lSOURCE < DES) {
    DIREC = 1;
  } else {
    DIREC = -1;
  }
  for (I = 1; I <= abs(lSOURCE - DES); I++) {
    commdelay_local->PATHLEN = commdelay_local->PATHLEN + 1;
    commdelay_local->PATH[commdelay_local->PATHLEN] =
        commdelay_local->PATH[commdelay_local->PATHLEN - 1] + DIREC * STEP;
  }
}

void RMOVE(CommdelayLocal *cl, int lSOURCE, int DES, int STEP) {
  int DIREC, I, DIST;
  DIST = abs(lSOURCE - DES);
  if (lSOURCE < DES) {
    DIREC = 1;
  } else {
    DIREC = -1;
  }
  if (TOPDIM < 2 * DIST) {
    DIREC = -DIREC;
    DIST = TOPDIM - DIST;
  }
  for (I = 1; I <= DIST; I++) {
    cl->PATHLEN = cl->PATHLEN + 1;
    lSOURCE = lSOURCE + DIREC;
    if (lSOURCE == TOPDIM) {
      lSOURCE = 0;
      cl->PATH[cl->PATHLEN] = cl->PATH[cl->PATHLEN - 1] - STEP * (TOPDIM - 1);
    } else if (lSOURCE == -1) {
      lSOURCE = TOPDIM - 1;
      cl->PATH[cl->PATHLEN] = cl->PATH[cl->PATHLEN - 1] + STEP * (TOPDIM - 1);
    } else {
      cl->PATH[cl->PATHLEN] = cl->PATH[cl->PATHLEN - 1] + DIREC * STEP;
    }
  }
}

void SCHEDULE(CommdelayLocal *cl, double &ARRIVAL) {
  BUSYPNT CURPNT;
  BUSYPNT PREV;
  BUSYPNT ITEM;
  int I, PROCNUM;
  bool DONE;
  for (I = 1; I <= cl->PATHLEN; I++) {
    ARRIVAL = ARRIVAL + 2;
    PROCNUM = cl->PATH[I];
    CURPNT = cl->il->PROCTAB[PROCNUM].BUSYLIST;
    PREV = nullptr;
    DONE = false;
    do {
      if (CURPNT == nullptr) {
        DONE = true;
        ITEM = (BUSYPNT)std::calloc(1, sizeof(struct BUSYTYPE));
        // NEW(ITEM);
        ITEM->FIRST = ARRIVAL;
        ITEM->LAST = ARRIVAL;
        ITEM->NEXT = nullptr;
        if (PREV == nullptr) {
          cl->il->PROCTAB[PROCNUM].BUSYLIST = ITEM;
        } else {
          PREV->NEXT = ITEM;
        }
      } else {
        if (ARRIVAL < CURPNT->FIRST - 1) {
          DONE = true;
          // NEW(ITEM);
          ITEM = (BUSYPNT)std::calloc(1, sizeof(struct BUSYTYPE));
          ITEM->FIRST = ARRIVAL;
          ITEM->LAST = ARRIVAL;
          ITEM->NEXT = CURPNT;
          if (PREV != nullptr) {
            PREV->NEXT = ITEM;
          } else {
            cl->il->PROCTAB[PROCNUM].BUSYLIST = ITEM;
          }
        } else if (ARRIVAL == CURPNT->FIRST - 1) {
          DONE = true;
          CURPNT->FIRST = CURPNT->FIRST - 1;
        } else if (ARRIVAL <= CURPNT->LAST + 1) {
          DONE = true;
          CURPNT->LAST = CURPNT->LAST + 1;
          ARRIVAL = CURPNT->LAST;
          PREV = CURPNT;
          CURPNT = CURPNT->NEXT;
          if (CURPNT != nullptr) {
            if (CURPNT->FIRST == ARRIVAL + 1) {
              PREV->LAST = CURPNT->LAST;
              PREV->NEXT = CURPNT->NEXT;
              // fprintf(STANDARD_OUTPUT, "free commdelay1\n");
              std::free(CURPNT);
            }
          }
        }
        if (!DONE) {
          PREV = CURPNT;
          CURPNT = CURPNT->NEXT;
          if (PREV->LAST < cl->PASTINTERVAL) {
            // fprintf(STANDARD_OUTPUT, "free commdelay2\n");
            std::free(PREV);
            cl->il->PROCTAB[PROCNUM].BUSYLIST = CURPNT;
            PREV = nullptr;
          }
        }
      }
    } while (!DONE);
  }
}

int COMMDELAY(InterpLocal *il, int SOURCE, int DEST, int LEN) {

  struct CommdelayLocal cl;
  int rtn;
  memset(&cl, 0, sizeof(cl));
  cl.il = il;
  cl.NUMPACK = LEN / 3;
  if (LEN % 3 != 0) {
    cl.NUMPACK = cl.NUMPACK + 1;
  }
  if ((!il->CONGESTION) || (topology == Symbol::SHAREDSY)) {
    switch (topology) {
    case Symbol::SHAREDSY:
      rtn = 0;
      break;
    case Symbol::FULLCONNSY:
    case Symbol::CLUSTERSY:
      cl.DIST = 1;
      break;
    case Symbol::HYPERCUBESY:
      cl.T1 = SOURCE;
      cl.T2 = DEST;
      cl.DIST = 0;
      while (cl.T1 + cl.T2 > 0) {
        if (cl.T1 % 2 != cl.T2 % 2) {
          cl.DIST = cl.DIST + 1;
        }
        cl.T1 = cl.T1 / 2;
        cl.T2 = cl.T2 / 2;
      }
      break;
    case Symbol::LINESY:
      cl.DIST = abs(SOURCE - DEST);
      break;
    case Symbol::MESH2SY:
      cl.T1 = TOPDIM;
      cl.T2 = std::abs(SOURCE / cl.T1 - DEST / cl.T1);
      cl.T3 = std::abs(SOURCE % cl.T1 - DEST % cl.T1);
      cl.DIST = (cl.T2 + cl.T3);
      break;
    case Symbol::MESH3SY:
      cl.T2 = TOPDIM;
      cl.T1 = TOPDIM * cl.T2;
      cl.T3 = SOURCE % cl.T1;
      cl.T4 = DEST % cl.T1;
      cl.DIST = abs(cl.T3 / cl.T2 - cl.T4 / cl.T2) +
                abs(cl.T3 % cl.T2 - cl.T4 % cl.T2) +
                abs(SOURCE / cl.T1 - DEST / cl.T1);
      break;
    case Symbol::RINGSY:
      cl.T1 = abs(SOURCE - DEST);
      cl.T2 = TOPDIM - cl.T1;
      if (cl.T1 < cl.T2) {
        cl.DIST = cl.T1;
      } else {
        cl.DIST = cl.T2;
      }
      break;
    case Symbol::TORUSSY:
      cl.T1 = TOPDIM;
      cl.T3 = abs(SOURCE / cl.T1 - DEST / cl.T1);
      if (cl.T3 > cl.T1 / 2) {
        cl.T3 = cl.T1 - cl.T3;
      }
      cl.T4 = abs(SOURCE % cl.T1 - DEST % cl.T1);
      if (cl.T4 > cl.T1 / 2) {
        cl.T4 = cl.T1 - cl.T4;
      }
      cl.DIST = (cl.T3 + cl.T4);
      break;
    default:
      break;
    }
    if (topology != Symbol::SHAREDSY) {
      rtn = (int)std::round((cl.DIST + (cl.NUMPACK - 1) / 2.0) * il->TOPDELAY);
    }
  } else {
    cl.PATH[0] = SOURCE;
    cl.PATHLEN = 0;
    switch (topology) {
    case Symbol::FULLCONNSY:
    case Symbol::CLUSTERSY:
      cl.PATH[1] = DEST;
      cl.PATHLEN = 1;
      break;
    case Symbol::HYPERCUBESY:
      cl.T1 = 1;
      cl.T2 = SOURCE;
      for (cl.I = 1; cl.I <= TOPDIM; cl.I++) {
        cl.T3 = DEST / cl.T1 % 2;
        cl.T4 = SOURCE / cl.T1 % 2;
        if (cl.T3 != cl.T4) {
          if (cl.T3 == 1) {
            cl.T2 = cl.T2 + cl.T1;
          } else {
            cl.T2 = cl.T2 - cl.T1;
          }
          cl.PATHLEN = cl.PATHLEN + 1;
          cl.PATH[cl.PATHLEN] = cl.T2;
        }
        cl.T1 = cl.T1 * 2;
      }
      break;
    case Symbol::LINESY:
      MOVE(&cl, SOURCE, DEST, 1);
      break;
    case Symbol::MESH2SY:
      MOVE(&cl, SOURCE % TOPDIM, DEST % TOPDIM, 1);
      MOVE(&cl, SOURCE / TOPDIM, DEST / TOPDIM, TOPDIM);
      break;
    case Symbol::MESH3SY:
      cl.T1 = TOPDIM * TOPDIM;
      MOVE(&cl, SOURCE % TOPDIM, DEST % TOPDIM, 1);
      MOVE(&cl, SOURCE / TOPDIM % TOPDIM, DEST / TOPDIM % TOPDIM, TOPDIM);
      MOVE(&cl, SOURCE / cl.T1, DEST / cl.T1, TOPDIM * TOPDIM);
      break;
    case Symbol::RINGSY:
      RMOVE(&cl, SOURCE, DEST, 1);
      break;
    case Symbol::TORUSSY:
      RMOVE(&cl, SOURCE % TOPDIM, DEST % TOPDIM, 1);
      RMOVE(&cl, SOURCE / TOPDIM, DEST / TOPDIM, TOPDIM);
      break;
    default:
      break;
    }
    if (il->TOPDELAY != 0) {
      cl.PASTINTERVAL =
          (float)((il->CLOCK - 2 * TIMESTEP) / (il->TOPDELAY / 2.0));
      cl.NOWINTERVAL = (float)(il->CURPR->TIME / (il->TOPDELAY / 2.0) + 0.5);
      for (cl.I = 1; cl.I <= cl.NUMPACK; cl.I++) {
        cl.FINALINTERVAL = cl.NOWINTERVAL;
        SCHEDULE(&cl, cl.FINALINTERVAL);
      }
      rtn = (int)std::round((cl.FINALINTERVAL - cl.NOWINTERVAL) *
                            (il->TOPDELAY / 2.0));
    } else {
      rtn = 0;
    }
  }
  if ((SOURCE > highest_processor) || (DEST > highest_processor)) {
    il->PS = InterpLocal::PS::CPUCHK;
  }
  //        if (rtn != 0)
  //            fprintf(STANDARD_OUTPUT, "COMMDELAY source %d dest %d len %d returns
  //            %d\n", SOURCE, DEST, LEN, rtn);
  return rtn;
}

void TESTVAR(InterpLocal *il, int STKPNT) {
  // verify that the processor number corresponding to the passed stack location
  // is the current PDES's processor, altproc, or -1, otherwise REMCHK
  int PNUM;
  PNUM = il->SLOCATION[STKPNT];
  if ((PNUM != il->CURPR->PROCESSOR) && (PNUM != il->CURPR->ALTPROC) &&
      (PNUM != -1)) {
    il->PS = InterpLocal::PS::REMCHK;
  }
}
/*
 * if either stack top or top+1 is RTAG'ed and the other is not, move the
 * untagged value to the corresponding rstack and RTAG the moved stack element.
 */
bool ISREAL(InterpLocal *il) {
  STYPE *sp;
  RSTYPE *rp;
  int Tl;
  sp = il->S;
  rp = il->RS;
  Tl = il->CURPR->T;
  if ((sp[Tl] == RTAG) && (sp[Tl + 1] != RTAG)) {
    rp[Tl + 1] = sp[Tl + 1];
    sp[Tl + 1] = RTAG;
  }
  if ((sp[Tl] != RTAG) && (sp[Tl + 1] == RTAG)) {
    rp[Tl] = sp[Tl];
    sp[Tl] = RTAG;
  }
  return sp[Tl] == RTAG;
}
void procTime(PROCPNT prc, float incr, const char *id) {
  if (prc->PID == 0) {
    fprintf(STANDARD_OUTPUT, "%.1f = %.1f + %.1f %s\n", prc->TIME + incr, prc->TIME,
            incr, id);
  }
}
void TIMEINC(InterpLocal *il, int UNITS, const char *trc) {
  float STEP;
  struct InterpLocal::PROCTAB *ptab;
  PRD *proc;
  proc = il->CURPR;
  ptab = &il->PROCTAB[proc->PROCESSOR];
  STEP = ptab->SPEED * (float)UNITS;
  if (debug & DBGTIME)
    procTime(proc, STEP, trc);
  proc->TIME += STEP;
  ptab->VIRTIME += STEP;
  ptab->BRKTIME += STEP;
  ptab->PROTIME += STEP;
  proc->VIRTUALTIME += STEP;
  if (proc->SEQON) {
    if (debug & DBGSQTME)
      fprintf(STANDARD_OUTPUT, "%.1f = %.1f + %.1f SEQ\n", il->SEQTIME + STEP,
              il->SEQTIME, STEP);
    il->SEQTIME += STEP;
  }
}

void SLICE(InterpLocal *il) {
  bool DONE, DEADLOCK;
  ACTPNT PREV;
  int COUNT;
  struct InterpLocal::PROCTAB *ptab;
  PRD *proc;
  PREV = il->ACPCUR;
  il->ACPCUR = il->ACPCUR->NEXT;
  DONE = false;
  COUNT = 0;
  DEADLOCK = true;
  do {
    COUNT = COUNT + 1;
    if (il->ACPCUR == il->ACPHEAD) {
      il->CLOCK += TIMESTEP;
    }
    proc = il->ACPCUR->PDES;
    if (proc->STATE == PRD::STATE::DELAYED) {
      if (proc->WAKETIME < il->CLOCK) {
        proc->STATE = PRD::STATE::READY;
        if (proc->WAKETIME > proc->TIME) {
          proc->TIME = proc->WAKETIME;
          if (debug & DBGTIME)
            procTime(proc, 0.0, "SLICE1-WAKE");
        }
      } else {
        proc->TIME = il->CLOCK;
        if (debug & DBGTIME)
          procTime(proc, 0.0, "SLICE2-CLOCK");
        DEADLOCK = false;
      }
    }
    if (proc->STATE == PRD::STATE::RUNNING) {
      DEADLOCK = false;
      if (proc->TIME < il->CLOCK) {
        DONE = true;
        il->CURPR = proc;
      }
    } else if (proc->STATE == PRD::STATE::READY) {
      DEADLOCK = false; // maybe bug, maybe C++ problem yet to be corrected
      ptab = &il->PROCTAB[proc->PROCESSOR];
      //                if (ptab->RUNPROC != nullptr && ptab->RUNPROC != proc)
      //                    fprintf(STANDARD_OUTPUT, "ptab(%d) != proc(%d) virtime %.1f
      //                    starttime %.1f\n",
      //                            ptab->RUNPROC->PID, proc->PID,
      //                            ptab->VIRTIME, ptab->STARTTIME);
      if (ptab->RUNPROC == nullptr) {
        il->CURPR = proc;
        DONE = true;
        ptab->STARTTIME = ptab->VIRTIME;
        proc->STATE = PRD::STATE::RUNNING;
        ptab->RUNPROC = il->CURPR;
      } else if ((ptab->VIRTIME >= ptab->STARTTIME + SWITCHLIMIT) &&
                 (ptab->RUNPROC->PRIORITY <= proc->PRIORITY)) {
        il->CURPR = proc;
        DONE = true;
        ptab->STARTTIME = ptab->VIRTIME;
        ptab->RUNPROC->STATE = PRD::STATE::READY;
        TIMEINC(il, SWITCHTIME, "slc1");
        proc->STATE = PRD::STATE::RUNNING;
        ptab->RUNPROC = il->CURPR;
        //                    fprintf(STANDARD_OUTPUT, "switch ptab runproc\n");
      } else {
        if (COUNT > PMAX && !DEADLOCK) {
          fprintf(STANDARD_OUTPUT, "loop in SLICE\n");
          DEADLOCK = true;
        }
        proc->TIME = il->CLOCK;
        if (debug & DBGTIME)
          procTime(proc, 0.0, "SLICE3-CLOCK");
      }
    } else if (proc->STATE == PRD::STATE::BLOCKED) {
      proc->TIME = il->CLOCK;
      if (debug & DBGTIME)
        procTime(proc, 0.0, "SLICE4-CLOCK");
    }
    if (proc->STATE == PRD::STATE::SPINNING) {
      if (il->S[il->S[proc->T]] == 0) {
        DEADLOCK = false;
        if (proc->TIME < il->CLOCK) {
          DONE = true;
          il->CURPR = proc;
          proc->STATE = PRD::STATE::RUNNING;
        }
      } else {
        proc->TIME = il->CLOCK;
        if (debug & DBGTIME)
          procTime(proc, 0.0, "SLICE5-CLOCK");
      }
    }
    if (proc->STATE == PRD::STATE::TERMINATED) {
      PREV->NEXT = il->ACPCUR->NEXT;
      if (il->ACPHEAD == il->ACPCUR) {
        il->ACPHEAD = il->ACPCUR->NEXT;
      }
      if (il->ACPTAIL == il->ACPCUR) {
        il->ACPTAIL = PREV;
      }
      if (debug & DBGPROC) {
        fprintf(STANDARD_OUTPUT, "slice terminated/freed process %d\n", proc->PID);
        snapPDES(il, proc);
      }
      std::free(proc);
      std::free(il->ACPCUR);
      il->ACPCUR = PREV->NEXT;
    } else if (!DONE) {
      PREV = il->ACPCUR;
      il->ACPCUR = il->ACPCUR->NEXT;
    }
  } while (!DONE && !((COUNT > PMAX) && DEADLOCK));
  // if (il->CURPR->PID == 0)
  //     fprintf(STANDARD_OUTPUT, "dispatch %d time %.1f seqtime %.1f\n", il->CURPR->PID,
  //     il->CURPR->TIME, il->SEQTIME);
  if (DEADLOCK) {
    cout << "    DEADLOCK:  All Processes are Blocked" << '\n';
    cout << "For Further Information Type STATUS Command" << '\n';
    println(STANDARD_OUTPUT, "CLOCK {:.1f} PS {}, DONE {:d}, COUNT {}, DEADLOCK {:d}",
            il->CLOCK, static_cast<std::int8_t>(il->PS), DONE, COUNT, DEADLOCK);
    il->PS = InterpLocal::PS::DEAD;
    dumpDeadlock(il);
  }
}

void EXECUTE(InterpLocal *interp_local) {
  //        ProcessState PS;
  //        PROCPNT CURPR;
  //        int LC;
  //        double USAGE;
  //        char CH;
  //        int T;
  //        std::vector<int> STACK(STACKSIZE);
  PROCPNT current_process = interp_local->CURPR;
  PROCPNT process = nullptr;
  struct InterpLocal::PROCTAB *process_metadata = nullptr;
  struct InterpLocal::Channel *channel = nullptr;
  // --- debugging ---

  //        int lct = 0;
  //        int mon = -1, val = 82;
  //        int braddr = 0;
  //        bool watch = false, breakat = false;
  // ---------
  ExLocal executor{};

  // memset(&el, 0, sizeof(ExLocal));
  // el.il = il;
  executor.log10 = ln10;
  // dumpPDES(CURPR);
  do {
    if (interp_local->PS != InterpLocal::PS::BREAK) {
      if ((!interp_local->NOSWITCH &&
           current_process->TIME >= interp_local->CLOCK) ||
          current_process->STATE != PRD::STATE::RUNNING) {
        //                    fprintf(STANDARD_OUTPUT, "state %s noswitch %d time %.1f
        //                    clock %.1f",
        //                            nameState(CURPR->STATE), il->NOSWITCH,
        //                            CURPR->TIME, il->CLOCK);
        SLICE(interp_local);
        current_process = interp_local->CURPR;
        //                    fprintf(STANDARD_OUTPUT, " new process %d processor %d\n",
        //                    CURPR->PID, CURPR->PROCESSOR);
      }
    }

    if (interp_local->PS == InterpLocal::PS::FIN ||
        interp_local->PS == InterpLocal::PS::DEAD) {
      // goto label_999;
      continue;
    }

    executor.IR = code.at(current_process->PC);
    current_process->PC++;
    TIMEINC(interp_local, 1, "exe1");

    if (interp_local->NUMBRK > 0 && interp_local->MAXSTEPS < 0 &&
        !interp_local->RESTART) {
      for (int i = 1; i <= BRKMAX; i++) {
        if (interp_local->BRKTAB[i] == current_process->PC - 1) {
          interp_local->PS = InterpLocal::PS::BREAK;
        }
      }
    }

    interp_local->RESTART = false;

    if (interp_local->MAXSTEPS > 0 &&
        current_process == interp_local->STEPROC) {
      line_count = current_process->PC - 1;
      if (line_count < interp_local->STARTLOC ||
          line_count > interp_local->ENDLOC) {
        interp_local->MAXSTEPS--;
        interp_local->STARTLOC = line_count;
        interp_local->ENDLOC = LOCATION[GETLNUM(line_count) + 1] - 1;
        if (interp_local->MAXSTEPS == 0) {
          interp_local->PS = InterpLocal::PS::BREAK;
        }
      }
    }

    if (interp_local->PROFILEON) {
      if (interp_local->CLOCK >= interp_local->PROTIME) {
        if (interp_local->PROLINECNT == 0) {
          std::cout << "TIME: "
                    << std::round(interp_local->PROTIME - interp_local->PROSTEP)
                    << '\n';
          if (interp_local->FIRSTPROC < 10) {
            std::cout << interp_local->FIRSTPROC;
          } else {
            std::cout << " ";
          }
          for (int i = 1; i <= (4 - (interp_local->FIRSTPROC % 5)); i++) {
            if (interp_local->FIRSTPROC + i < 10 &&
                interp_local->FIRSTPROC + i <= interp_local->LASTPROC) {
              std::cout << std::setw(2) << interp_local->FIRSTPROC + i;
            } else {
              std::cout << "  ";
            }
          }
          for (int i = interp_local->FIRSTPROC + 1; i <= interp_local->LASTPROC;
               i++) {
            if (i % 5 == 0) {
              if (i < 10) {
                std::cout << std::setw(2) << i << "         ";
              } else {
                std::cout << i << "       ";
              }
            }
          }
          std::cout << std::endl;
        }
        for (int i = interp_local->FIRSTPROC; i <= interp_local->LASTPROC;
             i++) {
          interp_local->USAGE =
              (float)(interp_local->PROCTAB[i].PROTIME / interp_local->PROSTEP);
          if (interp_local->PROCTAB[i].STATUS ==
              InterpLocal::PROCTAB::STATUS::NEVERUSED) {
            CH = ' ';
          } else if (interp_local->USAGE < 0.25) {
            CH = '.';
          } else if (interp_local->USAGE < 0.5) {
            CH = '-';
          } else if (interp_local->USAGE < 0.75) {
            CH = '+';
          } else {
            CH = '*';
          }
          std::cout << CH << " ";
          interp_local->PROCTAB[i].PROTIME = 0;
        }
        std::cout << std::endl;
        interp_local->PROLINECNT = (interp_local->PROLINECNT + 1) % 20;
        interp_local->PROTIME += interp_local->PROSTEP;
      }
    }

    if (interp_local->ALARMON && interp_local->ALARMENABLED) {
      if (interp_local->CLOCK >= interp_local->ALARMTIME) {
        std::cout << std::endl;
        std::cout << "Alarm Went Off at Time "
                  << std::round(interp_local->ALARMTIME) << std::endl;
        if (interp_local->CLOCK > interp_local->ALARMTIME + 2 * TIMESTEP) {
          std::cout << "Current Time is " << std::round(interp_local->CLOCK)
                    << std::endl;
        }
        interp_local->PS = InterpLocal::PS::BREAK;
        interp_local->ALARMENABLED = false;
      }
    }

    if (interp_local->PS == InterpLocal::PS::BREAK) {
      current_process->PC--;
      TIMEINC(interp_local, -1, "exe2");
      interp_local->BLINE = GETLNUM(current_process->PC);
      //                std::cout << std::endl;
      //                std::cout << "Break At " << il->BLINE << std::endl;
      fprintf(STANDARD_OUTPUT, "\nBreak At %d", interp_local->BLINE);
      executor.H1 = current_process->B;
      interp_local->INX = interp_local->S[current_process->B + 4];
      while (TAB[interp_local->INX].name[0] == '*') {
        executor.H1 = interp_local->S[executor.H1 + 3];
        interp_local->INX = interp_local->S[executor.H1 + 4];
      }
      if (executor.H1 == 0) {
        std::cout << "  In Function " << PROGNAME << std::endl;
      } else {
        std::cout << "  In Function " << TAB[interp_local->INX].name
                  << std::endl;
      }
      std::cout << "Process Number  " << current_process->PID << std::endl;
      interp_local->STEPROC = current_process;
      if (interp_local->MAXSTEPS == 0) {
        double R1 = current_process->TIME - interp_local->STEPTIME;
        if (R1 > 0) {
          executor.H2 = (int)std::round(
              (current_process->VIRTUALTIME - interp_local->VIRSTEPTIME) / R1 *
              100.0);
        } else {
          executor.H2 = 0;
        }
        if (executor.H2 > 100) {
          executor.H2 = 100;
        }
        std::cout << "Step Time is " << std::round(R1) << ".  Process running "
                  << executor.H2 << " percent." << std::endl;
      }
      std::cout << std::endl;
    } else {
      //                if (watch)
      //                {
      //                    if (il->S[mon] != val)
      //                    {
      //                        fprintf(STANDARD_OUTPUT, "----------modified %d at
      //                        %d----------\n", il->S[mon], CURPR->PC); watch =
      //                        false;
      //                    }
      //                }
      //                else
      //                {
      //                    if (mon >= 0 && il->S[mon] == val)
      //                    {
      //                        fprintf(STANDARD_OUTPUT, "----------monitor set at
      //                        %d----------\n", CURPR->PC); watch = true;
      //                    }
      //                }
      //                if (breakat)
      //                {
      //                    if (CURPR->PC == braddr)
      //                    {
      //                        fprintf(STANDARD_OUTPUT, "----------break at at
      //                        %d----------\n", CURPR->PC);
      //                    }
      //                }
      if (debug & DBGINST) {
        //                    if (CURPR->PROCESSOR == 0 && CURPR->PC < 81 &&
        //                    CURPR->PC > 18)
        //                    {
        fprintf(STANDARD_OUTPUT, "proc %3d stk %5d [%5d] ", current_process->PROCESSOR,
                current_process->T, interp_local->S[current_process->T]);
        dumpInst(current_process->PC - 1);
        //                    }
      }
      //                if (CURPR->PID == 1)
      //                    dumpInst(CURPR->PC - 1);
      //                if (++lct > 2000000)
      //                    throw std::exception();
      switch (executor.IR.F) {
      case 0: // push DISPLAY[op1]+op2 (stack frame location of variable op2)
        current_process->T++;
        if (current_process->T > current_process->STACKSIZE) {
          interp_local->PS = InterpLocal::PS::STKCHK;
        } else {
          interp_local->S[current_process->T] =
              current_process->DISPLAY[executor.IR.X] + executor.IR.Y;
          //                            fprintf(STANDARD_OUTPUT, "%3d: %d %d,%d %s %s\n",
          //                            CURPR->PC - 1,
          //                                el.IR.F, el.IR.X, el.IR.Y,
          //                                opcodes[el.IR.F], lookupSym(el.IR.X,
          //                                el.IR.Y));
        }
        break;
      case 1: {
        current_process->T++;
        if (current_process->T > current_process->STACKSIZE) {
          interp_local->PS = InterpLocal::PS::STKCHK;
        } else {
          executor.H1 = current_process->DISPLAY[executor.IR.X] + executor.IR.Y;
          if (topology != Symbol::SHAREDSY) {
            TESTVAR(interp_local, executor.H1);
          }
          if (interp_local->NUMTRACE > 0) {
            CHKVAR(interp_local, executor.H1);
          }
          interp_local->S[current_process->T] = interp_local->S[executor.H1];
          if (interp_local->S[current_process->T] == RTAG) {
            interp_local->RS[current_process->T] =
                interp_local->RS[executor.H1];
          }
          //                            fprintf(STANDARD_OUTPUT, "%3d: %d %d,%d %s %s\n",
          //                            CURPR->PC - 1,
          //                                    el.IR.F, el.IR.X, el.IR.Y,
          //                                    opcodes[el.IR.F],
          //                                    lookupSym(el.IR.X, el.IR.Y));
        }
        break;
      }
      case 2: {
        current_process->T++;
        if (current_process->T > current_process->STACKSIZE) {
          interp_local->PS = InterpLocal::PS::STKCHK;
        } else {
          executor.H1 =
              interp_local
                  ->S[current_process->DISPLAY[executor.IR.X] + executor.IR.Y];
          if (topology != Symbol::SHAREDSY) {
            TESTVAR(interp_local, executor.H1);
          }
          if (interp_local->NUMTRACE > 0) {
            CHKVAR(interp_local, executor.H1);
          }
          interp_local->S[current_process->T] = interp_local->S[executor.H1];
          if (interp_local->S[current_process->T] == RTAG) {
            interp_local->RS[current_process->T] =
                interp_local->RS[executor.H1];
          }
        }
        break;
      }
      case 3: {
        executor.H1 = executor.IR.Y;
        executor.H2 = executor.IR.X;
        executor.H3 = current_process->B;
        do {
          current_process->DISPLAY[executor.H1] = executor.H3;
          executor.H1--;
          executor.H3 = interp_local->S[executor.H3 + 2];
        } while (executor.H1 != executor.H2);
        break;
      }
      case 4: {
        current_process->NUMCHILDREN = 0;
        current_process->SEQON = false;
        break;
      }
      case 5: {
        if (current_process->NUMCHILDREN > 0) {
          current_process->STATE = PRD::STATE::BLOCKED;
          interp_local->PROCTAB[current_process->PROCESSOR].RUNPROC = nullptr;
        }
        current_process->SEQON = true;
        break;
      }
      case 6: { // noop
        break;
      }
      case 7: {
        current_process->PC = executor.IR.Y;
        current_process->SEQON = true;
        break;
      }
      case 8: {
        switch (executor.IR.Y) {
        case 10: {
          interp_local->R1 = interp_local->RS[current_process->T];
          if (interp_local->R1 >= 0 ||
              interp_local->R1 == (int)(interp_local->R1)) {
            interp_local->S[current_process->T] = (int)(interp_local->R1);
          } else {
            interp_local->S[current_process->T] =
                (int)(interp_local->R1) - 1; // ?? should go toward 0
          }
          if (abs(interp_local->S[current_process->T]) > NMAX) {
            interp_local->PS = InterpLocal::PS::INTCHK;
          }
          break;
        }
        case 19: {
          current_process->T++;
          if (current_process->T > current_process->STACKSIZE) {
            interp_local->PS = InterpLocal::PS::STKCHK;
          } else {
            interp_local->S[current_process->T] = current_process->PROCESSOR;
          }
          break;
        }
        case 20: {
          current_process->T++;
          if (current_process->T > current_process->STACKSIZE) {
            interp_local->PS = InterpLocal::PS::STKCHK;
          } else {
            interp_local->S[current_process->T] = RTAG;
            interp_local->RS[current_process->T] = current_process->TIME;
          }
          break;
        }
        case 21: {
          current_process->T++;
          if (current_process->T > current_process->STACKSIZE) {
            interp_local->PS = InterpLocal::PS::STKCHK;
          } else {
            interp_local->S[current_process->T] = RTAG;
            interp_local->RS[current_process->T] = interp_local->SEQTIME;
          }
          break;
        }
        case 22: {
          current_process->T++;
          if (current_process->T > current_process->STACKSIZE) {
            interp_local->PS = InterpLocal::PS::STKCHK;
          } else {
            interp_local->S[current_process->T] = current_process->PID;
          }
          break;
        }
        case 23: {
          current_process->T++;
          if (current_process->T > current_process->STACKSIZE) {
            interp_local->PS = InterpLocal::PS::STKCHK;
          } else {
            interp_local->S[current_process->T] = 10;
          }
          break;
        }
        }
        break;
      }
      case 9: {
        if (current_process->PID == 0) {
          for (executor.I = 1; executor.I <= highest_processor; executor.I++) {
            process = static_cast<PRD *>(calloc(1, sizeof(PROCESSDESCRIPTOR)));
            process->PC = 0;
            process->PID = interp_local->NEXTID++;
            if (interp_local->NEXTID > PIDMAX) {
              interp_local->PS = InterpLocal::PS::PROCCHK;
            }
            process->VIRTUALTIME = 0;
            process->DISPLAY.swap(current_process->DISPLAY);
            interp_local->PTEMP =
                static_cast<ACTPNT>(calloc(1, sizeof(ACTIVEPROCESS)));
            interp_local->PTEMP->PDES = process;
            interp_local->PTEMP->NEXT = interp_local->ACPTAIL->NEXT;
            interp_local->ACPTAIL->NEXT = interp_local->PTEMP;
            interp_local->ACPTAIL = interp_local->PTEMP;
            executor.H1 = FINDFRAME(interp_local, interp_local->STKMAIN);
            if ((debug & DBGPROC) != 0) {
              println(STANDARD_OUTPUT, "opc {} findframe {} length {}, response {}",
                      executor.IR.F, current_process->PID,
                      interp_local->STKMAIN, executor.H1);
            }
            if (executor.H1 > 0) {
              process->T = executor.H1 + BTAB[2].VSIZE - 1;
              process->STACKSIZE = executor.H1 + interp_local->STKMAIN - 1;
              process->B = executor.H1;
              process->BASE = executor.H1;
              for (executor.J = executor.H1;
                   executor.J <= executor.H1 + interp_local->STKMAIN - 1;
                   executor.J++) {
                interp_local->S[executor.J] = 0;
                interp_local->SLOCATION[executor.J] = executor.I;
                interp_local->RS[executor.I] = 0.0;
              }
              for (executor.J = executor.H1;
                   executor.J <= executor.H1 + BASESIZE - 1; executor.J++) {
                interp_local->STARTMEM[executor.I] =
                    -interp_local->STARTMEM[executor.I];
              }
              interp_local->S[executor.H1 + 1] = 0;
              interp_local->S[executor.H1 + 2] = 0;
              interp_local->S[executor.H1 + 3] = -1;
              interp_local->S[executor.H1 + 4] = BTAB[1].LAST;
              interp_local->S[executor.H1 + 5] = 1;
              interp_local->S[executor.H1 + 6] = interp_local->STKMAIN;
              process->DISPLAY[1] = executor.H1;
            }
            process->TIME = current_process->TIME;
            process->STATE = PRD::STATE::READY;
            process->FORLEVEL = current_process->FORLEVEL;
            process->READSTATUS = PRD::READSTATUS::NONE;
            process->FORKCOUNT = 1;
            process->MAXFORKTIME = 0;
            process->JOINSEM = 0;
            process->PARENT = current_process;
            process->PRIORITY = PRD::PRIORITY::LOW;
            process->ALTPROC = -1;
            process->SEQON = true;
            process->GROUPREP = false;
            process->PROCESSOR = executor.I;
            if (interp_local->PROCTAB[executor.I].STATUS ==
                InterpLocal::PROCTAB::STATUS::NEVERUSED) {
              interp_local->USEDPROCS++;
            }
            interp_local->PROCTAB[executor.I].STATUS =
                InterpLocal::PROCTAB::STATUS::FULL;
            interp_local->PROCTAB[executor.I].NUMPROC++;
            executor.J = 1;
            while (process->DISPLAY[executor.J] != -1) {
              interp_local->S[process->DISPLAY[executor.J] + 5]++;
              if (debug & DBGRELEASE) {
                println(STANDARD_OUTPUT, "{} ref ct {} ct={}", executor.IR.F,
                        current_process->PID,
                        interp_local->S[process->DISPLAY[executor.J] + 5]);
              }
              executor.J++;
            }
            if (debug & DBGPROC) {
              println(STANDARD_OUTPUT, "opc {} newproc pid {}", executor.IR.F,
                      process->PID);
            }
            current_process->FORKCOUNT += 1;
          }
          current_process->PC = current_process->PC + 3;
        }
        break;
      }
      case 10: { // jmp op2
        current_process->PC = executor.IR.Y;
        break;
      }
      case 11: // if0 pop,->op2
        if (interp_local->S[current_process->T] == 0) {
          current_process->PC = executor.IR.Y;
        }
        current_process->T--;
        break;
      case 12: {
        executor.H1 = interp_local->S[current_process->T];
        current_process->T--;
        executor.H2 = executor.IR.Y;
        executor.H3 = 0;
        do {
          if (code[executor.H2].F != 13) {
            executor.H3 = 1;
            interp_local->PS = InterpLocal::PS::CASCHK;
          } else if (code[executor.H2].X == -1 ||
                     code[executor.H2].Y == executor.H1) {
            executor.H3 = 1;
            current_process->PC = code[executor.H2 + 1].Y;
          } else {
            executor.H2 = executor.H2 + 2;
          }
        } while (executor.H3 == 0);
        break;
      }
      case 13: {
        executor.H1 = executor.IR.X;
        executor.H2 = executor.IR.Y;
        executor.H3 = FINDFRAME(interp_local, executor.H2 + 1);
        if (debug & DBGPROC) {
          println("opc {} findframe {} length {}, response {}", executor.IR.F, current_process->PID, executor.H2 + 1, executor.H3); }
        if (executor.H3 < 0) {
          interp_local->PS = InterpLocal::PS::STKCHK;
        } else {
          for (executor.I = 0; executor.I < executor.H2; executor.I++) {
            interp_local->S[executor.H3 + executor.I] =
                (unsigned char)STAB[executor.H1 + executor.I];
          }
          interp_local->S[executor.H3 + executor.H2] = 0;
          current_process->T++;
          if (current_process->T > current_process->STACKSIZE) {
            interp_local->PS = InterpLocal::PS::STKCHK;
          } else {
            interp_local->S[current_process->T] = executor.H3;
          }
        }
        break;
      }
      case 14: { // push from stack top stack frame loc
        executor.H1 = interp_local->S[current_process->T];
        //                        if (CURPR->PROCESSOR == 0)
        //                            fprintf(STANDARD_OUTPUT, "14: proc %d pc %d stack
        //                            %d\n", CURPR->PROCESSOR, CURPR->PC,
        //                            CURPR->T);
        current_process->T++;
        if (current_process->T > current_process->STACKSIZE) {
          interp_local->PS = InterpLocal::PS::STKCHK;
        } else {
          interp_local->S[current_process->T] = interp_local->S[executor.H1];
          if (interp_local->S[current_process->T] == RTAG) {
            interp_local->RS[current_process->T] =
                interp_local->RS[executor.H1];
          }
        }
        break;
      }
      case 15: { // add
        current_process->T--;
        interp_local->S[current_process->T] =
            interp_local->S[current_process->T] +
            interp_local->S[current_process->T + 1];
        break;
      }
      case 18: { // IR.Y is index of a function in TAB[] symbol table
                 // get stack frame for block execution callblk op
        if (TAB[executor.IR.Y].address == 0) {
          interp_local->PS = InterpLocal::PS::FUNCCHK;
        } else if (TAB[executor.IR.Y].address > 0) {
          executor.H1 = BTAB[TAB[executor.IR.Y].reference].VSIZE + WORKSIZE;
          executor.H2 = FINDFRAME(interp_local, executor.H1);
          if (debug & DBGPROC) {
            fprintf(STANDARD_OUTPUT, "opc %d findframe %d length %d, response %d\n",
                    executor.IR.F, current_process->PID, executor.H1,
                    executor.H2);
          }
          if (executor.H2 >= 0) {
            interp_local->S[executor.H2 + 7] = current_process->T;
            current_process->T = executor.H2 - 1;
            current_process->STACKSIZE = executor.H1 + executor.H2 - 1;
            current_process->T += 8;
            interp_local->S[current_process->T - 4] = executor.H1 - 1;
            interp_local->S[current_process->T - 3] = executor.IR.Y;
            for (executor.I = executor.H2;
                 executor.I <= executor.H2 + BASESIZE - 1; executor.I++) {
              interp_local->STARTMEM[executor.I] =
                  -interp_local->STARTMEM[executor.I];
            }
          }
        }
        break;
      }
      case 19: {
        if (TAB[executor.IR.X].address < 0) {
          EXECLIB(interp_local, &executor, current_process,
                  -TAB[executor.IR.X].address);
        } else {
          executor.H1 = current_process->T -
                        executor.IR.Y; // base of frame acquired by 18
          executor.H2 = interp_local->S[executor.H1 + 4]; // function TAB index
          executor.H3 = TAB[executor.H2].LEV;             // TAB[stk + 4].LEV
          current_process->DISPLAY[executor.H3 + 1] =
              executor.H1; // DISPLAY[H3 + 1]
          for (executor.I = executor.H3 + 2; executor.I <= LMAX;
               executor.I++) { // clear DISPLAY to top w/-1
            current_process->DISPLAY[executor.I] = -1;
          }
          interp_local->S[executor.H1 + 6] =
              interp_local->S[executor.H1 + 3] + 1;
          executor.H4 = interp_local->S[executor.H1 + 3] + executor.H1;
          interp_local->S[executor.H1 + 1] = current_process->PC; // return addr
          interp_local->S[executor.H1 + 2] =
              current_process->DISPLAY[executor.H3];
          interp_local->S[executor.H1 + 3] = current_process->B;
          interp_local->S[executor.H1 + 5] = 1;
          for (executor.H3 = current_process->T + 1; executor.H3 <= executor.H4;
               executor.H3++) {
            interp_local->S[executor.H3] = 0;
            interp_local->RS[executor.H3] = 0.0;
          }
          for (executor.H3 = executor.H1; executor.H3 <= executor.H4;
               executor.H3++) {
            interp_local->SLOCATION[executor.H3] = current_process->PROCESSOR;
          }
          current_process->B = executor.H1;
          current_process->T = executor.H4 - WORKSIZE;
          current_process->PC =
              TAB[executor.H2].address; // jump to block address
          TIMEINC(interp_local, 3, "cs19");
        }
        break;
      }
      case 20: { // swap
        executor.H1 = interp_local->S[current_process->T];
        executor.RH1 = interp_local->RS[current_process->T];
        interp_local->S[current_process->T] =
            interp_local->S[current_process->T - 1];
        interp_local->RS[current_process->T] =
            interp_local->RS[current_process->T - 1];
        interp_local->S[current_process->T - 1] = executor.H1;
        interp_local->RS[current_process->T - 1] = executor.RH1;
        break;
      }
      case 21: { // load array Y[T]
        // op2 is array table index
        // stack top is array subscript
        // top - 1 is array base in stack
        // pop stack
        // replace top with stack element at array base plus subscript
        executor.H1 = executor.IR.Y;
        executor.H2 = ATAB[executor.H1].LOW;
        executor.H3 = interp_local->S[current_process->T]; // array subscript
        if (executor.H3 < executor.H2 ||
            ((executor.H3 > ATAB[executor.H1].HIGH) &&
             (ATAB[executor.H1].HIGH > 0))) {
          interp_local->PS = InterpLocal::PS::INXCHK;
        } else {
          current_process->T--;
          executor.H4 =
              interp_local->S[current_process->T]; // added for debugging
          interp_local->S[current_process->T] =
              interp_local->S[current_process->T] +
              (executor.H3 - executor.H2) * ATAB[executor.H1].ELSIZE;
          // fprintf(STANDARD_OUTPUT, "\nindex %d array stack base %d stack loc %d\n",
          // el.H3, el.H4, il->S[CURPR->T]);
        }
        break;
      }
      case 22: {
        executor.H1 = interp_local->S[current_process->T];
        if ((executor.H1 <= 0) || (executor.H1 >= STMAX)) {
          interp_local->PS = InterpLocal::PS::REFCHK;
        } else {
          current_process->T--;
          executor.H2 = executor.IR.Y + current_process->T;
          if (topology != Symbol::SHAREDSY) {
            TESTVAR(interp_local, executor.H1);
          }
          if (executor.H2 > current_process->STACKSIZE) {
            interp_local->PS = InterpLocal::PS::STKCHK;
          } else {
            while (current_process->T < executor.H2) {
              current_process->T++;
              if (interp_local->NUMTRACE > 0) {
                CHKVAR(interp_local, executor.H1);
              }
              interp_local->S[current_process->T] =
                  interp_local->S[executor.H1];
              if (interp_local->S[executor.H1] == RTAG)
                interp_local->RS[current_process->T] =
                    interp_local->RS[executor.H1];
              executor.H1++;
            }
          }
        }
        break;
      }
      case 23: {
        executor.H1 = interp_local->S[current_process->T - 1];
        executor.H2 = interp_local->S[current_process->T];
        if ((executor.H1 <= 0) || (executor.H2 <= 0) ||
            (executor.H1 >= STMAX) || (executor.H2 >= STMAX)) {
          interp_local->PS = InterpLocal::PS::REFCHK;
        } else {
          executor.H3 = executor.H1 + executor.IR.X;
          if (topology != Symbol::SHAREDSY) {
            TESTVAR(interp_local, executor.H2);
            if (executor.IR.Y == 0) {
              TESTVAR(interp_local, executor.H1);
            } else if (interp_local->CONGESTION) {
              executor.H4 = COMMDELAY(interp_local, current_process->PROCESSOR,
                                      interp_local->SLOCATION[executor.H1],
                                      executor.IR.X);
            }
          }
          while (executor.H1 < executor.H3) {
            if (interp_local->NUMTRACE > 0) {
              CHKVAR(interp_local, executor.H1);
              CHKVAR(interp_local, executor.H2);
            }
            if (interp_local->STARTMEM[executor.H1] <= 0) {
              interp_local->PS = InterpLocal::PS::REFCHK;
            }
            interp_local->S[executor.H1] = interp_local->S[executor.H2];
            if (interp_local->S[executor.H2] == RTAG) {
              interp_local->RS[executor.H1] = interp_local->RS[executor.H2];
            }
            executor.H1++;
            executor.H2++;
          }
          TIMEINC(interp_local, executor.IR.X / 5, "cs23");
          interp_local->S[current_process->T - 1] =
              interp_local->S[current_process->T];
          current_process->T--;
        }
        break;
      }
      case 24: { // push imm op2
        current_process->T++;
        if (current_process->T > current_process->STACKSIZE) {
          interp_local->PS = InterpLocal::PS::STKCHK;
        } else {
          interp_local->S[current_process->T] = (int)executor.IR.Y;
        }
        break;
      }
      case 26: { // convreal
        interp_local->RS[current_process->T] =
            interp_local->S[current_process->T];
        interp_local->S[current_process->T] = RTAG;
        break;
      }
      case 27: {
        int IORESULT = 0;
#if defined(__APPLE__)
        *__error() = 0;
#elif defined(__linux__) || defined(_WIN32)
        errno = 0;
#endif
        executor.H1 = interp_local->S[current_process->T];
        if (topology != Symbol::SHAREDSY) {
          TESTVAR(interp_local, executor.H1);
        }
        if (interp_local->NUMTRACE > 0) {
          CHKVAR(interp_local, executor.H1);
        }
        if (INPUTFILE_FLAG) {
          if (feof(INPUT)) {
            interp_local->PS = InterpLocal::PS::REDCHK;
          }
        } else if (feof(STANDARD_INPUT)) {
          interp_local->PS = InterpLocal::PS::REDCHK;
        }
        if (interp_local->PS != InterpLocal::PS::REDCHK) {
          if (!INPUTFILE_FLAG) {
            switch (executor.IR.Y) {
            case 1: {
              interp_local->S[executor.H1] = RTAG;
              // INPUT >> il->RS[el.H1];
              // fscanf(STANDARD_INPUT, "%lf", &il->RS[el.H1]);
              fscanf(STANDARD_INPUT, "%s", executor.buf.str().data());
              interp_local->RS[executor.H1] =
                  strtod(executor.buf.str().data(), nullptr);
#if defined(__APPLE__)
              if (*__error() != 0)
#elif defined(__linux__) || defined(_WIN32)
              if (errno != 0)
#endif
                IORESULT = -1;
              break;
            }
            case 2: {
              // INPUT >> il->S[el.H1];
              // fscanf(STANDARD_INPUT, "%d", &il->S[el.H1]);
              fscanf(STANDARD_INPUT, "%s", executor.buf.str().data());
              interp_local->S[executor.H1] = static_cast<int>(
                  strtol(executor.buf.str().data(), nullptr, 10));
#if defined(__APPLE__)
              if (*__error() != 0) {
#elif defined(__linux__) || defined(_WIN32)
              if (errno != 0) {
#endif
                IORESULT = -1;
              }
              break;
            }
            case 4: {
              // char CH;
              //  INPUT >> CH;
              // il->S[el.H1] = int(CH);
              interp_local->S[executor.H1] = fgetc(STANDARD_INPUT);
              if (0 > interp_local->S[executor.H1])
                IORESULT = -1;
              break;
            }
            }
          } else {
            switch (executor.IR.Y) {
            case 1: {
              interp_local->S[executor.H1] = RTAG;
              // INPUT >> il->RS[el.H1];
              // fscanf(INPUT, "%lf", &il->RS[el.H1]);
              fscanf(INPUT, "%s", executor.buf.str().data());
              interp_local->RS[executor.H1] =
                  strtod(executor.buf.str().data(), nullptr);
#if defined(__APPLE__)
              if (*__error() != 0)
#elif defined(__linux__) || defined(_WIN32)
              if (errno != 0)
#endif
                IORESULT = -1;
              break;
            }
            case 2: {
              // INPUT >> il->S[el.H1];
              // fscanf(STANDARD_INPUT, "%d", &il->S[el.H1]);
              fscanf(INPUT, "%s", executor.buf.str().data());
              interp_local->S[executor.H1] = static_cast<int>(
                  strtol(executor.buf.str().data(), nullptr, 10));
#if defined(__APPLE__)
              if (*__error() != 0)
#elif defined(__linux__) || (_WIN32)
              if (errno != 0)
#endif
                IORESULT = -1;
              break;
            }
            case 4: {
              // char CH;
              // INPUT >> CH;
              // il->S[el.H1] = int(CH);
              // il->S[el.H1] = fgetc(STANDARD_INPUT);
              interp_local->S[executor.H1] = fgetc(INPUT);
              if (0 > interp_local->S[executor.H1]) {
                IORESULT = -1;
              }
              break;
            }
            }
          }
          if (IORESULT != 0) {
            interp_local->PS = InterpLocal::PS::DATACHK;
          }
        }
        current_process->T--;
        break;
      }
      case 28: { // write from string table
        executor.H1 = interp_local->S[current_process->T];
        executor.H2 = executor.IR.Y;
        current_process->T--;
        interp_local->CHRCNT = interp_local->CHRCNT + executor.H1;
        if (interp_local->CHRCNT > LINELENG) {
          interp_local->PS = InterpLocal::PS::LNGCHK;
        }
        do {
          if (!OUTPUTFILE_FLAG) {
            std::fputc(STAB[executor.H2], STANDARD_OUTPUT);
          } else {
            std::fputc(STAB[executor.H2], OUTPUT);
          }
          executor.H1--;
          executor.H2++;
          if (STAB[executor.H2] == (char)10 || STAB[executor.H2] == (char)13) {
            interp_local->CHRCNT = 0;
          }
        } while (executor.H1 != 0);
        break;
      }
      case 29: { //  outwidth output formatted with WIDTH
        if (interp_local->COUTWIDTH <= 0) {
          interp_local->CHRCNT =
              interp_local->CHRCNT + interp_local->FLD[executor.IR.Y];
          if (interp_local->CHRCNT > LINELENG) {
            interp_local->PS = InterpLocal::PS::LNGCHK;
          } else {
            if (!OUTPUTFILE_FLAG) {
              switch (executor.IR.Y) {
              case 2: {
                fprintf(STANDARD_OUTPUT, "%*d", interp_local->FLD[2],
                        interp_local->S[current_process->T]);
                break;
              }
              case 3: {
                fprintf(STDOUT, "%*s", interp_local->FLD[3],
                        (ITOB(interp_local->S[current_process->T])) ? "TRUE"
                                                                    : "FALSE");
                break;
              }
              case 4: {
                if ((interp_local->S[current_process->T] < CHARL) ||
                    (interp_local->S[current_process->T] > CHARH)) {
                  interp_local->PS = InterpLocal::PS::CHRCHK;
                } else {
                  fprintf(STDOUT, "%*c", interp_local->FLD[4],
                          (char)(interp_local->S[current_process->T]));
                  if ((interp_local->S[current_process->T] == 10) ||
                      (interp_local->S[current_process->T] == 13)) {
                    interp_local->CHRCNT = 0;
                  }
                }
                break;
              }
              }
            } else {
              switch (executor.IR.Y) {
              case 2: {
                fprintf(OUTPUT, "%*d", interp_local->FLD[2],
                        interp_local->S[current_process->T]);
                break;
              }
              case 3: {
                fprintf(OUTPUT, "%*s", interp_local->FLD[3],
                        (ITOB(interp_local->S[current_process->T])) ? "true"
                                                                    : "false");
                break;
              }
              case 4: {
                if ((interp_local->S[current_process->T] < CHARL) ||
                    (interp_local->S[current_process->T] > CHARH)) {
                  interp_local->PS = InterpLocal::PS::CHRCHK;
                } else {
                  fprintf(OUTPUT, "%*c", interp_local->FLD[4],
                          char(interp_local->S[current_process->T]));
                  if ((interp_local->S[current_process->T] == 10) ||
                      (interp_local->S[current_process->T] == 13)) {
                    interp_local->CHRCNT = 0;
                  }
                }
                break;
              }
              }
            }
          }
        } else {
          interp_local->CHRCNT = interp_local->CHRCNT + interp_local->COUTWIDTH;
          executor.H1 = interp_local->S[current_process->T];
          if (interp_local->CHRCNT > LINELENG) {
            interp_local->PS = InterpLocal::PS::LNGCHK;
          } else {
            if (!OUTPUTFILE_FLAG) {
              switch (executor.IR.Y) {
              case 2: {
                // std::cout << el.H1;
                fprintf(STDOUT, "%*d", interp_local->COUTWIDTH, executor.H1);
                break;
              }
              case 3: {
                // std::cout << ITOB(el.H1);
                fprintf(STDOUT, "%*s", interp_local->COUTWIDTH,
                        (ITOB(executor.H1)) ? "true" : "false");
                break;
              }
              case 4: {
                if ((executor.H1 < CHARL) || (executor.H1 > CHARH)) {
                  interp_local->PS = InterpLocal::PS::CHRCHK;
                } else {
                  // std::cout << char(el.H1);
                  fprintf(STDOUT, "%*c", interp_local->COUTWIDTH,
                          (char)executor.H1);
                  if ((executor.H1 == 10) || (executor.H1 == 13)) {
                    interp_local->CHRCNT = 0;
                  }
                }
                break;
              }
              }
            } else {
              switch (executor.IR.Y) {
              case 2: {
                fprintf(OUTPUT, "%*d", interp_local->COUTWIDTH, executor.H1);
                break;
              }
              case 3: {
                fprintf(OUTPUT, "%*s", interp_local->COUTWIDTH,
                        (ITOB(executor.H1)) ? "true" : "false");
                break;
              }
              case 4: {
                if ((executor.H1 < CHARL) || (executor.H1 > CHARH)) {
                  interp_local->PS = InterpLocal::PS::CHRCHK;
                } else {
                  fprintf(OUTPUT, "%*c", interp_local->COUTWIDTH,
                          (char)executor.H1);
                  if ((executor.H1 == 10) || (executor.H1 == 13)) {
                    interp_local->CHRCNT = 0;
                  }
                }
                break;
              }
              }
            }
          }
        }
        current_process->T--;
        interp_local->COUTWIDTH = -1;
        break;
      }
      case 30: { // pop and set out width or out prec
        if (executor.IR.Y == 1) {
          interp_local->COUTWIDTH = interp_local->S[current_process->T];
        } else {
          interp_local->COUTPREC = interp_local->S[current_process->T];
        }
        current_process->T = current_process->T - 1;
        break;
      }
      case 31: { // main  end
        if (current_process->FORKCOUNT > 1) {
          current_process->STATE = PRD::STATE::BLOCKED;
          current_process->PC = current_process->PC - 1;
          interp_local->PROCTAB[current_process->PROCESSOR].RUNPROC = nullptr;
          current_process->FORKCOUNT = current_process->FORKCOUNT - 1;
        } else {
          interp_local->PS = InterpLocal::PS::FIN;
          if (debug & DBGPROC) {
            fprintf(STDOUT, "opc %d terminated pid %d\n", executor.IR.F,
                    current_process->PID);
            if (current_process->PID == 0)
              unreleased(interp_local);
          }
          current_process->STATE = PROCESSDESCRIPTOR::STATE::TERMINATED;
        }
        break;
      }
      case 32: {
        if (interp_local->S[current_process->B + 5] == 1) {
          if (debug & DBGRELEASE) {
            fprintf(STDOUT, "%d release %d fm=%d ln=%d\n", executor.IR.F,
                    current_process->PID, current_process->B,
                    interp_local->S[current_process->B + 6]);
          }
          RELEASE(interp_local, current_process->B,
                  interp_local->S[current_process->B + 6]);
        } else {
          interp_local->S[current_process->B + 5] -= 1;
          if (debug & DBGRELEASE) {
            fprintf(STDOUT, "%d ref ct %d ct=%d\n", executor.IR.F,
                    current_process->PID,
                    interp_local->S[current_process->B + 5]);
          }
        }
        executor.H1 = TAB[interp_local->S[current_process->B + 4]].LEV;
        current_process->DISPLAY[executor.H1 + 1] = -1;
        current_process->PC = interp_local->S[current_process->B + 1];
        current_process->T = interp_local->S[current_process->B + 7];
        current_process->B = interp_local->S[current_process->B + 3];
        if ((current_process->T >= current_process->B - 1) &&
            (current_process->T <
             current_process->B + interp_local->S[current_process->B + 6])) {
          current_process->STACKSIZE =
              current_process->B + interp_local->S[current_process->B + 6] - 1;
        } else {
          current_process->STACKSIZE = current_process->BASE + WORKSIZE - 1;
        }
        break;
      }
      case 33: { // release block resources ?
        executor.H1 = TAB[interp_local->S[current_process->B + 4]].LEV;
        executor.H2 = current_process->T;
        current_process->DISPLAY[executor.H1 + 1] = -1;
        if (interp_local->S[current_process->B + 5] == 1) {
          if (debug & DBGRELEASE) {
            // fprintf(STDOUT, "release base %d, length %d\n", CURPR->B,
            // il->S[CURPR->B+6]);
            fprintf(STDOUT, "%d release %d fm=%d ln=%d\n", executor.IR.F,
                    current_process->PID, current_process->B,
                    interp_local->S[current_process->B + 6]);
          }
          RELEASE(interp_local, current_process->B,
                  interp_local->S[current_process->B + 6]);
        } else {
          // fprintf(STDOUT, "release adjacent %d\n", il->S[CURPR->B+6]);
          interp_local->S[current_process->B + 5] =
              interp_local->S[current_process->B + 5] - 1;
          if (debug & DBGRELEASE) {
            fprintf(STDOUT, "%d ref ct %d ct=%d\n", executor.IR.F,
                    current_process->PID,
                    interp_local->S[current_process->B + 5]);
          }
        }
        current_process->T = interp_local->S[current_process->B + 7] + 1;
        interp_local->S[current_process->T] = interp_local->S[executor.H2];
        if (interp_local->S[current_process->T] == RTAG) {
          interp_local->RS[current_process->T] = interp_local->RS[executor.H2];
        }
        current_process->PC = interp_local->S[current_process->B + 1];
        current_process->B = interp_local->S[current_process->B + 3];
        if ((current_process->T >= current_process->B - 1) &&
            (current_process->T <
             current_process->B + interp_local->S[current_process->B + 6])) {
          current_process->STACKSIZE =
              current_process->B + interp_local->S[current_process->B + 6] - 1;
        } else {
          current_process->STACKSIZE = current_process->BASE + WORKSIZE - 1;
        }
        break;
      }
      case 34: { // get variable stack location and replace top with value
        // load indirect ->[T] to T
        executor.H1 = interp_local->S[current_process->T];
        if ((executor.H1 <= 0) || (executor.H1 >= STMAX)) {
          interp_local->PS = InterpLocal::PS::REFCHK;
        } else {
          if (topology != Symbol::SHAREDSY) {
            TESTVAR(interp_local, executor.H1);
          }
          if (interp_local->NUMTRACE > 0) {
            CHKVAR(interp_local, executor.H1);
          }
          if (interp_local->S[executor.H1] == RTAG) {
            interp_local->RS[current_process->T] =
                interp_local->RS[executor.H1];
          }
          interp_local->S[current_process->T] = interp_local->S[executor.H1];
        }
        break;
      }
      case 35: // not
        interp_local->S[current_process->T] =
            BTOI(!(ITOB(interp_local->S[current_process->T])));
        break;
      case 36: { // negate
        if (interp_local->S[current_process->T] == RTAG) {
          interp_local->RS[current_process->T] =
              -interp_local->RS[current_process->T];
        } else {
          interp_local->S[current_process->T] =
              -interp_local->S[current_process->T];
        }
        break;
      }
      case 37: { // out real with prec/width or none
        executor.RH1 = interp_local->RS[current_process->T];
        if (executor.RH1 < 1) {
          executor.H2 = 0;
        } else {
          executor.H2 = 1 + (int)std::floor(std::log(std::fabs(executor.RH1)) /
                                            executor.log10);
        }
        if (interp_local->COUTWIDTH <= 0) {
          executor.H1 = interp_local->FLD[1];
        } else {
          executor.H1 = interp_local->COUTWIDTH;
        }
        if ((interp_local->COUTPREC > 0) &&
            (interp_local->COUTPREC + 3 < executor.H1)) {
          executor.H1 = interp_local->COUTPREC + 3;
        }
        interp_local->CHRCNT = interp_local->CHRCNT + executor.H1;
        if (interp_local->CHRCNT > LINELENG) {
          interp_local->PS = InterpLocal::PS::LNGCHK;
        } else {
          if (!OUTPUTFILE_FLAG) {
            if (interp_local->COUTPREC <= 0) {
              // cout << el.RH1;
              fprintf(STDOUT, "%*f", executor.H1, executor.RH1);
            } else {
              if (interp_local->COUTWIDTH <= 0) {
                // cout << el.RH1 << ":" << il->COUTPREC - el.H2;
                fprintf(STDOUT, "%*.*f", interp_local->COUTPREC + 3,
                        interp_local->COUTPREC - executor.H2, executor.RH1);
              } else {
                // cout << el.RH1 << ":" << il->COUTWIDTH << ":" << il->COUTPREC
                // - el.H2;
                fprintf(STDOUT, "%*.*f", interp_local->COUTWIDTH,
                        interp_local->COUTPREC - executor.H2, executor.RH1);
              }
            }
          } else {
            if (interp_local->COUTPREC <= 0) {
              // OUTPUT << el.RH1;
              fprintf(OUTPUT, "%*f", executor.H1, executor.RH1);
            } else {
              if (interp_local->COUTWIDTH <= 0) {
                fprintf(OUTPUT, "%*.*f", interp_local->COUTPREC + 3,
                        interp_local->COUTPREC - executor.H2, executor.RH1);
              } else {
                fprintf(OUTPUT, "%*.*f", interp_local->COUTWIDTH,
                        interp_local->COUTPREC - executor.H2, executor.RH1);
              }
            }
          }
        }
        current_process->T = current_process->T - 1;
        interp_local->COUTWIDTH = -1;
        break;
      }
      case 38: { // get saved DISPLAY (a stack location for a variable) copy top
                 // to it
        // [[T-1]] <- [T]
        // [T-1] <- [T]
        // T <- T-1
        // stindstk
        executor.H1 = interp_local->S[current_process->T - 1];
        if (executor.H1 <= 0 || executor.H1 >= STMAX) {
          interp_local->PS = InterpLocal::PS::REFCHK;
        } else {
          if (interp_local->STARTMEM[executor.H1] <= 0) {
            interp_local->PS = InterpLocal::PS::REFCHK;
          }
          interp_local->S[executor.H1] = interp_local->S[current_process->T];
          if (topology != Symbol::SHAREDSY) {
            if (executor.IR.Y == 0) {
              TESTVAR(interp_local, executor.H1);
            } else if (interp_local->CONGESTION) {
              executor.H2 = COMMDELAY(interp_local, current_process->PROCESSOR,
                                      interp_local->SLOCATION[executor.H1], 1);
            }
          }
          if (interp_local->NUMTRACE > 0) {
            CHKVAR(interp_local, executor.H1);
          }
          if (interp_local->S[current_process->T] == RTAG) {
            interp_local->RS[executor.H1] =
                interp_local->RS[current_process->T];
          }
          interp_local->S[current_process->T - 1] =
              interp_local->S[current_process->T];
          if (interp_local->S[current_process->T] == RTAG) {
            interp_local->RS[current_process->T - 1] =
                interp_local->RS[current_process->T];
          }
          current_process->T = current_process->T - 1;
        }
        break;
      }
      case 45: { // pop and compare EQ
        current_process->T = current_process->T - 1;
        if (ISREAL(interp_local)) {
          interp_local->S[current_process->T] =
              BTOI(interp_local->RS[current_process->T] ==
                   interp_local->RS[current_process->T + 1]);
        } else {
          interp_local->S[current_process->T] =
              BTOI(interp_local->S[current_process->T] ==
                   interp_local->S[current_process->T + 1]);
        }
        break;
      }
      case 46: { // pop and compare NE
        current_process->T = current_process->T - 1;
        if (ISREAL(interp_local)) {
          interp_local->S[current_process->T] =
              BTOI(interp_local->RS[current_process->T] !=
                   interp_local->RS[current_process->T + 1]);
        } else {
          interp_local->S[current_process->T] =
              BTOI(interp_local->S[current_process->T] !=
                   interp_local->S[current_process->T + 1]);
        }
        break;
      }
      case 47: { // pop, then compare top op top+1, replace top with comparison
                 // result
        //  pop and compare LT
        current_process->T = current_process->T - 1;
        if (ISREAL(interp_local)) {
          interp_local->S[current_process->T] =
              BTOI(interp_local->RS[current_process->T] <
                   interp_local->RS[current_process->T + 1]);
        } else {
          interp_local->S[current_process->T] =
              BTOI(interp_local->S[current_process->T] <
                   interp_local->S[current_process->T + 1]);
        }
        break;
      }
      case 48: { // pop and compare LE
        current_process->T = current_process->T - 1;
        if (ISREAL(interp_local)) {
          interp_local->S[current_process->T] =
              BTOI(interp_local->RS[current_process->T] <=
                   interp_local->RS[current_process->T + 1]);
        } else {
          interp_local->S[current_process->T] =
              BTOI(interp_local->S[current_process->T] <=
                   interp_local->S[current_process->T + 1]);
        }
        break;
      }
      case 49: { // pop and compare GT
        current_process->T = current_process->T - 1;
        if (ISREAL(interp_local)) {
          interp_local->S[current_process->T] =
              BTOI(interp_local->RS[current_process->T] >
                   interp_local->RS[current_process->T + 1]);
        } else {
          interp_local->S[current_process->T] =
              BTOI(interp_local->S[current_process->T] >
                   interp_local->S[current_process->T + 1]);
        }
        break;
      }
      case 50: { // pop and compare GE
        current_process->T = current_process->T - 1;
        if (ISREAL(interp_local)) {
          interp_local->S[current_process->T] =
              BTOI(interp_local->RS[current_process->T] >=
                   interp_local->RS[current_process->T + 1]);
        } else {
          interp_local->S[current_process->T] =
              BTOI(interp_local->S[current_process->T] >=
                   interp_local->S[current_process->T + 1]);
        }
        break;
      }
      case 51: { // pop OR
        current_process->T = current_process->T - 1;
        interp_local->S[current_process->T] =
            BTOI(ITOB(interp_local->S[current_process->T]) ||
                 ITOB(interp_local->S[current_process->T + 1]));
        break;
      }
      case 52: { // plus
        current_process->T = current_process->T - 1;
        if (ISREAL(interp_local)) {
          interp_local->RS[current_process->T] =
              interp_local->RS[current_process->T] +
              interp_local->RS[current_process->T + 1];
          if (interp_local->RS[current_process->T] != 0) {
            executor.RH1 = log(fabs(interp_local->RS[current_process->T])) /
                           executor.log10;
            if ((executor.RH1 >= 292.5) || (executor.RH1 <= -292.5)) {
              interp_local->PS = InterpLocal::PS::OVRCHK;
            }
          }
        } else {
          interp_local->S[current_process->T] =
              interp_local->S[current_process->T] +
              interp_local->S[current_process->T + 1];
          if (abs(interp_local->S[current_process->T]) > NMAX) {
            interp_local->PS = InterpLocal::PS::INTCHK;
          }
        }
        break;
      }
      case 53: { // minus
        current_process->T = current_process->T - 1;
        if (ISREAL(interp_local)) {
          interp_local->RS[current_process->T] =
              interp_local->RS[current_process->T] -
              interp_local->RS[current_process->T + 1];
          if (interp_local->RS[current_process->T] != 0) {
            executor.RH1 = log(fabs(interp_local->RS[current_process->T])) /
                           executor.log10;
            if ((executor.RH1 >= 292.5) || (executor.RH1 <= -292.5)) {
              interp_local->PS = InterpLocal::PS::OVRCHK;
            }
          }
        } else {
          interp_local->S[current_process->T] =
              interp_local->S[current_process->T] -
              interp_local->S[current_process->T + 1];
          if (abs(interp_local->S[current_process->T]) > NMAX) {
            interp_local->PS = InterpLocal::PS::INTCHK;
          }
        }
        break;
      }
      case 56: { // and
        current_process->T = current_process->T - 1;
        interp_local->S[current_process->T] =
            BTOI(ITOB(interp_local->S[current_process->T]) &&
                 ITOB(interp_local->S[current_process->T + 1]));
        break;
      }
      case 57: { // times
        current_process->T = current_process->T - 1;
        if (ISREAL(interp_local)) {
          interp_local->RS[current_process->T] =
              interp_local->RS[current_process->T] *
              interp_local->RS[current_process->T + 1];
          if (interp_local->RS[current_process->T] != 0) {
            executor.RH1 = log(fabs(interp_local->RS[current_process->T])) /
                           executor.log10;
            if ((executor.RH1 >= 292.5) || (executor.RH1 <= -292.5)) {
              interp_local->PS = InterpLocal::PS::OVRCHK;
            }
          }
        } else {
          interp_local->S[current_process->T] =
              interp_local->S[current_process->T] *
              interp_local->S[current_process->T + 1];
          if (abs(interp_local->S[current_process->T]) > NMAX) {
            interp_local->PS = InterpLocal::PS::INTCHK;
          }
        }
        break;
      }
      case 58: { // int div
        current_process->T = current_process->T - 1;
        if (interp_local->S[current_process->T + 1] == 0) {
          interp_local->PS = InterpLocal::PS::DIVCHK;
        } else {
          interp_local->S[current_process->T] =
              interp_local->S[current_process->T] /
              interp_local->S[current_process->T + 1];
        }
        break;
      }
      case 59: { // int mod
        current_process->T = current_process->T - 1;
        if (interp_local->S[current_process->T + 1] == 0) {
          interp_local->PS = InterpLocal::PS::DIVCHK;
        } else {
          interp_local->S[current_process->T] =
              interp_local->S[current_process->T] %
              interp_local->S[current_process->T + 1];
        }
        break;
      }
      case 62: {
        if (!INPUTFILE_FLAG) {
          if (feof(STANDARD_INPUT)) {
            interp_local->PS = InterpLocal::PS::REDCHK;
          } else {
            fgetc(STANDARD_INPUT);
          }
        } else {
          if (feof(INPUT)) {
            interp_local->PS = InterpLocal::PS::REDCHK;
          } else {
            fgetc(INPUT);
          }
        }
        break;
      }
      case 63: { // write endl
        if (!OUTPUTFILE_FLAG) {
          fputc('\n', STDOUT);
        } else {
          fputc('\n', OUTPUT);
        }
        interp_local->LNCNT = interp_local->LNCNT + 1;
        interp_local->CHRCNT = 0;
        if (interp_local->LNCNT > LINELIMIT) {
          interp_local->PS = InterpLocal::PS::LINCHK;
        }
        break;
      }
      case 64:
      case 65:
      case 71: { // [T] is stk loc of channel
        switch (executor.IR.F) {
        case 64:
          executor.H1 = current_process->DISPLAY[executor.IR.X] + executor.IR.Y;
          break;
        case 65:
          executor.H1 =
              interp_local
                  ->S[current_process->DISPLAY[executor.IR.X] + executor.IR.Y];
          break;
        case 71:
          executor.H1 = interp_local->S[current_process->T];
          break;
        }
        executor.H2 = interp_local->SLOCATION[executor.H1];
        interp_local->CNUM = interp_local->S[executor.H1];
        if (debug & DBGRECV) {
          fprintf(STDOUT, "recv %d pid %d pc %d state %s rdstatus %s chan %d\n",
                  executor.IR.F, current_process->PID, current_process->PC,
                  nameState(current_process->STATE),
                  nameRdstatus(current_process->READSTATUS),
                  interp_local->CNUM);
        }
        if (interp_local->NUMTRACE > 0) {
          CHKVAR(interp_local, executor.H1);
        }
        if (interp_local->CNUM == 0) {
          interp_local->CNUM =
              FIND(interp_local); // get and initialize unused channel
          interp_local->S[executor.H1] =
              interp_local->CNUM; // store channel in stack loc
        }
        if (current_process->READSTATUS == PRD::READSTATUS::NONE) {
          current_process->READSTATUS = PRD::READSTATUS::ATCHANNEL;
        }
        channel = &interp_local->CHAN[interp_local->CNUM];
        // WITH CHAN[CNUM]
        if (!(*channel).MOVED && (*channel).READER < 0) {
          interp_local->SLOCATION[executor.H1] = current_process->PROCESSOR;
          (*channel).READER = current_process->PID;
        }
        if (topology != Symbol::SHAREDSY)
          TESTVAR(interp_local, executor.H1); // channel in stk
        if ((*channel).READTIME < interp_local->CLOCK - TIMESTEP) {
          (*channel).READTIME = interp_local->CLOCK - TIMESTEP;
        }
        interp_local->PNT = (*channel).HEAD;
        executor.B1 = (*channel).SEM == 0;
        if (!executor.B1) {
          executor.B1 =
              interp_local->DATE[interp_local->PNT] > current_process->TIME;
        }
        if (executor.B1) {
          interp_local->PTEMP = (ACTPNT)calloc(1, sizeof(ACTIVEPROCESS));
          interp_local->PTEMP->PDES = current_process;
          interp_local->PTEMP->NEXT = nullptr;
          interp_local->PROCTAB[current_process->PROCESSOR].RUNPROC = nullptr;
          if ((*channel).WAIT == nullptr) {
            executor.H3 = 1;
            (*channel).WAIT = interp_local->PTEMP;
            if ((*channel).SEM != 0) {
              current_process->STATE = PRD::STATE::DELAYED;
              current_process->WAKETIME = interp_local->DATE[interp_local->PNT];
            } else {
              current_process->STATE = PRD::STATE::BLOCKED;
            }
          } else {
            interp_local->RTEMP = (*channel).WAIT;
            while (interp_local->RTEMP->NEXT != nullptr) {
              interp_local->RTEMP = interp_local->RTEMP->NEXT;
            }
            interp_local->RTEMP->NEXT = interp_local->PTEMP;
            current_process->STATE = PRD::STATE::BLOCKED;
          }
          current_process->PC = current_process->PC - 1;
          interp_local->NOSWITCH = false;
        } else {
          if (current_process->READSTATUS != PRD::READSTATUS::HASTICKET) {
            if ((*channel).READTIME > current_process->TIME) {
              current_process->TIME = (*channel).READTIME;
              if (debug & DBGTIME)
                procTime(current_process, 0.0, "case 64,65,71");
              current_process->READSTATUS = PRD::READSTATUS::HASTICKET;
              current_process->PC = current_process->PC - 1;
              interp_local->NOSWITCH = false;
              (*channel).READTIME = (*channel).READTIME + CHANTIME;
              goto L699;
            } else {
              (*channel).READTIME = (*channel).READTIME + CHANTIME;
            }
          }
          TIMEINC(interp_local, CHANTIME, "cs64");
          (*channel).SEM = (*channel).SEM - 1;
          (*channel).HEAD = interp_local->LINK[interp_local->PNT];
          if ((executor.IR.F == 64) || (executor.IR.F == 65)) {
            current_process->T = current_process->T + 1;
          }
          if (current_process->T > current_process->STACKSIZE) {
            interp_local->PS = InterpLocal::PS::STKCHK;
          } else {
            interp_local->S[current_process->T] =
                interp_local->VALUE[interp_local->PNT];
            if (interp_local->S[current_process->T] == RTAG) {
              interp_local->RS[current_process->T] =
                  interp_local->RVALUE[interp_local->PNT];
            }
          }
          interp_local->LINK[interp_local->PNT] = interp_local->FREE;
          interp_local->FREE = interp_local->PNT;
          current_process->READSTATUS = PRD::READSTATUS::NONE;
          if ((*channel).WAIT != nullptr) {
            if ((*channel).WAIT->PDES ==
                current_process) { // remove CURPR from wait list
              interp_local->PTEMP = (*channel).WAIT;
              (*channel).WAIT = (*channel).WAIT->NEXT;
              if (debug & DBGPROC) {
                fprintf(STDOUT, "remove pid %d from wait list\n",
                        current_process->PID);
              }
              std::free(interp_local->PTEMP); // free ACTPNT
            }
            if ((*channel).WAIT != nullptr) { // set next on wait list
              if ((*channel).SEM == 0) {
                (*channel).WAIT->PDES->STATE = PRD::STATE::BLOCKED;
              } else {
                (*channel).WAIT->PDES->STATE = PRD::STATE::DELAYED;
                (*channel).WAIT->PDES->WAKETIME =
                    interp_local->DATE[(*channel).HEAD];
              }
            }
          }
        }
      L699:
        if (debug & DBGRECV) {
          fprintf(
              STDOUT, "recv(e) pid %d state %s rdstatus %s chan %d WPID %d\n",
              current_process->PID, nameState(current_process->STATE),
              nameRdstatus(current_process->READSTATUS), interp_local->CNUM,
              ((*channel).WAIT != nullptr) ? (*channel).WAIT->PDES->PID : -1);
        }
        break;
      }
      case 66:
      case 92: {
        executor.J =
            interp_local
                ->S[current_process->T - 1]; // stack loc of channel number
        interp_local->CNUM = interp_local->S[executor.J]; // channel number
        executor.H2 = interp_local->SLOCATION[executor.J];
        if (interp_local->NUMTRACE > 0)
          CHKVAR(interp_local, interp_local->S[current_process->T - 1]);
        if (interp_local->CNUM == 0) {
          interp_local->CNUM = FIND(interp_local);
          interp_local->S[interp_local->S[current_process->T - 1]] =
              interp_local->CNUM;
        }
        if (debug & DBGSEND) {
          fprintf(STDOUT, "sendchan %d pid %d var [[T]] %d chan %d\n",
                  executor.IR.F, current_process->PID,
                  interp_local->S[interp_local->S[current_process->T - 1]],
                  interp_local->CNUM);
        }
        executor.H1 = COMMDELAY(interp_local, current_process->PROCESSOR,
                                executor.H2, executor.IR.Y);
        // WITH CHAN[CNUM] DO
        // il->CHAN[il->CNUM]
        channel = &interp_local->CHAN[interp_local->CNUM];
        {
          if (interp_local->FREE == 0)
            interp_local->PS = InterpLocal::PS::BUFCHK;
          else {
            TIMEINC(interp_local, CHANTIME, "cs66");
            executor.K = interp_local->FREE;
            interp_local->DATE[executor.K] =
                current_process->TIME + executor.H1;
            interp_local->FREE = interp_local->LINK[interp_local->FREE];
            interp_local->LINK[executor.K] = 0;
            if ((*channel).HEAD == 0) {
              (*channel).HEAD = executor.K;
            } else {
              interp_local->PNT = (*channel).HEAD;
              executor.I = 0;
              executor.B1 = true;
              while (interp_local->PNT != 0 && executor.B1) {
                if (executor.I != 0) {
                  executor.TGAP = (int)(interp_local->DATE[interp_local->PNT] -
                                        interp_local->DATE[executor.I]);
                } else {
                  executor.TGAP = CHANTIME + 3;
                }
                if (interp_local->DATE[interp_local->PNT] >
                        interp_local->DATE[executor.K] &&
                    executor.TGAP > CHANTIME + 1) {
                  executor.B1 = false;
                } else {
                  executor.I = interp_local->PNT;
                  interp_local->PNT = interp_local->LINK[interp_local->PNT];
                }
              }
              interp_local->LINK[executor.K] = interp_local->PNT;
              if (executor.I == 0) {
                (*channel).HEAD = executor.K;
              } else {
                interp_local->LINK[executor.I] = executor.K;
                if (interp_local->DATE[executor.K] <
                    interp_local->DATE[executor.I] + CHANTIME)
                  interp_local->DATE[executor.K] =
                      interp_local->DATE[executor.I] + CHANTIME;
              }
            }
            if (topology == Symbol::SHAREDSY) {
              current_process->TIME = interp_local->DATE[executor.K];
              if (debug & DBGTIME)
                procTime(current_process, 0.0, "case 66,92");
            }
            if ((*channel).HEAD == executor.K &&
                (*channel).WAIT != nullptr) { // WITH WAIT->PDES
              process = interp_local->CHAN[interp_local->CNUM].WAIT->PDES;
              process->STATE = PRD::STATE::DELAYED;
              process->WAKETIME = interp_local->DATE[executor.K];
            }
            if (executor.IR.F == 66) {
              interp_local->VALUE[executor.K] =
                  interp_local->S[current_process->T];
              if (interp_local->S[current_process->T] == RTAG)
                interp_local->RVALUE[executor.K] =
                    interp_local->RS[current_process->T];
            } else {
              interp_local->VALUE[executor.K] = RTAG;
              interp_local->RVALUE[executor.K] =
                  interp_local->S[current_process->T];
              interp_local->RS[current_process->T] =
                  interp_local->S[current_process->T];
              interp_local->S[current_process->T] = RTAG;
            }
            interp_local->S[current_process->T - 1] =
                interp_local->S[current_process->T];
            if (interp_local->S[current_process->T] == RTAG)
              interp_local->RS[current_process->T - 1] =
                  interp_local->RS[current_process->T];
            current_process->T -= 2;
            (*channel).SEM += 1;
          }
        }
        break;
      }
      case 67:
      case 74: {
        interp_local->NOSWITCH = false;
        TIMEINC(interp_local, CREATETIME, "cs67");
        process = (PROCPNT)calloc(1, sizeof(PROCESSDESCRIPTOR));
        process->PC = current_process->PC + 1;
        process->PID = interp_local->NEXTID++;
        // il->NEXTID += 1;
        if (interp_local->NEXTID > PIDMAX) {
          interp_local->PS = InterpLocal::PS::PROCCHK;
        }
        process->VIRTUALTIME = 0;
        for (int i = 0; i <= LMAX; i++) {
          process->DISPLAY[i] = current_process->DISPLAY[i];
        }
        process->B = current_process->B;
        interp_local->PTEMP = (ACTPNT)calloc(1, sizeof(ACTIVEPROCESS));
        interp_local->PTEMP->PDES = process;
        interp_local->PTEMP->NEXT = interp_local->ACPTAIL->NEXT;
        interp_local->ACPTAIL->NEXT = interp_local->PTEMP;
        interp_local->ACPTAIL = interp_local->PTEMP;
        process->T = FINDFRAME(interp_local, WORKSIZE) - 1;
        if (debug & DBGPROC) {
          fprintf(STDOUT, "opc %d findframe %d length %d, response %d\n",
                  executor.IR.F, process->PID, WORKSIZE, process->T + 1);
        }
        process->STACKSIZE = process->T + WORKSIZE;
        process->BASE = process->T + 1;
        process->TIME = current_process->TIME;
        // proc->NUMCHILDREN = 0;
        // proc->MAXCHILDTIME = 0;
        if (executor.IR.Y != 1) {
          process->WAKETIME =
              current_process->TIME +
              COMMDELAY(interp_local, current_process->PROCESSOR,
                        interp_local->S[current_process->T], 1);
          if (process->WAKETIME > process->TIME) {
            process->STATE = PRD::STATE::DELAYED;
          } else {
            process->STATE = PRD::STATE::READY;
          }
        }
        process->FORLEVEL = current_process->FORLEVEL;
        process->READSTATUS = PRD::READSTATUS::NONE;
        process->FORKCOUNT = 1;
        process->MAXFORKTIME = 0;
        process->JOINSEM = 0;
        process->PARENT = current_process;
        process->PRIORITY = PRD::PRIORITY::LOW;
        process->ALTPROC = -1;
        process->SEQON = true;
        process->GROUPREP = false;
        process->PROCESSOR = interp_local->S[current_process->T];
        if (process->PROCESSOR > highest_processor || process->PROCESSOR < 0) {
          interp_local->PS = InterpLocal::PS::CPUCHK;
        } else {
          if (interp_local->PROCTAB[process->PROCESSOR].STATUS ==
              InterpLocal::PROCTAB::STATUS::NEVERUSED) {
            interp_local->USEDPROCS += 1;
          }
          interp_local->PROCTAB[process->PROCESSOR].STATUS =
              InterpLocal::PROCTAB::STATUS::FULL;
          interp_local->PROCTAB[process->PROCESSOR].NUMPROC += 1;
        }
        current_process->T = current_process->T - 1;
        if (process->T > 0) {
          executor.J = 0;
          while (process->FORLEVEL > executor.J) {
            process->T = process->T + 1;
            executor.H1 = current_process->BASE + executor.J;
            interp_local->S[process->T] = interp_local->S[executor.H1];
            interp_local->SLOCATION[process->T] =
                interp_local->SLOCATION[executor.H1];
            interp_local->RS[process->T] = interp_local->RS[executor.H1];
            executor.J = executor.J + 1;
          }
          if (executor.IR.F == 74) {
            process->FORLEVEL = process->FORLEVEL + 1;
            executor.H1 = interp_local->S[current_process->T - 2];
            executor.H2 = interp_local->S[current_process->T - 1];
            executor.H3 = interp_local->S[current_process->T];
            executor.H4 = interp_local->S[current_process->T - 3];
            process->T = process->T + 1;
            interp_local->S[process->T] = executor.H1;
            interp_local->SLOCATION[process->T] = executor.H4;
            interp_local->RS[process->T] = process->PC;
            process->T = process->T + 1;
            if (executor.H1 + executor.H3 <= executor.H2) {
              interp_local->S[process->T] = executor.H1 + executor.H3 - 1;
            } else {
              interp_local->S[process->T] = executor.H2;
            }
          }
        }
        executor.J = 1;
        while (process->DISPLAY[executor.J] != -1) {
          interp_local->S[process->DISPLAY[executor.J] + 5] += 1;
          if (debug & DBGRELEASE) {
            fprintf(STDOUT, "%d ref ct %d ct=%d\n", executor.IR.F,
                    current_process->PID,
                    interp_local->S[process->DISPLAY[executor.J] + 5]);
          }
          executor.J = executor.J + 1;
        }
        if (executor.IR.Y == 1) {
          process->STATE = PRD::STATE::RUNNING;
          current_process->STATE = PRD::STATE::BLOCKED;
          process->TIME = current_process->TIME;
          process->PRIORITY = PRD::PRIORITY::HIGH;
          process->ALTPROC = process->PROCESSOR;
          process->PROCESSOR = current_process->PROCESSOR;
          interp_local->PROCTAB[process->PROCESSOR].RUNPROC = process;
          interp_local->PROCTAB[process->PROCESSOR].NUMPROC += 1;
        }
        if (executor.IR.F == 74) {
          if (current_process->NUMCHILDREN == 0)
            current_process->MAXCHILDTIME = current_process->TIME;
          current_process->NUMCHILDREN += 1;
        } else {
          current_process->FORKCOUNT += 1;
        }
        if (debug & DBGPROC) {
          fprintf(STDOUT, "opc %d newproc pid %d\n", executor.IR.F,
                  process->PID);
        }
        //                        fprintf(STDOUT, "fork processsor %d alt %d
        //                        status %s\n", proc->PROCESSOR, proc->ALTPROC,
        //                              prcsrStatus(il->PROCTAB[proc->PROCESSOR].STATUS));
        break;
      }
      case 69:
      case 70: {
        if (current_process->FORKCOUNT > 1) {
          current_process->FORKCOUNT -= 1;
          current_process->STATE = PRD::STATE::BLOCKED;
          interp_local->PROCTAB[current_process->PROCESSOR].RUNPROC = nullptr;
          current_process->PC = current_process->PC - 1;
        } else {
          for (int i = LMAX; i > 0; i--) {
            executor.J = current_process->DISPLAY[i];
            if (executor.J != -1) {
              interp_local->S[executor.J + 5] =
                  interp_local->S[executor.J + 5] - 1;
              if (interp_local->S[executor.J + 5] == 0) {
                if (debug & DBGRELEASE) {
                  println(STDOUT, "{} releas1 {} fm={} ln={}", executor.IR.F,
                          current_process->PID, executor.J,
                          interp_local->S[executor.J + 6]);
                }
                RELEASE(interp_local, executor.J,
                        interp_local->S[executor.J + 6]);
              } else {
                if (debug & DBGRELEASE) {
                  println(STDOUT, "{} ref ct {} ct={}", executor.IR.F,
                          current_process->PID,
                          interp_local->S[executor.J + 5]);
                }
              }
            }
          }
          if (!mpi_mode) {
            if (debug & DBGRELEASE) {
              println(STDOUT, "{} releas2 {} fm={} ln={}", executor.IR.F,
                      current_process->PID, current_process->BASE, WORKSIZE);
            }
            RELEASE(interp_local, current_process->BASE, WORKSIZE);
          }
          if (mpi_mode && interp_local->MPIINIT[current_process->PROCESSOR] &&
              !interp_local->MPIFIN[current_process->PROCESSOR]) {
            interp_local->PS = InterpLocal::PS::MPIFINCHK;
          }
          for (int i = 1; i <= OPCHMAX; i++) {
            if (interp_local->CHAN[i].READER == current_process->PID) {
              interp_local->CHAN[i].READER = -1;
            }
          }

          current_process->SEQON = false;
          TIMEINC(interp_local, CREATETIME - 1, "cs69");
          executor.H1 = COMMDELAY(interp_local, current_process->PROCESSOR,
                                  current_process->PARENT->PROCESSOR, 1);
          interp_local->R1 = executor.H1 + current_process->TIME;
          // with CURPR->PARENT
          process = current_process->PARENT;

          switch (executor.IR.F) {
          case 70: {
            process->NUMCHILDREN = process->NUMCHILDREN - 1;
            if (process->MAXCHILDTIME < interp_local->R1) {
              process->MAXCHILDTIME = interp_local->R1;
            }
            if (process->NUMCHILDREN == 0) {
              process->STATE = PRD::STATE::DELAYED;
              process->WAKETIME = process->MAXCHILDTIME;
            }
            break;
          }

          case 69: {
            process->FORKCOUNT = process->FORKCOUNT - 1;
            if (process->MAXFORKTIME < interp_local->R1) {
              process->MAXFORKTIME = interp_local->R1;
            }
            if (process->JOINSEM == -1) {
              if (interp_local->R1 > process->TIME) {
                process->WAKETIME = interp_local->R1;
                process->STATE = PRD::STATE::DELAYED;
              } else {
                process->STATE = PRD::STATE::READY;
              }
            }

            process->JOINSEM = process->JOINSEM + 1;
            if (process->FORKCOUNT == 0) {
              if (process->MAXFORKTIME > process->TIME) {
                process->WAKETIME = process->MAXFORKTIME;
                process->STATE = PRD::STATE::DELAYED;
              } else {
                process->STATE = PRD::STATE::READY;
              }
            }

            break;
          }
          }

          if ((debug & DBGPROC) != 0) {
            println(STDOUT, "opc {} terminated pid {}", executor.IR.F,
                    current_process->PID);
          }
          current_process->STATE = PRD::STATE::TERMINATED;
          process_metadata = &interp_local->PROCTAB[current_process->PROCESSOR];
          process_metadata->NUMPROC -= 1;
          process_metadata->RUNPROC = nullptr;
          if (process_metadata->NUMPROC == 0) {
            process_metadata->STATUS = InterpLocal::PROCTAB::STATUS::EMPTY;
          }
        }
        break;
      }
      case 73: {
        current_process->T = current_process->T + 1;
        if (current_process->T > current_process->STACKSIZE) {
          interp_local->PS = InterpLocal::PS::STKCHK;
        } else {
          executor.H1 = current_process->DISPLAY[executor.IR.X] + executor.IR.Y;
          for (int i = current_process->BASE;
               i <= current_process->BASE + current_process->FORLEVEL - 1;
               i++) {
            if (interp_local->SLOCATION[i] == executor.H1) {
              interp_local->S[current_process->T] = interp_local->S[i];
            }
          }
        }
        break;
      }
      case 75: {
        // [T] = group size (GROUPING)
        // [T-1] = processes
        // [T-2] = initial for index
        // [T-3] = index stk loc
        current_process->FORINDEX = current_process->T - 2;
        if (interp_local->S[current_process->T] <= 0) {
          interp_local->PS = InterpLocal::PS::GRPCHK;
        } else if (interp_local->S[current_process->T - 2] >
                   interp_local->S[current_process->T - 1]) {
          current_process->T = current_process->T - 4;
          current_process->PC = executor.IR.Y;
        } else {
          current_process->PRIORITY = PRD::PRIORITY::HIGH;
        }
        break;
      }
      case 76: {
        // [T-3] is stk frame?
        // [T-2] is counter
        // [T-1] is limit
        // [T] is increment
        // [T-2] += [T]
        // if [T-2] <= [T-1] jmp IR.Y
        // else pop 4 off stack, priority = LOW
        interp_local->S[current_process->T - 2] +=
            interp_local->S[current_process->T];
        if (interp_local->S[current_process->T - 2] <=
            interp_local->S[current_process->T - 1]) {
          current_process->PC = executor.IR.Y;
        } else {
          current_process->T = current_process->T - 4;
          current_process->PRIORITY = PRD::PRIORITY::LOW;
        }
        break;
      }
      case 78: { // wakepar
        if (current_process->GROUPREP) {
          current_process->ALTPROC = -1;
        } else {
          // with CURPR->PARENT
          current_process->PARENT->STATE = PRD::STATE::RUNNING;
          current_process->PARENT->TIME = current_process->TIME;
          if (debug & DBGTIME)
            procTime(current_process->PARENT, 0.0, "case 78");
          interp_local->PROCTAB[current_process->PROCESSOR].NUMPROC -= 1;
          interp_local->PROCTAB[current_process->PROCESSOR].RUNPROC =
              current_process->PARENT;
          current_process->WAKETIME =
              current_process->TIME +
              COMMDELAY(interp_local, current_process->PROCESSOR,
                        current_process->ALTPROC, 1 + executor.IR.Y);
          if (current_process->WAKETIME > current_process->TIME) {
            current_process->STATE = PRD::STATE::DELAYED;
          } else {
            current_process->STATE = PRD::STATE::READY;
          }
          current_process->PROCESSOR = current_process->ALTPROC;
          current_process->ALTPROC = -1;
          current_process->PRIORITY = PRD::PRIORITY::LOW;
        }
        break;
      }
      case 79: {
        current_process->T = current_process->T + 1;
        if (current_process->T > current_process->STACKSIZE) {
          interp_local->PS = InterpLocal::PS::STKCHK;
        } else {
          // il->S[T] = RTAG;
          // il->RS[T] = CONTABLE[el.IR.Y];
          executor.I = -1;
          executor.J = -1;
          executor.B1 = false;
          executor.H1 = MAXINT;
          executor.H2 = -1; // ?? added DE
          do {
            executor.I += 1;
            process_metadata = &interp_local->PROCTAB[executor.I];
            // with PROCTAB[I]
            switch (process_metadata->STATUS) {
            case InterpLocal::PROCTAB::STATUS::EMPTY:
              executor.B1 = true;
              break;
            case InterpLocal::PROCTAB::STATUS::NEVERUSED:
              if (executor.J == -1)
                executor.J = executor.I;
              break;
            case InterpLocal::PROCTAB::STATUS::FULL:
              if (process_metadata->NUMPROC < executor.H1) {
                executor.H2 = executor.I;
                executor.H1 = process_metadata->NUMPROC;
              }
              break;
            case InterpLocal::PROCTAB::STATUS::RESERVED:
              if (process_metadata->NUMPROC + 1 < executor.H1) // +1 fixed dde
              {
                executor.H2 = executor.I;
                executor.H1 = process_metadata->NUMPROC + 1;
              }
              break;
            }
          } while (!executor.B1 && executor.I != highest_processor);
          if (executor.B1) {
            interp_local->S[current_process->T] = executor.I;
          } else if (executor.J > -1) {
            interp_local->S[current_process->T] = executor.J;
            interp_local->USEDPROCS += 1;
          } else
            interp_local->S[current_process->T] = executor.H2;
          // fprintf(STDOUT, "find processsor %d status %s\n", il->S[CURPR->T],
          //         prcsrStatus(il->PROCTAB[il->S[CURPR->T]].STATUS));
          interp_local->PROCTAB[interp_local->S[current_process->T]].STATUS =
              InterpLocal::PROCTAB::STATUS::RESERVED;
        }
        break;
      }
      case 80: {
        interp_local->SLOCATION[interp_local->S[current_process->T]] =
            interp_local->S[current_process->T - 1];
        current_process->T = current_process->T - 1;
        break;
      }
      case 81: {
        ++current_process->T;
        TIMEINC(interp_local, -1, "cs81");
        if (current_process->T > current_process->STACKSIZE) {
          interp_local->PS = InterpLocal::PS::STKCHK;
        } else {
          interp_local->S[current_process->T] =
              interp_local->S[current_process->FORINDEX];
        }
        break;
      }
      case 82: { // initarray initialization
        for (int i = executor.IR.X; i < executor.IR.Y; i++) {
          executor.H1 = interp_local->S[current_process->T];
          interp_local->S[executor.H1] = INITABLE[i].IVAL;
          // fprintf(STDOUT, "initarray stack %d val %d", el.H1,
          // INITABLE[i].IVAL);
          if (INITABLE[i].IVAL == RTAG) {
            interp_local->RS[executor.H1] = INITABLE[i].RVAL;
            // fprintf(STDOUT, " real val %f", INITABLE[i].RVAL);
          }
          // fprintf(STDOUT, "\n");
          interp_local->S[current_process->T] =
              interp_local->S[current_process->T] + 1;
        }
        TIMEINC(interp_local, executor.IR.Y - executor.IR.X / 5, "cs82");
        break;
      }
      case 83: { // zeroarr
        executor.H1 = interp_local->S[current_process->T];
        for (int i = executor.H1; i < executor.H1 + executor.IR.X; i++) {
          if (executor.IR.Y == 2) {
            interp_local->S[i] = RTAG;
            interp_local->RS[i] = 0.0;
          } else {
            interp_local->S[i] = 0;
          }
        }
        current_process->T--;
        TIMEINC(interp_local, executor.IR.X / 10, "cs83");
        break;
      }
      case 84: { // dup
        ++current_process->T;
        TIMEINC(interp_local, -1, "cs84");
        if (current_process->T > current_process->STACKSIZE) {
          interp_local->PS = InterpLocal::PS::STKCHK;
        } else {
          interp_local->S[current_process->T] =
              interp_local->S[current_process->T - 1];
        }
        break;
      }
      case 85: { // join?
        if (current_process->JOINSEM > 0) {
          current_process->JOINSEM -= 1;
        } else {
          current_process->JOINSEM = -1;
          current_process->STATE = PRD::STATE::BLOCKED;
          interp_local->PROCTAB[current_process->PROCESSOR].RUNPROC = nullptr;
        }
        break;
      }
      case 86: {
        if (topology != Symbol::SHAREDSY) {
          TESTVAR(interp_local, interp_local->S[current_process->T]);
        }
        break;
      }
      case 87: { // push real constant from constant table
        current_process->T = current_process->T + 1;
        if (current_process->T > current_process->STACKSIZE) {
          interp_local->PS = InterpLocal::PS::STKCHK;
        } else {
          interp_local->S[current_process->T] = RTAG;
          interp_local->RS[current_process->T] = CONTABLE[executor.IR.Y];
        }
        break;
      }
      case 88: {
        current_process->T = current_process->T - 1;
        if (interp_local->S[current_process->T] != RTAG) {
          interp_local->RS[current_process->T] =
              interp_local->S[current_process->T];
          interp_local->S[current_process->T] = RTAG;
        }
        if (interp_local->S[current_process->T + 1] != RTAG) {
          interp_local->RS[current_process->T + 1] =
              interp_local->S[current_process->T + 1];
          interp_local->S[current_process->T + 1] = RTAG;
        }
        if (interp_local->RS[current_process->T + 1] == 0) {
          interp_local->PS = InterpLocal::PS::DIVCHK;
        } else {
          interp_local->RS[current_process->T] =
              interp_local->RS[current_process->T] /
              interp_local->RS[current_process->T + 1];
          if (interp_local->RS[current_process->T] != 0) {
            executor.RH1 = log(fabs(interp_local->RS[current_process->T])) /
                           executor.log10;
            if (executor.RH1 >= 292.5 || executor.RH1 <= -292.5) {
              interp_local->PS = InterpLocal::PS::OVRCHK;
            }
          }
        }
        break;
      }
      case 89:
        interp_local->NOSWITCH = true;
        break;
      case 90:
        interp_local->NOSWITCH = false;
        break;
      case 91: {
        executor.H1 = interp_local->S[current_process->T - 1];
        if (topology != Symbol::SHAREDSY) {
          if (executor.IR.Y == 0) {
            TESTVAR(interp_local, executor.H1);
          } else if (interp_local->CONGESTION) {
            executor.H1 = COMMDELAY(interp_local, current_process->PROCESSOR,
                                    interp_local->SLOCATION[executor.H1], 1);
          }
        }
        if (interp_local->NUMTRACE > 0) {
          CHKVAR(interp_local, executor.H1);
        }
        interp_local->RS[executor.H1] = interp_local->S[current_process->T];
        interp_local->S[executor.H1] = RTAG;
        interp_local->RS[current_process->T - 1] =
            interp_local->S[current_process->T];
        interp_local->S[current_process->T - 1] = RTAG;
        current_process->T = current_process->T - 1;
        break;
      }
      case 93: { // duration
        executor.H1 = interp_local->S[current_process->T];
        current_process->T -= 1;
        if (current_process->TIME + executor.H1 > interp_local->CLOCK) {
          current_process->STATE = PRD::STATE::DELAYED;
          current_process->WAKETIME = current_process->TIME + executor.H1;
          interp_local->PROCTAB[current_process->PROCESSOR].RUNPROC = nullptr;
        } else {
          if (debug & DBGTIME)
            procTime(current_process, (float)executor.H1, "case 93");
          current_process->TIME += executor.H1;
        }
        break;
      }
      case 94:
      case 95:
      case 96: {
        switch (executor.IR.F) {
        case 94:
          executor.H1 = interp_local->S[current_process->T];
          current_process->T = current_process->T - 1;
          break;
        case 95:
          executor.H1 = current_process->DISPLAY[executor.IR.X] + executor.IR.Y;
          break;
        case 96:
          executor.H1 =
              interp_local
                  ->S[current_process->DISPLAY[executor.IR.X] + executor.IR.Y];
          break;
        }
        interp_local->CNUM = interp_local->S[executor.H1];
        if (interp_local->NUMTRACE > 0) {
          CHKVAR(interp_local, executor.H1);
        }
        if (interp_local->CNUM == 0) {
          interp_local->CNUM = FIND(interp_local);
          interp_local->S[executor.H1] = interp_local->CNUM;
        }
        if (!interp_local->CHAN[interp_local->CNUM].MOVED &&
            interp_local->CHAN[interp_local->CNUM].READER < 0) {
          interp_local->SLOCATION[executor.H1] = current_process->PROCESSOR;
          interp_local->CHAN[interp_local->CNUM].READER = current_process->PID;
        }
        if (topology != Symbol::SHAREDSY) {
          TESTVAR(interp_local, executor.H1);
        }
        current_process->T = current_process->T + 1;
        if (current_process->T > current_process->STACKSIZE) {
          interp_local->PS = InterpLocal::PS::STKCHK;
        } else {
          interp_local->S[current_process->T] = 1;
          if (interp_local->CNUM == 0 ||
              interp_local->CHAN[interp_local->CNUM].SEM == 0 ||
              interp_local->DATE[interp_local->CHAN[interp_local->CNUM].HEAD] >
                  current_process->TIME) {
            interp_local->S[current_process->T] = 0;
          }
        }
        break;
      }
      case 97: {
        executor.H1 = interp_local->S[current_process->T];
        executor.H2 = executor.IR.Y;
        for (executor.I = executor.H1;
             executor.I <= executor.H1 + executor.H2 - 1; executor.I++) {
          interp_local->SLOCATION[executor.I] =
              interp_local->S[current_process->T - 1];
        }
        current_process->T = current_process->T - 1;
        break;
      }
      case 98:
      case 113: { // send copy msg
        executor.H2 = interp_local->S[current_process->T];
        executor.H1 = FINDFRAME(interp_local, executor.IR.Y);
        if (debug & DBGSEND) {
          fprintf(STDOUT, "send findframe %d pid %d from %d to %d len %d\n",
                  executor.IR.F, current_process->PID, executor.H2, executor.H1,
                  executor.IR.Y);
        }
        if (executor.H2 <= 0 || executor.H2 >= STMAX) {
          interp_local->PS = InterpLocal::PS::REFCHK;
        } else if (executor.H1 < 0) {
          interp_local->PS = InterpLocal::PS::STORCHK;
        } else {
          executor.H3 = executor.H1 + executor.IR.Y;
          if (topology != Symbol::SHAREDSY) {
            TESTVAR(interp_local, executor.H2);
          }
          if (debug & DBGSEND) {
            fprintf(STDOUT, "copymsg");
          }
          while (executor.H1 < executor.H3) {
            if (interp_local->NUMTRACE > 0) {
              CHKVAR(interp_local, executor.H2);
            }
            interp_local->S[executor.H1] = interp_local->S[executor.H2];
            interp_local->SLOCATION[executor.H1] = -1;
            if (interp_local->S[executor.H2] == RTAG) {
              interp_local->RS[executor.H1] = interp_local->RS[executor.H2];
            }
            if (debug & DBGSEND) {
              if (interp_local->S[executor.H2] != RTAG)
                fprintf(STDOUT, ":%d", interp_local->S[executor.H2]);
              else
                fprintf(STDOUT, ":%.1f", interp_local->RS[executor.H2]);
            }
            executor.H1 = executor.H1 + 1;
            executor.H2 = executor.H2 + 1;
          }
          if (debug & DBGSEND) {
            fprintf(STDOUT, "\n");
          }
          executor.H2 = interp_local->S[current_process->T];
          interp_local->S[current_process->T] = executor.H3 - executor.IR.Y;
          if (executor.IR.F == 98 &&
              interp_local->SLOCATION[executor.H2] == -1) {
            if (debug & DBGSEND) {
              fprintf(STDOUT, "send(rel) %d pid % d from %d len %d\n",
                      executor.IR.F, current_process->PID, executor.H2,
                      executor.IR.Y);
            }
            if (debug & DBGSEND) {
              fprintf(STDOUT, "%d send release fm=%d ln=%d\n", executor.IR.F,
                      executor.H2, executor.IR.Y);
            }
            RELEASE(interp_local, executor.H2, executor.IR.Y);
          }
          TIMEINC(interp_local, executor.IR.Y / 5, "cs98");
          if (debug & DBGSEND) {
            fprintf(STDOUT, "send(x) %d pid %d var [T] %d chan [T-1] %d\n",
                    executor.IR.F, current_process->PID,
                    interp_local->S[current_process->T],
                    interp_local->S[interp_local->S[current_process->T - 1]]);
          }
        }
        break;
      }
      case 101: { // lock
        if (topology != Symbol::SHAREDSY) {
          interp_local->PS = InterpLocal::PS::LOCKCHK;
        }
        if (interp_local->NUMTRACE > 0) {
          CHKVAR(interp_local, interp_local->S[current_process->T]);
        }
        if (interp_local->S[interp_local->S[current_process->T]] == 0) {
          interp_local->S[interp_local->S[current_process->T]] = 1;
          current_process->T = current_process->T - 1;
        } else {
          current_process->STATE = PRD::STATE::SPINNING;
          current_process->PC = current_process->PC - 1;
        }
        break;
      }
      case 102: { // unlock
        if (topology != Symbol::SHAREDSY) {
          interp_local->PS = InterpLocal::PS::LOCKCHK;
        }
        if (interp_local->NUMTRACE > 0) {
          CHKVAR(interp_local, interp_local->S[current_process->T]);
        }
        interp_local->S[interp_local->S[current_process->T]] = 0;
        current_process->T = current_process->T - 1;
        break;
      }
      case 104: {
        if (interp_local->S[current_process->T - 1] <
            interp_local->S[current_process->T]) {
          interp_local->S[current_process->T - 1] += 1;
          current_process->PC = executor.IR.X;
          if (executor.IR.Y == 1) {
            current_process->ALTPROC = current_process->PARENT->PROCESSOR;
            current_process->GROUPREP = true;
          }
        }
        break;
      }
      case 105: {
        if (interp_local->S[current_process->B + 5] == 1) {
          if (debug & DBGRELEASE) {
            println("{} release {} fm={} ln={}", executor.IR.F,
                    current_process->PID, current_process->B,
                    interp_local->S[current_process->B + 6]);
          }
          RELEASE(interp_local, current_process->B,
                  interp_local->S[current_process->B + 6]);
        } else {
          interp_local->S[current_process->B + 5] =
              interp_local->S[current_process->B + 5] - 1;
          if (debug & DBGRELEASE) {
            fprintf(STDOUT, "%d fm ref ct %d ct=%d\n", executor.IR.F,
                    current_process->PID,
                    interp_local->S[current_process->B + 5]);
          }
        }
        executor.H1 = TAB[interp_local->S[current_process->B + 4]].LEV;
        current_process->DISPLAY[executor.H1 + 1] = -1;
        current_process->T = interp_local->S[current_process->B + 7];
        current_process->B = interp_local->S[current_process->B + 3];
        if (current_process->T >= current_process->BASE - 1 &&
            current_process->T < current_process->BASE + WORKSIZE) {
          current_process->STACKSIZE = current_process->BASE + WORKSIZE - 1;
        } else {
          current_process->STACKSIZE =
              current_process->B + interp_local->S[current_process->B + 6] - 1;
        }
        break;
      }
      case 106: {
        TIMEINC(interp_local, -1, "c106");
        current_process->SEQON = false;
        break;
      }
      case 107: {
        TIMEINC(interp_local, -1, "c107");
        current_process->SEQON = true;
        break;
      }
      case 108: { // increment
        executor.H1 = interp_local->S[current_process->T];
        if (executor.H1 <= 0 || executor.H1 >= STMAX) {
          interp_local->PS = InterpLocal::PS::REFCHK;
        } else {
          if (topology != Symbol::SHAREDSY) {
            TESTVAR(interp_local, executor.H1);
          }
          if (interp_local->NUMTRACE > 0) {
            CHKVAR(interp_local, executor.H1);
          }
          switch (executor.IR.Y) {
          case 0:
            if (interp_local->S[executor.H1] == RTAG) {
              interp_local->RS[executor.H1] =
                  interp_local->RS[executor.H1] + executor.IR.X;
              interp_local->RS[current_process->T] =
                  interp_local->RS[executor.H1];
              interp_local->S[current_process->T] = RTAG;
            } else {
              interp_local->S[executor.H1] =
                  interp_local->S[executor.H1] + executor.IR.X;
              interp_local->S[current_process->T] =
                  interp_local->S[executor.H1];
              if (abs(interp_local->S[current_process->T]) > NMAX) {
                interp_local->PS = InterpLocal::PS::INTCHK;
              }
            }
            break;
          case 1:
            if (interp_local->S[executor.H1] == RTAG) {
              interp_local->RS[current_process->T] =
                  interp_local->RS[executor.H1];
              interp_local->S[current_process->T] = RTAG;
              interp_local->RS[executor.H1] =
                  interp_local->RS[executor.H1] + executor.IR.X;
            } else {
              interp_local->S[current_process->T] =
                  interp_local->S[executor.H1];
              interp_local->S[executor.H1] =
                  interp_local->S[executor.H1] + executor.IR.X;
              if (abs(interp_local->S[executor.H1]) > NMAX) {
                interp_local->PS = InterpLocal::PS::INTCHK;
              }
            }
            break;
          }
          if (interp_local->STARTMEM[executor.H1] <= 0) {
            interp_local->PS = InterpLocal::PS::REFCHK;
          }
        }
        break;
      }
      case 109: { // decrement
        executor.H1 = interp_local->S[current_process->T];
        if (executor.H1 <= 0 || executor.H1 >= STMAX) {
          interp_local->PS = InterpLocal::PS::REFCHK;
        } else {
          if (topology != Symbol::SHAREDSY) {
            TESTVAR(interp_local, executor.H1);
          }
          if (interp_local->NUMTRACE > 0) {
            CHKVAR(interp_local, executor.H1);
          }
          switch (executor.IR.Y) {
          case 0:
            if (interp_local->S[executor.H1] == RTAG) {
              interp_local->RS[executor.H1] =
                  interp_local->RS[executor.H1] - executor.IR.X;
              interp_local->RS[current_process->T] =
                  interp_local->RS[executor.H1];
              interp_local->S[current_process->T] = RTAG;
            } else {
              interp_local->S[executor.H1] =
                  interp_local->S[executor.H1] - executor.IR.X;
              interp_local->S[current_process->T] =
                  interp_local->S[executor.H1];
              if (abs(interp_local->S[current_process->T]) > NMAX) {
                interp_local->PS = InterpLocal::PS::INTCHK;
              }
            }
            break;
          case 1:
            if (interp_local->S[executor.H1] == RTAG) {
              interp_local->RS[current_process->T] =
                  interp_local->RS[executor.H1];
              interp_local->S[current_process->T] = RTAG;
              interp_local->RS[executor.H1] =
                  interp_local->RS[executor.H1] - executor.IR.X;
            } else {
              interp_local->S[current_process->T] =
                  interp_local->S[executor.H1];
              interp_local->S[executor.H1] =
                  interp_local->S[executor.H1] - executor.IR.X;
              if (abs(interp_local->S[executor.H1]) > NMAX) {
                interp_local->PS = InterpLocal::PS::INTCHK;
              }
            }
            break;
          }
          if (interp_local->STARTMEM[executor.H1] <= 0) {
            interp_local->PS = InterpLocal::PS::REFCHK;
          }
        }
        break;
      }
      case 110: {
        if (executor.IR.Y == 0) {
          executor.H1 = interp_local->S[current_process->T];
          executor.H2 = interp_local->S[current_process->T - 1];
        } else {
          executor.H1 = interp_local->S[current_process->T - 1];
          executor.H2 = interp_local->S[current_process->T];
        }
        current_process->T = current_process->T - 1;
        interp_local->S[current_process->T] =
            executor.H2 + executor.IR.X * executor.H1;
        break;
      }
      case 111: // pop
        current_process->T = current_process->T - 1;
        break;
      case 112: { // release if SLOC[T] == -1 no processor
        if (interp_local->SLOCATION[interp_local->S[current_process->T]] ==
            -1) {
          if (debug & DBGRELEASE) {
            fprintf(STDOUT, "%d release %d fm=%d ln=%d\n", executor.IR.F,
                    current_process->PID, interp_local->S[current_process->T],
                    executor.IR.Y);
          }
          RELEASE(interp_local, interp_local->S[current_process->T],
                  executor.IR.Y);
        }
        break;
      }
      case 114: {
        if (interp_local->S[current_process->T] != 0) {
          interp_local->S[current_process->T] = 1;
        }
      }
      case 115: {
        executor.H1 = interp_local->S[current_process->T - 1];
        interp_local->SLOCATION[executor.H1] =
            interp_local->S[current_process->T];
        interp_local->CNUM = interp_local->S[executor.H1];
        if (interp_local->CNUM == 0) {
          interp_local->CNUM = FIND(interp_local);
          interp_local->S[executor.H1] = interp_local->CNUM;
        }
        interp_local->CHAN[interp_local->CNUM].MOVED = true;
        current_process->T = current_process->T - 2;
        break;
      }

        /* instruction cases go here */
      default:
         println("Missing Code {}", executor.IR.F);
        break;
      }
    }
  } while (interp_local->PS == InterpLocal::PS::RUN);
  // label_999:
  print('\n');
} // EXECUTE
} // namespace Cstar
