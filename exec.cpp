//
// Created by Dan Evans on 1/29/24.
//
#include <algorithm>
#include <cerrno>
#include <cmath>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <numbers>
#include <print>

#include "cs_compile.h"
#include "cs_exec.h"
#include "cs_global.h"
#include "cs_interpret.h"

namespace cs {
  using std::cout;
  using std::println;
  using std::numbers::ln10;

#define MAXINT 32767
  // enum PS {RUN, BREAK, FIN, DEAD, STKCHK};
  //    typedef struct ACTIVEPROCESS {
  //        ProcessDescriptor* PDES;
  //        ActiveProcess* NEXT;
  //    } ACTIVEPROCESS;

  struct CommdelayLocal {
    int T1, T2, T3, T4, I, DIST, NUMPACK, PATHLEN;
    int path[PMAX + 1];
    double PASTINTERVAL, NOWINTERVAL, FINALINTERVAL;
    Interpreter* il;
  };

  extern ALFA PROGNAME;
  extern void snapPDES(Interpreter*, ProcessDescriptor*);
  extern void dumpPDES(ProcessDescriptor*);
  extern void dumpInst(int);
  extern void dumpLInst(int, int*);
  extern void dumpDeadlock(Interpreter* il);
  extern const char* name_state(State);
  extern const char* name_read_status(ReadStatus);
  extern const char* processor_status(Status st);
  extern const char* lookupSym(int, int);
  extern void showInstTrace(bool);
  extern int GETLNUM(int);
  extern int BTOI(bool);
  extern int ITOB(int);
  extern int FIND(Interpreter* il);
  extern void RELEASE(Interpreter* il, int BASE, int LENGTH);
  extern void unreleased(Interpreter*);
  extern int FINDFRAME(Interpreter* il, int LENGTH);
  extern void EXECLIB(
    Interpreter* il,
    ExLocal*,
    ProcessDescriptor* CURPR,
    int ID
  );

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

  void showInstTrace(bool flag) {
    if (flag) debug |= DBGINST;
    else debug &= ~DBGINST;
  }

  void CHKVAR(Interpreter* interp_local, int STKLOC) {
    // check if the variable at the passed stack location is being traced
    int i = 0;
    int j = 0;
    for (i = 1; i <= VARMAX; i++) {
      if (STKLOC == interp_local->TRCTAB[i].MEMLOC) {
        std::cout << "Reference to Trace Variable "
          << interp_local->TRCTAB[i].NAME << '\n';
        // CURPR* curpr = new CURPR;
        interp_local->BLINE = GETLNUM(interp_local->CURPR->PC - 1);
        std::print("Line Number{:5}  In Function ", interp_local->BLINE);
        j = interp_local->CURPR->B;
        interp_local->INX = interp_local->S[interp_local->CURPR->B + 4];
        while (TAB[interp_local->INX].name[0] == '*') {
          j = interp_local->S[j + 3];
          interp_local->INX = interp_local->S[j + 4];
        }
        if (j == 0) std::cout << PROGNAME << '\n';
        else std::cout << TAB[interp_local->INX].name << '\n';
        //                std::cout << "Process Number" << il->CURPR->PID <<
        //                std::endl; std::cout << std::endl;
        std::println("Process Number{:4}\n", interp_local->CURPR->PID);
        interp_local->STEPROC = interp_local->CURPR;
        interp_local->PS = PS::BREAK;
      }
    }
  }

  void MOVE(CommdelayLocal* commdelay_local, int lSOURCE, int DES, int STEP) {
    int DIREC = 0;
    int I = 0;
    if (lSOURCE < DES) DIREC = 1;
    else DIREC = -1;
    for (I = 1; I <= abs(lSOURCE - DES); I++) {
      commdelay_local->PATHLEN = commdelay_local->PATHLEN + 1;
      commdelay_local->path[commdelay_local->PATHLEN] =
        commdelay_local->path[commdelay_local->PATHLEN - 1] + DIREC * STEP;
    }
  }

  void RMOVE(CommdelayLocal* cl, int lSOURCE, int DES, int STEP) {
    int DIREC, I, DIST;
    DIST = abs(lSOURCE - DES);
    if (lSOURCE < DES)
      DIREC = 1;
    else
      DIREC = -1;
    if (TOPDIM < 2 * DIST) {
      DIREC = -DIREC;
      DIST = TOPDIM - DIST;
    }
    for (I = 1; I <= DIST; I++) {
      cl->PATHLEN = cl->PATHLEN + 1;
      lSOURCE = lSOURCE + DIREC;
      if (lSOURCE == TOPDIM) {
        lSOURCE = 0;
        cl->path[cl->PATHLEN] = cl->path[cl->PATHLEN - 1] - STEP * (TOPDIM - 1);
      } else if (lSOURCE == -1) {
        lSOURCE = TOPDIM - 1;
        cl->path[cl->PATHLEN] = cl->path[cl->PATHLEN - 1] + STEP * (TOPDIM - 1);
      } else
        cl->path[cl->PATHLEN] = cl->path[cl->PATHLEN - 1] + DIREC * STEP;
    }
  }

  void SCHEDULE(CommdelayLocal* cl, double &ARRIVAL) {
    BUSYPNT CURPNT;
    BUSYPNT PREV;
    BUSYPNT ITEM;
    int I, PROCNUM;
    bool DONE;
    for (I = 1; I <= cl->PATHLEN; I++) {
      ARRIVAL = ARRIVAL + 2;
      PROCNUM = cl->path[I];
      CURPNT = cl->il->PROCTAB[PROCNUM].BUSYLIST;
      PREV = nullptr;
      DONE = false;
      do {
        if (CURPNT == nullptr) {
          DONE = true;
          ITEM = (BUSYPNT)std::calloc(1, sizeof(BUSYTYPE));
          // NEW(ITEM);
          ITEM->FIRST = ARRIVAL;
          ITEM->LAST = ARRIVAL;
          ITEM->NEXT = nullptr;
          if (PREV == nullptr) cl->il->PROCTAB[PROCNUM].BUSYLIST = ITEM;
          else PREV->NEXT = ITEM;
        } else {
          if (ARRIVAL < CURPNT->FIRST - 1) {
            DONE = true;
            // NEW(ITEM);
            ITEM = static_cast<BUSYPNT>(std::calloc(1, sizeof(BUSYTYPE)));
            ITEM->FIRST = ARRIVAL;
            ITEM->LAST = ARRIVAL;
            ITEM->NEXT = CURPNT;
            if (PREV != nullptr) PREV->NEXT = ITEM;
            else cl->il->PROCTAB[PROCNUM].BUSYLIST = ITEM;
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

  int COMMDELAY(Interpreter* il, int SOURCE, int DEST, int LEN) {
    CommdelayLocal cl;
    int rtn = 0;
    memset(&cl, 0, sizeof(cl));
    cl.il = il;
    cl.NUMPACK = LEN / 3;
    if (LEN % 3 != 0)
      cl.NUMPACK = cl.NUMPACK + 1;
    if (!il->CONGESTION || topology == Symbol::SHAREDSY) {
      switch (topology) {
        case Symbol::SHAREDSY: rtn = 0;
          break;
        case Symbol::FULLCONNSY:
        case Symbol::CLUSTERSY: cl.DIST = 1;
          break;
        case Symbol::HYPERCUBESY: cl.T1 = SOURCE;
          cl.T2 = DEST;
          cl.DIST = 0;
          while (cl.T1 + cl.T2 > 0) {
            if (cl.T1 % 2 != cl.T2 % 2) cl.DIST = cl.DIST + 1;
            cl.T1 = cl.T1 / 2;
            cl.T2 = cl.T2 / 2;
          }
          break;
        case Symbol::LINESY: cl.DIST = abs(SOURCE - DEST);
          break;
        case Symbol::MESH2SY: cl.T1 = TOPDIM;
          cl.T2 = std::abs(SOURCE / cl.T1 - DEST / cl.T1);
          cl.T3 = std::abs(SOURCE % cl.T1 - DEST % cl.T1);
          cl.DIST = cl.T2 + cl.T3;
          break;
        case Symbol::MESH3SY: cl.T2 = TOPDIM;
          cl.T1 = TOPDIM * cl.T2;
          cl.T3 = SOURCE % cl.T1;
          cl.T4 = DEST % cl.T1;
          cl.DIST = abs(cl.T3 / cl.T2 - cl.T4 / cl.T2) +
                    abs(cl.T3 % cl.T2 - cl.T4 % cl.T2) +
                    abs(SOURCE / cl.T1 - DEST / cl.T1);
          break;
        case Symbol::RINGSY: cl.T1 = abs(SOURCE - DEST);
          cl.T2 = TOPDIM - cl.T1;
          if (cl.T1 < cl.T2) cl.DIST = cl.T1;
          else cl.DIST = cl.T2;
          break;
        case Symbol::TORUSSY: cl.T1 = TOPDIM;
          cl.T3 = abs(SOURCE / cl.T1 - DEST / cl.T1);
          if (cl.T3 > cl.T1 / 2) cl.T3 = cl.T1 - cl.T3;
          cl.T4 = abs(SOURCE % cl.T1 - DEST % cl.T1);
          if (cl.T4 > cl.T1 / 2) cl.T4 = cl.T1 - cl.T4;
          cl.DIST = cl.T3 + cl.T4;
          break;
        default: break;
      }
      if (topology != Symbol::SHAREDSY) {
        rtn = static_cast<int>(std::round(
          (cl.DIST + (cl.NUMPACK - 1) / 2.0) * il->TOPDELAY
        ));
      }
    } else {
      cl.path[0] = SOURCE;
      cl.PATHLEN = 0;
      switch (topology) {
        case Symbol::FULLCONNSY:
        case Symbol::CLUSTERSY: cl.path[1] = DEST;
          cl.PATHLEN = 1;
          break;
        case Symbol::HYPERCUBESY: cl.T1 = 1;
          cl.T2 = SOURCE;
          for (cl.I = 1; cl.I <= TOPDIM; cl.I++) {
            cl.T3 = DEST / cl.T1 % 2;
            cl.T4 = SOURCE / cl.T1 % 2;
            if (cl.T3 != cl.T4) {
              if (cl.T3 == 1) cl.T2 = cl.T2 + cl.T1;
              else cl.T2 = cl.T2 - cl.T1;
              cl.PATHLEN = cl.PATHLEN + 1;
              cl.path[cl.PATHLEN] = cl.T2;
            }
            cl.T1 = cl.T1 * 2;
          }
          break;
        case Symbol::LINESY: MOVE(&cl, SOURCE, DEST, 1);
          break;
        case Symbol::MESH2SY: MOVE(&cl, SOURCE % TOPDIM, DEST % TOPDIM, 1);
          MOVE(&cl, SOURCE / TOPDIM, DEST / TOPDIM, TOPDIM);
          break;
        case Symbol::MESH3SY: cl.T1 = TOPDIM * TOPDIM;
          MOVE(&cl, SOURCE % TOPDIM, DEST % TOPDIM, 1);
          MOVE(&cl, SOURCE / TOPDIM % TOPDIM, DEST / TOPDIM % TOPDIM, TOPDIM);
          MOVE(&cl, SOURCE / cl.T1, DEST / cl.T1, TOPDIM * TOPDIM);
          break;
        case Symbol::RINGSY: RMOVE(&cl, SOURCE, DEST, 1);
          break;
        case Symbol::TORUSSY: RMOVE(&cl, SOURCE % TOPDIM, DEST % TOPDIM, 1);
          RMOVE(&cl, SOURCE / TOPDIM, DEST / TOPDIM, TOPDIM);
          break;
        default: break;
      }
      if (il->TOPDELAY != 0) {
        cl.PASTINTERVAL =
          static_cast<float>((il->CLOCK - 2 * TIMESTEP) / (il->TOPDELAY / 2.0));
        cl.NOWINTERVAL = static_cast<float>(
          il->CURPR->TIME / (il->TOPDELAY / 2.0) + 0.5);
        for (cl.I = 1; cl.I <= cl.NUMPACK; cl.I++) {
          cl.FINALINTERVAL = cl.NOWINTERVAL;
          SCHEDULE(&cl, cl.FINALINTERVAL);
        }
        rtn = static_cast<int>(std::round(
          (cl.FINALINTERVAL - cl.NOWINTERVAL) * (il->TOPDELAY / 2.0)
        ));
      } else
        rtn = 0;
    }
    if (SOURCE > highest_processor || DEST > highest_processor)
      il->PS = PS::CPUCHK;
    //        if (rtn != 0)
    //            fprintf(STANDARD_OUTPUT, "COMMDELAY source %d dest %d len %d returns
    //            %d\n", SOURCE, DEST, LEN, rtn);
    return rtn;
  }

  void TESTVAR(Interpreter* il, int STKPNT) {
    // verify that the processor number corresponding to the passed stack location
    // is the current PDES's processor, altproc, or -1, otherwise REMCHK
    if (const int PNUM = il->SLOCATION[STKPNT];
      PNUM != il->CURPR->PROCESSOR && PNUM != il->CURPR->ALTPROC &&
      PNUM != -1)
      il->PS = PS::REMCHK;
  }

  /*
   * if either stack top or top+1 is RTAG'ed and the other is not, move the
   * untagged value to the corresponding rstack and RTAG the moved stack element.
   */
  bool ISREAL(Interpreter* il) {
    STYPE* sp;
    RSTYPE* rp;
    int Tl;
    sp = il->S;
    rp = il->RS;
    Tl = il->CURPR->T;
    if (sp[Tl] == RTAG && sp[Tl + 1] != RTAG) {
      rp[Tl + 1] = sp[Tl + 1];
      sp[Tl + 1] = RTAG;
    }
    if (sp[Tl] != RTAG && sp[Tl + 1] == RTAG) {
      rp[Tl] = sp[Tl];
      sp[Tl] = RTAG;
    }
    return sp[Tl] == RTAG;
  }

  void procTime(ProcessDescriptor* prc, float incr, const char* id) {
    if (prc->PID == 0)
      std::print("{} = {} + {} {}\n", prc->TIME + incr, prc->TIME, incr, id);
  }

  void TIMEINC(Interpreter* il, int UNITS, const char* trc) {
    ProcessDescriptor* proc = il->CURPR;
    ProcessTable* table = &il->PROCTAB[proc->PROCESSOR];
    const float step = table->SPEED * static_cast<float>(UNITS);
    if (debug & DBGTIME) procTime(proc, step, trc);
    proc->TIME += step;
    table->VIRTIME += step;
    table->BRKTIME += step;
    table->PROTIME += step;
    proc->VIRTUALTIME += step;
    if (proc->SEQON) {
      if (debug & DBGSQTME)
        std::print("{} = {} + {} SEQ\n", il->SEQTIME + step, il->SEQTIME, step);
      il->SEQTIME += step;
    }
  }

  void SLICE(Interpreter* il) {
    ActiveProcess* prev = il->ACPCUR;
    il->ACPCUR = il->ACPCUR->NEXT;
    bool done = false;
    int count = 0;
    bool deadlock = true;
    do {
      count = count + 1;
      if (il->ACPCUR == il->ACPHEAD)
        il->CLOCK += TIMESTEP;
      ProcessDescriptor* proc = il->ACPCUR->PDES;
      if (proc->STATE == State::DELAYED) {
        if (proc->WAKETIME < il->CLOCK) {
          proc->STATE = State::READY;
          if (proc->WAKETIME > proc->TIME) {
            proc->TIME = proc->WAKETIME;
            if (debug & DBGTIME) procTime(proc, 0.0, "SLICE1-WAKE");
          }
        } else {
          proc->TIME = il->CLOCK;
          if (debug & DBGTIME) procTime(proc, 0.0, "SLICE2-CLOCK");
          deadlock = false;
        }
      }
      if (proc->STATE == State::RUNNING) {
        deadlock = false;
        if (proc->TIME < il->CLOCK) {
          done = true;
          il->CURPR = proc;
        }
      } else if (proc->STATE == State::READY) {
        deadlock = false; // maybe bug, maybe C++ problem yet to be corrected
        ProcessTable* ptab = &il->PROCTAB[proc->PROCESSOR];
        //                if (ptab->RUNPROC != nullptr && ptab->RUNPROC != proc)
        //                    fprintf(STANDARD_OUTPUT, "ptab(%d) != proc(%d) virtime %.1f
        //                    starttime %.1f\n",
        //                            ptab->RUNPROC->PID, proc->PID,
        //                            ptab->VIRTIME, ptab->STARTTIME);
        if (ptab->RUNPROC == nullptr) {
          il->CURPR = proc;
          done = true;
          ptab->STARTTIME = ptab->VIRTIME;
          proc->STATE = State::RUNNING;
          ptab->RUNPROC = il->CURPR;
        } else if (ptab->VIRTIME >= ptab->STARTTIME + SWITCHLIMIT &&
                   ptab->RUNPROC->PRIORITY <= proc->PRIORITY) {
          il->CURPR = proc;
          done = true;
          ptab->STARTTIME = ptab->VIRTIME;
          ptab->RUNPROC->STATE = State::READY;
          TIMEINC(il, SWITCHTIME, "slc1");
          proc->STATE = State::RUNNING;
          ptab->RUNPROC = il->CURPR;
          //                    fprintf(STANDARD_OUTPUT, "switch ptab runproc\n");
        } else {
          if (count > PMAX && !deadlock) {
            std::println("loop in SLICE");
            deadlock = true;
          }
          proc->TIME = il->CLOCK;
          if (debug & DBGTIME)
            procTime(proc, 0.0, "SLICE3-CLOCK");
        }
      } else if (proc->STATE == State::BLOCKED) {
        proc->TIME = il->CLOCK;
        if (debug & DBGTIME) procTime(proc, 0.0, "SLICE4-CLOCK");
      }
      if (proc->STATE == State::SPINNING) {
        if (il->S[il->S[proc->T]] == 0) {
          deadlock = false;
          if (proc->TIME < il->CLOCK) {
            done = true;
            il->CURPR = proc;
            proc->STATE = State::RUNNING;
          }
        } else {
          proc->TIME = il->CLOCK;
          if (debug & DBGTIME) procTime(proc, 0.0, "SLICE5-CLOCK");
        }
      }
      if (proc->STATE == State::TERMINATED) {
        prev->NEXT = il->ACPCUR->NEXT;
        if (il->ACPHEAD == il->ACPCUR)
          il->ACPHEAD = il->ACPCUR->NEXT;
        if (il->ACPTAIL == il->ACPCUR)
          il->ACPTAIL = prev;
        if (debug & DBGPROC) {
          std::println("slice terminated/freed process {}", proc->PID);
          snapPDES(il, proc);
        }
        std::free(proc);
        std::free(il->ACPCUR);
        il->ACPCUR = prev->NEXT;
      } else if (!done) {
        prev = il->ACPCUR;
        il->ACPCUR = il->ACPCUR->NEXT;
      }
    } while (!done && !(count > PMAX && deadlock));
    // if (il->CURPR->PID == 0)
    //     fprintf(STANDARD_OUTPUT, "dispatch %d time %.1f seqtime %.1f\n", il->CURPR->PID,
    //     il->CURPR->TIME, il->SEQTIME);
    if (deadlock) {
      cout << "    DEADLOCK:  All Processes are Blocked" << '\n';
      cout << "For Further Information Type STATUS Command" << '\n';
      std::println(
        "CLOCK {} PS {}, DONE {}, COUNT {}, DEADLOCK {}",
        il->CLOCK,
        static_cast<std::int8_t>(il->PS),
        done,
        count,
        deadlock
      );
      il->PS = PS::DEAD;
      dumpDeadlock(il);
    }
  }

  void EXECUTE(Interpreter* interp) {
    //        ProcessState PS;
    //        ProcessDescriptor* CURPR;
    //        int LC;
    //        double USAGE;
    //        char CH;
    //        int T;
    //        std::vector<int> STACK(STACKSIZE);
    ProcessDescriptor* current_process = interp->CURPR;
    ProcessDescriptor* process = nullptr;
    ProcessTable* process_metadata = nullptr;
    Channel* channel = nullptr;
    // --- debugging ---

    //        int lct = 0;
    //        int mon = -1, val = 82;
    //        int braddr = 0;
    //        bool watch = false, breakat = false;
    // ---------
    ExLocal exec{};

    // memset(&el, 0, sizeof(ExLocal));
    // el.il = il;
    exec.log10 = ln10;
    // dumpPDES(CURPR);
    do {
      if (interp->PS != PS::BREAK) {
        if ((!interp->NOSWITCH &&
             current_process->TIME >= interp->CLOCK) ||
            current_process->STATE != State::RUNNING) {
          //                    fprintf(STANDARD_OUTPUT, "state %s noswitch %d time %.1f
          //                    clock %.1f",
          //                            nameState(CURPR->STATE), il->NOSWITCH,
          //                            CURPR->TIME, il->CLOCK);
          SLICE(interp);
          current_process = interp->CURPR;
          //                    fprintf(STANDARD_OUTPUT, " new process %d processor %d\n",
          //                    CURPR->PID, CURPR->PROCESSOR);
        }
      }

      if (interp->PS == PS::FIN ||
          interp->PS == PS::DEAD) {
        // goto label_999;
        continue;
      }

      exec.IR = code.at(current_process->PC);
      current_process->PC++;
      TIMEINC(interp, 1, "exe1");

      if (interp->NUMBRK > 0 && interp->MAXSTEPS < 0 &&
          !interp->RESTART) {
        for (int i = 1; i <= BRKMAX; i++) {
          if (interp->BRKTAB[i] == current_process->PC - 1)
            interp->PS = PS::BREAK;
        }
      }

      interp->RESTART = false;

      if (interp->MAXSTEPS > 0 &&
          current_process == interp->STEPROC) {
        line_count = current_process->PC - 1;
        if (line_count < interp->STARTLOC ||
            line_count > interp->ENDLOC) {
          interp->MAXSTEPS--;
          interp->STARTLOC = line_count;
          interp->ENDLOC = LOCATION[GETLNUM(line_count) + 1] - 1;
          if (interp->MAXSTEPS == 0)
            interp->PS = PS::BREAK;
        }
      }

      if (interp->PROFILEON) {
        if (interp->CLOCK >= interp->PROTIME) {
          if (interp->PROLINECNT == 0) {
            std::cout << "TIME: "
              << std::round(interp->PROTIME - interp->PROSTEP)
              << '\n';
            if (interp->FIRSTPROC < 10)
              std::cout << interp->FIRSTPROC;
            else
              std::cout << " ";
            for (int i = 1; i <= 4 - interp->FIRSTPROC % 5; i++) {
              if (interp->FIRSTPROC + i < 10 &&
                  interp->FIRSTPROC + i <= interp->LASTPROC)
                std::cout << std::setw(2) << interp->FIRSTPROC + i;
              else
                std::cout << "  ";
            }
            for (int i = interp->FIRSTPROC + 1;
                 i <= interp->LASTPROC;
                 i++) {
              if (i % 5 == 0) {
                if (i < 10)
                  std::cout << std::setw(2) << i << "         ";
                else
                  std::cout << i << "       ";
              }
            }
            std::cout << std::endl;
          }
          for (int i = interp->FIRSTPROC; i <= interp->LASTPROC;
               i++) {
            interp->USAGE =
              static_cast<float>(interp->PROCTAB[i].PROTIME / interp->PROSTEP);
            if (interp->PROCTAB[i].STATUS ==
                Status::NEVERUSED)
              CH = ' ';
            else if (interp->USAGE < 0.25)
              CH = '.';
            else if (interp->USAGE < 0.5)
              CH = '-';
            else if (interp->USAGE < 0.75)
              CH = '+';
            else
              CH = '*';
            std::cout << CH << " ";
            interp->PROCTAB[i].PROTIME = 0;
          }
          std::cout << std::endl;
          interp->PROLINECNT = (interp->PROLINECNT + 1) % 20;
          interp->PROTIME += interp->PROSTEP;
        }
      }

      if (interp->ALARMON && interp->ALARMENABLED) {
        if (interp->CLOCK >= interp->ALARMTIME) {
          std::cout << std::endl;
          std::cout << "Alarm Went Off at Time "
            << std::round(interp->ALARMTIME) << std::endl;
          if (interp->CLOCK > interp->ALARMTIME + 2 * TIMESTEP) {
            std::cout << "Current Time is " << std::round(interp->CLOCK)
              << std::endl;
          }
          interp->PS = PS::BREAK;
          interp->ALARMENABLED = false;
        }
      }

      if (interp->PS == PS::BREAK) {
        current_process->PC--;
        TIMEINC(interp, -1, "exe2");
        interp->BLINE = GETLNUM(current_process->PC);
        //                std::cout << std::endl;
        //                std::cout << "Break At " << il->BLINE << std::endl;
        std::print("\nBreak At %d", interp->BLINE);
        exec.H1 = current_process->B;
        interp->INX = interp->S[current_process->B + 4];
        while (TAB[interp->INX].name[0] == '*') {
          exec.H1 = interp->S[exec.H1 + 3];
          interp->INX = interp->S[exec.H1 + 4];
        }
        if (exec.H1 == 0) std::println("\tIn Function {}", PROGNAME);
        else std::println("\tn Function {}", TAB[interp->INX].name);
        std::cout << "Process Number  " << current_process->PID << std::endl;
        interp->STEPROC = current_process;
        if (interp->MAXSTEPS == 0) {
          double R1 = current_process->TIME - interp->STEPTIME;
          if (R1 > 0) {
            exec.H2 = static_cast<int>(std::round(
              (current_process->VIRTUALTIME - interp->VIRSTEPTIME) / R1 * 100.0
            ));
          } else
            exec.H2 = 0;
          if (exec.H2 > 100)
            exec.H2 = 100;
          std::cout << "Step Time is " << std::round(R1) <<
            ".  Process running "
            << exec.H2 << " percent." << std::endl;
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
          std::print(
            "proc {:3d} stk {:5d} [{:5d}] ",
            current_process->PROCESSOR,
            current_process->T,
            interp->S[current_process->T]
          );
          dumpInst(current_process->PC - 1);
          //                    }
        }
        //                if (CURPR->PID == 1)
        //                    dumpInst(CURPR->PC - 1);
        //                if (++lct > 2000000)
        //                    throw std::exception();
        switch (exec.IR.F) {
          case 0:
            // push DISPLAY[op1]+op2 (stack frame location of variable op2)
            current_process->T++;
            if (current_process->T > current_process->STACKSIZE)
              interp->PS = PS::STKCHK;
            else {
              interp->S[current_process->T] =
                current_process->DISPLAY[exec.IR.X] + exec.IR.Y;
              //                            fprintf(STANDARD_OUTPUT, "%3d: %d %d,%d %s %s\n",
              //                            CURPR->PC - 1,
              //                                el.IR.F, el.IR.X, el.IR.Y,
              //                                opcodes[el.IR.F], lookupSym(el.IR.X,
              //                                el.IR.Y));
            }
            break;
          case 1: {
            current_process->T++;
            if (current_process->T > current_process->STACKSIZE)
              interp->PS = PS::STKCHK;
            else {
              exec.H1 = current_process->DISPLAY[exec.IR.X] + exec.
                        IR.Y;
              if (topology != Symbol::SHAREDSY)
                TESTVAR(interp, exec.H1);
              if (interp->NUMTRACE > 0)
                CHKVAR(interp, exec.H1);
              interp->S[current_process->T] = interp->S[exec.
                H1];
              if (interp->S[current_process->T] == RTAG) {
                interp->RS[current_process->T] =
                  interp->RS[exec.H1];
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
            if (current_process->T > current_process->STACKSIZE)
              interp->PS = PS::STKCHK;
            else {
              exec.H1 =
                interp
                ->S[current_process->DISPLAY[exec.IR.X] + exec.IR.Y];
              if (topology != Symbol::SHAREDSY)
                TESTVAR(interp, exec.H1);
              if (interp->NUMTRACE > 0)
                CHKVAR(interp, exec.H1);
              interp->S[current_process->T] = interp->S[exec.
                H1];
              if (interp->S[current_process->T] == RTAG) {
                interp->RS[current_process->T] =
                  interp->RS[exec.H1];
              }
            }
            break;
          }
          case 3: {
            exec.H1 = exec.IR.Y;
            exec.H2 = exec.IR.X;
            exec.H3 = current_process->B;
            do {
              current_process->DISPLAY[exec.H1] = exec.H3;
              exec.H1--;
              exec.H3 = interp->S[exec.H3 + 2];
            } while (exec.H1 != exec.H2);
            break;
          }
          case 4: {
            current_process->NUMCHILDREN = 0;
            current_process->SEQON = false;
            break;
          }
          case 5: {
            if (current_process->NUMCHILDREN > 0) {
              current_process->STATE = State::BLOCKED;
              interp->PROCTAB[current_process->PROCESSOR].RUNPROC =
                nullptr;
            }
            current_process->SEQON = true;
            break;
          }
          case 6: {
            // noop
            break;
          }
          case 7: {
            current_process->PC = exec.IR.Y;
            current_process->SEQON = true;
            break;
          }
          case 8: {
            switch (exec.IR.Y) {
              case 10: {
                interp->R1 = interp->RS[current_process->T];
                if (interp->R1 >= 0 ||
                    interp->R1 == (int)interp->R1)
                  interp->S[current_process->T] = (int)interp->R1;
                else {
                  interp->S[current_process->T] =
                    (int)interp->R1 - 1; // ?? should go toward 0
                }
                if (abs(interp->S[current_process->T]) > NMAX)
                  interp->PS = PS::INTCHK;
                break;
              }
              case 19: {
                current_process->T++;
                if (current_process->T > current_process->STACKSIZE)
                  interp->PS = PS::STKCHK;
                else {
                  interp->S[current_process->T] = current_process->
                    PROCESSOR;
                }
                break;
              }
              case 20: {
                current_process->T++;
                if (current_process->T > current_process->STACKSIZE)
                  interp->PS = PS::STKCHK;
                else {
                  interp->S[current_process->T] = RTAG;
                  interp->RS[current_process->T] = current_process->TIME;
                }
                break;
              }
              case 21: {
                current_process->T++;
                if (current_process->T > current_process->STACKSIZE)
                  interp->PS = PS::STKCHK;
                else {
                  interp->S[current_process->T] = RTAG;
                  interp->RS[current_process->T] = interp->SEQTIME;
                }
                break;
              }
              case 22: {
                current_process->T++;
                if (current_process->T > current_process->STACKSIZE)
                  interp->PS = PS::STKCHK;
                else
                  interp->S[current_process->T] = current_process->PID;
                break;
              }
              case 23: {
                current_process->T++;
                if (current_process->T > current_process->STACKSIZE)
                  interp->PS = PS::STKCHK;
                else
                  interp->S[current_process->T] = 10;
                break;
              }
            }
            break;
          }
          case 9: {
            if (current_process->PID == 0) {
              for (exec.I = 1; exec.I <= highest_processor; exec.I
                   ++) {
                process = static_cast<ProcessDescriptor*>(calloc(
                  1,
                  sizeof(ProcessDescriptor)
                ));
                process->PC = 0;
                process->PID = interp->NEXTID++;
                if (interp->NEXTID > PIDMAX)
                  interp->PS = PS::PROCCHK;
                process->VIRTUALTIME = 0;
                process->DISPLAY.swap(current_process->DISPLAY);
                interp->PTEMP =
                  static_cast<ActiveProcess*>(calloc(1, sizeof(ActiveProcess)));
                interp->PTEMP->PDES = process;
                interp->PTEMP->NEXT = interp->ACPTAIL->NEXT;
                interp->ACPTAIL->NEXT = interp->PTEMP;
                interp->ACPTAIL = interp->PTEMP;
                exec.H1 = FINDFRAME(interp, interp->STKMAIN);
                if ((debug & DBGPROC) != 0) {
                  std::println(
                    "opc {} findframe {} length {}, response {}",
                    exec.IR.F,
                    current_process->PID,
                    interp->STKMAIN,
                    exec.H1
                  );
                }
                if (exec.H1 > 0) {
                  process->T = exec.H1 + BTAB[2].VSIZE - 1;
                  process->STACKSIZE = exec.H1 + interp->STKMAIN - 1;
                  process->B = exec.H1;
                  process->BASE = exec.H1;
                  for (exec.J = exec.H1;
                       exec.J <= exec.H1 + interp->STKMAIN - 1;
                       exec.J++) {
                    interp->S[exec.J] = 0;
                    interp->SLOCATION[exec.J] = exec.I;
                    interp->RS[exec.I] = 0.0;
                  }
                  for (exec.J = exec.H1;
                       exec.J <= exec.H1 + BASESIZE - 1; exec.J++) {
                    interp->STARTMEM[exec.I] =
                      -interp->STARTMEM[exec.I];
                  }
                  interp->S[exec.H1 + 1] = 0;
                  interp->S[exec.H1 + 2] = 0;
                  interp->S[exec.H1 + 3] = -1;
                  interp->S[exec.H1 + 4] = BTAB[1].LAST;
                  interp->S[exec.H1 + 5] = 1;
                  interp->S[exec.H1 + 6] = interp->STKMAIN;
                  process->DISPLAY[1] = exec.H1;
                }
                process->TIME = current_process->TIME;
                process->STATE = State::READY;
                process->FORLEVEL = current_process->FORLEVEL;
                process->READSTATUS = ReadStatus::NONE;
                process->FORKCOUNT = 1;
                process->MAXFORKTIME = 0;
                process->JOINSEM = 0;
                process->PARENT = current_process;
                process->PRIORITY = PRIORITY::LOW;
                process->ALTPROC = -1;
                process->SEQON = true;
                process->GROUPREP = false;
                process->PROCESSOR = exec.I;
                if (interp->PROCTAB[exec.I].STATUS ==
                    Status::NEVERUSED)
                  interp->USEDPROCS++;
                interp->PROCTAB[exec.I].STATUS =
                  Status::FULL;
                interp->PROCTAB[exec.I].NUMPROC++;
                exec.J = 1;
                while (process->DISPLAY[exec.J] != -1) {
                  interp->S[process->DISPLAY[exec.J] + 5]++;
                  if (debug & DBGRELEASE) {
                    std::println(
                      "{} ref ct {} ct={}",
                      exec.IR.F,
                      current_process->PID,
                      interp->S[process->DISPLAY[exec.J] + 5]
                    );
                  }
                  exec.J++;
                }
                if (debug & DBGPROC) {
                  std::println(
                    "opc {} newproc pid {}",
                    exec.IR.F,
                    process->PID
                  );
                }
                current_process->FORKCOUNT += 1;
              }
              current_process->PC = current_process->PC + 3;
            }
            break;
          }
          case 10: {
            // jmp op2
            current_process->PC = exec.IR.Y;
            break;
          }
          case 11: // if0 pop,->op2
            if (interp->S[current_process->T] == 0)
              current_process->PC = exec.IR.Y;
            current_process->T--;
            break;
          case 12: {
            exec.H1 = interp->S[current_process->T];
            current_process->T--;
            exec.H2 = exec.IR.Y;
            exec.H3 = 0;
            do {
              if (code[exec.H2].F != 13) {
                exec.H3 = 1;
                interp->PS = PS::CASCHK;
              } else if (code[exec.H2].X == -1 ||
                         code[exec.H2].Y == exec.H1) {
                exec.H3 = 1;
                current_process->PC = code[exec.H2 + 1].Y;
              } else
                exec.H2 = exec.H2 + 2;
            } while (exec.H3 == 0);
            break;
          }
          case 13: {
            exec.H1 = exec.IR.X;
            exec.H2 = exec.IR.Y;
            exec.H3 = FINDFRAME(interp, exec.H2 + 1);
            if (debug & DBGPROC) {
              std::println(
                "opc {} findframe {} length {}, response {}",
                exec.IR.F,
                current_process->PID,
                exec.H2 + 1,
                exec.H3
              );
            }
            if (exec.H3 < 0)
              interp->PS = PS::STKCHK;
            else {
              for (exec.I = 0; exec.I < exec.H2; exec.I++) {
                interp->S[exec.H3 + exec.I] =
                  static_cast<unsigned char>(STAB[exec.H1 + exec.I]);
              }
              interp->S[exec.H3 + exec.H2] = 0;
              current_process->T++;
              if (current_process->T > current_process->STACKSIZE)
                interp->PS = PS::STKCHK;
              else
                interp->S[current_process->T] = exec.H3;
            }
            break;
          }
          case 14: {
            // push from stack top stack frame loc
            exec.H1 = interp->S[current_process->T];
            //                        if (CURPR->PROCESSOR == 0)
            //                            fprintf(STANDARD_OUTPUT, "14: proc %d pc %d stack
            //                            %d\n", CURPR->PROCESSOR, CURPR->PC,
            //                            CURPR->T);
            current_process->T++;
            if (current_process->T > current_process->STACKSIZE)
              interp->PS = PS::STKCHK;
            else {
              interp->S[current_process->T] = interp->S[exec.
                H1];
              if (interp->S[current_process->T] == RTAG) {
                interp->RS[current_process->T] =
                  interp->RS[exec.H1];
              }
            }
            break;
          }
          case 15: {
            // add
            current_process->T--;
            interp->S[current_process->T] =
              interp->S[current_process->T] +
              interp->S[current_process->T + 1];
            break;
          }
          case 18: {
            // IR.Y is index of a function in TAB[] symbol table
            // get stack frame for block execution callblk op
            if (TAB[exec.IR.Y].address == 0)
              interp->PS = PS::FUNCCHK;
            else if (TAB[exec.IR.Y].address > 0) {
              exec.H1 = BTAB[TAB[exec.IR.Y].reference].VSIZE + WORKSIZE;
              exec.H2 = FINDFRAME(interp, exec.H1);
              if (debug & DBGPROC) {
                std::println(
                  "opc {} findframe {} length {}, response {}",
                  exec.IR.F,
                  current_process->PID,
                  exec.H1,
                  exec.H2
                );
              }
              if (exec.H2 >= 0) {
                interp->S[exec.H2 + 7] = current_process->T;
                current_process->T = exec.H2 - 1;
                current_process->STACKSIZE = exec.H1 + exec.H2 - 1;
                current_process->T += 8;
                interp->S[current_process->T - 4] = exec.H1 - 1;
                interp->S[current_process->T - 3] = exec.IR.Y;
                for (exec.I = exec.H2;
                     exec.I <= exec.H2 + BASESIZE - 1; exec.I++) {
                  interp->STARTMEM[exec.I] =
                    -interp->STARTMEM[exec.I];
                }
              }
            }
            break;
          }
          case 19: {
            if (TAB[exec.IR.X].address < 0) {
              EXECLIB(
                interp,
                &exec,
                current_process,
                -TAB[exec.IR.X].address
              );
            } else {
              exec.H1 = current_process->T -
                        exec.IR.Y; // base of frame acquired by 18
              exec.H2 = interp->S[exec.H1 + 4];
              // function TAB index
              exec.H3 = TAB[exec.H2].LEV; // TAB[stk + 4].LEV
              current_process->DISPLAY[exec.H3 + 1] =
                exec.H1; // DISPLAY[H3 + 1]
              for (exec.I = exec.H3 + 2; exec.I <= LMAX;
                   exec.I++) {
                // clear DISPLAY to top w/-1
                current_process->DISPLAY[exec.I] = -1;
              }
              interp->S[exec.H1 + 6] =
                interp->S[exec.H1 + 3] + 1;
              exec.H4 = interp->S[exec.H1 + 3] + exec.H1;
              interp->S[exec.H1 + 1] = current_process->PC;
              // return addr
              interp->S[exec.H1 + 2] =
                current_process->DISPLAY[exec.H3];
              interp->S[exec.H1 + 3] = current_process->B;
              interp->S[exec.H1 + 5] = 1;
              for (exec.H3 = current_process->T + 1;
                   exec.H3 <= exec.H4;
                   exec.H3++) {
                interp->S[exec.H3] = 0;
                interp->RS[exec.H3] = 0.0;
              }
              for (exec.H3 = exec.H1; exec.H3 <= exec.H4;
                   exec.H3++) {
                interp->SLOCATION[exec.H3] = current_process->
                  PROCESSOR;
              }
              current_process->B = exec.H1;
              current_process->T = exec.H4 - WORKSIZE;
              current_process->PC =
                TAB[exec.H2].address; // jump to block address
              TIMEINC(interp, 3, "cs19");
            }
            break;
          }
          case 20: {
            // swap
            exec.H1 = interp->S[current_process->T];
            exec.RH1 = interp->RS[current_process->T];
            interp->S[current_process->T] =
              interp->S[current_process->T - 1];
            interp->RS[current_process->T] =
              interp->RS[current_process->T - 1];
            interp->S[current_process->T - 1] = exec.H1;
            interp->RS[current_process->T - 1] = exec.RH1;
            break;
          }
          case 21: {
            // load array Y[T]
            // op2 is array table index
            // stack top is array subscript
            // top - 1 is array base in stack
            // pop stack
            // replace top with stack element at array base plus subscript
            exec.H1 = exec.IR.Y;
            exec.H2 = ATAB[exec.H1].LOW;
            exec.H3 = interp->S[current_process->T];
            // array subscript
            if (exec.H3 < exec.H2 ||
                (exec.H3 > ATAB[exec.H1].HIGH &&
                 ATAB[exec.H1].HIGH > 0))
              interp->PS = PS::INXCHK;
            else {
              current_process->T--;
              exec.H4 =
                interp->S[current_process->T]; // added for debugging
              interp->S[current_process->T] =
                interp->S[current_process->T] +
                (exec.H3 - exec.H2) * ATAB[exec.H1].ELSIZE;
              // fprintf(STANDARD_OUTPUT, "\nindex %d array stack base %d stack loc %d\n",
              // el.H3, el.H4, il->S[CURPR->T]);
            }
            break;
          }
          case 22: {
            exec.H1 = interp->S[current_process->T];
            if (exec.H1 <= 0 || exec.H1 >= STMAX)
              interp->PS = PS::REFCHK;
            else {
              current_process->T--;
              exec.H2 = exec.IR.Y + current_process->T;
              if (topology != Symbol::SHAREDSY)
                TESTVAR(interp, exec.H1);
              if (exec.H2 > current_process->STACKSIZE)
                interp->PS = PS::STKCHK;
              else {
                while (current_process->T < exec.H2) {
                  current_process->T++;
                  if (interp->NUMTRACE > 0)
                    CHKVAR(interp, exec.H1);
                  interp->S[current_process->T] =
                    interp->S[exec.H1];
                  if (interp->S[exec.H1] == RTAG) {
                    interp->RS[current_process->T] =
                      interp->RS[exec.H1];
                  }
                  exec.H1++;
                }
              }
            }
            break;
          }
          case 23: {
            exec.H1 = interp->S[current_process->T - 1];
            exec.H2 = interp->S[current_process->T];
            if (exec.H1 <= 0 || exec.H2 <= 0 ||
                exec.H1 >= STMAX || exec.H2 >= STMAX)
              interp->PS = PS::REFCHK;
            else {
              exec.H3 = exec.H1 + exec.IR.X;
              if (topology != Symbol::SHAREDSY) {
                TESTVAR(interp, exec.H2);
                if (exec.IR.Y == 0)
                  TESTVAR(interp, exec.H1);
                else if (interp->CONGESTION) {
                  exec.H4 = COMMDELAY(
                    interp,
                    current_process->PROCESSOR,
                    interp->SLOCATION[exec.H1],
                    exec.IR.X
                  );
                }
              }
              while (exec.H1 < exec.H3) {
                if (interp->NUMTRACE > 0) {
                  CHKVAR(interp, exec.H1);
                  CHKVAR(interp, exec.H2);
                }
                if (interp->STARTMEM[exec.H1] <= 0)
                  interp->PS = PS::REFCHK;
                interp->S[exec.H1] = interp->S[exec.H2];
                if (interp->S[exec.H2] == RTAG)
                  interp->RS[exec.H1] = interp->RS[exec.H2];
                exec.H1++;
                exec.H2++;
              }
              TIMEINC(interp, exec.IR.X / 5, "cs23");
              interp->S[current_process->T - 1] =
                interp->S[current_process->T];
              current_process->T--;
            }
            break;
          }
          case 24: {
            // push imm op2
            current_process->T++;
            if (current_process->T > current_process->STACKSIZE)
              interp->PS = PS::STKCHK;
            else
              interp->S[current_process->T] = (int)exec.IR.Y;
            break;
          }
          case 26: {
            // convreal
            interp->RS[current_process->T] =
              interp->S[current_process->T];
            interp->S[current_process->T] = RTAG;
            break;
          }
          case 27: {
            int IORESULT = 0;
#if defined(__APPLE__)
        *__error() = 0;
#elif defined(__linux__) || defined(_WIN32)
            errno = 0;
#endif
            exec.H1 = interp->S[current_process->T];
            if (topology != Symbol::SHAREDSY)
              TESTVAR(interp, exec.H1);
            if (interp->NUMTRACE > 0)
              CHKVAR(interp, exec.H1);
            if (INPUTFILE_FLAG) {
              if (INPUT.eof())
                interp->PS = PS::REDCHK;
            } else if (cout.eof())
              interp->PS = PS::REDCHK;
            if (interp->PS != PS::REDCHK) {
              if (!INPUTFILE_FLAG) {
                switch (exec.IR.Y) {
                  case 1: {
                    interp->S[exec.H1] = RTAG;
                    // INPUT >> il->RS[el.H1];
                    // fscanf(STANDARD_INPUT, "%lf", &il->RS[el.H1]);
                    fscanf(STANDARD_INPUT, "%s", exec.buf.str().data());
                    interp->RS[exec.H1] =
                      strtod(exec.buf.str().data(), nullptr);
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
                    fscanf(STANDARD_INPUT, "%s", exec.buf.str().data());
                    interp->S[exec.H1] = static_cast<int>(
                      strtol(exec.buf.str().data(), nullptr, 10));
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
                    interp->S[exec.H1] = fgetc(STANDARD_INPUT);
                    if (0 > interp->S[exec.H1])
                      IORESULT = -1;
                    break;
                  }
                  default: ;
                }
              } else {
                switch (exec.IR.Y) {
                  case 1: {
                    interp->S[exec.H1] = RTAG;
                    // INPUT >> il->RS[el.H1];
                    // fscanf(INPUT, "%lf", &il->RS[el.H1]);
                    INPUT >> exec.buf;
                    interp->RS[exec.H1] = stod(exec.buf);
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
                    INPUT >> exec.buf;
                    interp->S[exec.H1] = static_cast<int>(std::stol(exec.buf));
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
                    interp->S[exec.H1] = fgetc(INPUT);
                    if (0 > interp->S[exec.H1])
                      IORESULT = -1;
                    break;
                  }
                  default: ;
                }
              }
              if (IORESULT != 0)
                interp->PS = PS::DATACHK;
            }
            current_process->T--;
            break;
          }
          case 28: {
            // write from string table
            exec.H1 = interp->S[current_process->T];
            exec.H2 = exec.IR.Y;
            current_process->T--;
            interp->CHRCNT = interp->CHRCNT + exec.H1;
            if (interp->CHRCNT > LINELENG)
              interp->PS = PS::LNGCHK;
            do {
              if (!OUTPUTFILE_FLAG) cout << STAB[exec.H2];
              else OUTPUT << STAB[exec.H2];
              exec.H1--;
              exec.H2++;
              if (STAB[exec.H2] == static_cast<char>(10) || STAB[exec.H2] ==
                  static_cast<char>(13))
                interp->CHRCNT = 0;
            } while (exec.H1 != 0);
            break;
          }
          case 29: {
            //  outwidth output formatted with WIDTH
            if (interp->COUTWIDTH <= 0) {
              interp->CHRCNT =
                interp->CHRCNT + interp->FLD[exec.IR.Y];
              if (interp->CHRCNT > LINELENG)
                interp->PS = PS::LNGCHK;
              else {
                if (!OUTPUTFILE_FLAG) {
                  switch (exec.IR.Y) {
                    case 2: {
                      std::print(
                        "{:*d}",
                        interp->FLD[2],
                        interp->S[current_process->T]
                      );
                      break;
                    }
                    case 3: {
                      std::print(
                        "{:*s}",
                        interp->FLD[3],
                        ITOB(interp->S[current_process->T]) ? "TRUE" : "FALSE"
                      );
                      break;
                    }
                    case 4: {
                      if (interp->S[current_process->T] < CHARL ||
                          interp->S[current_process->T] > CHARH)
                        interp->PS = PS::CHRCHK;
                      else {
                        std::print(
                          "{:*c}",
                          interp->FLD[4],
                          static_cast<char>(interp->S[current_process->T])
                        );
                        if (interp->S[current_process->T] == 10 ||
                            interp->S[current_process->T] == 13)
                          interp->CHRCNT = 0;
                      }
                      break;
                    }
                    default: break;
                  }
                } else {
                  switch (exec.IR.Y) {
                    case 2: {
                      std::print(
                        "{:*d}",
                        interp->FLD[2],
                        interp->S[current_process->T]
                      );
                      break;
                    }
                    case 3: {
                      std::print(
                        "{:*s}",
                        interp->FLD[3],
                        ITOB(interp->S[current_process->T]) ? "true" : "false"
                      );
                      break;
                    }
                    case 4: {
                      if (interp->S[current_process->T] < CHARL ||
                          interp->S[current_process->T] > CHARH)
                        interp->PS = PS::CHRCHK;
                      else {
                        fprintf(
                          OUTPUT,
                          "%*c",
                          interp->FLD[4],
                          static_cast<char>(interp->S[current_process->T])
                        );
                        if (interp->S[current_process->T] == 10 ||
                            interp->S[current_process->T] == 13)
                          interp->CHRCNT = 0;
                      }
                      break;
                    }
                    default: break;
                  }
                }
              }
            } else {
              interp->CHRCNT =
                interp->CHRCNT + interp->COUTWIDTH;
              exec.H1 = interp->S[current_process->T];
              if (interp->CHRCNT > LINELENG)
                interp->PS = PS::LNGCHK;
              else {
                if (!OUTPUTFILE_FLAG) {
                  switch (exec.IR.Y) {
                    case 2: {
                      // std::cout << el.H1;
                      fprintf(
                        STDOUT,
                        "%*d",
                        interp->COUTWIDTH,
                        exec.H1
                      );
                      break;
                    }
                    case 3: {
                      // std::cout << ITOB(el.H1);
                      fprintf(
                        STDOUT,
                        "%*s",
                        interp->COUTWIDTH,
                        ITOB(exec.H1) ? "true" : "false"
                      );
                      break;
                    }
                    case 4: {
                      if (exec.H1 < CHARL || exec.H1 > CHARH)
                        interp->PS = PS::CHRCHK;
                      else {
                        // std::cout << char(el.H1);
                        fprintf(
                          STDOUT,
                          "%*c",
                          interp->COUTWIDTH,
                          (char)exec.H1
                        );
                        if (exec.H1 == 10 || exec.H1 == 13)
                          interp->CHRCNT = 0;
                      }
                      break;
                    }
                  }
                } else {
                  switch (exec.IR.Y) {
                    case 2: {
                      fprintf(
                        OUTPUT,
                        "%*d",
                        interp->COUTWIDTH,
                        exec.H1
                      );
                      break;
                    }
                    case 3: {
                      fprintf(
                        OUTPUT,
                        "%*s",
                        interp->COUTWIDTH,
                        ITOB(exec.H1) ? "true" : "false"
                      );
                      break;
                    }
                    case 4: {
                      if (exec.H1 < CHARL || exec.H1 > CHARH)
                        interp->PS = PS::CHRCHK;
                      else {
                        fprintf(
                          OUTPUT,
                          "%*c",
                          interp->COUTWIDTH,
                          (char)exec.H1
                        );
                        if (exec.H1 == 10 || exec.H1 == 13)
                          interp->CHRCNT = 0;
                      }
                      break;
                    }
                  }
                }
              }
            }
            current_process->T--;
            interp->COUTWIDTH = -1;
            break;
          }
          case 30: {
            // pop and set out width or out prec
            if (exec.IR.Y == 1)
              interp->COUTWIDTH = interp->S[current_process->T];
            else
              interp->COUTPREC = interp->S[current_process->T];
            current_process->T = current_process->T - 1;
            break;
          }
          case 31: {
            // main  end
            if (current_process->FORKCOUNT > 1) {
              current_process->STATE = State::BLOCKED;
              current_process->PC = current_process->PC - 1;
              interp->PROCTAB[current_process->PROCESSOR].RUNPROC =
                nullptr;
              current_process->FORKCOUNT = current_process->FORKCOUNT - 1;
            } else {
              interp->PS = PS::FIN;
              if (debug & DBGPROC) {
                fprintf(
                  STDOUT,
                  "opc %d terminated pid %d\n",
                  exec.IR.F,
                  current_process->PID
                );
                if (current_process->PID == 0)
                  unreleased(interp);
              }
              current_process->STATE = State::TERMINATED;
            }
            break;
          }
          case 32: {
            if (interp->S[current_process->B + 5] == 1) {
              if (debug & DBGRELEASE) {
                fprintf(
                  STDOUT,
                  "%d release %d fm=%d ln=%d\n",
                  exec.IR.F,
                  current_process->PID,
                  current_process->B,
                  interp->S[current_process->B + 6]
                );
              }
              RELEASE(
                interp,
                current_process->B,
                interp->S[current_process->B + 6]
              );
            } else {
              interp->S[current_process->B + 5] -= 1;
              if (debug & DBGRELEASE) {
                fprintf(
                  STDOUT,
                  "%d ref ct %d ct=%d\n",
                  exec.IR.F,
                  current_process->PID,
                  interp->S[current_process->B + 5]
                );
              }
            }
            exec.H1 = TAB[interp->S[current_process->B + 4]].LEV;
            current_process->DISPLAY[exec.H1 + 1] = -1;
            current_process->PC = interp->S[current_process->B + 1];
            current_process->T = interp->S[current_process->B + 7];
            current_process->B = interp->S[current_process->B + 3];
            if (current_process->T >= current_process->B - 1 &&
                current_process->T <
                current_process->B + interp->S[
                  current_process->B + 6]) {
              current_process->STACKSIZE =
                current_process->B + interp->S[current_process->B + 6] -
                1;
            } else
              current_process->STACKSIZE = current_process->BASE + WORKSIZE - 1;
            break;
          }
          case 33: {
            // release block resources ?
            exec.H1 = TAB[interp->S[current_process->B + 4]].LEV;
            exec.H2 = current_process->T;
            current_process->DISPLAY[exec.H1 + 1] = -1;
            if (interp->S[current_process->B + 5] == 1) {
              if (debug & DBGRELEASE) {
                // fprintf(STDOUT, "release base %d, length %d\n", CURPR->B,
                // il->S[CURPR->B+6]);
                fprintf(
                  STDOUT,
                  "%d release %d fm=%d ln=%d\n",
                  exec.IR.F,
                  current_process->PID,
                  current_process->B,
                  interp->S[current_process->B + 6]
                );
              }
              RELEASE(
                interp,
                current_process->B,
                interp->S[current_process->B + 6]
              );
            } else {
              // fprintf(STDOUT, "release adjacent %d\n", il->S[CURPR->B+6]);
              interp->S[current_process->B + 5] =
                interp->S[current_process->B + 5] - 1;
              if (debug & DBGRELEASE) {
                fprintf(
                  STDOUT,
                  "%d ref ct %d ct=%d\n",
                  exec.IR.F,
                  current_process->PID,
                  interp->S[current_process->B + 5]
                );
              }
            }
            current_process->T = interp->S[current_process->B + 7] + 1;
            interp->S[current_process->T] = interp->S[exec.H2];
            if (interp->S[current_process->T] == RTAG) {
              interp->RS[current_process->T] = interp->RS[exec.
                H2];
            }
            current_process->PC = interp->S[current_process->B + 1];
            current_process->B = interp->S[current_process->B + 3];
            if (current_process->T >= current_process->B - 1 &&
                current_process->T <
                current_process->B + interp->S[
                  current_process->B + 6]) {
              current_process->STACKSIZE =
                current_process->B + interp->S[current_process->B + 6] -
                1;
            } else
              current_process->STACKSIZE = current_process->BASE + WORKSIZE - 1;
            break;
          }
          case 34: {
            // get variable stack location and replace top with value
            // load indirect ->[T] to T
            exec.H1 = interp->S[current_process->T];
            if (exec.H1 <= 0 || exec.H1 >= STMAX)
              interp->PS = PS::REFCHK;
            else {
              if (topology != Symbol::SHAREDSY)
                TESTVAR(interp, exec.H1);
              if (interp->NUMTRACE > 0)
                CHKVAR(interp, exec.H1);
              if (interp->S[exec.H1] == RTAG) {
                interp->RS[current_process->T] =
                  interp->RS[exec.H1];
              }
              interp->S[current_process->T] = interp->S[exec.
                H1];
            }
            break;
          }
          case 35: // not
            interp->S[current_process->T] =
              BTOI(!ITOB(interp->S[current_process->T]));
            break;
          case 36: {
            // negate
            if (interp->S[current_process->T] == RTAG) {
              interp->RS[current_process->T] =
                -interp->RS[current_process->T];
            } else {
              interp->S[current_process->T] =
                -interp->S[current_process->T];
            }
            break;
          }
          case 37: {
            // out real with prec/width or none
            exec.RH1 = interp->RS[current_process->T];
            if (exec.RH1 < 1)
              exec.H2 = 0;
            else {
              exec.H2 = 1 + (int)std::floor(
                          std::log(std::fabs(exec.RH1)) /
                          exec.log10
                        );
            }
            if (interp->COUTWIDTH <= 0)
              exec.H1 = interp->FLD[1];
            else
              exec.H1 = interp->COUTWIDTH;
            if (interp->COUTPREC > 0 &&
                interp->COUTPREC + 3 < exec.H1)
              exec.H1 = interp->COUTPREC + 3;
            interp->CHRCNT = interp->CHRCNT + exec.H1;
            if (interp->CHRCNT > LINELENG)
              interp->PS = PS::LNGCHK;
            else {
              if (!OUTPUTFILE_FLAG) {
                if (interp->COUTPREC <= 0) {
                  // cout << el.RH1;
                  fprintf(STDOUT, "%*f", exec.H1, exec.RH1);
                } else {
                  if (interp->COUTWIDTH <= 0) {
                    // cout << el.RH1 << ":" << il->COUTPREC - el.H2;
                    fprintf(
                      STDOUT,
                      "%*.*f",
                      interp->COUTPREC + 3,
                      interp->COUTPREC - exec.H2,
                      exec.RH1
                    );
                  } else {
                    // cout << el.RH1 << ":" << il->COUTWIDTH << ":" << il->COUTPREC
                    // - el.H2;
                    fprintf(
                      STDOUT,
                      "%*.*f",
                      interp->COUTWIDTH,
                      interp->COUTPREC - exec.H2,
                      exec.RH1
                    );
                  }
                }
              } else {
                if (interp->COUTPREC <= 0) {
                  // OUTPUT << el.RH1;
                  fprintf(OUTPUT, "%*f", exec.H1, exec.RH1);
                } else {
                  if (interp->COUTWIDTH <= 0) {
                    fprintf(
                      OUTPUT,
                      "%*.*f",
                      interp->COUTPREC + 3,
                      interp->COUTPREC - exec.H2,
                      exec.RH1
                    );
                  } else {
                    fprintf(
                      OUTPUT,
                      "%*.*f",
                      interp->COUTWIDTH,
                      interp->COUTPREC - exec.H2,
                      exec.RH1
                    );
                  }
                }
              }
            }
            current_process->T = current_process->T - 1;
            interp->COUTWIDTH = -1;
            break;
          }
          case 38: {
            // get saved DISPLAY (a stack location for a variable) copy top
            // to it
            // [[T-1]] <- [T]
            // [T-1] <- [T]
            // T <- T-1
            // stindstk
            exec.H1 = interp->S[current_process->T - 1];
            if (exec.H1 <= 0 || exec.H1 >= STMAX)
              interp->PS = PS::REFCHK;
            else {
              if (interp->STARTMEM[exec.H1] <= 0)
                interp->PS = PS::REFCHK;
              interp->S[exec.H1] = interp->S[current_process->
                T];
              if (topology != Symbol::SHAREDSY) {
                if (exec.IR.Y == 0)
                  TESTVAR(interp, exec.H1);
                else if (interp->CONGESTION) {
                  exec.H2 = COMMDELAY(
                    interp,
                    current_process->PROCESSOR,
                    interp->SLOCATION[exec.H1],
                    1
                  );
                }
              }
              if (interp->NUMTRACE > 0)
                CHKVAR(interp, exec.H1);
              if (interp->S[current_process->T] == RTAG) {
                interp->RS[exec.H1] =
                  interp->RS[current_process->T];
              }
              interp->S[current_process->T - 1] =
                interp->S[current_process->T];
              if (interp->S[current_process->T] == RTAG) {
                interp->RS[current_process->T - 1] =
                  interp->RS[current_process->T];
              }
              current_process->T = current_process->T - 1;
            }
            break;
          }
          case 45: {
            // pop and compare EQ
            current_process->T = current_process->T - 1;
            if (ISREAL(interp)) {
              interp->S[current_process->T] =
                BTOI(
                  interp->RS[current_process->T] ==
                  interp->RS[current_process->T + 1]
                );
            } else {
              interp->S[current_process->T] =
                BTOI(
                  interp->S[current_process->T] ==
                  interp->S[current_process->T + 1]
                );
            }
            break;
          }
          case 46: {
            // pop and compare NE
            current_process->T = current_process->T - 1;
            if (ISREAL(interp)) {
              interp->S[current_process->T] =
                BTOI(
                  interp->RS[current_process->T] !=
                  interp->RS[current_process->T + 1]
                );
            } else {
              interp->S[current_process->T] =
                BTOI(
                  interp->S[current_process->T] !=
                  interp->S[current_process->T + 1]
                );
            }
            break;
          }
          case 47: {
            // pop, then compare top op top+1, replace top with comparison
            // result
            //  pop and compare LT
            current_process->T = current_process->T - 1;
            if (ISREAL(interp)) {
              interp->S[current_process->T] =
                BTOI(
                  interp->RS[current_process->T] <
                  interp->RS[current_process->T + 1]
                );
            } else {
              interp->S[current_process->T] =
                BTOI(
                  interp->S[current_process->T] <
                  interp->S[current_process->T + 1]
                );
            }
            break;
          }
          case 48: {
            // pop and compare LE
            current_process->T = current_process->T - 1;
            if (ISREAL(interp)) {
              interp->S[current_process->T] =
                BTOI(
                  interp->RS[current_process->T] <=
                  interp->RS[current_process->T + 1]
                );
            } else {
              interp->S[current_process->T] =
                BTOI(
                  interp->S[current_process->T] <=
                  interp->S[current_process->T + 1]
                );
            }
            break;
          }
          case 49: {
            // pop and compare GT
            current_process->T = current_process->T - 1;
            if (ISREAL(interp)) {
              interp->S[current_process->T] =
                BTOI(
                  interp->RS[current_process->T] >
                  interp->RS[current_process->T + 1]
                );
            } else {
              interp->S[current_process->T] =
                BTOI(
                  interp->S[current_process->T] >
                  interp->S[current_process->T + 1]
                );
            }
            break;
          }
          case 50: {
            // pop and compare GE
            current_process->T = current_process->T - 1;
            if (ISREAL(interp)) {
              interp->S[current_process->T] =
                BTOI(
                  interp->RS[current_process->T] >=
                  interp->RS[current_process->T + 1]
                );
            } else {
              interp->S[current_process->T] =
                BTOI(
                  interp->S[current_process->T] >=
                  interp->S[current_process->T + 1]
                );
            }
            break;
          }
          case 51: {
            // pop OR
            current_process->T = current_process->T - 1;
            interp->S[current_process->T] =
              BTOI(
                ITOB(interp->S[current_process->T]) ||
                ITOB(interp->S[current_process->T + 1])
              );
            break;
          }
          case 52: {
            // plus
            current_process->T = current_process->T - 1;
            if (ISREAL(interp)) {
              interp->RS[current_process->T] =
                interp->RS[current_process->T] +
                interp->RS[current_process->T + 1];
              if (interp->RS[current_process->T] != 0) {
                exec.RH1 = log(fabs(interp->RS[current_process->T])) /
                           exec.log10;
                if (exec.RH1 >= 292.5 || exec.RH1 <= -292.5)
                  interp->PS = PS::OVRCHK;
              }
            } else {
              interp->S[current_process->T] =
                interp->S[current_process->T] +
                interp->S[current_process->T + 1];
              if (abs(interp->S[current_process->T]) > NMAX)
                interp->PS = PS::INTCHK;
            }
            break;
          }
          case 53: {
            // minus
            current_process->T = current_process->T - 1;
            if (ISREAL(interp)) {
              interp->RS[current_process->T] =
                interp->RS[current_process->T] -
                interp->RS[current_process->T + 1];
              if (interp->RS[current_process->T] != 0) {
                exec.RH1 = log(fabs(interp->RS[current_process->T])) /
                           exec.log10;
                if (exec.RH1 >= 292.5 || exec.RH1 <= -292.5)
                  interp->PS = PS::OVRCHK;
              }
            } else {
              interp->S[current_process->T] =
                interp->S[current_process->T] -
                interp->S[current_process->T + 1];
              if (abs(interp->S[current_process->T]) > NMAX)
                interp->PS = PS::INTCHK;
            }
            break;
          }
          case 56: {
            // and
            current_process->T = current_process->T - 1;
            interp->S[current_process->T] =
              BTOI(
                ITOB(interp->S[current_process->T]) &&
                ITOB(interp->S[current_process->T + 1])
              );
            break;
          }
          case 57: {
            // times
            current_process->T = current_process->T - 1;
            if (ISREAL(interp)) {
              interp->RS[current_process->T] =
                interp->RS[current_process->T] *
                interp->RS[current_process->T + 1];
              if (interp->RS[current_process->T] != 0) {
                exec.RH1 = log(fabs(interp->RS[current_process->T])) /
                           exec.log10;
                if (exec.RH1 >= 292.5 || exec.RH1 <= -292.5)
                  interp->PS = PS::OVRCHK;
              }
            } else {
              interp->S[current_process->T] =
                interp->S[current_process->T] *
                interp->S[current_process->T + 1];
              if (abs(interp->S[current_process->T]) > NMAX)
                interp->PS = PS::INTCHK;
            }
            break;
          }
          case 58: {
            // int div
            current_process->T = current_process->T - 1;
            if (interp->S[current_process->T + 1] == 0)
              interp->PS = PS::DIVCHK;
            else {
              interp->S[current_process->T] =
                interp->S[current_process->T] /
                interp->S[current_process->T + 1];
            }
            break;
          }
          case 59: {
            // int mod
            current_process->T = current_process->T - 1;
            if (interp->S[current_process->T + 1] == 0)
              interp->PS = PS::DIVCHK;
            else {
              interp->S[current_process->T] =
                interp->S[current_process->T] %
                interp->S[current_process->T + 1];
            }
            break;
          }
          case 62: {
            if (!INPUTFILE_FLAG) {
              if (feof(STANDARD_INPUT))
                interp->PS = PS::REDCHK;
              else
                fgetc(STANDARD_INPUT);
            } else {
              if (feof(INPUT))
                interp->PS = PS::REDCHK;
              else
                fgetc(INPUT);
            }
            break;
          }
          case 63: {
            // write endl
            if (!OUTPUTFILE_FLAG)
              fputc('\n', STDOUT);
            else
              fputc('\n', OUTPUT);
            interp->LNCNT = interp->LNCNT + 1;
            interp->CHRCNT = 0;
            if (interp->LNCNT > LINELIMIT)
              interp->PS = PS::LINCHK;
            break;
          }
          case 64:
          case 65:
          case 71: {
            // [T] is stk loc of channel
            switch (exec.IR.F) {
              case 64: exec.H1 =
                       current_process->DISPLAY[exec.IR.X] + exec.IR.Y;
                break;
              case 65: exec.H1 =
                       interp
                       ->S[current_process->DISPLAY[exec.IR.X] + exec.IR
                           .Y];
                break;
              case 71: exec.H1 = interp->S[current_process->T];
                break;
            }
            exec.H2 = interp->SLOCATION[exec.H1];
            interp->CNUM = interp->S[exec.H1];
            if (debug & DBGRECV) {
              fprintf(
                STDOUT,
                "recv %d pid %d pc %d state %s rdstatus %s chan %d\n",
                exec.IR.F,
                current_process->PID,
                current_process->PC,
                name_state(current_process->STATE),
                name_read_status(current_process->READSTATUS),
                interp->CNUM
              );
            }
            if (interp->NUMTRACE > 0)
              CHKVAR(interp, exec.H1);
            if (interp->CNUM == 0) {
              interp->CNUM =
                FIND(interp); // get and initialize unused channel
              interp->S[exec.H1] =
                interp->CNUM; // store channel in stack loc
            }
            if (current_process->READSTATUS ==
                ReadStatus::NONE) {
              current_process->READSTATUS =
                ReadStatus::ATCHANNEL;
            }
            channel = &interp->CHAN[interp->CNUM];
            // WITH CHAN[CNUM]
            if (!(*channel).MOVED && (*channel).READER < 0) {
              interp->SLOCATION[exec.H1] = current_process->PROCESSOR;
              (*channel).READER = current_process->PID;
            }
            if (topology != Symbol::SHAREDSY)
              TESTVAR(interp, exec.H1); // channel in stk
            if ((*channel).READTIME < interp->CLOCK - TIMESTEP) {
              (*channel).READTIME = interp->CLOCK - TIMESTEP;
            }
            interp->PNT = (*channel).HEAD;
            exec.B1 = (*channel).SEM == 0;
            if (!exec.B1) {
              exec.B1 =
                interp->DATE[interp->PNT] > current_process->TIME;
            }
            if (exec.B1) {
              interp->PTEMP = (ActiveProcess*)calloc(
                1,
                sizeof(ActiveProcess)
              );
              interp->PTEMP->PDES = current_process;
              interp->PTEMP->NEXT = nullptr;
              interp->PROCTAB[current_process->PROCESSOR].RUNPROC =
                nullptr;
              if ((*channel).WAIT == nullptr) {
                exec.H3 = 1;
                (*channel).WAIT = interp->PTEMP;
                if ((*channel).SEM != 0) {
                  current_process->STATE = State::DELAYED;
                  current_process->WAKETIME = interp->DATE[interp->
                    PNT];
                } else
                  current_process->STATE = State::BLOCKED;
              } else {
                interp->RTEMP = (*channel).WAIT;
                while (interp->RTEMP->NEXT != nullptr)
                  interp->RTEMP = interp->RTEMP->NEXT;
                interp->RTEMP->NEXT = interp->PTEMP;
                current_process->STATE = State::BLOCKED;
              }
              current_process->PC = current_process->PC - 1;
              interp->NOSWITCH = false;
            } else {
              if (current_process->READSTATUS !=
                  ReadStatus::HASTICKET) {
                if ((*channel).READTIME > current_process->TIME) {
                  current_process->TIME = (*channel).READTIME;
                  if (debug & DBGTIME)
                    procTime(current_process, 0.0, "case 64,65,71");
                  current_process->READSTATUS =
                    ReadStatus::HASTICKET;
                  current_process->PC = current_process->PC - 1;
                  interp->NOSWITCH = false;
                  (*channel).READTIME = (*channel).READTIME + CHANTIME;
                  goto L699;
                } else
                  (*channel).READTIME = (*channel).READTIME + CHANTIME;
              }
              TIMEINC(interp, CHANTIME, "cs64");
              (*channel).SEM = (*channel).SEM - 1;
              (*channel).HEAD = interp->LINK[interp->PNT];
              if (exec.IR.F == 64 || exec.IR.F == 65)
                current_process->T = current_process->T + 1;
              if (current_process->T > current_process->STACKSIZE)
                interp->PS = PS::STKCHK;
              else {
                interp->S[current_process->T] =
                  interp->VALUE[interp->PNT];
                if (interp->S[current_process->T] == RTAG) {
                  interp->RS[current_process->T] =
                    interp->RVALUE[interp->PNT];
                }
              }
              interp->LINK[interp->PNT] = interp->FREE;
              interp->FREE = interp->PNT;
              current_process->READSTATUS = ReadStatus::NONE;
              if ((*channel).WAIT != nullptr) {
                if ((*channel).WAIT->PDES ==
                    current_process) {
                  // remove CURPR from wait list
                  interp->PTEMP = (*channel).WAIT;
                  (*channel).WAIT = (*channel).WAIT->NEXT;
                  if (debug & DBGPROC) {
                    fprintf(
                      STDOUT,
                      "remove pid %d from wait list\n",
                      current_process->PID
                    );
                  }
                  std::free(interp->PTEMP); // free ActiveProcess*
                }
                if ((*channel).WAIT != nullptr) {
                  // set next on wait list
                  if ((*channel).SEM == 0) {
                    (*channel).WAIT->PDES->STATE =
                      State::BLOCKED;
                  } else {
                    (*channel).WAIT->PDES->STATE =
                      State::DELAYED;
                    (*channel).WAIT->PDES->WAKETIME =
                      interp->DATE[(*channel).HEAD];
                  }
                }
              }
            }
          L699:
            if (debug & DBGRECV) {
              fprintf(
                STDOUT,
                "recv(e) pid %d state %s rdstatus %s chan %d WPID %d\n",
                current_process->PID,
                name_state(current_process->STATE),
                name_read_status(current_process->READSTATUS),
                interp->CNUM,
                (*channel).WAIT != nullptr ? (*channel).WAIT->PDES->PID : -1
              );
            }
            break;
          }
          case 66:
          case 92: {
            exec.J =
              interp
              ->S[current_process->T - 1]; // stack loc of channel number
            interp->CNUM = interp->S[exec.J]; // channel number
            exec.H2 = interp->SLOCATION[exec.J];
            if (interp->NUMTRACE > 0)
              CHKVAR(interp, interp->S[current_process->T - 1]);
            if (interp->CNUM == 0) {
              interp->CNUM = FIND(interp);
              interp->S[interp->S[current_process->T - 1]] =
                interp->CNUM;
            }
            if (debug & DBGSEND) {
              fprintf(
                STDOUT,
                "sendchan %d pid %d var [[T]] %d chan %d\n",
                exec.IR.F,
                current_process->PID,
                interp->S[interp->S[current_process->T - 1]],
                interp->CNUM
              );
            }
            exec.H1 = COMMDELAY(
              interp,
              current_process->PROCESSOR,
              exec.H2,
              exec.IR.Y
            );
            // WITH CHAN[CNUM] DO
            // il->CHAN[il->CNUM]
            channel = &interp->CHAN[interp->CNUM]; {
              if (interp->FREE == 0)
                interp->PS = PS::BUFCHK;
              else {
                TIMEINC(interp, CHANTIME, "cs66");
                exec.K = interp->FREE;
                interp->DATE[exec.K] =
                  current_process->TIME + exec.H1;
                interp->FREE = interp->LINK[interp->FREE];
                interp->LINK[exec.K] = 0;
                if ((*channel).HEAD == 0)
                  (*channel).HEAD = exec.K;
                else {
                  interp->PNT = (*channel).HEAD;
                  exec.I = 0;
                  exec.B1 = true;
                  while (interp->PNT != 0 && exec.B1) {
                    if (exec.I != 0) {
                      exec.TGAP = (int)(
                        interp->DATE[interp->PNT] -
                        interp->DATE[exec.I]);
                    } else
                      exec.TGAP = CHANTIME + 3;
                    if (interp->DATE[interp->PNT] >
                        interp->DATE[exec.K] &&
                        exec.TGAP > CHANTIME + 1)
                      exec.B1 = false;
                    else {
                      exec.I = interp->PNT;
                      interp->PNT = interp->LINK[interp->PNT];
                    }
                  }
                  interp->LINK[exec.K] = interp->PNT;
                  if (exec.I == 0)
                    (*channel).HEAD = exec.K;
                  else {
                    interp->LINK[exec.I] = exec.K;
                    if (interp->DATE[exec.K] <
                        interp->DATE[exec.I] + CHANTIME) {
                      interp->DATE[exec.K] =
                        interp->DATE[exec.I] + CHANTIME;
                    }
                  }
                }
                if (topology == Symbol::SHAREDSY) {
                  current_process->TIME = interp->DATE[exec.K];
                  if (debug & DBGTIME)
                    procTime(current_process, 0.0, "case 66,92");
                }
                if ((*channel).HEAD == exec.K &&
                    (*channel).WAIT != nullptr) {
                  // WITH WAIT->PDES
                  process = interp->CHAN[interp->CNUM].WAIT->PDES;
                  process->STATE = State::DELAYED;
                  process->WAKETIME = interp->DATE[exec.K];
                }
                if (exec.IR.F == 66) {
                  interp->VALUE[exec.K] =
                    interp->S[current_process->T];
                  if (interp->S[current_process->T] == RTAG) {
                    interp->RVALUE[exec.K] =
                      interp->RS[current_process->T];
                  }
                } else {
                  interp->VALUE[exec.K] = RTAG;
                  interp->RVALUE[exec.K] =
                    interp->S[current_process->T];
                  interp->RS[current_process->T] =
                    interp->S[current_process->T];
                  interp->S[current_process->T] = RTAG;
                }
                interp->S[current_process->T - 1] =
                  interp->S[current_process->T];
                if (interp->S[current_process->T] == RTAG) {
                  interp->RS[current_process->T - 1] =
                    interp->RS[current_process->T];
                }
                current_process->T -= 2;
                (*channel).SEM += 1;
              }
            }
            break;
          }
          case 67:
          case 74: {
            interp->NOSWITCH = false;
            TIMEINC(interp, CREATETIME, "cs67");
            process = static_cast<ProcessDescriptor*>(calloc(
              1,
              sizeof(ProcessDescriptor)
            ));
            process->PC = current_process->PC + 1;
            process->PID = interp->NEXTID++;
            // il->NEXTID += 1;
            if (interp->NEXTID > PIDMAX)
              interp->PS = PS::PROCCHK;
            process->VIRTUALTIME = 0;
            for (int i = 0; i <= LMAX; i++)
              process->DISPLAY[i] = current_process->DISPLAY[i];
            process->B = current_process->B;
            interp->PTEMP = (ActiveProcess*)calloc(
              1,
              sizeof(ActiveProcess)
            );
            interp->PTEMP->PDES = process;
            interp->PTEMP->NEXT = interp->ACPTAIL->NEXT;
            interp->ACPTAIL->NEXT = interp->PTEMP;
            interp->ACPTAIL = interp->PTEMP;
            process->T = FINDFRAME(interp, WORKSIZE) - 1;
            if (debug & DBGPROC) {
              fprintf(
                STDOUT,
                "opc %d findframe %d length %d, response %d\n",
                exec.IR.F,
                process->PID,
                WORKSIZE,
                process->T + 1
              );
            }
            process->STACKSIZE = process->T + WORKSIZE;
            process->BASE = process->T + 1;
            process->TIME = current_process->TIME;
            // proc->NUMCHILDREN = 0;
            // proc->MAXCHILDTIME = 0;
            if (exec.IR.Y != 1) {
              process->WAKETIME =
                current_process->TIME +
                COMMDELAY(
                  interp,
                  current_process->PROCESSOR,
                  interp->S[current_process->T],
                  1
                );
              if (process->WAKETIME > process->TIME)
                process->STATE = State::DELAYED;
              else
                process->STATE = State::READY;
            }
            process->FORLEVEL = current_process->FORLEVEL;
            process->READSTATUS = ReadStatus::NONE;
            process->FORKCOUNT = 1;
            process->MAXFORKTIME = 0;
            process->JOINSEM = 0;
            process->PARENT = current_process;
            process->PRIORITY = PRIORITY::LOW;
            process->ALTPROC = -1;
            process->SEQON = true;
            process->GROUPREP = false;
            process->PROCESSOR = interp->S[current_process->T];
            if (process->PROCESSOR > highest_processor || process->PROCESSOR <
                0)
              interp->PS = PS::CPUCHK;
            else {
              if (interp->PROCTAB[process->PROCESSOR].STATUS ==
                  Status::NEVERUSED)
                interp->USEDPROCS += 1;
              interp->PROCTAB[process->PROCESSOR].STATUS =
                Status::FULL;
              interp->PROCTAB[process->PROCESSOR].NUMPROC += 1;
            }
            current_process->T = current_process->T - 1;
            if (process->T > 0) {
              exec.J = 0;
              while (process->FORLEVEL > exec.J) {
                process->T = process->T + 1;
                exec.H1 = current_process->BASE + exec.J;
                interp->S[process->T] = interp->S[exec.H1];
                interp->SLOCATION[process->T] =
                  interp->SLOCATION[exec.H1];
                interp->RS[process->T] = interp->RS[exec.H1];
                exec.J = exec.J + 1;
              }
              if (exec.IR.F == 74) {
                process->FORLEVEL = process->FORLEVEL + 1;
                exec.H1 = interp->S[current_process->T - 2];
                exec.H2 = interp->S[current_process->T - 1];
                exec.H3 = interp->S[current_process->T];
                exec.H4 = interp->S[current_process->T - 3];
                process->T = process->T + 1;
                interp->S[process->T] = exec.H1;
                interp->SLOCATION[process->T] = exec.H4;
                interp->RS[process->T] = process->PC;
                process->T = process->T + 1;
                if (exec.H1 + exec.H3 <= exec.H2)
                  interp->S[process->T] = exec.H1 + exec.H3 - 1;
                else
                  interp->S[process->T] = exec.H2;
              }
            }
            exec.J = 1;
            while (process->DISPLAY[exec.J] != -1) {
              interp->S[process->DISPLAY[exec.J] + 5] += 1;
              if (debug & DBGRELEASE) {
                fprintf(
                  STDOUT,
                  "%d ref ct %d ct=%d\n",
                  exec.IR.F,
                  current_process->PID,
                  interp->S[process->DISPLAY[exec.J] + 5]
                );
              }
              exec.J = exec.J + 1;
            }
            if (exec.IR.Y == 1) {
              process->STATE = State::RUNNING;
              current_process->STATE = State::BLOCKED;
              process->TIME = current_process->TIME;
              process->PRIORITY = PRIORITY::HIGH;
              process->ALTPROC = process->PROCESSOR;
              process->PROCESSOR = current_process->PROCESSOR;
              interp->PROCTAB[process->PROCESSOR].RUNPROC = process;
              interp->PROCTAB[process->PROCESSOR].NUMPROC += 1;
            }
            if (exec.IR.F == 74) {
              if (current_process->NUMCHILDREN == 0)
                current_process->MAXCHILDTIME = current_process->TIME;
              current_process->NUMCHILDREN += 1;
            } else
              current_process->FORKCOUNT += 1;
            if (debug & DBGPROC) {
              fprintf(
                STDOUT,
                "opc %d newproc pid %d\n",
                exec.IR.F,
                process->PID
              );
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
              current_process->STATE = State::BLOCKED;
              interp->PROCTAB[current_process->PROCESSOR].RUNPROC =
                nullptr;
              current_process->PC = current_process->PC - 1;
            } else {
              for (int i = LMAX; i > 0; i--) {
                exec.J = current_process->DISPLAY[i];
                if (exec.J != -1) {
                  interp->S[exec.J + 5] =
                    interp->S[exec.J + 5] - 1;
                  if (interp->S[exec.J + 5] == 0) {
                    if (debug & DBGRELEASE) {
                      std::println(
                        "{} releas1 {} fm={} ln={}",
                        exec.IR.F,
                        current_process->PID,
                        exec.J,
                        interp->S[exec.J + 6]
                      );
                    }
                    RELEASE(
                      interp,
                      exec.J,
                      interp->S[exec.J + 6]
                    );
                  } else {
                    if (debug & DBGRELEASE) {
                      std::println(
                        "{} ref ct {} ct={}",
                        exec.IR.F,
                        current_process->PID,
                        interp->S[exec.J + 5]
                      );
                    }
                  }
                }
              }
              if (!mpi_mode) {
                if (debug & DBGRELEASE) {
                  std::println(
                    "{} releas2 {} fm={} ln={}",
                    exec.IR.F,
                    current_process->PID,
                    current_process->BASE,
                    WORKSIZE
                  );
                }
                RELEASE(interp, current_process->BASE, WORKSIZE);
              }
              if (mpi_mode && interp->MPIINIT[current_process->PROCESSOR]
                  &&
                  !interp->MPIFIN[current_process->PROCESSOR])
                interp->PS = PS::MPIFINCHK;
              for (int i = 1; i <= OPCHMAX; i++) {
                if (interp->CHAN[i].READER == current_process->PID)
                  interp->CHAN[i].READER = -1;
              }

              current_process->SEQON = false;
              TIMEINC(interp, CREATETIME - 1, "cs69");
              exec.H1 = COMMDELAY(
                interp,
                current_process->PROCESSOR,
                current_process->PARENT->PROCESSOR,
                1
              );
              interp->R1 = exec.H1 + current_process->TIME;
              // with CURPR->PARENT
              process = current_process->PARENT;

              switch (exec.IR.F) {
                case 70: {
                  process->NUMCHILDREN = process->NUMCHILDREN - 1;
                  if (process->MAXCHILDTIME < interp->R1)
                    process->MAXCHILDTIME = interp->R1;
                  if (process->NUMCHILDREN == 0) {
                    process->STATE = State::DELAYED;
                    process->WAKETIME = process->MAXCHILDTIME;
                  }
                  break;
                }

                case 69: {
                  process->FORKCOUNT = process->FORKCOUNT - 1;
                  if (process->MAXFORKTIME < interp->R1)
                    process->MAXFORKTIME = interp->R1;
                  if (process->JOINSEM == -1) {
                    if (interp->R1 > process->TIME) {
                      process->WAKETIME = interp->R1;
                      process->STATE = State::DELAYED;
                    } else
                      process->STATE = State::READY;
                  }

                  process->JOINSEM = process->JOINSEM + 1;
                  if (process->FORKCOUNT == 0) {
                    if (process->MAXFORKTIME > process->TIME) {
                      process->WAKETIME = process->MAXFORKTIME;
                      process->STATE = State::DELAYED;
                    } else
                      process->STATE = State::READY;
                  }

                  break;
                }
              }

              if ((debug & DBGPROC) != 0) {
                std::println(
                  "opc {} terminated pid {}",
                  exec.IR.F,
                  current_process->PID
                );
              }
              current_process->STATE = State::TERMINATED;
              process_metadata = &interp->PROCTAB[current_process->
                PROCESSOR];
              process_metadata->NUMPROC -= 1;
              process_metadata->RUNPROC = nullptr;
              if (process_metadata->NUMPROC == 0)
                process_metadata->STATUS = Status::EMPTY;
            }
            break;
          }
          case 73: {
            current_process->T = current_process->T + 1;
            if (current_process->T > current_process->STACKSIZE)
              interp->PS = PS::STKCHK;
            else {
              exec.H1 = current_process->DISPLAY[exec.IR.X] + exec.
                        IR.Y;
              for (int i = current_process->BASE;
                   i <= current_process->BASE + current_process->FORLEVEL - 1;
                   i++) {
                if (interp->SLOCATION[i] == exec.H1)
                  interp->S[current_process->T] = interp->S[i];
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
            if (interp->S[current_process->T] <= 0)
              interp->PS = PS::GRPCHK;
            else if (interp->S[current_process->T - 2] >
                     interp->S[current_process->T - 1]) {
              current_process->T = current_process->T - 4;
              current_process->PC = exec.IR.Y;
            } else
              current_process->PRIORITY = PRIORITY::HIGH;
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
            interp->S[current_process->T - 2] +=
              interp->S[current_process->T];
            if (interp->S[current_process->T - 2] <=
                interp->S[current_process->T - 1])
              current_process->PC = exec.IR.Y;
            else {
              current_process->T = current_process->T - 4;
              current_process->PRIORITY = PRIORITY::LOW;
            }
            break;
          }
          case 78: {
            // wakepar
            if (current_process->GROUPREP)
              current_process->ALTPROC = -1;
            else {
              // with CURPR->PARENT
              current_process->PARENT->STATE =
                State::RUNNING;
              current_process->PARENT->TIME = current_process->TIME;
              if (debug & DBGTIME)
                procTime(current_process->PARENT, 0.0, "case 78");
              interp->PROCTAB[current_process->PROCESSOR].NUMPROC -= 1;
              interp->PROCTAB[current_process->PROCESSOR].RUNPROC =
                current_process->PARENT;
              current_process->WAKETIME =
                current_process->TIME +
                COMMDELAY(
                  interp,
                  current_process->PROCESSOR,
                  current_process->ALTPROC,
                  1 + exec.IR.Y
                );
              if (current_process->WAKETIME > current_process->TIME)
                current_process->STATE = State::DELAYED;
              else
                current_process->STATE = State::READY;
              current_process->PROCESSOR = current_process->ALTPROC;
              current_process->ALTPROC = -1;
              current_process->PRIORITY = PRIORITY::LOW;
            }
            break;
          }
          case 79: {
            current_process->T = current_process->T + 1;
            if (current_process->T > current_process->STACKSIZE)
              interp->PS = PS::STKCHK;
            else {
              // il->S[T] = RTAG;
              // il->RS[T] = CONTABLE[el.IR.Y];
              exec.I = -1;
              exec.J = -1;
              exec.B1 = false;
              exec.H1 = MAXINT;
              exec.H2 = -1; // ?? added DE
              do {
                exec.I += 1;
                process_metadata = &interp->PROCTAB[exec.I];
                // with PROCTAB[I]
                switch (process_metadata->STATUS) {
                  case Status::EMPTY: exec.B1 = true;
                    break;
                  case Status::NEVERUSED: if (
                      exec.J == -1)
                      exec.J = exec.I;
                    break;
                  case Status::FULL: if (process_metadata
                                         ->NUMPROC < exec.H1) {
                      exec.H2 = exec.I;
                      exec.H1 = process_metadata->NUMPROC;
                    }
                    break; {
                    case Status::RESERVED: if (
                        process_metadata->NUMPROC + 1 < exec.H1)
                      // +1 fixed dde
                      {
                        exec.H2 = exec.I;
                        exec.H1 = process_metadata->NUMPROC + 1;
                      }
                    }
                    break;
                }
              } while (!exec.B1 && exec.I != highest_processor);
              if (exec.B1)
                interp->S[current_process->T] = exec.I;
              else if (exec.J > -1) {
                interp->S[current_process->T] = exec.J;
                interp->USEDPROCS += 1;
              } else
                interp->S[current_process->T] = exec.H2;
              // fprintf(STDOUT, "find processsor %d status %s\n", il->S[CURPR->T],
              //         prcsrStatus(il->PROCTAB[il->S[CURPR->T]].STATUS));
              interp->PROCTAB[interp->S[current_process->T]].STATUS =
                Status::RESERVED;
            }
            break;
          }
          case 80: {
            interp->SLOCATION[interp->S[current_process->T]] =
              interp->S[current_process->T - 1];
            current_process->T = current_process->T - 1;
            break;
          }
          case 81: {
            ++current_process->T;
            TIMEINC(interp, -1, "cs81");
            if (current_process->T > current_process->STACKSIZE)
              interp->PS = PS::STKCHK;
            else {
              interp->S[current_process->T] =
                interp->S[current_process->FORINDEX];
            }
            break;
          }
          case 82: {
            // initarray initialization
            for (int i = exec.IR.X; i < exec.IR.Y; i++) {
              exec.H1 = interp->S[current_process->T];
              interp->S[exec.H1] = INITABLE[i].IVAL;
              // fprintf(STDOUT, "initarray stack %d val %d", el.H1,
              // INITABLE[i].IVAL);
              if (INITABLE[i].IVAL == RTAG) {
                interp->RS[exec.H1] = INITABLE[i].RVAL;
                // fprintf(STDOUT, " real val %f", INITABLE[i].RVAL);
              }
              // fprintf(STDOUT, "\n");
              interp->S[current_process->T] =
                interp->S[current_process->T] + 1;
            }
            TIMEINC(interp, exec.IR.Y - exec.IR.X / 5, "cs82");
            break;
          }
          case 83: {
            // zeroarr
            exec.H1 = interp->S[current_process->T];
            for (int i = exec.H1; i < exec.H1 + exec.IR.X; i++) {
              if (exec.IR.Y == 2) {
                interp->S[i] = RTAG;
                interp->RS[i] = 0.0;
              } else
                interp->S[i] = 0;
            }
            current_process->T--;
            TIMEINC(interp, exec.IR.X / 10, "cs83");
            break;
          }
          case 84: {
            // dup
            ++current_process->T;
            TIMEINC(interp, -1, "cs84");
            if (current_process->T > current_process->STACKSIZE)
              interp->PS = PS::STKCHK;
            else {
              interp->S[current_process->T] =
                interp->S[current_process->T - 1];
            }
            break;
          }
          case 85: {
            // join?
            if (current_process->JOINSEM > 0)
              current_process->JOINSEM -= 1;
            else {
              current_process->JOINSEM = -1;
              current_process->STATE = State::BLOCKED;
              interp->PROCTAB[current_process->PROCESSOR].RUNPROC =
                nullptr;
            }
            break;
          }
          case 86: {
            if (topology != Symbol::SHAREDSY)
              TESTVAR(interp, interp->S[current_process->T]);
            break;
          }
          case 87: {
            // push real constant from constant table
            current_process->T = current_process->T + 1;
            if (current_process->T > current_process->STACKSIZE)
              interp->PS = PS::STKCHK;
            else {
              interp->S[current_process->T] = RTAG;
              interp->RS[current_process->T] = CONTABLE[exec.IR.Y];
            }
            break;
          }
          case 88: {
            current_process->T = current_process->T - 1;
            if (interp->S[current_process->T] != RTAG) {
              interp->RS[current_process->T] =
                interp->S[current_process->T];
              interp->S[current_process->T] = RTAG;
            }
            if (interp->S[current_process->T + 1] != RTAG) {
              interp->RS[current_process->T + 1] =
                interp->S[current_process->T + 1];
              interp->S[current_process->T + 1] = RTAG;
            }
            if (interp->RS[current_process->T + 1] == 0)
              interp->PS = PS::DIVCHK;
            else {
              interp->RS[current_process->T] =
                interp->RS[current_process->T] /
                interp->RS[current_process->T + 1];
              if (interp->RS[current_process->T] != 0) {
                exec.RH1 = log(fabs(interp->RS[current_process->T])) /
                           exec.log10;
                if (exec.RH1 >= 292.5 || exec.RH1 <= -292.5)
                  interp->PS = PS::OVRCHK;
              }
            }
            break;
          }
          case 89: interp->NOSWITCH = true;
            break;
          case 90: interp->NOSWITCH = false;
            break;
          case 91: {
            exec.H1 = interp->S[current_process->T - 1];
            if (topology != Symbol::SHAREDSY) {
              if (exec.IR.Y == 0)
                TESTVAR(interp, exec.H1);
              else if (interp->CONGESTION) {
                exec.H1 = COMMDELAY(
                  interp,
                  current_process->PROCESSOR,
                  interp->SLOCATION[exec.H1],
                  1
                );
              }
            }
            if (interp->NUMTRACE > 0)
              CHKVAR(interp, exec.H1);
            interp->RS[exec.H1] = interp->S[current_process->T];
            interp->S[exec.H1] = RTAG;
            interp->RS[current_process->T - 1] =
              interp->S[current_process->T];
            interp->S[current_process->T - 1] = RTAG;
            current_process->T = current_process->T - 1;
            break;
          }
          case 93: {
            // duration
            exec.H1 = interp->S[current_process->T];
            current_process->T -= 1;
            if (current_process->TIME + exec.H1 > interp->CLOCK) {
              current_process->STATE = State::DELAYED;
              current_process->WAKETIME = current_process->TIME + exec.H1;
              interp->PROCTAB[current_process->PROCESSOR].RUNPROC =
                nullptr;
            } else {
              if (debug & DBGTIME)
                procTime(
                  current_process,
                  static_cast<float>(exec.H1),
                  "case 93"
                );
              current_process->TIME += exec.H1;
            }
            break;
          }
          case 94:
          case 95:
          case 96: {
            switch (exec.IR.F) {
              case 94: exec.H1 = interp->S[current_process->T];
                current_process->T = current_process->T - 1;
                break;
              case 95: exec.H1 =
                       current_process->DISPLAY[exec.IR.X] + exec.IR.Y;
                break;
              case 96: exec.H1 =
                       interp
                       ->S[current_process->DISPLAY[exec.IR.X] + exec.IR
                           .Y];
                break;
              default: break;
            }
            interp->CNUM = interp->S[exec.H1];
            if (interp->NUMTRACE > 0)
              CHKVAR(interp, exec.H1);
            if (interp->CNUM == 0) {
              interp->CNUM = FIND(interp);
              interp->S[exec.H1] = interp->CNUM;
            }
            if (!interp->CHAN[interp->CNUM].MOVED &&
                interp->CHAN[interp->CNUM].READER < 0) {
              interp->SLOCATION[exec.H1] = current_process->PROCESSOR;
              interp->CHAN[interp->CNUM].READER = current_process->
                PID;
            }
            if (topology != Symbol::SHAREDSY)
              TESTVAR(interp, exec.H1);
            current_process->T = current_process->T + 1;
            if (current_process->T > current_process->STACKSIZE)
              interp->PS = PS::STKCHK;
            else {
              interp->S[current_process->T] = 1;
              if (interp->CNUM == 0 ||
                  interp->CHAN[interp->CNUM].SEM == 0 ||
                  interp->DATE[interp->CHAN[interp->CNUM].
                    HEAD] >
                  current_process->TIME)
                interp->S[current_process->T] = 0;
            }
            break;
          }
          case 97: {
            exec.H1 = interp->S[current_process->T];
            exec.H2 = exec.IR.Y;
            for (exec.I = exec.H1;
                 exec.I <= exec.H1 + exec.H2 - 1; exec.I++) {
              interp->SLOCATION[exec.I] =
                interp->S[current_process->T - 1];
            }
            current_process->T = current_process->T - 1;
            break;
          }
          case 98:
          case 113: {
            // send copy msg
            exec.H2 = interp->S[current_process->T];
            exec.H1 = FINDFRAME(interp, exec.IR.Y);
            if (debug & DBGSEND) {
              std::print(
                "send findframe {} pid {} from {} to {} len {}\n",
                exec.IR.F,
                current_process->PID,
                exec.H2,
                exec.H1,
                exec.IR.Y
              );
            }
            if (exec.H2 <= 0 || exec.H2 >= STMAX)
              interp->PS = PS::REFCHK;
            else if (exec.H1 < 0)
              interp->PS = PS::STORCHK;
            else {
              exec.H3 = exec.H1 + exec.IR.Y;
              if (topology != Symbol::SHAREDSY)
                TESTVAR(interp, exec.H2);
              if (debug & DBGSEND)
                std::print("copymsg");
              while (exec.H1 < exec.H3) {
                if (interp->NUMTRACE > 0)
                  CHKVAR(interp, exec.H2);
                interp->S[exec.H1] = interp->S[exec.H2];
                interp->SLOCATION[exec.H1] = -1;
                if (interp->S[exec.H2] == RTAG)
                  interp->RS[exec.H1] = interp->RS[exec.H2];
                if (debug & DBGSEND) {
                  if (interp->S[exec.H2] != RTAG)
                    std::print(":{}", interp->S[exec.H2]);
                  else
                    std::print(":{}", interp->RS[exec.H2]);
                }
                exec.H1 = exec.H1 + 1;
                exec.H2 = exec.H2 + 1;
              }
              if (debug & DBGSEND)
                std::print("\n");
              exec.H2 = interp->S[current_process->T];
              interp->S[current_process->T] = exec.H3 - exec.IR.Y;
              if (exec.IR.F == 98 &&
                  interp->SLOCATION[exec.H2] == -1) {
                if (debug & DBGSEND) {
                  std::println(
                    "send(rel) {} pid {} from {} len {}",
                    exec.IR.F,
                    current_process->PID,
                    exec.H2,
                    exec.IR.Y
                  );
                }
                if (debug & DBGSEND) {
                  std::println(
                    "{} send release fm={} ln={}",
                    exec.IR.F,
                    exec.H2,
                    exec.IR.Y
                  );
                }
                RELEASE(interp, exec.H2, exec.IR.Y);
              }
              TIMEINC(interp, exec.IR.Y / 5, "cs98");
              if (debug & DBGSEND) {
                std::print(
                  "send(x) {} pid {} var [T] {} chan [T-1] {}",
                  exec.IR.F,
                  current_process->PID,
                  interp->S[current_process->T],
                  interp->S[interp->S[current_process->T - 1]]
                );
              }
            }
            break;
          }
          case 101: {
            // lock
            if (topology != Symbol::SHAREDSY)
              interp->PS = PS::LOCKCHK;
            if (interp->NUMTRACE > 0)
              CHKVAR(interp, interp->S[current_process->T]);
            if (interp->S[interp->S[current_process->T]] == 0) {
              interp->S[interp->S[current_process->T]] = 1;
              current_process->T = current_process->T - 1;
            } else {
              current_process->STATE = State::SPINNING;
              current_process->PC = current_process->PC - 1;
            }
            break;
          }
          case 102: {
            // unlock
            if (topology != Symbol::SHAREDSY)
              interp->PS = PS::LOCKCHK;
            if (interp->NUMTRACE > 0)
              CHKVAR(interp, interp->S[current_process->T]);
            interp->S[interp->S[current_process->T]] = 0;
            current_process->T = current_process->T - 1;
            break;
          }
          case 104: {
            if (interp->S[current_process->T - 1] <
                interp->S[current_process->T]) {
              interp->S[current_process->T - 1] += 1;
              current_process->PC = exec.IR.X;
              if (exec.IR.Y == 1) {
                current_process->ALTPROC = current_process->PARENT->PROCESSOR;
                current_process->GROUPREP = true;
              }
            }
            break;
          }
          case 105: {
            if (interp->S[current_process->B + 5] == 1) {
              if (debug & DBGRELEASE) {
                println(
                  "{} release {} fm={} ln={}",
                  exec.IR.F,
                  current_process->PID,
                  current_process->B,
                  interp->S[current_process->B + 6]
                );
              }
              RELEASE(
                interp,
                current_process->B,
                interp->S[current_process->B + 6]
              );
            } else {
              interp->S[current_process->B + 5] =
                interp->S[current_process->B + 5] - 1;
              if (debug & DBGRELEASE) {
                fprintf(
                  STDOUT,
                  "%d fm ref ct %d ct=%d\n",
                  exec.IR.F,
                  current_process->PID,
                  interp->S[current_process->B + 5]
                );
              }
            }
            exec.H1 = TAB[interp->S[current_process->B + 4]].LEV;
            current_process->DISPLAY[exec.H1 + 1] = -1;
            current_process->T = interp->S[current_process->B + 7];
            current_process->B = interp->S[current_process->B + 3];
            if (current_process->T >= current_process->BASE - 1 &&
                current_process->T < current_process->BASE + WORKSIZE)
              current_process->STACKSIZE = current_process->BASE + WORKSIZE - 1;
            else {
              current_process->STACKSIZE =
                current_process->B + interp->S[current_process->B + 6] -
                1;
            }
            break;
          }
          case 106: {
            TIMEINC(interp, -1, "c106");
            current_process->SEQON = false;
            break;
          }
          case 107: {
            TIMEINC(interp, -1, "c107");
            current_process->SEQON = true;
            break;
          }
          case 108: {
            // increment
            exec.H1 = interp->S[current_process->T];
            if (exec.H1 <= 0 || exec.H1 >= STMAX)
              interp->PS = PS::REFCHK;
            else {
              if (topology != Symbol::SHAREDSY)
                TESTVAR(interp, exec.H1);
              if (interp->NUMTRACE > 0)
                CHKVAR(interp, exec.H1);
              switch (exec.IR.Y) {
                case 0: if (interp->S[exec.H1] == RTAG) {
                    interp->RS[exec.H1] =
                      interp->RS[exec.H1] + exec.IR.X;
                    interp->RS[current_process->T] =
                      interp->RS[exec.H1];
                    interp->S[current_process->T] = RTAG;
                  } else {
                    interp->S[exec.H1] =
                      interp->S[exec.H1] + exec.IR.X;
                    interp->S[current_process->T] =
                      interp->S[exec.H1];
                    if (abs(interp->S[current_process->T]) > NMAX)
                      interp->PS = PS::INTCHK;
                  }
                  break;
                case 1: if (interp->S[exec.H1] == RTAG) {
                    interp->RS[current_process->T] =
                      interp->RS[exec.H1];
                    interp->S[current_process->T] = RTAG;
                    interp->RS[exec.H1] =
                      interp->RS[exec.H1] + exec.IR.X;
                  } else {
                    interp->S[current_process->T] =
                      interp->S[exec.H1];
                    interp->S[exec.H1] =
                      interp->S[exec.H1] + exec.IR.X;
                    if (abs(interp->S[exec.H1]) > NMAX)
                      interp->PS = PS::INTCHK;
                  }
                  break;
              }
              if (interp->STARTMEM[exec.H1] <= 0)
                interp->PS = PS::REFCHK;
            }
            break;
          }
          case 109: {
            // decrement
            exec.H1 = interp->S[current_process->T];
            if (exec.H1 <= 0 || exec.H1 >= STMAX)
              interp->PS = PS::REFCHK;
            else {
              if (topology != Symbol::SHAREDSY)
                TESTVAR(interp, exec.H1);
              if (interp->NUMTRACE > 0)
                CHKVAR(interp, exec.H1);
              switch (exec.IR.Y) {
                case 0: if (interp->S[exec.H1] == RTAG) {
                    interp->RS[exec.H1] =
                      interp->RS[exec.H1] - exec.IR.X;
                    interp->RS[current_process->T] =
                      interp->RS[exec.H1];
                    interp->S[current_process->T] = RTAG;
                  } else {
                    interp->S[exec.H1] =
                      interp->S[exec.H1] - exec.IR.X;
                    interp->S[current_process->T] =
                      interp->S[exec.H1];
                    if (abs(interp->S[current_process->T]) > NMAX)
                      interp->PS = PS::INTCHK;
                  }
                  break;
                case 1: if (interp->S[exec.H1] == RTAG) {
                    interp->RS[current_process->T] =
                      interp->RS[exec.H1];
                    interp->S[current_process->T] = RTAG;
                    interp->RS[exec.H1] =
                      interp->RS[exec.H1] - exec.IR.X;
                  } else {
                    interp->S[current_process->T] =
                      interp->S[exec.H1];
                    interp->S[exec.H1] =
                      interp->S[exec.H1] - exec.IR.X;
                    if (abs(interp->S[exec.H1]) > NMAX)
                      interp->PS = PS::INTCHK;
                  }
                  break;
              }
              if (interp->STARTMEM[exec.H1] <= 0)
                interp->PS = PS::REFCHK;
            }
            break;
          }
          case 110: {
            if (exec.IR.Y == 0) {
              exec.H1 = interp->S[current_process->T];
              exec.H2 = interp->S[current_process->T - 1];
            } else {
              exec.H1 = interp->S[current_process->T - 1];
              exec.H2 = interp->S[current_process->T];
            }
            current_process->T = current_process->T - 1;
            interp->S[current_process->T] =
              exec.H2 + exec.IR.X * exec.H1;
            break;
          }
          case 111: // pop
            current_process->T = current_process->T - 1;
            break;
          case 112: {
            // release if SLOC[T] == -1 no processor
            if (interp->SLOCATION[interp->S[current_process->T]] ==
                -1) {
              if (debug & DBGRELEASE) {
                fprintf(
                  STDOUT,
                  "%d release %d fm=%d ln=%d\n",
                  exec.IR.F,
                  current_process->PID,
                  interp->S[current_process->T],
                  exec.IR.Y
                );
              }
              RELEASE(
                interp,
                interp->S[current_process->T],
                exec.IR.Y
              );
            }
            break;
          }
          case 114: {
            if (interp->S[current_process->T] != 0)
              interp->S[current_process->T] = 1;
          }
          case 115: {
            exec.H1 = interp->S[current_process->T - 1];
            interp->SLOCATION[exec.H1] =
              interp->S[current_process->T];
            interp->CNUM = interp->S[exec.H1];
            if (interp->CNUM == 0) {
              interp->CNUM = FIND(interp);
              interp->S[exec.H1] = interp->CNUM;
            }
            interp->CHAN[interp->CNUM].MOVED = true;
            current_process->T = current_process->T - 2;
            break;
          }

          /* instruction cases go here */
          default: println("Missing Code {}", exec.IR.F);
            break;
        }
      }
    } while (interp->PS == PS::RUN);
    // label_999:
    std::print("\n");
  } // EXECUTE
} // namespace Cstar
