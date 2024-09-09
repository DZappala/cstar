//
// Created by Dan Evans on 1/29/24.
//
#include "cs_compile.h"
#include "cs_exec.h"
#include "cs_global.h"
#include "cs_interpret.h"
#include <cerrno>
#include <cmath>
#include <cstring>
#include <iomanip>
#include <iostream>
namespace Cstar {
#define MAXINT 32767
// enum PS {RUN, BREAK, FIN, DEAD, STKCHK};
struct ACTIVEPROCESS;
typedef struct PROCESSDESCRIPTOR PRD;
typedef struct PROCESSDESCRIPTOR *PROCPNT;
typedef struct ACTIVEPROCESS *ACTPNT;
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
void CHKVAR(InterpLocal *il, int STKLOC) {
  // check if the variable at the passed stack location is being traced
  int I, J;
  for (I = 1; I <= VARMAX; I++) {
    if (STKLOC == il->TRCTAB[I].MEMLOC) {
      std::cout << "Reference to Trace Variable " << il->TRCTAB[I].NAME
                << std::endl;
      // CURPR* curpr = new CURPR;
      il->BLINE = GETLNUM(il->CURPR->PC - 1);
      fprintf(STDOUT, "Line Number%5d  In Function ", il->BLINE);
      J = il->CURPR->B;
      il->INX = il->S[il->CURPR->B + 4];
      while (TAB[il->INX].NAME[0] == '*') {
        J = il->S[J + 3];
        il->INX = il->S[J + 4];
      }
      if (J == 0) {
        std::cout << PROGNAME << std::endl;
      } else {
        std::cout << TAB[il->INX].NAME << std::endl;
      }
      //                std::cout << "Process Number" << il->CURPR->PID <<
      //                std::endl; std::cout << std::endl;
      fprintf(STDOUT, "Process Number%4d\n\n", il->CURPR->PID);
      il->STEPROC = il->CURPR;
      il->PS = InterpLocal::PS::BREAK;
    }
  }
}

void MOVE(CommdelayLocal *cl, int lSRC, int DES, int STEP) {
  int DIREC, I;
  if (lSRC < DES) {
    DIREC = 1;
  } else {
    DIREC = -1;
  }
  for (I = 1; I <= abs(lSRC - DES); I++) {
    cl->PATHLEN = cl->PATHLEN + 1;
    cl->PATH[cl->PATHLEN] = cl->PATH[cl->PATHLEN - 1] + DIREC * STEP;
  }
}

void RMOVE(CommdelayLocal *cl, int lSRC, int DES, int STEP) {
  int DIREC, I, DIST;
  DIST = abs(lSRC - DES);
  if (lSRC < DES) {
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
    lSRC = lSRC + DIREC;
    if (lSRC == TOPDIM) {
      lSRC = 0;
      cl->PATH[cl->PATHLEN] = cl->PATH[cl->PATHLEN - 1] - STEP * (TOPDIM - 1);
    } else if (lSRC == -1) {
      lSRC = TOPDIM - 1;
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
              // fprintf(STDOUT, "free commdelay1\n");
              std::free(CURPNT);
            }
          }
        }
        if (!DONE) {
          PREV = CURPNT;
          CURPNT = CURPNT->NEXT;
          if (PREV->LAST < cl->PASTINTERVAL) {
            // fprintf(STDOUT, "free commdelay2\n");
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
  if ((!il->CONGESTION) || (TOPOLOGY == SHAREDSY)) {
    switch (TOPOLOGY) {
    case SHAREDSY:
      rtn = 0;
      break;
    case FULLCONNSY:
    case CLUSTERSY:
      cl.DIST = 1;
      break;
    case HYPERCUBESY:
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
    case LINESY:
      cl.DIST = abs(SOURCE - DEST);
      break;
    case MESH2SY:
      cl.T1 = TOPDIM;
      cl.T2 = std::abs(SOURCE / cl.T1 - DEST / cl.T1);
      cl.T3 = std::abs(SOURCE % cl.T1 - DEST % cl.T1);
      cl.DIST = (cl.T2 + cl.T3);
      break;
    case MESH3SY:
      cl.T2 = TOPDIM;
      cl.T1 = TOPDIM * cl.T2;
      cl.T3 = SOURCE % cl.T1;
      cl.T4 = DEST % cl.T1;
      cl.DIST = abs(cl.T3 / cl.T2 - cl.T4 / cl.T2) +
                abs(cl.T3 % cl.T2 - cl.T4 % cl.T2) +
                abs(SOURCE / cl.T1 - DEST / cl.T1);
      break;
    case RINGSY:
      cl.T1 = abs(SOURCE - DEST);
      cl.T2 = TOPDIM - cl.T1;
      if (cl.T1 < cl.T2) {
        cl.DIST = cl.T1;
      } else {
        cl.DIST = cl.T2;
      }
      break;
    case TORUSSY:
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
    if (TOPOLOGY != SHAREDSY) {
      rtn = (int)std::round((cl.DIST + (cl.NUMPACK - 1) / 2.0) * il->TOPDELAY);
    }
  } else {
    cl.PATH[0] = SOURCE;
    cl.PATHLEN = 0;
    switch (TOPOLOGY) {
    case FULLCONNSY:
    case CLUSTERSY:
      cl.PATH[1] = DEST;
      cl.PATHLEN = 1;
      break;
    case HYPERCUBESY:
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
    case LINESY:
      MOVE(&cl, SOURCE, DEST, 1);
      break;
    case MESH2SY:
      MOVE(&cl, SOURCE % TOPDIM, DEST % TOPDIM, 1);
      MOVE(&cl, SOURCE / TOPDIM, DEST / TOPDIM, TOPDIM);
      break;
    case MESH3SY:
      cl.T1 = TOPDIM * TOPDIM;
      MOVE(&cl, SOURCE % TOPDIM, DEST % TOPDIM, 1);
      MOVE(&cl, SOURCE / TOPDIM % TOPDIM, DEST / TOPDIM % TOPDIM, TOPDIM);
      MOVE(&cl, SOURCE / cl.T1, DEST / cl.T1, TOPDIM * TOPDIM);
      break;
    case RINGSY:
      RMOVE(&cl, SOURCE, DEST, 1);
      break;
    case TORUSSY:
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
  if ((SOURCE > HIGHESTPROCESSOR) || (DEST > HIGHESTPROCESSOR)) {
    il->PS = InterpLocal::PS::CPUCHK;
  }
  //        if (rtn != 0)
  //            fprintf(STDOUT, "COMMDELAY source %d dest %d len %d returns
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
    fprintf(STDOUT, "%.1f = %.1f + %.1f %s\n", prc->TIME + incr, prc->TIME,
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
      fprintf(STDOUT, "%.1f = %.1f + %.1f SEQ\n", il->SEQTIME + STEP,
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
      //                    fprintf(STDOUT, "ptab(%d) != proc(%d) virtime %.1f
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
        //                    fprintf(STDOUT, "switch ptab runproc\n");
      } else {
        if (COUNT > PMAX && !DEADLOCK) {
          fprintf(STDOUT, "loop in SLICE\n");
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
        fprintf(STDOUT, "slice terminated/freed process %d\n", proc->PID);
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
  //     fprintf(STDOUT, "dispatch %d time %.1f seqtime %.1f\n", il->CURPR->PID,
  //     il->CURPR->TIME, il->SEQTIME);
  if (DEADLOCK) {
    std::cout << "    DEADLOCK:  All Processes are Blocked" << std::endl;
    std::cout << "For Further Information Type STATUS Command" << std::endl;
    fprintf(STDOUT, "CLOCK %.1f PS %d, DONE %d, COUNT %d, DEADLOCK %d\n",
            il->CLOCK, il->PS, DONE, COUNT, DEADLOCK);
    il->PS = InterpLocal::PS::DEAD;
    dumpDeadlock(il);
  }
}

void EXECUTE(InterpLocal *il) {
  //        ProcessState PS;
  //        PROCPNT CURPR;
  //        int LC;
  //        double USAGE;
  //        char CH;
  //        int T;
  //        std::vector<int> STACK(STACKSIZE);
  PROCPNT CURPR = il->CURPR;
  PROCPNT proc;
  struct InterpLocal::PROCTAB *prtb;
  struct InterpLocal::Channel *chan;
  // --- debugging ---

  //        int lct = 0;
  //        int mon = -1, val = 82;
  //        int braddr = 0;
  //        bool watch = false, breakat = false;
  // ---------
  ExLocal el = {0, 0,   0,         0,     0,       0,   0,
                0, 0.0, {0, 0, 0}, false, nullptr, 0.0, {0}};
  // memset(&el, 0, sizeof(ExLocal));
  // el.il = il;
  el.log10 = log(10);
  // dumpPDES(CURPR);
  do {
    if (il->PS != InterpLocal::PS::BREAK) {
      if ((!il->NOSWITCH && CURPR->TIME >= il->CLOCK) ||
          CURPR->STATE != PRD::STATE::RUNNING) {
        //                    fprintf(STDOUT, "state %s noswitch %d time %.1f
        //                    clock %.1f",
        //                            nameState(CURPR->STATE), il->NOSWITCH,
        //                            CURPR->TIME, il->CLOCK);
        SLICE(il);
        CURPR = il->CURPR;
        //                    fprintf(STDOUT, " new process %d processor %d\n",
        //                    CURPR->PID, CURPR->PROCESSOR);
      }
    }

    if (il->PS == InterpLocal::PS::FIN || il->PS == InterpLocal::PS::DEAD) {
      // goto label_999;
      continue;
    }

    el.IR = CODE[CURPR->PC];
    CURPR->PC++;
    TIMEINC(il, 1, "exe1");

    if (il->NUMBRK > 0 && il->MAXSTEPS < 0 && !il->RESTART) {
      for (int i = 1; i <= BRKMAX; i++) {
        if (il->BRKTAB[i] == CURPR->PC - 1) {
          il->PS = InterpLocal::PS::BREAK;
        }
      }
    }

    il->RESTART = false;

    if (il->MAXSTEPS > 0 && CURPR == il->STEPROC) {
      LC = CURPR->PC - 1;
      if (LC < il->STARTLOC || LC > il->ENDLOC) {
        il->MAXSTEPS--;
        il->STARTLOC = LC;
        il->ENDLOC = LOCATION[GETLNUM(LC) + 1] - 1;
        if (il->MAXSTEPS == 0) {
          il->PS = InterpLocal::PS::BREAK;
        }
      }
    }

    if (il->PROFILEON) {
      if (il->CLOCK >= il->PROTIME) {
        if (il->PROLINECNT == 0) {
          std::cout << "TIME: " << std::round(il->PROTIME - il->PROSTEP)
                    << std::endl;
          if (il->FIRSTPROC < 10) {
            std::cout << il->FIRSTPROC;
          } else {
            std::cout << " ";
          }
          for (int i = 1; i <= (4 - (il->FIRSTPROC % 5)); i++) {
            if (il->FIRSTPROC + i < 10 && il->FIRSTPROC + i <= il->LASTPROC) {
              std::cout << std::setw(2) << il->FIRSTPROC + i;
            } else {
              std::cout << "  ";
            }
          }
          for (int i = il->FIRSTPROC + 1; i <= il->LASTPROC; i++) {
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
        for (int i = il->FIRSTPROC; i <= il->LASTPROC; i++) {
          il->USAGE = (float)(il->PROCTAB[i].PROTIME / il->PROSTEP);
          if (il->PROCTAB[i].STATUS ==
              InterpLocal::PROCTAB::STATUS::NEVERUSED) {
            CH = ' ';
          } else if (il->USAGE < 0.25) {
            CH = '.';
          } else if (il->USAGE < 0.5) {
            CH = '-';
          } else if (il->USAGE < 0.75) {
            CH = '+';
          } else {
            CH = '*';
          }
          std::cout << CH << " ";
          il->PROCTAB[i].PROTIME = 0;
        }
        std::cout << std::endl;
        il->PROLINECNT = (il->PROLINECNT + 1) % 20;
        il->PROTIME += il->PROSTEP;
      }
    }

    if (il->ALARMON && il->ALARMENABLED) {
      if (il->CLOCK >= il->ALARMTIME) {
        std::cout << std::endl;
        std::cout << "Alarm Went Off at Time " << std::round(il->ALARMTIME)
                  << std::endl;
        if (il->CLOCK > il->ALARMTIME + 2 * TIMESTEP) {
          std::cout << "Current Time is " << std::round(il->CLOCK) << std::endl;
        }
        il->PS = InterpLocal::PS::BREAK;
        il->ALARMENABLED = false;
      }
    }

    if (il->PS == InterpLocal::PS::BREAK) {
      CURPR->PC--;
      TIMEINC(il, -1, "exe2");
      il->BLINE = GETLNUM(CURPR->PC);
      //                std::cout << std::endl;
      //                std::cout << "Break At " << il->BLINE << std::endl;
      fprintf(STDOUT, "\nBreak At %d", il->BLINE);
      el.H1 = CURPR->B;
      il->INX = il->S[CURPR->B + 4];
      while (TAB[il->INX].NAME[0] == '*') {
        el.H1 = il->S[el.H1 + 3];
        il->INX = il->S[el.H1 + 4];
      }
      if (el.H1 == 0) {
        std::cout << "  In Function " << PROGNAME << std::endl;
      } else {
        std::cout << "  In Function " << TAB[il->INX].NAME << std::endl;
      }
      std::cout << "Process Number  " << CURPR->PID << std::endl;
      il->STEPROC = CURPR;
      if (il->MAXSTEPS == 0) {
        double R1 = CURPR->TIME - il->STEPTIME;
        if (R1 > 0) {
          el.H2 = (int)std::round((CURPR->VIRTUALTIME - il->VIRSTEPTIME) / R1 *
                                  100.0);
        } else {
          el.H2 = 0;
        }
        if (el.H2 > 100) {
          el.H2 = 100;
        }
        std::cout << "Step Time is " << std::round(R1) << ".  Process running "
                  << el.H2 << " percent." << std::endl;
      }
      std::cout << std::endl;
    } else {
      //                if (watch)
      //                {
      //                    if (il->S[mon] != val)
      //                    {
      //                        fprintf(STDOUT, "----------modified %d at
      //                        %d----------\n", il->S[mon], CURPR->PC); watch =
      //                        false;
      //                    }
      //                }
      //                else
      //                {
      //                    if (mon >= 0 && il->S[mon] == val)
      //                    {
      //                        fprintf(STDOUT, "----------monitor set at
      //                        %d----------\n", CURPR->PC); watch = true;
      //                    }
      //                }
      //                if (breakat)
      //                {
      //                    if (CURPR->PC == braddr)
      //                    {
      //                        fprintf(STDOUT, "----------break at at
      //                        %d----------\n", CURPR->PC);
      //                    }
      //                }
      if (debug & DBGINST) {
        //                    if (CURPR->PROCESSOR == 0 && CURPR->PC < 81 &&
        //                    CURPR->PC > 18)
        //                    {
        fprintf(STDOUT, "proc %3d stk %5d [%5d] ", CURPR->PROCESSOR, CURPR->T,
                il->S[CURPR->T]);
        dumpInst(CURPR->PC - 1);
        //                    }
      }
      //                if (CURPR->PID == 1)
      //                    dumpInst(CURPR->PC - 1);
      //                if (++lct > 2000000)
      //                    throw std::exception();
      switch (el.IR.F) {
      case 0: // push DISPLAY[op1]+op2 (stack frame location of variable op2)
        CURPR->T++;
        if (CURPR->T > CURPR->STACKSIZE) {
          il->PS = InterpLocal::PS::STKCHK;
        } else {
          il->S[CURPR->T] = CURPR->DISPLAY[el.IR.X] + el.IR.Y;
          //                            fprintf(STDOUT, "%3d: %d %d,%d %s %s\n",
          //                            CURPR->PC - 1,
          //                                el.IR.F, el.IR.X, el.IR.Y,
          //                                opcodes[el.IR.F], lookupSym(el.IR.X,
          //                                el.IR.Y));
        }
        break;
      case 1: {
        CURPR->T++;
        if (CURPR->T > CURPR->STACKSIZE) {
          il->PS = InterpLocal::PS::STKCHK;
        } else {
          el.H1 = CURPR->DISPLAY[el.IR.X] + el.IR.Y;
          if (TOPOLOGY != SHAREDSY) {
            TESTVAR(il, el.H1);
          }
          if (il->NUMTRACE > 0) {
            CHKVAR(il, el.H1);
          }
          il->S[CURPR->T] = il->S[el.H1];
          if (il->S[CURPR->T] == RTAG) {
            il->RS[CURPR->T] = il->RS[el.H1];
          }
          //                            fprintf(STDOUT, "%3d: %d %d,%d %s %s\n",
          //                            CURPR->PC - 1,
          //                                    el.IR.F, el.IR.X, el.IR.Y,
          //                                    opcodes[el.IR.F],
          //                                    lookupSym(el.IR.X, el.IR.Y));
        }
        break;
      }
      case 2: {
        CURPR->T++;
        if (CURPR->T > CURPR->STACKSIZE) {
          il->PS = InterpLocal::PS::STKCHK;
        } else {
          el.H1 = il->S[CURPR->DISPLAY[el.IR.X] + el.IR.Y];
          if (TOPOLOGY != SHAREDSY) {
            TESTVAR(il, el.H1);
          }
          if (il->NUMTRACE > 0) {
            CHKVAR(il, el.H1);
          }
          il->S[CURPR->T] = il->S[el.H1];
          if (il->S[CURPR->T] == RTAG) {
            il->RS[CURPR->T] = il->RS[el.H1];
          }
        }
        break;
      }
      case 3: {
        el.H1 = el.IR.Y;
        el.H2 = el.IR.X;
        el.H3 = CURPR->B;
        do {
          CURPR->DISPLAY[el.H1] = el.H3;
          el.H1--;
          el.H3 = il->S[el.H3 + 2];
        } while (el.H1 != el.H2);
        break;
      }
      case 4: {
        CURPR->NUMCHILDREN = 0;
        CURPR->SEQON = false;
        break;
      }
      case 5: {
        if (CURPR->NUMCHILDREN > 0) {
          CURPR->STATE = PRD::STATE::BLOCKED;
          il->PROCTAB[CURPR->PROCESSOR].RUNPROC = nullptr;
        }
        CURPR->SEQON = true;
        break;
      }
      case 6: { // noop
        break;
      }
      case 7: {
        CURPR->PC = el.IR.Y;
        CURPR->SEQON = true;
        break;
      }
      case 8: {
        switch (el.IR.Y) {
        case 10: {
          il->R1 = il->RS[CURPR->T];
          if (il->R1 >= 0 || il->R1 == (int)(il->R1)) {
            il->S[CURPR->T] = (int)(il->R1);
          } else {
            il->S[CURPR->T] = (int)(il->R1) - 1; // ?? should go toward 0
          }
          if (abs(il->S[CURPR->T]) > NMAX) {
            il->PS = InterpLocal::PS::INTCHK;
          }
          break;
        }
        case 19: {
          CURPR->T++;
          if (CURPR->T > CURPR->STACKSIZE) {
            il->PS = InterpLocal::PS::STKCHK;
          } else {
            il->S[CURPR->T] = CURPR->PROCESSOR;
          }
          break;
        }
        case 20: {
          CURPR->T++;
          if (CURPR->T > CURPR->STACKSIZE) {
            il->PS = InterpLocal::PS::STKCHK;
          } else {
            il->S[CURPR->T] = RTAG;
            il->RS[CURPR->T] = CURPR->TIME;
          }
          break;
        }
        case 21: {
          CURPR->T++;
          if (CURPR->T > CURPR->STACKSIZE) {
            il->PS = InterpLocal::PS::STKCHK;
          } else {
            il->S[CURPR->T] = RTAG;
            il->RS[CURPR->T] = il->SEQTIME;
          }
          break;
        }
        case 22: {
          CURPR->T++;
          if (CURPR->T > CURPR->STACKSIZE) {
            il->PS = InterpLocal::PS::STKCHK;
          } else {
            il->S[CURPR->T] = CURPR->PID;
          }
          break;
        }
        case 23: {
          CURPR->T++;
          if (CURPR->T > CURPR->STACKSIZE) {
            il->PS = InterpLocal::PS::STKCHK;
          } else {
            il->S[CURPR->T] = 10;
          }
          break;
        }
        }
        break;
      }
      case 9: {
        if (CURPR->PID == 0) {
          for (el.I = 1; el.I <= HIGHESTPROCESSOR; el.I++) {
            proc = (PRD *)calloc(1, sizeof(PROCESSDESCRIPTOR));
            proc->PC = 0;
            proc->PID = il->NEXTID++;
            if (il->NEXTID > PIDMAX) {
              il->PS = InterpLocal::PS::PROCCHK;
            }
            proc->VIRTUALTIME = 0;
            memcpy(proc->DISPLAY, CURPR->DISPLAY, sizeof(CURPR->DISPLAY));
            il->PTEMP = (ACTPNT)calloc(1, sizeof(ACTIVEPROCESS));
            il->PTEMP->PDES = proc;
            il->PTEMP->NEXT = il->ACPTAIL->NEXT;
            il->ACPTAIL->NEXT = il->PTEMP;
            il->ACPTAIL = il->PTEMP;
            el.H1 = FINDFRAME(il, il->STKMAIN);
            if (debug & DBGPROC) {
              fprintf(STDOUT, "opc %d findframe %d length %d, response %d\n",
                      el.IR.F, CURPR->PID, il->STKMAIN, el.H1);
            }
            if (el.H1 > 0) {
              proc->T = el.H1 + BTAB[2].VSIZE - 1;
              proc->STACKSIZE = el.H1 + il->STKMAIN - 1;
              proc->B = el.H1;
              proc->BASE = el.H1;
              for (el.J = el.H1; el.J <= el.H1 + il->STKMAIN - 1; el.J++) {
                il->S[el.J] = 0;
                il->SLOCATION[el.J] = el.I;
                il->RS[el.I] = 0.0;
              }
              for (el.J = el.H1; el.J <= el.H1 + BASESIZE - 1; el.J++) {
                il->STARTMEM[el.I] = -il->STARTMEM[el.I];
              }
              il->S[el.H1 + 1] = 0;
              il->S[el.H1 + 2] = 0;
              il->S[el.H1 + 3] = -1;
              il->S[el.H1 + 4] = BTAB[1].LAST;
              il->S[el.H1 + 5] = 1;
              il->S[el.H1 + 6] = il->STKMAIN;
              proc->DISPLAY[1] = el.H1;
            }
            proc->TIME = CURPR->TIME;
            proc->STATE = PRD::STATE::READY;
            proc->FORLEVEL = CURPR->FORLEVEL;
            proc->READSTATUS = PRD::NONE;
            proc->FORKCOUNT = 1;
            proc->MAXFORKTIME = 0;
            proc->JOINSEM = 0;
            proc->PARENT = CURPR;
            proc->PRIORITY = PRD::PRIORITY::LOW;
            proc->ALTPROC = -1;
            proc->SEQON = true;
            proc->GROUPREP = false;
            proc->PROCESSOR = el.I;
            if (il->PROCTAB[el.I].STATUS ==
                InterpLocal::PROCTAB::STATUS::NEVERUSED) {
              il->USEDPROCS++;
            }
            il->PROCTAB[el.I].STATUS = InterpLocal::PROCTAB::STATUS::FULL;
            il->PROCTAB[el.I].NUMPROC++;
            el.J = 1;
            while (proc->DISPLAY[el.J] != -1) {
              il->S[proc->DISPLAY[el.J] + 5]++;
              if (debug & DBGRELEASE) {
                fprintf(STDOUT, "%d ref ct %d ct=%d\n", el.IR.F, CURPR->PID,
                        il->S[proc->DISPLAY[el.J] + 5]);
              }
              el.J++;
            }
            if (debug & DBGPROC) {
              fprintf(STDOUT, "opc %d newproc pid %d\n", el.IR.F, proc->PID);
            }
            CURPR->FORKCOUNT += 1;
          }
          CURPR->PC = CURPR->PC + 3;
        }
        break;
      }
      case 10: { // jmp op2
        CURPR->PC = (int)el.IR.Y;
        break;
      }
      case 11: // if0 pop,->op2
        if (il->S[CURPR->T] == 0) {
          CURPR->PC = el.IR.Y;
        }
        CURPR->T--;
        break;
      case 12: {
        el.H1 = il->S[CURPR->T];
        CURPR->T--;
        el.H2 = el.IR.Y;
        el.H3 = 0;
        do {
          if (CODE[el.H2].F != 13) {
            el.H3 = 1;
            il->PS = InterpLocal::PS::CASCHK;
          } else if (CODE[el.H2].X == -1 || CODE[el.H2].Y == el.H1) {
            el.H3 = 1;
            CURPR->PC = CODE[el.H2 + 1].Y;
          } else {
            el.H2 = el.H2 + 2;
          }
        } while (el.H3 == 0);
        break;
      }
      case 13: {
        el.H1 = el.IR.X;
        el.H2 = el.IR.Y;
        el.H3 = FINDFRAME(il, el.H2 + 1);
        if (debug & DBGPROC) {
          fprintf(STDOUT, "opc %d findframe %d length %d, response %d\n",
                  el.IR.F, CURPR->PID, el.H2 + 1, el.H3);
        }
        if (el.H3 < 0) {
          il->PS = InterpLocal::PS::STKCHK;
        } else {
          for (el.I = 0; el.I < el.H2; el.I++) {
            il->S[el.H3 + el.I] = (unsigned char)STAB[el.H1 + el.I];
          }
          il->S[el.H3 + el.H2] = 0;
          CURPR->T++;
          if (CURPR->T > CURPR->STACKSIZE) {
            il->PS = InterpLocal::PS::STKCHK;
          } else {
            il->S[CURPR->T] = el.H3;
          }
        }
        break;
      }
      case 14: { // push from stack top stack frame loc
        el.H1 = il->S[CURPR->T];
        //                        if (CURPR->PROCESSOR == 0)
        //                            fprintf(STDOUT, "14: proc %d pc %d stack
        //                            %d\n", CURPR->PROCESSOR, CURPR->PC,
        //                            CURPR->T);
        CURPR->T++;
        if (CURPR->T > CURPR->STACKSIZE) {
          il->PS = InterpLocal::PS::STKCHK;
        } else {
          il->S[CURPR->T] = il->S[el.H1];
          if (il->S[CURPR->T] == RTAG) {
            il->RS[CURPR->T] = il->RS[el.H1];
          }
        }
        break;
      }
      case 15: { // add
        CURPR->T--;
        il->S[CURPR->T] = il->S[CURPR->T] + il->S[CURPR->T + 1];
        break;
      }
      case 18: { // IR.Y is index of a function in TAB[] symbol table
                 // get stack frame for block execution callblk op
        if (TAB[el.IR.Y].ADR == 0) {
          il->PS = InterpLocal::PS::FUNCCHK;
        } else if (TAB[el.IR.Y].ADR > 0) {
          el.H1 = BTAB[TAB[el.IR.Y].REF].VSIZE + WORKSIZE;
          el.H2 = FINDFRAME(il, el.H1);
          if (debug & DBGPROC) {
            fprintf(STDOUT, "opc %d findframe %d length %d, response %d\n",
                    el.IR.F, CURPR->PID, el.H1, el.H2);
          }
          if (el.H2 >= 0) {
            il->S[el.H2 + 7] = CURPR->T;
            CURPR->T = el.H2 - 1;
            CURPR->STACKSIZE = el.H1 + el.H2 - 1;
            CURPR->T += 8;
            il->S[CURPR->T - 4] = el.H1 - 1;
            il->S[CURPR->T - 3] = el.IR.Y;
            for (el.I = el.H2; el.I <= el.H2 + BASESIZE - 1; el.I++) {
              il->STARTMEM[el.I] = -il->STARTMEM[el.I];
            }
          }
        }
        break;
      }
      case 19: {
        if (TAB[el.IR.X].ADR < 0) {
          EXECLIB(il, &el, CURPR, -TAB[el.IR.X].ADR);
        } else {
          el.H1 = CURPR->T - el.IR.Y;        // base of frame acquired by 18
          el.H2 = il->S[el.H1 + 4];          // function TAB index
          el.H3 = TAB[el.H2].LEV;            // TAB[stk + 4].LEV
          CURPR->DISPLAY[el.H3 + 1] = el.H1; // DISPLAY[H3 + 1]
          for (el.I = el.H3 + 2; el.I <= LMAX;
               el.I++) { // clear DISPLAY to top w/-1
            CURPR->DISPLAY[el.I] = -1;
          }
          il->S[el.H1 + 6] = il->S[el.H1 + 3] + 1;
          el.H4 = il->S[el.H1 + 3] + el.H1;
          il->S[el.H1 + 1] = CURPR->PC; // return addr
          il->S[el.H1 + 2] = CURPR->DISPLAY[el.H3];
          il->S[el.H1 + 3] = CURPR->B;
          il->S[el.H1 + 5] = 1;
          for (el.H3 = CURPR->T + 1; el.H3 <= el.H4; el.H3++) {
            il->S[el.H3] = 0;
            il->RS[el.H3] = 0.0;
          }
          for (el.H3 = el.H1; el.H3 <= el.H4; el.H3++) {
            il->SLOCATION[el.H3] = CURPR->PROCESSOR;
          }
          CURPR->B = el.H1;
          CURPR->T = el.H4 - WORKSIZE;
          CURPR->PC = TAB[el.H2].ADR; // jump to block address
          TIMEINC(il, 3, "cs19");
        }
        break;
      }
      case 20: { // swap
        el.H1 = il->S[CURPR->T];
        el.RH1 = il->RS[CURPR->T];
        il->S[CURPR->T] = il->S[CURPR->T - 1];
        il->RS[CURPR->T] = il->RS[CURPR->T - 1];
        il->S[CURPR->T - 1] = el.H1;
        il->RS[CURPR->T - 1] = el.RH1;
        break;
      }
      case 21: { // load array Y[T]
        // op2 is array table index
        // stack top is array subscript
        // top - 1 is array base in stack
        // pop stack
        // replace top with stack element at array base plus subscript
        el.H1 = el.IR.Y;
        el.H2 = ATAB[el.H1].LOW;
        el.H3 = il->S[CURPR->T]; // array subscript
        if (el.H3 < el.H2 ||
            ((el.H3 > ATAB[el.H1].HIGH) && (ATAB[el.H1].HIGH > 0))) {
          il->PS = InterpLocal::PS::INXCHK;
        } else {
          CURPR->T--;
          el.H4 = il->S[CURPR->T]; // added for debugging
          il->S[CURPR->T] =
              il->S[CURPR->T] + (el.H3 - el.H2) * ATAB[el.H1].ELSIZE;
          // fprintf(STDOUT, "\nindex %d array stack base %d stack loc %d\n",
          // el.H3, el.H4, il->S[CURPR->T]);
        }
        break;
      }
      case 22: {
        el.H1 = il->S[CURPR->T];
        if ((el.H1 <= 0) || (el.H1 >= STMAX)) {
          il->PS = InterpLocal::PS::REFCHK;
        } else {
          CURPR->T--;
          el.H2 = el.IR.Y + CURPR->T;
          if (TOPOLOGY != SHAREDSY) {
            TESTVAR(il, el.H1);
          }
          if (el.H2 > CURPR->STACKSIZE) {
            il->PS = InterpLocal::PS::STKCHK;
          } else {
            while (CURPR->T < el.H2) {
              CURPR->T++;
              if (il->NUMTRACE > 0) {
                CHKVAR(il, el.H1);
              }
              il->S[CURPR->T] = il->S[el.H1];
              if (il->S[el.H1] == RTAG)
                il->RS[CURPR->T] = il->RS[el.H1];
              el.H1++;
            }
          }
        }
        break;
      }
      case 23: {
        el.H1 = il->S[CURPR->T - 1];
        el.H2 = il->S[CURPR->T];
        if ((el.H1 <= 0) || (el.H2 <= 0) || (el.H1 >= STMAX) ||
            (el.H2 >= STMAX)) {
          il->PS = InterpLocal::PS::REFCHK;
        } else {
          el.H3 = el.H1 + el.IR.X;
          if (TOPOLOGY != SHAREDSY) {
            TESTVAR(il, el.H2);
            if (el.IR.Y == 0) {
              TESTVAR(il, el.H1);
            } else if (il->CONGESTION) {
              el.H4 = COMMDELAY(il, CURPR->PROCESSOR, il->SLOCATION[el.H1],
                                el.IR.X);
            }
          }
          while (el.H1 < el.H3) {
            if (il->NUMTRACE > 0) {
              CHKVAR(il, el.H1);
              CHKVAR(il, el.H2);
            }
            if (il->STARTMEM[el.H1] <= 0) {
              il->PS = InterpLocal::PS::REFCHK;
            }
            il->S[el.H1] = il->S[el.H2];
            if (il->S[el.H2] == RTAG) {
              il->RS[el.H1] = il->RS[el.H2];
            }
            el.H1++;
            el.H2++;
          }
          TIMEINC(il, el.IR.X / 5, "cs23");
          il->S[CURPR->T - 1] = il->S[CURPR->T];
          CURPR->T--;
        }
        break;
      }
      case 24: { // push imm op2
        CURPR->T++;
        if (CURPR->T > CURPR->STACKSIZE) {
          il->PS = InterpLocal::PS::STKCHK;
        } else {
          il->S[CURPR->T] = (int)el.IR.Y;
        }
        break;
      }
      case 26: { // convreal
        il->RS[CURPR->T] = il->S[CURPR->T];
        il->S[CURPR->T] = RTAG;
        break;
      }
      case 27: {
        int IORESULT = 0;
#if defined(__APPLE__)
        *__error() = 0;
#elif defined(__linux__) || defined(_WIN32)
        errno = 0;
#endif
        el.H1 = il->S[CURPR->T];
        if (TOPOLOGY != SHAREDSY) {
          TESTVAR(il, el.H1);
        }
        if (il->NUMTRACE > 0) {
          CHKVAR(il, el.H1);
        }
        if (INPUTFILE) {
          if (feof(INP)) {
            il->PS = InterpLocal::PS::REDCHK;
          }
        } else if (feof(STDIN)) {
          il->PS = InterpLocal::PS::REDCHK;
        }
        if (il->PS != InterpLocal::PS::REDCHK) {
          if (!INPUTFILE) {
            switch (el.IR.Y) {
            case 1: {
              il->S[el.H1] = RTAG;
              // INPUT >> il->RS[el.H1];
              // fscanf(STDIN, "%lf", &il->RS[el.H1]);
              fscanf(STDIN, "%s", el.buf);
              il->RS[el.H1] = strtod(el.buf, nullptr);
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
              // fscanf(STDIN, "%d", &il->S[el.H1]);
              fscanf(STDIN, "%s", el.buf);
              il->S[el.H1] = (int)strtol(el.buf, nullptr, 10);
#if defined(__APPLE__)
              if (*__error() != 0)
#elif defined(__linux__) || defined(_WIN32)
              if (errno != 0)
#endif
                IORESULT = -1;
              break;
            }
            case 4: {
              // char CH;
              //  INPUT >> CH;
              // il->S[el.H1] = int(CH);
              il->S[el.H1] = fgetc(STDIN);
              if (0 > il->S[el.H1])
                IORESULT = -1;
              break;
            }
            }
          } else {
            switch (el.IR.Y) {
            case 1: {
              il->S[el.H1] = RTAG;
              // INP >> il->RS[el.H1];
              // fscanf(INP, "%lf", &il->RS[el.H1]);
              fscanf(INP, "%s", el.buf);
              il->RS[el.H1] = strtod(el.buf, nullptr);
#if defined(__APPLE__)
              if (*__error() != 0)
#elif defined(__linux__) || defined(_WIN32)
              if (errno != 0)
#endif
                IORESULT = -1;
              break;
            }
            case 2: {
              // INP >> il->S[el.H1];
              // fscanf(STDIN, "%d", &il->S[el.H1]);
              fscanf(INP, "%s", el.buf);
              il->S[el.H1] = (int)strtol(el.buf, nullptr, 10);
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
              // INP >> CH;
              // il->S[el.H1] = int(CH);
              // il->S[el.H1] = fgetc(STDIN);
              il->S[el.H1] = fgetc(INP);
              if (0 > il->S[el.H1])
                IORESULT = -1;
              break;
            }
            }
          }
          if (IORESULT != 0) {
            il->PS = InterpLocal::PS::DATACHK;
          }
        }
        CURPR->T--;
        break;
      }
      case 28: { // write from string table
        el.H1 = il->S[CURPR->T];
        el.H2 = el.IR.Y;
        CURPR->T--;
        il->CHRCNT = il->CHRCNT + el.H1;
        if (il->CHRCNT > LINELENG) {
          il->PS = InterpLocal::PS::LNGCHK;
        }
        do {
          if (!OUTPUTFILE) {
            std::fputc(STAB[el.H2], STDOUT);
          } else {
            std::fputc(STAB[el.H2], OUTP);
          }
          el.H1--;
          el.H2++;
          if (STAB[el.H2] == (char)10 || STAB[el.H2] == (char)13) {
            il->CHRCNT = 0;
          }
        } while (el.H1 != 0);
        break;
      }
      case 29: { //  outwidth output formatted with WIDTH
        if (il->COUTWIDTH <= 0) {
          il->CHRCNT = il->CHRCNT + il->FLD[el.IR.Y];
          if (il->CHRCNT > LINELENG) {
            il->PS = InterpLocal::PS::LNGCHK;
          } else {
            if (!OUTPUTFILE) {
              switch (el.IR.Y) {
              case 2: {
                fprintf(STDOUT, "%*d", il->FLD[2], il->S[CURPR->T]);
                break;
              }
              case 3: {
                fprintf(STDOUT, "%*s", il->FLD[3],
                        (ITOB(il->S[CURPR->T])) ? "TRUE" : "FALSE");
                break;
              }
              case 4: {
                if ((il->S[CURPR->T] < CHARL) || (il->S[CURPR->T] > CHARH)) {
                  il->PS = InterpLocal::PS::CHRCHK;
                } else {
                  fprintf(STDOUT, "%*c", il->FLD[4], (char)(il->S[CURPR->T]));
                  if ((il->S[CURPR->T] == 10) || (il->S[CURPR->T] == 13)) {
                    il->CHRCNT = 0;
                  }
                }
                break;
              }
              }
            } else {
              switch (el.IR.Y) {
              case 2: {
                fprintf(OUTP, "%*d", il->FLD[2], il->S[CURPR->T]);
                break;
              }
              case 3: {
                fprintf(OUTP, "%*s", il->FLD[3],
                        (ITOB(il->S[CURPR->T])) ? "true" : "false");
                break;
              }
              case 4: {
                if ((il->S[CURPR->T] < CHARL) || (il->S[CURPR->T] > CHARH)) {
                  il->PS = InterpLocal::PS::CHRCHK;
                } else {
                  fprintf(OUTP, "%*c", il->FLD[4], char(il->S[CURPR->T]));
                  if ((il->S[CURPR->T] == 10) || (il->S[CURPR->T] == 13)) {
                    il->CHRCNT = 0;
                  }
                }
                break;
              }
              }
            }
          }
        } else {
          il->CHRCNT = il->CHRCNT + il->COUTWIDTH;
          el.H1 = il->S[CURPR->T];
          if (il->CHRCNT > LINELENG) {
            il->PS = InterpLocal::PS::LNGCHK;
          } else {
            if (!OUTPUTFILE) {
              switch (el.IR.Y) {
              case 2: {
                // std::cout << el.H1;
                fprintf(STDOUT, "%*d", il->COUTWIDTH, el.H1);
                break;
              }
              case 3: {
                // std::cout << ITOB(el.H1);
                fprintf(STDOUT, "%*s", il->COUTWIDTH,
                        (ITOB(el.H1)) ? "true" : "false");
                break;
              }
              case 4: {
                if ((el.H1 < CHARL) || (el.H1 > CHARH)) {
                  il->PS = InterpLocal::PS::CHRCHK;
                } else {
                  // std::cout << char(el.H1);
                  fprintf(STDOUT, "%*c", il->COUTWIDTH, (char)el.H1);
                  if ((el.H1 == 10) || (el.H1 == 13)) {
                    il->CHRCNT = 0;
                  }
                }
                break;
              }
              }
            } else {
              switch (el.IR.Y) {
              case 2: {
                fprintf(OUTP, "%*d", il->COUTWIDTH, el.H1);
                break;
              }
              case 3: {
                fprintf(OUTP, "%*s", il->COUTWIDTH,
                        (ITOB(el.H1)) ? "true" : "false");
                break;
              }
              case 4: {
                if ((el.H1 < CHARL) || (el.H1 > CHARH)) {
                  il->PS = InterpLocal::PS::CHRCHK;
                } else {
                  fprintf(OUTP, "%*c", il->COUTWIDTH, (char)el.H1);
                  if ((el.H1 == 10) || (el.H1 == 13)) {
                    il->CHRCNT = 0;
                  }
                }
                break;
              }
              }
            }
          }
        }
        CURPR->T--;
        il->COUTWIDTH = -1;
        break;
      }
      case 30: { // pop and set out width or out prec
        if (el.IR.Y == 1) {
          il->COUTWIDTH = il->S[CURPR->T];
        } else {
          il->COUTPREC = il->S[CURPR->T];
        }
        CURPR->T = CURPR->T - 1;
        break;
      }
      case 31: { // main  end
        if (CURPR->FORKCOUNT > 1) {
          CURPR->STATE = PRD::STATE::BLOCKED;
          CURPR->PC = CURPR->PC - 1;
          il->PROCTAB[CURPR->PROCESSOR].RUNPROC = nullptr;
          CURPR->FORKCOUNT = CURPR->FORKCOUNT - 1;
        } else {
          il->PS = InterpLocal::PS::FIN;
          if (debug & DBGPROC) {
            fprintf(STDOUT, "opc %d terminated pid %d\n", el.IR.F, CURPR->PID);
            if (CURPR->PID == 0)
              unreleased(il);
          }
          CURPR->STATE = PROCESSDESCRIPTOR::TERMINATED;
        }
        break;
      }
      case 32: {
        if (il->S[CURPR->B + 5] == 1) {
          if (debug & DBGRELEASE) {
            fprintf(STDOUT, "%d release %d fm=%d ln=%d\n", el.IR.F, CURPR->PID,
                    CURPR->B, il->S[CURPR->B + 6]);
          }
          RELEASE(il, CURPR->B, il->S[CURPR->B + 6]);
        } else {
          il->S[CURPR->B + 5] -= 1;
          if (debug & DBGRELEASE) {
            fprintf(STDOUT, "%d ref ct %d ct=%d\n", el.IR.F, CURPR->PID,
                    il->S[CURPR->B + 5]);
          }
        }
        el.H1 = TAB[il->S[CURPR->B + 4]].LEV;
        CURPR->DISPLAY[el.H1 + 1] = -1;
        CURPR->PC = il->S[CURPR->B + 1];
        CURPR->T = il->S[CURPR->B + 7];
        CURPR->B = il->S[CURPR->B + 3];
        if ((CURPR->T >= CURPR->B - 1) &&
            (CURPR->T < CURPR->B + il->S[CURPR->B + 6])) {
          CURPR->STACKSIZE = CURPR->B + il->S[CURPR->B + 6] - 1;
        } else {
          CURPR->STACKSIZE = CURPR->BASE + WORKSIZE - 1;
        }
        break;
      }
      case 33: { // release block resources ?
        el.H1 = TAB[il->S[CURPR->B + 4]].LEV;
        el.H2 = CURPR->T;
        CURPR->DISPLAY[el.H1 + 1] = -1;
        if (il->S[CURPR->B + 5] == 1) {
          if (debug & DBGRELEASE) {
            // fprintf(STDOUT, "release base %d, length %d\n", CURPR->B,
            // il->S[CURPR->B+6]);
            fprintf(STDOUT, "%d release %d fm=%d ln=%d\n", el.IR.F, CURPR->PID,
                    CURPR->B, il->S[CURPR->B + 6]);
          }
          RELEASE(il, CURPR->B, il->S[CURPR->B + 6]);
        } else {
          // fprintf(STDOUT, "release adjacent %d\n", il->S[CURPR->B+6]);
          il->S[CURPR->B + 5] = il->S[CURPR->B + 5] - 1;
          if (debug & DBGRELEASE) {
            fprintf(STDOUT, "%d ref ct %d ct=%d\n", el.IR.F, CURPR->PID,
                    il->S[CURPR->B + 5]);
          }
        }
        CURPR->T = il->S[CURPR->B + 7] + 1;
        il->S[CURPR->T] = il->S[el.H2];
        if (il->S[CURPR->T] == RTAG) {
          il->RS[CURPR->T] = il->RS[el.H2];
        }
        CURPR->PC = il->S[CURPR->B + 1];
        CURPR->B = il->S[CURPR->B + 3];
        if ((CURPR->T >= CURPR->B - 1) &&
            (CURPR->T < CURPR->B + il->S[CURPR->B + 6])) {
          CURPR->STACKSIZE = CURPR->B + il->S[CURPR->B + 6] - 1;
        } else {
          CURPR->STACKSIZE = CURPR->BASE + WORKSIZE - 1;
        }
        break;
      }
      case 34: { // get variable stack location and replace top with value
        // load indirect ->[T] to T
        el.H1 = il->S[CURPR->T];
        if ((el.H1 <= 0) || (el.H1 >= STMAX)) {
          il->PS = InterpLocal::PS::REFCHK;
        } else {
          if (TOPOLOGY != SHAREDSY) {
            TESTVAR(il, el.H1);
          }
          if (il->NUMTRACE > 0) {
            CHKVAR(il, el.H1);
          }
          if (il->S[el.H1] == RTAG) {
            il->RS[CURPR->T] = il->RS[el.H1];
          }
          il->S[CURPR->T] = il->S[el.H1];
        }
        break;
      }
      case 35: // not
        il->S[CURPR->T] = BTOI(!(ITOB(il->S[CURPR->T])));
        break;
      case 36: { // negate
        if (il->S[CURPR->T] == RTAG) {
          il->RS[CURPR->T] = -il->RS[CURPR->T];
        } else {
          il->S[CURPR->T] = -il->S[CURPR->T];
        }
        break;
      }
      case 37: { // out real with prec/width or none
        el.RH1 = il->RS[CURPR->T];
        if (el.RH1 < 1) {
          el.H2 = 0;
        } else {
          el.H2 = 1 + (int)std::floor(std::log(std::fabs(el.RH1)) / el.log10);
        }
        if (il->COUTWIDTH <= 0) {
          el.H1 = il->FLD[1];
        } else {
          el.H1 = il->COUTWIDTH;
        }
        if ((il->COUTPREC > 0) && (il->COUTPREC + 3 < el.H1)) {
          el.H1 = il->COUTPREC + 3;
        }
        il->CHRCNT = il->CHRCNT + el.H1;
        if (il->CHRCNT > LINELENG) {
          il->PS = InterpLocal::PS::LNGCHK;
        } else {
          if (!OUTPUTFILE) {
            if (il->COUTPREC <= 0) {
              // cout << el.RH1;
              fprintf(STDOUT, "%*f", el.H1, el.RH1);
            } else {
              if (il->COUTWIDTH <= 0) {
                // cout << el.RH1 << ":" << il->COUTPREC - el.H2;
                fprintf(STDOUT, "%*.*f", il->COUTPREC + 3, il->COUTPREC - el.H2,
                        el.RH1);
              } else {
                // cout << el.RH1 << ":" << il->COUTWIDTH << ":" << il->COUTPREC
                // - el.H2;
                fprintf(STDOUT, "%*.*f", il->COUTWIDTH, il->COUTPREC - el.H2,
                        el.RH1);
              }
            }
          } else {
            if (il->COUTPREC <= 0) {
              // OUTP << el.RH1;
              fprintf(OUTP, "%*f", el.H1, el.RH1);
            } else {
              if (il->COUTWIDTH <= 0) {
                fprintf(OUTP, "%*.*f", il->COUTPREC + 3, il->COUTPREC - el.H2,
                        el.RH1);
              } else {
                fprintf(OUTP, "%*.*f", il->COUTWIDTH, il->COUTPREC - el.H2,
                        el.RH1);
              }
            }
          }
        }
        CURPR->T = CURPR->T - 1;
        il->COUTWIDTH = -1;
        break;
      }
      case 38: { // get saved DISPLAY (a stack location for a variable) copy top
                 // to it
        // [[T-1]] <- [T]
        // [T-1] <- [T]
        // T <- T-1
        // stindstk
        el.H1 = il->S[CURPR->T - 1];
        if (el.H1 <= 0 || el.H1 >= STMAX) {
          il->PS = InterpLocal::PS::REFCHK;
        } else {
          if (il->STARTMEM[el.H1] <= 0) {
            il->PS = InterpLocal::PS::REFCHK;
          }
          il->S[el.H1] = il->S[CURPR->T];
          if (TOPOLOGY != SHAREDSY) {
            if (el.IR.Y == 0) {
              TESTVAR(il, el.H1);
            } else if (il->CONGESTION) {
              el.H2 = COMMDELAY(il, CURPR->PROCESSOR, il->SLOCATION[el.H1], 1);
            }
          }
          if (il->NUMTRACE > 0) {
            CHKVAR(il, el.H1);
          }
          if (il->S[CURPR->T] == RTAG) {
            il->RS[el.H1] = il->RS[CURPR->T];
          }
          il->S[CURPR->T - 1] = il->S[CURPR->T];
          if (il->S[CURPR->T] == RTAG) {
            il->RS[CURPR->T - 1] = il->RS[CURPR->T];
          }
          CURPR->T = CURPR->T - 1;
        }
        break;
      }
      case 45: { // pop and compare EQ
        CURPR->T = CURPR->T - 1;
        if (ISREAL(il)) {
          il->S[CURPR->T] = BTOI(il->RS[CURPR->T] == il->RS[CURPR->T + 1]);
        } else {
          il->S[CURPR->T] = BTOI(il->S[CURPR->T] == il->S[CURPR->T + 1]);
        }
        break;
      }
      case 46: { // pop and compare NE
        CURPR->T = CURPR->T - 1;
        if (ISREAL(il)) {
          il->S[CURPR->T] = BTOI(il->RS[CURPR->T] != il->RS[CURPR->T + 1]);
        } else {
          il->S[CURPR->T] = BTOI(il->S[CURPR->T] != il->S[CURPR->T + 1]);
        }
        break;
      }
      case 47: { // pop, then compare top op top+1, replace top with comparison
                 // result
        //  pop and compare LT
        CURPR->T = CURPR->T - 1;
        if (ISREAL(il)) {
          il->S[CURPR->T] = BTOI(il->RS[CURPR->T] < il->RS[CURPR->T + 1]);
        } else {
          il->S[CURPR->T] = BTOI(il->S[CURPR->T] < il->S[CURPR->T + 1]);
        }
        break;
      }
      case 48: { // pop and compare LE
        CURPR->T = CURPR->T - 1;
        if (ISREAL(il)) {
          il->S[CURPR->T] = BTOI(il->RS[CURPR->T] <= il->RS[CURPR->T + 1]);
        } else {
          il->S[CURPR->T] = BTOI(il->S[CURPR->T] <= il->S[CURPR->T + 1]);
        }
        break;
      }
      case 49: { // pop and compare GT
        CURPR->T = CURPR->T - 1;
        if (ISREAL(il)) {
          il->S[CURPR->T] = BTOI(il->RS[CURPR->T] > il->RS[CURPR->T + 1]);
        } else {
          il->S[CURPR->T] = BTOI(il->S[CURPR->T] > il->S[CURPR->T + 1]);
        }
        break;
      }
      case 50: { // pop and compare GE
        CURPR->T = CURPR->T - 1;
        if (ISREAL(il)) {
          il->S[CURPR->T] = BTOI(il->RS[CURPR->T] >= il->RS[CURPR->T + 1]);
        } else {
          il->S[CURPR->T] = BTOI(il->S[CURPR->T] >= il->S[CURPR->T + 1]);
        }
        break;
      }
      case 51: { // pop OR
        CURPR->T = CURPR->T - 1;
        il->S[CURPR->T] =
            BTOI(ITOB(il->S[CURPR->T]) || ITOB(il->S[CURPR->T + 1]));
        break;
      }
      case 52: { // plus
        CURPR->T = CURPR->T - 1;
        if (ISREAL(il)) {
          il->RS[CURPR->T] = il->RS[CURPR->T] + il->RS[CURPR->T + 1];
          if (il->RS[CURPR->T] != 0) {
            el.RH1 = log(fabs(il->RS[CURPR->T])) / el.log10;
            if ((el.RH1 >= 292.5) || (el.RH1 <= -292.5)) {
              il->PS = InterpLocal::PS::OVRCHK;
            }
          }
        } else {
          il->S[CURPR->T] = il->S[CURPR->T] + il->S[CURPR->T + 1];
          if (abs(il->S[CURPR->T]) > NMAX) {
            il->PS = InterpLocal::PS::INTCHK;
          }
        }
        break;
      }
      case 53: { // minus
        CURPR->T = CURPR->T - 1;
        if (ISREAL(il)) {
          il->RS[CURPR->T] = il->RS[CURPR->T] - il->RS[CURPR->T + 1];
          if (il->RS[CURPR->T] != 0) {
            el.RH1 = log(fabs(il->RS[CURPR->T])) / el.log10;
            if ((el.RH1 >= 292.5) || (el.RH1 <= -292.5)) {
              il->PS = InterpLocal::PS::OVRCHK;
            }
          }
        } else {
          il->S[CURPR->T] = il->S[CURPR->T] - il->S[CURPR->T + 1];
          if (abs(il->S[CURPR->T]) > NMAX) {
            il->PS = InterpLocal::PS::INTCHK;
          }
        }
        break;
      }
      case 56: { // and
        CURPR->T = CURPR->T - 1;
        il->S[CURPR->T] =
            BTOI(ITOB(il->S[CURPR->T]) && ITOB(il->S[CURPR->T + 1]));
        break;
      }
      case 57: { // times
        CURPR->T = CURPR->T - 1;
        if (ISREAL(il)) {
          il->RS[CURPR->T] = il->RS[CURPR->T] * il->RS[CURPR->T + 1];
          if (il->RS[CURPR->T] != 0) {
            el.RH1 = log(fabs(il->RS[CURPR->T])) / el.log10;
            if ((el.RH1 >= 292.5) || (el.RH1 <= -292.5)) {
              il->PS = InterpLocal::PS::OVRCHK;
            }
          }
        } else {
          il->S[CURPR->T] = il->S[CURPR->T] * il->S[CURPR->T + 1];
          if (abs(il->S[CURPR->T]) > NMAX) {
            il->PS = InterpLocal::PS::INTCHK;
          }
        }
        break;
      }
      case 58: { // int div
        CURPR->T = CURPR->T - 1;
        if (il->S[CURPR->T + 1] == 0) {
          il->PS = InterpLocal::PS::DIVCHK;
        } else {
          il->S[CURPR->T] = il->S[CURPR->T] / il->S[CURPR->T + 1];
        }
        break;
      }
      case 59: { // int mod
        CURPR->T = CURPR->T - 1;
        if (il->S[CURPR->T + 1] == 0) {
          il->PS = InterpLocal::PS::DIVCHK;
        } else {
          il->S[CURPR->T] = il->S[CURPR->T] % il->S[CURPR->T + 1];
        }
        break;
      }
      case 62: {
        if (!INPUTFILE) {
          if (feof(STDIN)) {
            il->PS = InterpLocal::PS::REDCHK;
          } else {
            fgetc(STDIN);
          }
        } else {
          if (feof(INP)) {
            il->PS = InterpLocal::PS::REDCHK;
          } else {
            fgetc(INP);
          }
        }
        break;
      }
      case 63: { // write endl
        if (!OUTPUTFILE) {
          fputc('\n', STDOUT);
        } else {
          fputc('\n', OUTP);
        }
        il->LNCNT = il->LNCNT + 1;
        il->CHRCNT = 0;
        if (il->LNCNT > LINELIMIT) {
          il->PS = InterpLocal::PS::LINCHK;
        }
        break;
      }
      case 64:
      case 65:
      case 71: { // [T] is stk loc of channel
        switch (el.IR.F) {
        case 64:
          el.H1 = CURPR->DISPLAY[el.IR.X] + el.IR.Y;
          break;
        case 65:
          el.H1 = il->S[CURPR->DISPLAY[el.IR.X] + el.IR.Y];
          break;
        case 71:
          el.H1 = il->S[CURPR->T];
          break;
        }
        el.H2 = il->SLOCATION[el.H1];
        il->CNUM = il->S[el.H1];
        if (debug & DBGRECV) {
          fprintf(STDOUT, "recv %d pid %d pc %d state %s rdstatus %s chan %d\n",
                  el.IR.F, CURPR->PID, CURPR->PC, nameState(CURPR->STATE),
                  nameRdstatus(CURPR->READSTATUS), il->CNUM);
        }
        if (il->NUMTRACE > 0) {
          CHKVAR(il, el.H1);
        }
        if (il->CNUM == 0) {
          il->CNUM = FIND(il);     // get and initialize unused channel
          il->S[el.H1] = il->CNUM; // store channel in stack loc
        }
        if (CURPR->READSTATUS == PRD::READSTATUS::NONE) {
          CURPR->READSTATUS = PRD::READSTATUS::ATCHANNEL;
        }
        chan = &il->CHAN[il->CNUM];
        // WITH CHAN[CNUM]
        if (!(*chan).MOVED && (*chan).READER < 0) {
          il->SLOCATION[el.H1] = CURPR->PROCESSOR;
          (*chan).READER = CURPR->PID;
        }
        if (TOPOLOGY != SHAREDSY)
          TESTVAR(il, el.H1); // channel in stk
        if ((*chan).READTIME < il->CLOCK - TIMESTEP) {
          (*chan).READTIME = il->CLOCK - TIMESTEP;
        }
        il->PNT = (*chan).HEAD;
        el.B1 = (*chan).SEM == 0;
        if (!el.B1) {
          el.B1 = il->DATE[il->PNT] > CURPR->TIME;
        }
        if (el.B1) {
          il->PTEMP = (ACTPNT)calloc(1, sizeof(ACTIVEPROCESS));
          il->PTEMP->PDES = CURPR;
          il->PTEMP->NEXT = nullptr;
          il->PROCTAB[CURPR->PROCESSOR].RUNPROC = nullptr;
          if ((*chan).WAIT == nullptr) {
            el.H3 = 1;
            (*chan).WAIT = il->PTEMP;
            if ((*chan).SEM != 0) {
              CURPR->STATE = PRD::STATE::DELAYED;
              CURPR->WAKETIME = il->DATE[il->PNT];
            } else {
              CURPR->STATE = PRD::STATE::BLOCKED;
            }
          } else {
            il->RTEMP = (*chan).WAIT;
            while (il->RTEMP->NEXT != nullptr) {
              il->RTEMP = il->RTEMP->NEXT;
            }
            il->RTEMP->NEXT = il->PTEMP;
            CURPR->STATE = PRD::STATE::BLOCKED;
          }
          CURPR->PC = CURPR->PC - 1;
          il->NOSWITCH = false;
        } else {
          if (CURPR->READSTATUS != PRD::READSTATUS::HASTICKET) {
            if ((*chan).READTIME > CURPR->TIME) {
              CURPR->TIME = (*chan).READTIME;
              if (debug & DBGTIME)
                procTime(CURPR, 0.0, "case 64,65,71");
              CURPR->READSTATUS = PRD::READSTATUS::HASTICKET;
              CURPR->PC = CURPR->PC - 1;
              il->NOSWITCH = false;
              (*chan).READTIME = (*chan).READTIME + CHANTIME;
              goto L699;
            } else {
              (*chan).READTIME = (*chan).READTIME + CHANTIME;
            }
          }
          TIMEINC(il, CHANTIME, "cs64");
          (*chan).SEM = (*chan).SEM - 1;
          (*chan).HEAD = il->LINK[il->PNT];
          if ((el.IR.F == 64) || (el.IR.F == 65)) {
            CURPR->T = CURPR->T + 1;
          }
          if (CURPR->T > CURPR->STACKSIZE) {
            il->PS = InterpLocal::PS::STKCHK;
          } else {
            il->S[CURPR->T] = il->VALUE[il->PNT];
            if (il->S[CURPR->T] == RTAG) {
              il->RS[CURPR->T] = il->RVALUE[il->PNT];
            }
          }
          il->LINK[il->PNT] = il->FREE;
          il->FREE = il->PNT;
          CURPR->READSTATUS = PRD::READSTATUS::NONE;
          if ((*chan).WAIT != nullptr) {
            if ((*chan).WAIT->PDES == CURPR) { // remove CURPR from wait list
              il->PTEMP = (*chan).WAIT;
              (*chan).WAIT = (*chan).WAIT->NEXT;
              if (debug & DBGPROC) {
                fprintf(STDOUT, "remove pid %d from wait list\n", CURPR->PID);
              }
              std::free(il->PTEMP); // free ACTPNT
            }
            if ((*chan).WAIT != nullptr) { // set next on wait list
              if ((*chan).SEM == 0) {
                (*chan).WAIT->PDES->STATE = PRD::STATE::BLOCKED;
              } else {
                (*chan).WAIT->PDES->STATE = PRD::STATE::DELAYED;
                (*chan).WAIT->PDES->WAKETIME = il->DATE[(*chan).HEAD];
              }
            }
          }
        }
      L699:
        if (debug & DBGRECV) {
          fprintf(STDOUT,
                  "recv(e) pid %d state %s rdstatus %s chan %d WPID %d\n",
                  CURPR->PID, nameState(CURPR->STATE),
                  nameRdstatus(CURPR->READSTATUS), il->CNUM,
                  ((*chan).WAIT != nullptr) ? (*chan).WAIT->PDES->PID : -1);
        }
        break;
      }
      case 66:
      case 92: {
        el.J = il->S[CURPR->T - 1]; // stack loc of channel number
        il->CNUM = il->S[el.J];     // channel number
        el.H2 = il->SLOCATION[el.J];
        if (il->NUMTRACE > 0)
          CHKVAR(il, il->S[CURPR->T - 1]);
        if (il->CNUM == 0) {
          il->CNUM = FIND(il);
          il->S[il->S[CURPR->T - 1]] = il->CNUM;
        }
        if (debug & DBGSEND) {
          fprintf(STDOUT, "sendchan %d pid %d var [[T]] %d chan %d\n", el.IR.F,
                  CURPR->PID, il->S[il->S[CURPR->T - 1]], il->CNUM);
        }
        el.H1 = COMMDELAY(il, CURPR->PROCESSOR, el.H2, el.IR.Y);
        // WITH CHAN[CNUM] DO
        // il->CHAN[il->CNUM]
        chan = &il->CHAN[il->CNUM];
        {
          if (il->FREE == 0)
            il->PS = InterpLocal::PS::BUFCHK;
          else {
            TIMEINC(il, CHANTIME, "cs66");
            el.K = il->FREE;
            il->DATE[el.K] = CURPR->TIME + el.H1;
            il->FREE = il->LINK[il->FREE];
            il->LINK[el.K] = 0;
            if ((*chan).HEAD == 0) {
              (*chan).HEAD = el.K;
            } else {
              il->PNT = (*chan).HEAD;
              el.I = 0;
              el.B1 = true;
              while (il->PNT != 0 && el.B1) {
                if (el.I != 0) {
                  el.TGAP = (int)(il->DATE[il->PNT] - il->DATE[el.I]);
                } else {
                  el.TGAP = CHANTIME + 3;
                }
                if (il->DATE[il->PNT] > il->DATE[el.K] &&
                    el.TGAP > CHANTIME + 1) {
                  el.B1 = false;
                } else {
                  el.I = il->PNT;
                  il->PNT = il->LINK[il->PNT];
                }
              }
              il->LINK[el.K] = il->PNT;
              if (el.I == 0) {
                (*chan).HEAD = el.K;
              } else {
                il->LINK[el.I] = el.K;
                if (il->DATE[el.K] < il->DATE[el.I] + CHANTIME)
                  il->DATE[el.K] = il->DATE[el.I] + CHANTIME;
              }
            }
            if (TOPOLOGY == SHAREDSY) {
              CURPR->TIME = il->DATE[el.K];
              if (debug & DBGTIME)
                procTime(CURPR, 0.0, "case 66,92");
            }
            if ((*chan).HEAD == el.K &&
                (*chan).WAIT != nullptr) { // WITH WAIT->PDES
              proc = il->CHAN[il->CNUM].WAIT->PDES;
              proc->STATE = PRD::STATE::DELAYED;
              proc->WAKETIME = il->DATE[el.K];
            }
            if (el.IR.F == 66) {
              il->VALUE[el.K] = il->S[CURPR->T];
              if (il->S[CURPR->T] == RTAG)
                il->RVALUE[el.K] = il->RS[CURPR->T];
            } else {
              il->VALUE[el.K] = RTAG;
              il->RVALUE[el.K] = il->S[CURPR->T];
              il->RS[CURPR->T] = il->S[CURPR->T];
              il->S[CURPR->T] = RTAG;
            }
            il->S[CURPR->T - 1] = il->S[CURPR->T];
            if (il->S[CURPR->T] == RTAG)
              il->RS[CURPR->T - 1] = il->RS[CURPR->T];
            CURPR->T -= 2;
            (*chan).SEM += 1;
          }
        }
        break;
      }
      case 67:
      case 74: {
        il->NOSWITCH = false;
        TIMEINC(il, CREATETIME, "cs67");
        proc = (PROCPNT)calloc(1, sizeof(PROCESSDESCRIPTOR));
        proc->PC = CURPR->PC + 1;
        proc->PID = il->NEXTID++;
        // il->NEXTID += 1;
        if (il->NEXTID > PIDMAX) {
          il->PS = InterpLocal::PS::PROCCHK;
        }
        proc->VIRTUALTIME = 0;
        for (int i = 0; i <= LMAX; i++) {
          proc->DISPLAY[i] = CURPR->DISPLAY[i];
        }
        proc->B = CURPR->B;
        il->PTEMP = (ACTPNT)calloc(1, sizeof(ACTIVEPROCESS));
        il->PTEMP->PDES = proc;
        il->PTEMP->NEXT = il->ACPTAIL->NEXT;
        il->ACPTAIL->NEXT = il->PTEMP;
        il->ACPTAIL = il->PTEMP;
        proc->T = FINDFRAME(il, WORKSIZE) - 1;
        if (debug & DBGPROC) {
          fprintf(STDOUT, "opc %d findframe %d length %d, response %d\n",
                  el.IR.F, proc->PID, WORKSIZE, proc->T + 1);
        }
        proc->STACKSIZE = proc->T + WORKSIZE;
        proc->BASE = proc->T + 1;
        proc->TIME = CURPR->TIME;
        // proc->NUMCHILDREN = 0;
        // proc->MAXCHILDTIME = 0;
        if (el.IR.Y != 1) {
          proc->WAKETIME =
              CURPR->TIME + COMMDELAY(il, CURPR->PROCESSOR, il->S[CURPR->T], 1);
          if (proc->WAKETIME > proc->TIME) {
            proc->STATE = PRD::STATE::DELAYED;
          } else {
            proc->STATE = PRD::STATE::READY;
          }
        }
        proc->FORLEVEL = CURPR->FORLEVEL;
        proc->READSTATUS = PRD::READSTATUS::NONE;
        proc->FORKCOUNT = 1;
        proc->MAXFORKTIME = 0;
        proc->JOINSEM = 0;
        proc->PARENT = CURPR;
        proc->PRIORITY = PRD::PRIORITY::LOW;
        proc->ALTPROC = -1;
        proc->SEQON = true;
        proc->GROUPREP = false;
        proc->PROCESSOR = il->S[CURPR->T];
        if (proc->PROCESSOR > HIGHESTPROCESSOR || proc->PROCESSOR < 0) {
          il->PS = InterpLocal::PS::CPUCHK;
        } else {
          if (il->PROCTAB[proc->PROCESSOR].STATUS ==
              InterpLocal::PROCTAB::STATUS::NEVERUSED) {
            il->USEDPROCS += 1;
          }
          il->PROCTAB[proc->PROCESSOR].STATUS =
              InterpLocal::PROCTAB::STATUS::FULL;
          il->PROCTAB[proc->PROCESSOR].NUMPROC += 1;
        }
        CURPR->T = CURPR->T - 1;
        if (proc->T > 0) {
          el.J = 0;
          while (proc->FORLEVEL > el.J) {
            proc->T = proc->T + 1;
            el.H1 = CURPR->BASE + el.J;
            il->S[proc->T] = il->S[el.H1];
            il->SLOCATION[proc->T] = il->SLOCATION[el.H1];
            il->RS[proc->T] = il->RS[el.H1];
            el.J = el.J + 1;
          }
          if (el.IR.F == 74) {
            proc->FORLEVEL = proc->FORLEVEL + 1;
            el.H1 = il->S[CURPR->T - 2];
            el.H2 = il->S[CURPR->T - 1];
            el.H3 = il->S[CURPR->T];
            el.H4 = il->S[CURPR->T - 3];
            proc->T = proc->T + 1;
            il->S[proc->T] = el.H1;
            il->SLOCATION[proc->T] = el.H4;
            il->RS[proc->T] = proc->PC;
            proc->T = proc->T + 1;
            if (el.H1 + el.H3 <= el.H2) {
              il->S[proc->T] = el.H1 + el.H3 - 1;
            } else {
              il->S[proc->T] = el.H2;
            }
          }
        }
        el.J = 1;
        while (proc->DISPLAY[el.J] != -1) {
          il->S[proc->DISPLAY[el.J] + 5] += 1;
          if (debug & DBGRELEASE) {
            fprintf(STDOUT, "%d ref ct %d ct=%d\n", el.IR.F, CURPR->PID,
                    il->S[proc->DISPLAY[el.J] + 5]);
          }
          el.J = el.J + 1;
        }
        if (el.IR.Y == 1) {
          proc->STATE = PRD::STATE::RUNNING;
          CURPR->STATE = PRD::STATE::BLOCKED;
          proc->TIME = CURPR->TIME;
          proc->PRIORITY = PRD::PRIORITY::HIGH;
          proc->ALTPROC = proc->PROCESSOR;
          proc->PROCESSOR = CURPR->PROCESSOR;
          il->PROCTAB[proc->PROCESSOR].RUNPROC = proc;
          il->PROCTAB[proc->PROCESSOR].NUMPROC += 1;
        }
        if (el.IR.F == 74) {
          if (CURPR->NUMCHILDREN == 0)
            CURPR->MAXCHILDTIME = CURPR->TIME;
          CURPR->NUMCHILDREN += 1;
        } else {
          CURPR->FORKCOUNT += 1;
        }
        if (debug & DBGPROC) {
          fprintf(STDOUT, "opc %d newproc pid %d\n", el.IR.F, proc->PID);
        }
        //                        fprintf(STDOUT, "fork processsor %d alt %d
        //                        status %s\n", proc->PROCESSOR, proc->ALTPROC,
        //                              prcsrStatus(il->PROCTAB[proc->PROCESSOR].STATUS));
        break;
      }
      case 69:
      case 70: {
        if (CURPR->FORKCOUNT > 1) {
          CURPR->FORKCOUNT -= 1;
          CURPR->STATE = PRD::STATE::BLOCKED;
          il->PROCTAB[CURPR->PROCESSOR].RUNPROC = nullptr;
          CURPR->PC = CURPR->PC - 1;
        } else {
          for (int i = LMAX; i > 0; i--) {
            el.J = CURPR->DISPLAY[i];
            if (el.J != -1) {
              il->S[el.J + 5] = il->S[el.J + 5] - 1;
              if (il->S[el.J + 5] == 0) {
                if (debug & DBGRELEASE) {
                  fprintf(STDOUT, "%d releas1 %d fm=%d ln=%d\n", el.IR.F,
                          CURPR->PID, el.J, il->S[el.J + 6]);
                }
                RELEASE(il, el.J, il->S[el.J + 6]);
              } else {
                if (debug & DBGRELEASE) {
                  fprintf(STDOUT, "%d ref ct %d ct=%d\n", el.IR.F, CURPR->PID,
                          il->S[el.J + 5]);
                }
              }
            }
          }
          if (!MPIMODE) {
            if (debug & DBGRELEASE) {
              fprintf(STDOUT, "%d releas2 %d fm=%d ln=%d\n", el.IR.F,
                      CURPR->PID, CURPR->BASE, WORKSIZE);
            }
            RELEASE(il, CURPR->BASE, WORKSIZE);
          }
          if (MPIMODE && il->MPIINIT[CURPR->PROCESSOR] &&
              !il->MPIFIN[CURPR->PROCESSOR]) {
            il->PS = InterpLocal::PS::MPIFINCHK;
          }
          for (int i = 1; i <= OPCHMAX; i++) {
            if (il->CHAN[i].READER == CURPR->PID) {
              il->CHAN[i].READER = -1;
            }
          }
          CURPR->SEQON = false;
          TIMEINC(il, CREATETIME - 1, "cs69");
          el.H1 = COMMDELAY(il, CURPR->PROCESSOR, CURPR->PARENT->PROCESSOR, 1);
          il->R1 = el.H1 + CURPR->TIME;
          // with CURPR->PARENT
          proc = CURPR->PARENT;
          switch (el.IR.F) {
          case 70: {
            proc->NUMCHILDREN = proc->NUMCHILDREN - 1;
            if (proc->MAXCHILDTIME < il->R1) {
              proc->MAXCHILDTIME = il->R1;
            }
            if (proc->NUMCHILDREN == 0) {
              proc->STATE = PRD::STATE::DELAYED;
              proc->WAKETIME = proc->MAXCHILDTIME;
            }
            break;
          }
          case 69: {
            proc->FORKCOUNT = proc->FORKCOUNT - 1;
            if (proc->MAXFORKTIME < il->R1) {
              proc->MAXFORKTIME = il->R1;
            }
            if (proc->JOINSEM == -1) {
              if (il->R1 > proc->TIME) {
                proc->WAKETIME = il->R1;
                proc->STATE = PRD::STATE::DELAYED;
              } else {
                proc->STATE = PRD::STATE::READY;
              }
            }
            proc->JOINSEM = proc->JOINSEM + 1;
            if (proc->FORKCOUNT == 0) {
              if (proc->MAXFORKTIME > proc->TIME) {
                proc->WAKETIME = proc->MAXFORKTIME;
                proc->STATE = PRD::STATE::DELAYED;
              } else {
                proc->STATE = PRD::STATE::READY;
              }
            }
            break;
          }
          }
          if (debug & DBGPROC) {
            fprintf(STDOUT, "opc %d terminated pid %d\n", el.IR.F, CURPR->PID);
          }
          CURPR->STATE = PRD::STATE::TERMINATED;
          prtb = &il->PROCTAB[CURPR->PROCESSOR];
          prtb->NUMPROC -= 1;
          prtb->RUNPROC = nullptr;
          if (prtb->NUMPROC == 0) {
            prtb->STATUS = InterpLocal::PROCTAB::STATUS::EMPTY;
          }
        }
        break;
      }
      case 73: {
        CURPR->T = CURPR->T + 1;
        if (CURPR->T > CURPR->STACKSIZE) {
          il->PS = InterpLocal::PS::STKCHK;
        } else {
          el.H1 = CURPR->DISPLAY[el.IR.X] + el.IR.Y;
          for (int i = CURPR->BASE; i <= CURPR->BASE + CURPR->FORLEVEL - 1;
               i++) {
            if (il->SLOCATION[i] == el.H1) {
              il->S[CURPR->T] = il->S[i];
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
        CURPR->FORINDEX = CURPR->T - 2;
        if (il->S[CURPR->T] <= 0) {
          il->PS = InterpLocal::PS::GRPCHK;
        } else if (il->S[CURPR->T - 2] > il->S[CURPR->T - 1]) {
          CURPR->T = CURPR->T - 4;
          CURPR->PC = el.IR.Y;
        } else {
          CURPR->PRIORITY = PRD::PRIORITY::HIGH;
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
        il->S[CURPR->T - 2] += il->S[CURPR->T];
        if (il->S[CURPR->T - 2] <= il->S[CURPR->T - 1]) {
          CURPR->PC = el.IR.Y;
        } else {
          CURPR->T = CURPR->T - 4;
          CURPR->PRIORITY = PRD::PRIORITY::LOW;
        }
        break;
      }
      case 78: { // wakepar
        if (CURPR->GROUPREP) {
          CURPR->ALTPROC = -1;
        } else {
          // with CURPR->PARENT
          CURPR->PARENT->STATE = PRD::STATE::RUNNING;
          CURPR->PARENT->TIME = CURPR->TIME;
          if (debug & DBGTIME)
            procTime(CURPR->PARENT, 0.0, "case 78");
          il->PROCTAB[CURPR->PROCESSOR].NUMPROC -= 1;
          il->PROCTAB[CURPR->PROCESSOR].RUNPROC = CURPR->PARENT;
          CURPR->WAKETIME =
              CURPR->TIME +
              COMMDELAY(il, CURPR->PROCESSOR, CURPR->ALTPROC, 1 + el.IR.Y);
          if (CURPR->WAKETIME > CURPR->TIME) {
            CURPR->STATE = PRD::STATE::DELAYED;
          } else {
            CURPR->STATE = PRD::STATE::READY;
          }
          CURPR->PROCESSOR = CURPR->ALTPROC;
          CURPR->ALTPROC = -1;
          CURPR->PRIORITY = PRD::PRIORITY::LOW;
        }
        break;
      }
      case 79: {
        CURPR->T = CURPR->T + 1;
        if (CURPR->T > CURPR->STACKSIZE) {
          il->PS = InterpLocal::PS::STKCHK;
        } else {
          // il->S[T] = RTAG;
          // il->RS[T] = CONTABLE[el.IR.Y];
          el.I = -1;
          el.J = -1;
          el.B1 = false;
          el.H1 = MAXINT;
          el.H2 = -1; // ?? added DE
          do {
            el.I += 1;
            prtb = &il->PROCTAB[el.I];
            // with PROCTAB[I]
            switch (prtb->STATUS) {
            case InterpLocal::PROCTAB::STATUS::EMPTY:
              el.B1 = true;
              break;
            case InterpLocal::PROCTAB::STATUS::NEVERUSED:
              if (el.J == -1)
                el.J = el.I;
              break;
            case InterpLocal::PROCTAB::STATUS::FULL:
              if (prtb->NUMPROC < el.H1) {
                el.H2 = el.I;
                el.H1 = prtb->NUMPROC;
              }
              break;
            case InterpLocal::PROCTAB::STATUS::RESERVED:
              if (prtb->NUMPROC + 1 < el.H1) // +1 fixed dde
              {
                el.H2 = el.I;
                el.H1 = prtb->NUMPROC + 1;
              }
              break;
            }
          } while (!el.B1 && el.I != HIGHESTPROCESSOR);
          if (el.B1) {
            il->S[CURPR->T] = el.I;
          } else if (el.J > -1) {
            il->S[CURPR->T] = el.J;
            il->USEDPROCS += 1;
          } else
            il->S[CURPR->T] = el.H2;
          // fprintf(STDOUT, "find processsor %d status %s\n", il->S[CURPR->T],
          //         prcsrStatus(il->PROCTAB[il->S[CURPR->T]].STATUS));
          il->PROCTAB[il->S[CURPR->T]].STATUS =
              InterpLocal::PROCTAB::STATUS::RESERVED;
        }
        break;
      }
      case 80: {
        il->SLOCATION[il->S[CURPR->T]] = il->S[CURPR->T - 1];
        CURPR->T = CURPR->T - 1;
        break;
      }
      case 81: {
        ++CURPR->T;
        TIMEINC(il, -1, "cs81");
        if (CURPR->T > CURPR->STACKSIZE) {
          il->PS = InterpLocal::PS::STKCHK;
        } else {
          il->S[CURPR->T] = il->S[CURPR->FORINDEX];
        }
        break;
      }
      case 82: { // initarray initialization
        for (int i = el.IR.X; i < el.IR.Y; i++) {
          el.H1 = il->S[CURPR->T];
          il->S[el.H1] = INITABLE[i].IVAL;
          // fprintf(STDOUT, "initarray stack %d val %d", el.H1,
          // INITABLE[i].IVAL);
          if (INITABLE[i].IVAL == RTAG) {
            il->RS[el.H1] = INITABLE[i].RVAL;
            // fprintf(STDOUT, " real val %f", INITABLE[i].RVAL);
          }
          // fprintf(STDOUT, "\n");
          il->S[CURPR->T] = il->S[CURPR->T] + 1;
        }
        TIMEINC(il, el.IR.Y - el.IR.X / 5, "cs82");
        break;
      }
      case 83: { // zeroarr
        el.H1 = il->S[CURPR->T];
        for (int i = el.H1; i < el.H1 + el.IR.X; i++) {
          if (el.IR.Y == 2) {
            il->S[i] = RTAG;
            il->RS[i] = 0.0;
          } else {
            il->S[i] = 0;
          }
        }
        CURPR->T--;
        TIMEINC(il, el.IR.X / 10, "cs83");
        break;
      }
      case 84: { // dup
        ++CURPR->T;
        TIMEINC(il, -1, "cs84");
        if (CURPR->T > CURPR->STACKSIZE) {
          il->PS = InterpLocal::PS::STKCHK;
        } else {
          il->S[CURPR->T] = il->S[CURPR->T - 1];
        }
        break;
      }
      case 85: { // join?
        if (CURPR->JOINSEM > 0) {
          CURPR->JOINSEM -= 1;
        } else {
          CURPR->JOINSEM = -1;
          CURPR->STATE = PRD::STATE::BLOCKED;
          il->PROCTAB[CURPR->PROCESSOR].RUNPROC = nullptr;
        }
        break;
      }
      case 86: {
        if (TOPOLOGY != SHAREDSY) {
          TESTVAR(il, il->S[CURPR->T]);
        }
        break;
      }
      case 87: { // push real constant from constant table
        CURPR->T = CURPR->T + 1;
        if (CURPR->T > CURPR->STACKSIZE) {
          il->PS = InterpLocal::PS::STKCHK;
        } else {
          il->S[CURPR->T] = RTAG;
          il->RS[CURPR->T] = CONTABLE[el.IR.Y];
        }
        break;
      }
      case 88: {
        CURPR->T = CURPR->T - 1;
        if (il->S[CURPR->T] != RTAG) {
          il->RS[CURPR->T] = il->S[CURPR->T];
          il->S[CURPR->T] = RTAG;
        }
        if (il->S[CURPR->T + 1] != RTAG) {
          il->RS[CURPR->T + 1] = il->S[CURPR->T + 1];
          il->S[CURPR->T + 1] = RTAG;
        }
        if (il->RS[CURPR->T + 1] == 0) {
          il->PS = InterpLocal::PS::DIVCHK;
        } else {
          il->RS[CURPR->T] = il->RS[CURPR->T] / il->RS[CURPR->T + 1];
          if (il->RS[CURPR->T] != 0) {
            el.RH1 = log(fabs(il->RS[CURPR->T])) / el.log10;
            if (el.RH1 >= 292.5 || el.RH1 <= -292.5) {
              il->PS = InterpLocal::PS::OVRCHK;
            }
          }
        }
        break;
      }
      case 89:
        il->NOSWITCH = true;
        break;
      case 90:
        il->NOSWITCH = false;
        break;
      case 91: {
        el.H1 = il->S[CURPR->T - 1];
        if (TOPOLOGY != SHAREDSY) {
          if (el.IR.Y == 0) {
            TESTVAR(il, el.H1);
          } else if (il->CONGESTION) {
            el.H1 = COMMDELAY(il, CURPR->PROCESSOR, il->SLOCATION[el.H1], 1);
          }
        }
        if (il->NUMTRACE > 0) {
          CHKVAR(il, el.H1);
        }
        il->RS[el.H1] = il->S[CURPR->T];
        il->S[el.H1] = RTAG;
        il->RS[CURPR->T - 1] = il->S[CURPR->T];
        il->S[CURPR->T - 1] = RTAG;
        CURPR->T = CURPR->T - 1;
        break;
      }
      case 93: { // duration
        el.H1 = il->S[CURPR->T];
        CURPR->T -= 1;
        if (CURPR->TIME + el.H1 > il->CLOCK) {
          CURPR->STATE = PRD::STATE::DELAYED;
          CURPR->WAKETIME = CURPR->TIME + el.H1;
          il->PROCTAB[CURPR->PROCESSOR].RUNPROC = nullptr;
        } else {
          if (debug & DBGTIME)
            procTime(CURPR, (float)el.H1, "case 93");
          CURPR->TIME += el.H1;
        }
        break;
      }
      case 94:
      case 95:
      case 96: {
        switch (el.IR.F) {
        case 94:
          el.H1 = il->S[CURPR->T];
          CURPR->T = CURPR->T - 1;
          break;
        case 95:
          el.H1 = CURPR->DISPLAY[el.IR.X] + el.IR.Y;
          break;
        case 96:
          el.H1 = il->S[CURPR->DISPLAY[el.IR.X] + el.IR.Y];
          break;
        }
        il->CNUM = il->S[el.H1];
        if (il->NUMTRACE > 0) {
          CHKVAR(il, el.H1);
        }
        if (il->CNUM == 0) {
          il->CNUM = FIND(il);
          il->S[el.H1] = il->CNUM;
        }
        if (!il->CHAN[il->CNUM].MOVED && il->CHAN[il->CNUM].READER < 0) {
          il->SLOCATION[el.H1] = CURPR->PROCESSOR;
          il->CHAN[il->CNUM].READER = CURPR->PID;
        }
        if (TOPOLOGY != SHAREDSY) {
          TESTVAR(il, el.H1);
        }
        CURPR->T = CURPR->T + 1;
        if (CURPR->T > CURPR->STACKSIZE) {
          il->PS = InterpLocal::PS::STKCHK;
        } else {
          il->S[CURPR->T] = 1;
          if (il->CNUM == 0 || il->CHAN[il->CNUM].SEM == 0 ||
              il->DATE[il->CHAN[il->CNUM].HEAD] > CURPR->TIME) {
            il->S[CURPR->T] = 0;
          }
        }
        break;
      }
      case 97: {
        el.H1 = il->S[CURPR->T];
        el.H2 = el.IR.Y;
        for (el.I = el.H1; el.I <= el.H1 + el.H2 - 1; el.I++) {
          il->SLOCATION[el.I] = il->S[CURPR->T - 1];
        }
        CURPR->T = CURPR->T - 1;
        break;
      }
      case 98:
      case 113: { // send copy msg
        el.H2 = il->S[CURPR->T];
        el.H1 = FINDFRAME(il, el.IR.Y);
        if (debug & DBGSEND) {
          fprintf(STDOUT, "send findframe %d pid %d from %d to %d len %d\n",
                  el.IR.F, CURPR->PID, el.H2, el.H1, el.IR.Y);
        }
        if (el.H2 <= 0 || el.H2 >= STMAX) {
          il->PS = InterpLocal::PS::REFCHK;
        } else if (el.H1 < 0) {
          il->PS = InterpLocal::PS::STORCHK;
        } else {
          el.H3 = el.H1 + el.IR.Y;
          if (TOPOLOGY != SHAREDSY) {
            TESTVAR(il, el.H2);
          }
          if (debug & DBGSEND) {
            fprintf(STDOUT, "copymsg");
          }
          while (el.H1 < el.H3) {
            if (il->NUMTRACE > 0) {
              CHKVAR(il, el.H2);
            }
            il->S[el.H1] = il->S[el.H2];
            il->SLOCATION[el.H1] = -1;
            if (il->S[el.H2] == RTAG) {
              il->RS[el.H1] = il->RS[el.H2];
            }
            if (debug & DBGSEND) {
              if (il->S[el.H2] != RTAG)
                fprintf(STDOUT, ":%d", il->S[el.H2]);
              else
                fprintf(STDOUT, ":%.1f", il->RS[el.H2]);
            }
            el.H1 = el.H1 + 1;
            el.H2 = el.H2 + 1;
          }
          if (debug & DBGSEND) {
            fprintf(STDOUT, "\n");
          }
          el.H2 = il->S[CURPR->T];
          il->S[CURPR->T] = el.H3 - el.IR.Y;
          if (el.IR.F == 98 && il->SLOCATION[el.H2] == -1) {
            if (debug & DBGSEND) {
              fprintf(STDOUT, "send(rel) %d pid % d from %d len %d\n", el.IR.F,
                      CURPR->PID, el.H2, el.IR.Y);
            }
            if (debug & DBGSEND) {
              fprintf(STDOUT, "%d send release fm=%d ln=%d\n", el.IR.F, el.H2,
                      el.IR.Y);
            }
            RELEASE(il, el.H2, el.IR.Y);
          }
          TIMEINC(il, el.IR.Y / 5, "cs98");
          if (debug & DBGSEND) {
            fprintf(STDOUT, "send(x) %d pid %d var [T] %d chan [T-1] %d\n",
                    el.IR.F, CURPR->PID, il->S[CURPR->T],
                    il->S[il->S[CURPR->T - 1]]);
          }
        }
        break;
      }
      case 101: { // lock
        if (TOPOLOGY != SHAREDSY) {
          il->PS = InterpLocal::PS::LOCKCHK;
        }
        if (il->NUMTRACE > 0) {
          CHKVAR(il, il->S[CURPR->T]);
        }
        if (il->S[il->S[CURPR->T]] == 0) {
          il->S[il->S[CURPR->T]] = 1;
          CURPR->T = CURPR->T - 1;
        } else {
          CURPR->STATE = PRD::STATE::SPINNING;
          CURPR->PC = CURPR->PC - 1;
        }
        break;
      }
      case 102: { // unlock
        if (TOPOLOGY != SHAREDSY) {
          il->PS = InterpLocal::PS::LOCKCHK;
        }
        if (il->NUMTRACE > 0) {
          CHKVAR(il, il->S[CURPR->T]);
        }
        il->S[il->S[CURPR->T]] = 0;
        CURPR->T = CURPR->T - 1;
        break;
      }
      case 104: {
        if (il->S[CURPR->T - 1] < il->S[CURPR->T]) {
          il->S[CURPR->T - 1] += 1;
          CURPR->PC = el.IR.X;
          if (el.IR.Y == 1) {
            CURPR->ALTPROC = CURPR->PARENT->PROCESSOR;
            CURPR->GROUPREP = true;
          }
        }
        break;
      }
      case 105: {
        if (il->S[CURPR->B + 5] == 1) {
          if (debug & DBGRELEASE) {
            fprintf(STDOUT, "%d release %d fm=%d ln=%d\n", el.IR.F, CURPR->PID,
                    CURPR->B, il->S[CURPR->B + 6]);
          }
          RELEASE(il, CURPR->B, il->S[CURPR->B + 6]);
        } else {
          il->S[CURPR->B + 5] = il->S[CURPR->B + 5] - 1;
          if (debug & DBGRELEASE) {
            fprintf(STDOUT, "%d fm ref ct %d ct=%d\n", el.IR.F, CURPR->PID,
                    il->S[CURPR->B + 5]);
          }
        }
        el.H1 = TAB[il->S[CURPR->B + 4]].LEV;
        CURPR->DISPLAY[el.H1 + 1] = -1;
        CURPR->T = il->S[CURPR->B + 7];
        CURPR->B = il->S[CURPR->B + 3];
        if (CURPR->T >= CURPR->BASE - 1 && CURPR->T < CURPR->BASE + WORKSIZE) {
          CURPR->STACKSIZE = CURPR->BASE + WORKSIZE - 1;
        } else {
          CURPR->STACKSIZE = CURPR->B + il->S[CURPR->B + 6] - 1;
        }
        break;
      }
      case 106: {
        TIMEINC(il, -1, "c106");
        CURPR->SEQON = false;
        break;
      }
      case 107: {
        TIMEINC(il, -1, "c107");
        CURPR->SEQON = true;
        break;
      }
      case 108: { // increment
        el.H1 = il->S[CURPR->T];
        if (el.H1 <= 0 || el.H1 >= STMAX) {
          il->PS = InterpLocal::PS::REFCHK;
        } else {
          if (TOPOLOGY != SHAREDSY) {
            TESTVAR(il, el.H1);
          }
          if (il->NUMTRACE > 0) {
            CHKVAR(il, el.H1);
          }
          switch (el.IR.Y) {
          case 0:
            if (il->S[el.H1] == RTAG) {
              il->RS[el.H1] = il->RS[el.H1] + el.IR.X;
              il->RS[CURPR->T] = il->RS[el.H1];
              il->S[CURPR->T] = RTAG;
            } else {
              il->S[el.H1] = il->S[el.H1] + el.IR.X;
              il->S[CURPR->T] = il->S[el.H1];
              if (abs(il->S[CURPR->T]) > NMAX) {
                il->PS = InterpLocal::PS::INTCHK;
              }
            }
            break;
          case 1:
            if (il->S[el.H1] == RTAG) {
              il->RS[CURPR->T] = il->RS[el.H1];
              il->S[CURPR->T] = RTAG;
              il->RS[el.H1] = il->RS[el.H1] + el.IR.X;
            } else {
              il->S[CURPR->T] = il->S[el.H1];
              il->S[el.H1] = il->S[el.H1] + el.IR.X;
              if (abs(il->S[el.H1]) > NMAX) {
                il->PS = InterpLocal::PS::INTCHK;
              }
            }
            break;
          }
          if (il->STARTMEM[el.H1] <= 0) {
            il->PS = InterpLocal::PS::REFCHK;
          }
        }
        break;
      }
      case 109: { // decrement
        el.H1 = il->S[CURPR->T];
        if (el.H1 <= 0 || el.H1 >= STMAX) {
          il->PS = InterpLocal::PS::REFCHK;
        } else {
          if (TOPOLOGY != SHAREDSY) {
            TESTVAR(il, el.H1);
          }
          if (il->NUMTRACE > 0) {
            CHKVAR(il, el.H1);
          }
          switch (el.IR.Y) {
          case 0:
            if (il->S[el.H1] == RTAG) {
              il->RS[el.H1] = il->RS[el.H1] - el.IR.X;
              il->RS[CURPR->T] = il->RS[el.H1];
              il->S[CURPR->T] = RTAG;
            } else {
              il->S[el.H1] = il->S[el.H1] - el.IR.X;
              il->S[CURPR->T] = il->S[el.H1];
              if (abs(il->S[CURPR->T]) > NMAX) {
                il->PS = InterpLocal::PS::INTCHK;
              }
            }
            break;
          case 1:
            if (il->S[el.H1] == RTAG) {
              il->RS[CURPR->T] = il->RS[el.H1];
              il->S[CURPR->T] = RTAG;
              il->RS[el.H1] = il->RS[el.H1] - el.IR.X;
            } else {
              il->S[CURPR->T] = il->S[el.H1];
              il->S[el.H1] = il->S[el.H1] - el.IR.X;
              if (abs(il->S[el.H1]) > NMAX) {
                il->PS = InterpLocal::PS::INTCHK;
              }
            }
            break;
          }
          if (il->STARTMEM[el.H1] <= 0) {
            il->PS = InterpLocal::PS::REFCHK;
          }
        }
        break;
      }
      case 110: {
        if (el.IR.Y == 0) {
          el.H1 = il->S[CURPR->T];
          el.H2 = il->S[CURPR->T - 1];
        } else {
          el.H1 = il->S[CURPR->T - 1];
          el.H2 = il->S[CURPR->T];
        }
        CURPR->T = CURPR->T - 1;
        il->S[CURPR->T] = el.H2 + el.IR.X * el.H1;
        break;
      }
      case 111: // pop
        CURPR->T = CURPR->T - 1;
        break;
      case 112: { // release if SLOC[T] == -1 no processor
        if (il->SLOCATION[il->S[CURPR->T]] == -1) {
          if (debug & DBGRELEASE) {
            fprintf(STDOUT, "%d release %d fm=%d ln=%d\n", el.IR.F, CURPR->PID,
                    il->S[CURPR->T], el.IR.Y);
          }
          RELEASE(il, il->S[CURPR->T], el.IR.Y);
        }
        break;
      }
      case 114: {
        if (il->S[CURPR->T] != 0) {
          il->S[CURPR->T] = 1;
        }
      }
      case 115: {
        el.H1 = il->S[CURPR->T - 1];
        il->SLOCATION[el.H1] = il->S[CURPR->T];
        il->CNUM = il->S[el.H1];
        if (il->CNUM == 0) {
          il->CNUM = FIND(il);
          il->S[el.H1] = il->CNUM;
        }
        il->CHAN[il->CNUM].MOVED = true;
        CURPR->T = CURPR->T - 2;
        break;
      }

        /* instruction cases go here */
      default:
        fprintf(STDOUT, "Missing Code %d\n", el.IR.F);
        break;
      }
    }
  } while (il->PS == InterpLocal::PS::RUN);
  // label_999:
  fprintf(STDOUT, "\n");
} // EXECUTE
} // namespace Cstar
