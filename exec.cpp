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

#include "exec.hpp"

namespace cs {
using std::cout;
using std::println;
using std::numbers::ln10;

// enum PS {RUN, BREAK, FIN, DEAD, STKCHK};
//    typedef struct ACTIVEPROCESS {
//        ProcessDescriptor* PDES;
//        ActiveProcess* NEXT;
//    } ACTIVEPROCESS;

extern ALFA PROGNAME;
extern void snapPDES(Interpreter *, ProcessDescriptor *);
extern void dumpPDES(ProcessDescriptor *);
extern void dumpInst(int);
extern void dumpLInst(int, int *);
extern void dumpDeadlock(Interpreter *il);
extern const char *name_state(State);
extern const char *name_read_status(ReadStatus);
extern const char *processor_status(Status st);
extern const char *lookupSym(int, int);
extern void
  EXECLIB(Interpreter *il, ExLocal *, ProcessDescriptor *CURPR, int ID);

constexpr int switch_time = 5;
constexpr int create_time = 5;

auto Exec::show_inst_trace(bool flag) -> void {
  flag ? debug_flags.set(Debug::Instance) : debug_flags.reset(Debug::Instance);
}

void Exec::check_variable(int stack_location) {
  Interpreter *interp = commander->interp.get();
  // check if the variable at the passed stack location is being traced
  int i = 0;
  int j = 0;
  for (i = 1; i <= VARMAX; i++) {
    if (stack_location == interp->TRCTAB[i].MEMLOC) {
      std::println("Reference to Trace Variable {}", interp->TRCTAB[i].NAME);

      // CURPR* curpr = new CURPR;
      interp->BLINE = commander->get_lnum(interp->CURPR->PC - 1);
      std::print("Line Number: {}  In Function ", interp->BLINE);
      j = interp->CURPR->B;
      interp->INX = interp->S[interp->CURPR->B + 4];
      while (TAB[interp->INX].name[0] == '*') {
        j = interp->S[j + 3];
        interp->INX = interp->S[j + 4];
      }
      if (j == 0)
        println("{}", PROGNAME);
      else
        print("{}", TAB[interp->INX].name);
      //                std::cout << "Process Number" << il->CURPR->PID <<
      //                std::endl; std::cout << std::endl;
      std::println("Process Number {}\n", interp->CURPR->PID);
      interp->STEPROC = interp->CURPR;
      interp->PS = PS::BREAK;
    }
  }
}

auto Exec::move(int lsource, int dest, int step) -> void {
  int DIREC = 0;
  int I = 0;
  if (lsource < dest)
    DIREC = 1;
  else
    DIREC = -1;
  for (I = 1; I <= abs(lsource - dest); I++) {
    auto len = comm_delay->path_len;
    len += 1;

    auto path = comm_delay->path;
    path[len] = path[len - 1] + DIREC * step;
  }
}

auto Exec::rmove(int lsource, int dest, int step) -> void {
  int DIREC, I, DIST;
  DIST = abs(lsource - dest);
  if (lsource < dest)
    DIREC = 1;
  else
    DIREC = -1;
  if (TOPDIM < 2 * DIST) {
    DIREC = -DIREC;
    DIST = TOPDIM - DIST;
  }
  for (I = 1; I <= DIST; I++) {
    comm_delay->path_len = comm_delay->path_len + 1;
    lsource = lsource + DIREC;
    if (lsource == TOPDIM) {
      lsource = 0;
      comm_delay->path[comm_delay->path_len] =
        comm_delay->path[comm_delay->path_len - 1] - step * (TOPDIM - 1);
    } else if (lsource == -1) {
      lsource = TOPDIM - 1;
      comm_delay->path[comm_delay->path_len] =
        comm_delay->path[comm_delay->path_len - 1] + step * (TOPDIM - 1);
    } else
      comm_delay->path[comm_delay->path_len] =
        comm_delay->path[comm_delay->path_len - 1] + DIREC * step;
  }
}

auto Exec::schedule(double &arrival) -> void {
  BUSYPNT CURPNT;
  BUSYPNT PREV;
  BUSYPNT ITEM;
  int I, PROCNUM;
  bool DONE;
  for (I = 1; I <= comm_delay->path_len; I++) {
    arrival = arrival + 2;
    PROCNUM = comm_delay->path[I];
    CURPNT = commander->interp->PROCTAB[PROCNUM].BUSYLIST;
    PREV = nullptr;
    DONE = false;
    do {
      if (CURPNT == nullptr) {
        DONE = true;
        ITEM = (BUSYPNT)std::calloc(1, sizeof(BUSYTYPE));
        // NEW(ITEM);
        ITEM->FIRST = arrival;
        ITEM->LAST = arrival;
        ITEM->NEXT = nullptr;
        if (PREV == nullptr)
          commander->interp->PROCTAB[PROCNUM].BUSYLIST = ITEM;
        else
          PREV->NEXT = ITEM;
      } else {
        if (arrival < CURPNT->FIRST - 1) {
          DONE = true;
          // NEW(ITEM);
          ITEM = static_cast<BUSYPNT>(std::calloc(1, sizeof(BUSYTYPE)));
          ITEM->FIRST = arrival;
          ITEM->LAST = arrival;
          ITEM->NEXT = CURPNT;
          if (PREV != nullptr)
            PREV->NEXT = ITEM;
          else
            commander->interp->PROCTAB[PROCNUM].BUSYLIST = ITEM;
        } else if (arrival == CURPNT->FIRST - 1) {
          DONE = true;
          CURPNT->FIRST = CURPNT->FIRST - 1;
        } else if (arrival <= CURPNT->LAST + 1) {
          DONE = true;
          CURPNT->LAST = CURPNT->LAST + 1;
          arrival = CURPNT->LAST;
          PREV = CURPNT;
          CURPNT = CURPNT->NEXT;
          if (CURPNT != nullptr) {
            if (CURPNT->FIRST == arrival + 1) {
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
          if (PREV->LAST < comm_delay->past_interval) {
            // fprintf(STANDARD_OUTPUT, "free commdelay2\n");
            std::free(PREV);
            commander->interp->PROCTAB[PROCNUM].BUSYLIST = CURPNT;
            PREV = nullptr;
          }
        }
      }
    } while (!DONE);
  }
}

auto Exec::use_comm_delay(int SOURCE, int dest, int LEN) -> int {
  int rtn = 0;
  memset(&comm_delay, 0, sizeof(comm_delay));
  comm_delay->num_pack = LEN / 3;
  if (LEN % 3 != 0)
    comm_delay->num_pack = comm_delay->num_pack + 1;
  if (!commander->interp->CONGESTION || topology == Symbol::SHAREDSY) {
    switch (topology) {
      case Symbol::SHAREDSY: rtn = 0; break;
      case Symbol::FULLCONNSY:
      case Symbol::CLUSTERSY: comm_delay->dist = 1; break;
      case Symbol::HYPERCUBESY:
        comm_delay->t1 = SOURCE;
        comm_delay->t2 = dest;
        comm_delay->dist = 0;
        while (comm_delay->t1 + comm_delay->t2 > 0) {
          if (comm_delay->t1 % 2 != comm_delay->t2 % 2)
            comm_delay->dist = comm_delay->dist + 1;
          comm_delay->t1 = comm_delay->t1 / 2;
          comm_delay->t2 = comm_delay->2 / 2;
        }
        break;
      case Symbol::LINESY: comm_delay->dist = abs(SOURCE - dest); break;
      case Symbol::MESH2SY:
        comm_delay->t1 = TOPDIM;
        comm_delay->t2 =
          std::abs(SOURCE / comm_delay->t1 - dest / comm_delay->t1);
        comm_delay->t3 =
          std::abs(SOURCE % comm_delay->t1 - dest % comm_delay->t1);
        comm_delay->dist = comm_delay->t2 + comm_delay->t3;
        break;
      case Symbol::MESH3SY:
        comm_delay->tt = TOPDIM;
        comm_delay->t1 = TOPDIM * comm_delay->t2;
        comm_delay->t3 = SOURCE % comm_delay->t1;
        comm_delay->t4 = dest % comm_delay->t1;
        comm_delay->dist =
          abs(
            comm_delay->t3 / comm_delay->t2 - comm_delay->t4 / comm_delay->t2) +
          abs(
            comm_delay->t3 % comm_delay->t2 - comm_delay->t4 % comm_delay->t2) +
          abs(SOURCE / comm_delay->t1 - dest / comm_delay->t1);
        break;
      case Symbol::RINGSY:
        comm_delay->T1 = abs(SOURCE - dest);
        comm_delay->T2 = TOPDIM - comm_delay->T1;
        if (comm_delay->T1 < comm_delay->T2)
          comm_delay->DIST = comm_delay->T1;
        else
          comm_delay->DIST = comm_delay->T2;
        break;
      case Symbol::TORUSSY:
        comm_delay->T1 = TOPDIM;
        comm_delay->T3 = abs(SOURCE / comm_delay->T1 - dest / comm_delay->T1);
        if (comm_delay->T3 > comm_delay->T1 / 2)
          comm_delay->T3 = comm_delay->T1 - comm_delay->T3;
        comm_delay->T4 = abs(SOURCE % comm_delay->T1 - dest % comm_delay->T1);
        if (comm_delay->T4 > comm_delay->T1 / 2)
          comm_delay->T4 = comm_delay->T1 - comm_delay->T4;
        comm_delay->DIST = comm_delay->T3 + comm_delay->T4;
        break;
      default: break;
    }
    if (topology != Symbol::SHAREDSY) {
      rtn = static_cast<int>(std::round(
        (comm_delay->DIST + (comm_delay->NUMPACK - 1) / 2.0) * il->TOPDELAY));
    }
  } else {
    comm_delay->path[0] = SOURCE;
    comm_delay->path_len = 0;
    switch (topology) {
      case Symbol::FULLCONNSY:
      case Symbol::CLUSTERSY:
        comm_delay->path[1] = dest;
        comm_delay->path_len = 1;
        break;
      case Symbol::HYPERCUBESY:
        comm_delay->T1 = 1;
        comm_delay->T2 = SOURCE;
        for (comm_delay->I = 1; comm_delay->I <= TOPDIM; comm_delay->I++) {
          comm_delay->T3 = dest / comm_delay->T1 % 2;
          comm_delay->T4 = SOURCE / comm_delay->T1 % 2;
          if (comm_delay->T3 != comm_delay->T4) {
            if (comm_delay->T3 == 1)
              comm_delay->T2 = comm_delay->T2 + comm_delay->T1;
            else
              comm_delay->T2 = comm_delay->T2 - comm_delay->T1;
            comm_delay->path_len = comm_delay->path_len + 1;
            comm_delay->path[comm_delay->path_len] = comm_delay->T2;
          }
          comm_delay->T1 = comm_delay->T1 * 2;
        }
        break;
      case Symbol::LINESY: MOVE(&comm_delay, SOURCE, dest, 1); break;
      case Symbol::MESH2SY:
        MOVE(&comm_delay, SOURCE % TOPDIM, dest % TOPDIM, 1);
        MOVE(&comm_delay, SOURCE / TOPDIM, dest / TOPDIM, TOPDIM);
        break;
      case Symbol::MESH3SY:
        comm_delay->T1 = TOPDIM * TOPDIM;
        MOVE(&comm_delay, SOURCE % TOPDIM, dest % TOPDIM, 1);
        MOVE(
          &comm_delay,
          SOURCE / TOPDIM % TOPDIM,
          dest / TOPDIM % TOPDIM,
          TOPDIM);
        MOVE(
          &comm_delay,
          SOURCE / comm_delay->T1,
          dest / comm_delay->T1,
          TOPDIM * TOPDIM);
        break;
      case Symbol::RINGSY: RMOVE(&comm_delay, SOURCE, dest, 1); break;
      case Symbol::TORUSSY:
        RMOVE(&comm_delay, SOURCE % TOPDIM, dest % TOPDIM, 1);
        RMOVE(&comm_delay, SOURCE / TOPDIM, dest / TOPDIM, TOPDIM);
        break;
      default: break;
    }
    if (il->TOPDELAY != 0) {
      comm_delay->PASTINTERVAL = static_cast<float>(
        (il->comm_delayOCK - 2 * TIMESTEP) / (il->TOPDELAY / 2.0));
      comm_delay->NOWINTERVAL =
        static_cast<float>(il->CURPR->TIME / (il->TOPDELAY / 2.0) + 0.5);
      for (comm_delay->I = 1; comm_delay->I <= comm_delay->NUMPACK;
           comm_delay->I++) {
        comm_delay->FINALINTERVAL = comm_delay->NOWINTERVAL;
        SCHEDULE(&comm_delay, comm_delay->FINALINTERVAL);
      }
      rtn = static_cast<int>(std::round(
        (comm_delay->FINALINTERVAL - comm_delay->NOWINTERVAL) *
        (il->TOPDELAY / 2.0)));
    } else
      rtn = 0;
  }
  if (SOURCE > highest_processor || dest > highest_processor)
    il->PS = PS::CPUCHK;
  //        if (rtn != 0)
  //            fprintf(STANDARD_OUTPUT, "COMMDELAY source %d dest %d len %d
  //            returns %d\n", SOURCE, DEST, LEN, rtn);
  return rtn;
}

void TESTVAR(Interpreter *il, int STKPNT) {
  // verify that the processor number corresponding to the passed stack location
  // is the current PDES's processor, altproc, or -1, otherwise REMCHK
  if (const int PNUM = il->SLOCATION[STKPNT];
      PNUM != il->CURPR->PROCESSOR && PNUM != il->CURPR->ALTPROC && PNUM != -1)
    il->PS = PS::REMCHK;
}

/*
 * if either stack top or top+1 is RTAG'ed and the other is not, move the
 * untagged value to the corresponding rstack and RTAG the moved stack element.
 */
bool ISREAL(Interpreter *il) {
  STYPE *sp;
  RSTYPE *rp;
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

void procTime(ProcessDescriptor *prc, float incr, const char *id) {
  if (prc->PID == 0)
    std::print("{} = {} + {} {}\n", prc->TIME + incr, prc->TIME, incr, id);
}

void TIMEINC(Interpreter *il, int UNITS, const char *trc) {
  ProcessDescriptor *proc = il->CURPR;
  ProcessTable *table = &il->PROCTAB[proc->PROCESSOR];
  const float step = table->SPEED * static_cast<float>(UNITS);
  if (debug & DBGTIME)
    procTime(proc, step, trc);
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

void SLICE(Interpreter *il) {
  ActiveProcess *prev = il->ACPCUR;
  il->ACPCUR = il->ACPCUR->NEXT;
  bool done = false;
  int count = 0;
  bool deadlock = true;
  do {
    count = count + 1;
    if (il->ACPCUR == il->ACPHEAD)
      il->CLOCK += TIMESTEP;
    ProcessDescriptor *proc = il->ACPCUR->PDES;
    if (proc->STATE == State::DELAYED) {
      if (proc->WAKETIME < il->CLOCK) {
        proc->STATE = State::READY;
        if (proc->WAKETIME > proc->TIME) {
          proc->TIME = proc->WAKETIME;
          if (debug & DBGTIME)
            procTime(proc, 0.0, "SLICE1-WAKE");
        }
      } else {
        proc->TIME = il->CLOCK;
        if (debug & DBGTIME)
          procTime(proc, 0.0, "SLICE2-CLOCK");
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
      ProcessTable *ptab = &il->PROCTAB[proc->PROCESSOR];
      //                if (ptab->RUNPROC != nullptr && ptab->RUNPROC != proc)
      //                    fprintf(STANDARD_OUTPUT, "ptab(%d) != proc(%d)
      //                    virtime %.1f starttime %.1f\n",
      //                            ptab->RUNPROC->PID, proc->PID,
      //                            ptab->VIRTIME, ptab->STARTTIME);
      if (ptab->RUNPROC == nullptr) {
        il->CURPR = proc;
        done = true;
        ptab->STARTTIME = ptab->VIRTIME;
        proc->STATE = State::RUNNING;
        ptab->RUNPROC = il->CURPR;
      } else if (
        ptab->VIRTIME >= ptab->STARTTIME + SWITCHLIMIT &&
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
      if (debug & DBGTIME)
        procTime(proc, 0.0, "SLICE4-CLOCK");
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
        if (debug & DBGTIME)
          procTime(proc, 0.0, "SLICE5-CLOCK");
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
  //     fprintf(STANDARD_OUTPUT, "dispatch %d time %.1f seqtime %.1f\n",
  //     il->CURPR->PID, il->CURPR->TIME, il->SEQTIME);
  if (deadlock) {
    cout << "    DEADLOCK:  All Processes are Blocked" << '\n';
    cout << "For Further Information Type STATUS Command" << '\n';
    std::println(
      "CLOCK {} PS {}, DONE {}, COUNT {}, DEADLOCK {}",
      il->CLOCK,
      static_cast<std::int8_t>(il->PS),
      done,
      count,
      deadlock);
    il->PS = PS::DEAD;
    dumpDeadlock(il);
  }
}

void EXECUTE(Interpreter *interp) {
  //        ProcessState PS;
  //        ProcessDescriptor* CURPR;
  //        int LC;
  //        double USAGE;
  //        char CH;
  //        int T;
  //        std::vector<int> STACK(STACKSIZE);
  ProcessDescriptor *current_process = interp->CURPR;
  ProcessDescriptor *process = nullptr;
  ProcessTable *process_metadata = nullptr;
  Channel *channel = nullptr;
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
      if (
        (!interp->NOSWITCH && current_process->TIME >= interp->CLOCK) ||
        current_process->STATE != State::RUNNING) {
        //                    fprintf(STANDARD_OUTPUT, "state %s noswitch %d
        //                    time %.1f clock %.1f",
        //                            nameState(CURPR->STATE), il->NOSWITCH,
        //                            CURPR->TIME, il->CLOCK);
        SLICE(interp);
        current_process = interp->CURPR;
        //                    fprintf(STANDARD_OUTPUT, " new process %d
        //                    processor %d\n", CURPR->PID, CURPR->PROCESSOR);
      }
    }

    if (interp->PS == PS::FIN || interp->PS == PS::DEAD)
      continue;
    exec.IR = CODE.at(current_process->PC);
    current_process->PC++;
    TIMEINC(interp, 1, "exe1");

    if (interp->NUMBRK > 0 && interp->MAXSTEPS < 0 && !interp->RESTART) {
      for (int i = 1; i <= BRKMAX; i++) {
        if (interp->BRKTAB[i] == current_process->PC - 1)
          interp->PS = PS::BREAK;
      }
    }

    interp->RESTART = false;

    if (interp->MAXSTEPS > 0 && current_process == interp->STEPROC) {
      line_count = current_process->PC - 1;
      if (line_count < interp->STARTLOC || line_count > interp->ENDLOC) {
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
          std::print(
            "TIME: {}\n", std::round(interp->PROTIME - interp->PROSTEP));

          if (interp->FIRSTPROC < 10)
            std::print("{}", interp->FIRSTPROC);
          else
            std::print(" ");
          for (int i = 1; i <= 4 - interp->FIRSTPROC % 5; i++) {
            if (
              interp->FIRSTPROC + i < 10 &&
              interp->FIRSTPROC + i <= interp->LASTPROC) {
              std::cout << std::setw(2);
              std::print("{}", interp->FIRSTPROC + i);
            } else
              std::print("  ");
          }
          for (int i = interp->FIRSTPROC + 1; i <= interp->LASTPROC; i++) {
            if (i % 5 == 0) {
              if (i < 10) {
                std::cout << std::setw(2);
                std::print("{}         ", i);
              } else
                std::cout << i << "       ";
            }
          }
          std::print("\n");
        }

        for (int i = interp->FIRSTPROC; i <= interp->LASTPROC; i++) {
          interp->USAGE =
            static_cast<float>(interp->PROCTAB[i].PROTIME / interp->PROSTEP);
          if (interp->PROCTAB[i].STATUS == Status::NEVERUSED)
            CH = ' ';
          else if (interp->USAGE < 0.25)
            CH = '.';
          else if (interp->USAGE < 0.5)
            CH = '-';
          else if (interp->USAGE < 0.75)
            CH = '+';
          else
            CH = '*';
          std::print("{} ", CH);
          interp->PROCTAB[i].PROTIME = 0;
        }

        std::print("\n");
        interp->PROLINECNT = (interp->PROLINECNT + 1) % 20;
        interp->PROTIME += interp->PROSTEP;
      }
    }

    if (interp->ALARMON && interp->ALARMENABLED) {
      if (interp->CLOCK >= interp->ALARMTIME) {
        std::print(
          "\nAlarm Went Off at Time {}\n", std::round(interp->ALARMTIME));

        if (interp->CLOCK > interp->ALARMTIME + 2 * TIMESTEP)
          std::print("Current Time is {}\n", std::round(interp->CLOCK));

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
      if (exec.H1 == 0)
        std::println("\tIn Function {}", PROGNAME);
      else
        std::println("\tn Function {}", TAB[interp->INX].name);
      std::println("Process Number  {}", current_process->PID);
      interp->STEPROC = current_process;
      if (interp->MAXSTEPS == 0) {
        const double r1 = current_process->TIME - interp->STEPTIME;
        if (r1 > 0) {
          exec.H2 = static_cast<int>(std::round(
            (current_process->VIRTUALTIME - interp->VIRSTEPTIME) / r1 * 100.0));
        } else
          exec.H2 = 0;
        exec.H2 = std::min(exec.H2, 100);
        std::print(
          "Step Time is {}. Process running {} percent\n",
          std::round(r1),
          exec.H2);
      }
      std::print("\n");
    } else {
      //                if (watch)
      //                {
      //                    if (il->S[mon] != val)
      //                    {
      //                        fprintf(STANDARD_OUTPUT, "----------modified %d
      //                        at %d----------\n", il->S[mon], CURPR->PC);
      //                        watch = false;
      //                    }
      //                }
      //                else
      //                {
      //                    if (mon >= 0 && il->S[mon] == val)
      //                    {
      //                        fprintf(STANDARD_OUTPUT, "----------monitor set
      //                        at %d----------\n", CURPR->PC); watch = true;
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
      if ((debug & DBGINST) != 0) {
        //                    if (CURPR->PROCESSOR == 0 && CURPR->PC < 81 &&
        //                    CURPR->PC > 18)
        //                    {
        std::print(
          "proc {:3d} stk {:5d} [{:5d}] ",
          current_process->PROCESSOR,
          current_process->T,
          interp->S[current_process->T]);
        dumpInst(current_process->PC - 1);
        //                    }
      }
      //                if (CURPR->PID == 1)
      //                    dumpInst(CURPR->PC - 1);
      //                if (++lct > 2000000)
      //                    throw std::exception();
      switch (exec.IR.F) {
        case 0: {
          // push DISPLAY[op1]+op2 (stack frame location of variable op2)
          current_process->T++;
          if (current_process->T > current_process->STACKSIZE)
            interp->PS = PS::STKCHK;
          else {
            interp->S[current_process->T] =
              current_process->DISPLAY[exec.IR.X] + exec.IR.Y;
            //                            fprintf(STANDARD_OUTPUT, "%3d: %d
            //                            %d,%d %s %s\n", CURPR->PC - 1,
            //                                el.IR.F, el.IR.X, el.IR.Y,
            //                                opcodes[el.IR.F],
            //                                lookupSym(el.IR.X, el.IR.Y));
          }
          break;
        }
        case 1: {
          current_process->T++;
          if (current_process->T > current_process->STACKSIZE)
            interp->PS = PS::STKCHK;
          else {
            exec.H1 = current_process->DISPLAY[exec.IR.X] + exec.IR.Y;
            if (topology != Symbol::SHAREDSY)
              TESTVAR(interp, exec.H1);
            if (interp->NUMTRACE > 0)
              CHKVAR(interp, exec.H1);
            interp->S[current_process->T] = interp->S[exec.H1];

            if (interp->S[current_process->T] == RTAG)
              interp->RS[current_process->T] = interp->RS[exec.H1];
            //                            fprintf(STANDARD_OUTPUT, "%3d: %d
            //                            %d,%d %s %s\n", CURPR->PC - 1,
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
              interp->S[current_process->DISPLAY[exec.IR.X] + exec.IR.Y];

            if (topology != Symbol::SHAREDSY)
              TESTVAR(interp, exec.H1);
            if (interp->NUMTRACE > 0)
              CHKVAR(interp, exec.H1);
            interp->S[current_process->T] = interp->S[exec.H1];

            if (interp->S[current_process->T] == RTAG)
              interp->RS[current_process->T] = interp->RS[exec.H1];
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
            interp->PROCTAB[current_process->PROCESSOR].RUNPROC = nullptr;
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
              if (interp->R1 >= 0 || interp->R1 == static_cast<int>(interp->R1))
                interp->S[current_process->T] = static_cast<int>(interp->R1);
              else {
                // ?? should go toward 0
                interp->S[current_process->T] =
                  static_cast<int>(interp->R1) - 1;
              }

              if (abs(interp->S[current_process->T]) > NMAX)
                interp->PS = PS::INTCHK;
              break;
            }
            case 19: {
              current_process->T++;
              if (current_process->T > current_process->STACKSIZE)
                interp->PS = PS::STKCHK;
              else
                interp->S[current_process->T] = current_process->PROCESSOR;
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
            for (exec.I = 1; exec.I <= highest_processor; exec.I++) {
              process = static_cast<ProcessDescriptor *>(
                calloc(1, sizeof(ProcessDescriptor)));
              process->PC = 0;
              process->PID = interp->NEXTID++;
              if (interp->NEXTID > PIDMAX)
                interp->PS = PS::PROCCHK;
              process->VIRTUALTIME = 0;
              process->DISPLAY.swap(current_process->DISPLAY);
              interp->PTEMP =
                static_cast<ActiveProcess *>(calloc(1, sizeof(ActiveProcess)));
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
                  exec.H1);
              }
              if (exec.H1 > 0) {
                process->T = exec.H1 + BTAB[2].VSIZE - 1;
                process->STACKSIZE = exec.H1 + interp->STKMAIN - 1;
                process->B = exec.H1;
                process->BASE = exec.H1;
                for (exec.J = exec.H1; exec.J <= exec.H1 + interp->STKMAIN - 1;
                     exec.J++) {
                  interp->S[exec.J] = 0;
                  interp->SLOCATION[exec.J] = exec.I;
                  interp->RS[exec.I] = 0.0;
                }
                for (exec.J = exec.H1; exec.J <= exec.H1 + BASESIZE - 1;
                     exec.J++)
                  interp->STARTMEM[exec.I] = -interp->STARTMEM[exec.I];
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
              if (interp->PROCTAB[exec.I].STATUS == Status::NEVERUSED)
                interp->USEDPROCS++;
              interp->PROCTAB[exec.I].STATUS = Status::FULL;
              interp->PROCTAB[exec.I].NUMPROC++;
              exec.J = 1;

              while (process->DISPLAY[exec.J] != -1) {
                interp->S[process->DISPLAY[exec.J] + 5]++;
                if (debug & DBGRELEASE) {
                  std::println(
                    "{} ref ct {} ct={}",
                    exec.IR.F,
                    current_process->PID,
                    interp->S[process->DISPLAY[exec.J] + 5]);
                }
                exec.J++;
              }
              if ((debug & DBGPROC) != 0) {
                std::println("opc {} newproc pid {}", exec.IR.F, process->PID);
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
        case 11: {
          // if0 pop,->op2
          if (interp->S[current_process->T] == 0)
            current_process->PC = exec.IR.Y;
          current_process->T--;
          break;
        }
        case 12: {
          exec.H1 = interp->S[current_process->T];
          current_process->T--;
          exec.H2 = exec.IR.Y;
          exec.H3 = 0;
          do {
            if (CODE[exec.H2].F != 13) {
              exec.H3 = 1;
              interp->PS = PS::CASCHK;
            } else if (CODE[exec.H2].X == -1 || CODE[exec.H2].Y == exec.H1) {
              exec.H3 = 1;
              current_process->PC = CODE[exec.H2 + 1].Y;
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
              exec.H3);
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
          //                            fprintf(STANDARD_OUTPUT, "14: proc %d pc
          //                            %d stack %d\n", CURPR->PROCESSOR,
          //                            CURPR->PC, CURPR->T);
          current_process->T++;
          if (current_process->T > current_process->STACKSIZE)
            interp->PS = PS::STKCHK;
          else {
            interp->S[current_process->T] = interp->S[exec.H1];
            if (interp->S[current_process->T] == RTAG)
              interp->RS[current_process->T] = interp->RS[exec.H1];
          }
          break;
        }
        case 15: {
          // add
          current_process->T--;
          interp->S[current_process->T] =
            interp->S[current_process->T] + interp->S[current_process->T + 1];
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
                exec.H2);
            }
            if (exec.H2 >= 0) {
              interp->S[exec.H2 + 7] = current_process->T;
              current_process->T = exec.H2 - 1;
              current_process->STACKSIZE = exec.H1 + exec.H2 - 1;
              current_process->T += 8;
              interp->S[current_process->T - 4] = exec.H1 - 1;
              interp->S[current_process->T - 3] = exec.IR.Y;
              for (exec.I = exec.H2; exec.I <= exec.H2 + BASESIZE - 1; exec.I++)
                interp->STARTMEM[exec.I] = -interp->STARTMEM[exec.I];
            }
          }
          break;
        }
        case 19: {
          if (TAB[exec.IR.X].address < 0)
            EXECLIB(interp, &exec, current_process, -TAB[exec.IR.X].address);
          else {
            // base of frame acquired by 18
            exec.H1 = current_process->T - exec.IR.Y;
            exec.H2 = interp->S[exec.H1 + 4];

            // function TAB index
            // TAB[stk + 4].LEV
            exec.H3 = TAB[exec.H2].LEV;
            // DISPLAY[H3 + 1]
            current_process->DISPLAY[exec.H3 + 1] = exec.H1;
            // clear DISPLAY to top w/-1

            for (exec.I = exec.H3 + 2; exec.I <= LMAX; exec.I++)
              current_process->DISPLAY[exec.I] = -1;
            interp->S[exec.H1 + 6] = interp->S[exec.H1 + 3] + 1;
            exec.H4 = interp->S[exec.H1 + 3] + exec.H1;
            interp->S[exec.H1 + 1] = current_process->PC;
            // return addr
            interp->S[exec.H1 + 2] = current_process->DISPLAY[exec.H3];
            interp->S[exec.H1 + 3] = current_process->B;
            interp->S[exec.H1 + 5] = 1;

            for (exec.H3 = current_process->T + 1; exec.H3 <= exec.H4;
                 exec.H3++) {
              interp->S[exec.H3] = 0;
              interp->RS[exec.H3] = 0.0;
            }

            for (exec.H3 = exec.H1; exec.H3 <= exec.H4; exec.H3++)
              interp->SLOCATION[exec.H3] = current_process->PROCESSOR;

            current_process->B = exec.H1;
            current_process->T = exec.H4 - WORKSIZE;
            // jump to block address
            current_process->PC = TAB[exec.H2].address;
            TIMEINC(interp, 3, "cs19");
          }
          break;
        }
        case 20: {
          // swap
          exec.H1 = interp->S[current_process->T];
          exec.RH1 = interp->RS[current_process->T];
          interp->S[current_process->T] = interp->S[current_process->T - 1];
          interp->RS[current_process->T] = interp->RS[current_process->T - 1];
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
          if (
            exec.H3 < exec.H2 ||
            (exec.H3 > ATAB[exec.H1].HIGH && ATAB[exec.H1].HIGH > 0))
            interp->PS = PS::INXCHK;
          else {
            current_process->T--;

            // added for debugging
            exec.H4 = interp->S[current_process->T];
            interp->S[current_process->T] =
              interp->S[current_process->T] +
              (exec.H3 - exec.H2) * ATAB[exec.H1].ELSIZE;
            // fprintf(STANDARD_OUTPUT, "\nindex %d array stack base %d stack
            // loc %d\n", el.H3, el.H4, il->S[CURPR->T]);
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
                interp->S[current_process->T] = interp->S[exec.H1];

                if (interp->S[exec.H1] == RTAG)
                  interp->RS[current_process->T] = interp->RS[exec.H1];
                exec.H1++;
              }
            }
          }
          break;
        }
        case 23: {
          exec.H1 = interp->S[current_process->T - 1];
          exec.H2 = interp->S[current_process->T];
          if (
            exec.H1 <= 0 || exec.H2 <= 0 || exec.H1 >= STMAX ||
            exec.H2 >= STMAX)
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
                  exec.IR.X);
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
            interp->S[current_process->T - 1] = interp->S[current_process->T];
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
            interp->S[current_process->T] = exec.IR.Y;
          break;
        }
        case 26: {
          // convreal
          interp->RS[current_process->T] = interp->S[current_process->T];
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
                  INPUT >> exec.buf;
                  interp->RS[exec.H1] = std::stod(exec.buf);

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
                  interp->S[exec.H1] = std::stoi(exec.buf);

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
                  interp->S[exec.H1] = INPUT.get();
                  if (0 > interp->S[exec.H1])
                    IORESULT = -1;
                  break;
                }
                default:;
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
                  interp->S[exec.H1] = std::stoi(exec.buf);

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
                  interp->S[exec.H1] = INPUT.get();
                  if (0 > interp->S[exec.H1])
                    IORESULT = -1;
                  break;
                }
                default: break;
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
            if (!OUTPUTFILE_FLAG)
              cout << STAB[exec.H2];
            else
              OUTPUT << STAB[exec.H2];
            exec.H1--;
            exec.H2++;
            if (
              STAB[exec.H2] == static_cast<char>(10) ||
              STAB[exec.H2] == static_cast<char>(13))
              interp->CHRCNT = 0;
          } while (exec.H1 != 0);
          break;
        }
        case 29: {
          //  outwidth output formatted with WIDTH
          if (interp->COUTWIDTH <= 0) {
            interp->CHRCNT = interp->CHRCNT + interp->FLD[exec.IR.Y];
            if (interp->CHRCNT > LINELENG)
              interp->PS = PS::LNGCHK;
            else {
              if (!OUTPUTFILE_FLAG) {
                switch (exec.IR.Y) {
                  case 2: {
                    std::print(
                      "{}", interp->FLD[2], interp->S[current_process->T]);
                    break;
                  }
                  case 3: {
                    cout << std::boolalpha;
                    std::print(
                      "{}",
                      interp->FLD[3],
                      static_cast<bool>(interp->S[current_process->T]));
                    cout << std::noboolalpha;
                    break;
                  }
                  case 4: {
                    if (
                      interp->S[current_process->T] < CHARL ||
                      interp->S[current_process->T] > CHARH)
                      interp->PS = PS::CHRCHK;
                    else {
                      std::print(
                        "{}",
                        interp->FLD[4],
                        static_cast<char>(interp->S[current_process->T]));
                      if (
                        interp->S[current_process->T] == 10 ||
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
                      "{}", interp->FLD[2], interp->S[current_process->T]);
                    break;
                  }
                  case 3: {
                    cout << std::boolalpha;
                    std::print(
                      "{}", interp->FLD[3], interp->S[current_process->T] != 0);
                    cout << std::noboolalpha;
                    break;
                  }
                  case 4: {
                    if (
                      interp->S[current_process->T] < CHARL ||
                      interp->S[current_process->T] > CHARH)
                      interp->PS = PS::CHRCHK;
                    else {
                      std::print(
                        OUTPUT,
                        "{}",
                        interp->FLD[4],
                        static_cast<char>(interp->S[current_process->T]));
                      if (
                        interp->S[current_process->T] == 10 ||
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
            interp->CHRCNT = interp->CHRCNT + interp->COUTWIDTH;
            exec.H1 = interp->S[current_process->T];
            if (interp->CHRCNT > LINELENG)
              interp->PS = PS::LNGCHK;
            else {
              if (!OUTPUTFILE_FLAG) {
                switch (exec.IR.Y) {
                  case 2: {
                    // std::cout << el.H1;
                    std::print("{}", interp->COUTWIDTH, exec.H1);
                    break;
                  }
                  case 3: {
                    // std::cout << ITOB(el.H1);
                    std::cout << std::boolalpha;
                    std::print("{}", interp->COUTWIDTH, exec.H1);
                    std::cout << std::noboolalpha;
                    break;
                  }
                  case 4: {
                    if (exec.H1 < CHARL || exec.H1 > CHARH)
                      interp->PS = PS::CHRCHK;
                    else {
                      // std::cout << char(el.H1);
                      std::print(
                        "{}", interp->COUTWIDTH, static_cast<char>(exec.H1));
                      if (exec.H1 == 10 || exec.H1 == 13)
                        interp->CHRCNT = 0;
                    }
                    break;
                  }
                }
              } else {
                switch (exec.IR.Y) {
                  case 2: {
                    std::print("{}", interp->COUTWIDTH, exec.H1);
                    break;
                  }
                  case 3: {
                    cout << std::boolalpha;
                    std::print("{}", interp->COUTWIDTH, exec.H1);
                    cout << std::noboolalpha;
                    break;
                  }
                  case 4: {
                    if (exec.H1 < CHARL || exec.H1 > CHARH)
                      interp->PS = PS::CHRCHK;
                    else {
                      std::print(
                        "{}", interp->COUTWIDTH, static_cast<char>(exec.H1));
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
            interp->PROCTAB[current_process->PROCESSOR].RUNPROC = nullptr;
            current_process->FORKCOUNT = current_process->FORKCOUNT - 1;
          } else {
            interp->PS = PS::FIN;
            if (debug & DBGPROC) {
              std::println(
                "opc {} terminated pid {}", exec.IR.F, current_process->PID);
              if (current_process->PID == 0)
                unreleased(interp);
            }
            current_process->STATE = State::TERMINATED;
          }
          break;
        }
        case 32: {
          if (interp->S[current_process->B + 5] == 1) {
            if ((debug & DBGRELEASE) != 0) {
              std::println(
                "{} release {} fm={} ln={}",
                exec.IR.F,
                current_process->PID,
                current_process->B,
                interp->S[current_process->B + 6]);
            }
            RELEASE(
              interp, current_process->B, interp->S[current_process->B + 6]);
          } else {
            interp->S[current_process->B + 5] -= 1;
            if ((debug & DBGRELEASE) != 0) {
              std::println(
                "{} ref ct {} ct{}=",
                exec.IR.F,
                current_process->PID,
                interp->S[current_process->B + 5]);
            }
          }
          exec.H1 = TAB[interp->S[current_process->B + 4]].LEV;
          current_process->DISPLAY[exec.H1 + 1] = -1;
          current_process->PC = interp->S[current_process->B + 1];
          current_process->T = interp->S[current_process->B + 7];
          current_process->B = interp->S[current_process->B + 3];
          if (
            current_process->T >= current_process->B - 1 &&
            current_process->T <
              current_process->B + interp->S[current_process->B + 6]) {
            current_process->STACKSIZE =
              current_process->B + interp->S[current_process->B + 6] - 1;
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
              std::println(
                "{} release {} fm={} ln={}",
                exec.IR.F,
                current_process->PID,
                current_process->B,
                interp->S[current_process->B + 6]);
            }
            RELEASE(
              interp, current_process->B, interp->S[current_process->B + 6]);
          } else {
            // fprintf(STDOUT, "release adjacent %d\n", il->S[CURPR->B+6]);
            interp->S[current_process->B + 5] =
              interp->S[current_process->B + 5] - 1;
            if ((debug & DBGRELEASE) != 0) {
              std::print(
                "{} ref ct {} ct={}",
                exec.IR.F,
                current_process->PID,
                interp->S[current_process->B + 5]);
            }
          }
          current_process->T = interp->S[current_process->B + 7] + 1;
          interp->S[current_process->T] = interp->S[exec.H2];
          if (interp->S[current_process->T] == RTAG)
            interp->RS[current_process->T] = interp->RS[exec.H2];

          current_process->PC = interp->S[current_process->B + 1];
          current_process->B = interp->S[current_process->B + 3];
          if (
            current_process->T >= current_process->B - 1 &&
            current_process->T <
              current_process->B + interp->S[current_process->B + 6]) {
            current_process->STACKSIZE =
              current_process->B + interp->S[current_process->B + 6] - 1;
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
            if (interp->S[exec.H1] == RTAG)
              interp->RS[current_process->T] = interp->RS[exec.H1];
            interp->S[current_process->T] = interp->S[exec.H1];
          }
          break;
        }
        case 35: {
          // not
          interp->S[current_process->T] = static_cast<STYPE>(
            static_cast<bool>(interp->S[current_process->T]));
          break;
        }
        case 36: {
          // negate
          if (interp->S[current_process->T] == RTAG)
            interp->RS[current_process->T] = -interp->RS[current_process->T];
          else
            interp->S[current_process->T] = -interp->S[current_process->T];
          break;
        }
        case 37: {
          // out real with prec/width or none
          exec.RH1 = interp->RS[current_process->T];
          if (exec.RH1 < 1)
            exec.H2 = 0;
          else {
            exec.H2 = 1 + static_cast<int>(std::floor(
                            std::log(std::fabs(exec.RH1)) / exec.log10));
          }
          if (interp->COUTWIDTH <= 0)
            exec.H1 = interp->FLD[1];
          else
            exec.H1 = interp->COUTWIDTH;
          if (interp->COUTPREC > 0 && interp->COUTPREC + 3 < exec.H1)
            exec.H1 = interp->COUTPREC + 3;

          interp->CHRCNT = interp->CHRCNT + exec.H1;

          if (interp->CHRCNT > LINELENG)
            interp->PS = PS::LNGCHK;
          else {
            if (!OUTPUTFILE_FLAG) {
              if (interp->COUTPREC <= 0) {
                // cout << el.RH1;
                std::print("{}", exec.H1, exec.RH1);
              } else {
                if (interp->COUTWIDTH <= 0) {
                  // cout << el.RH1 << ":" << il->COUTPREC - el.H2;
                  std::print(
                    "{}", exec.H1, interp->COUTPREC - exec.H2, exec.RH1);
                } else {
                  // cout << el.RH1 << ":" << il->COUTWIDTH << ":" <<
                  // il->COUTPREC
                  // - el.H2;
                  std::print(
                    "{}",
                    interp->COUTWIDTH,
                    interp->COUTPREC - exec.H2,
                    exec.RH1);
                }
              }
            } else {
              if (interp->COUTPREC <= 0)
                std::print(OUTPUT, "{}", exec.H1, exec.RH1);
              else {
                if (interp->COUTWIDTH <= 0) {
                  std::print(
                    OUTPUT,
                    "{}",
                    exec.H1,
                    interp->COUTPREC - exec.H2,
                    exec.RH1);
                } else {
                  std::print(
                    OUTPUT,
                    "{}",
                    interp->COUTWIDTH,
                    interp->COUTPREC - exec.H2,
                    exec.RH1);
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
            interp->S[exec.H1] = interp->S[current_process->T];
            if (topology != Symbol::SHAREDSY) {
              if (exec.IR.Y == 0)
                TESTVAR(interp, exec.H1);
              else if (interp->CONGESTION) {
                exec.H2 = COMMDELAY(
                  interp,
                  current_process->PROCESSOR,
                  interp->SLOCATION[exec.H1],
                  1);
              }
            }
            if (interp->NUMTRACE > 0)
              CHKVAR(interp, exec.H1);
            if (interp->S[current_process->T] == RTAG)
              interp->RS[exec.H1] = interp->RS[current_process->T];
            interp->S[current_process->T - 1] = interp->S[current_process->T];
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
            interp->S[current_process->T] = static_cast<STYPE>(
              interp->RS[current_process->T] ==
              interp->RS[current_process->T + 1]);
          } else {
            interp->S[current_process->T] = static_cast<STYPE>(
              interp->S[current_process->T] ==
              interp->S[current_process->T + 1]);
          }
          break;
        }
        case 46: {
          // pop and compare NE
          current_process->T = current_process->T - 1;
          if (ISREAL(interp)) {
            interp->S[current_process->T] = static_cast<STYPE>(
              interp->RS[current_process->T] !=
              interp->RS[current_process->T + 1]);
          } else {
            interp->S[current_process->T] = static_cast<STYPE>(
              interp->S[current_process->T] !=
              interp->S[current_process->T + 1]);
          }
          break;
        }
        case 47: {
          // pop, then compare top op top+1, replace top with comparison
          // result
          //  pop and compare LT
          current_process->T = current_process->T - 1;
          if (ISREAL(interp)) {
            interp->S[current_process->T] = static_cast<STYPE>(
              interp->RS[current_process->T] <
              interp->RS[current_process->T + 1]);
          } else {
            interp->S[current_process->T] = static_cast<STYPE>(
              interp->S[current_process->T] <
              interp->S[current_process->T + 1]);
          }
          break;
        }
        case 48: {
          // pop and compare LE
          current_process->T = current_process->T - 1;
          if (ISREAL(interp)) {
            interp->S[current_process->T] = static_cast<STYPE>(
              interp->RS[current_process->T] <=
              interp->RS[current_process->T + 1]);
          } else {
            interp->S[current_process->T] = static_cast<STYPE>(
              interp->S[current_process->T] <=
              interp->S[current_process->T + 1]);
          }
          break;
        }
        case 49: {
          // pop and compare GT
          current_process->T = current_process->T - 1;
          if (ISREAL(interp)) {
            interp->S[current_process->T] = BTOI(
              interp->RS[current_process->T] >
              interp->RS[current_process->T + 1]);
          } else {
            interp->S[current_process->T] = BTOI(
              interp->S[current_process->T] >
              interp->S[current_process->T + 1]);
          }
          break;
        }
        case 50: {
          // pop and compare GE
          current_process->T = current_process->T - 1;
          if (ISREAL(interp)) {
            interp->S[current_process->T] = BTOI(
              interp->RS[current_process->T] >=
              interp->RS[current_process->T + 1]);
          } else {
            interp->S[current_process->T] = BTOI(
              interp->S[current_process->T] >=
              interp->S[current_process->T + 1]);
          }
          break;
        }
        case 51: {
          // pop OR
          current_process->T = current_process->T - 1;
          interp->S[current_process->T] = BTOI(
            ITOB(interp->S[current_process->T]) ||
            ITOB(interp->S[current_process->T + 1]));
          break;
        }
        case 52: {
          // plus
          current_process->T = current_process->T - 1;
          if (ISREAL(interp)) {
            interp->RS[current_process->T] = interp->RS[current_process->T] +
                                             interp->RS[current_process->T + 1];
            if (interp->RS[current_process->T] != 0) {
              exec.RH1 = log(fabs(interp->RS[current_process->T])) / exec.log10;
              if (exec.RH1 >= 292.5 || exec.RH1 <= -292.5)
                interp->PS = PS::OVRCHK;
            }
          } else {
            interp->S[current_process->T] =
              interp->S[current_process->T] + interp->S[current_process->T + 1];
            if (abs(interp->S[current_process->T]) > NMAX)
              interp->PS = PS::INTCHK;
          }
          break;
        }
        case 53: {
          // minus
          current_process->T = current_process->T - 1;
          if (ISREAL(interp)) {
            interp->RS[current_process->T] = interp->RS[current_process->T] -
                                             interp->RS[current_process->T + 1];
            if (interp->RS[current_process->T] != 0) {
              exec.RH1 = log(fabs(interp->RS[current_process->T])) / exec.log10;
              if (exec.RH1 >= 292.5 || exec.RH1 <= -292.5)
                interp->PS = PS::OVRCHK;
            }
          } else {
            interp->S[current_process->T] =
              interp->S[current_process->T] - interp->S[current_process->T + 1];
            if (abs(interp->S[current_process->T]) > NMAX)
              interp->PS = PS::INTCHK;
          }
          break;
        }
        case 56: {
          // and
          current_process->T = current_process->T - 1;
          interp->S[current_process->T] = BTOI(
            ITOB(interp->S[current_process->T]) &&
            ITOB(interp->S[current_process->T + 1]));
          break;
        }
        case 57: {
          // times
          current_process->T = current_process->T - 1;
          if (ISREAL(interp)) {
            interp->RS[current_process->T] = interp->RS[current_process->T] *
                                             interp->RS[current_process->T + 1];
            if (interp->RS[current_process->T] != 0) {
              exec.RH1 = log(fabs(interp->RS[current_process->T])) / exec.log10;
              if (exec.RH1 >= 292.5 || exec.RH1 <= -292.5)
                interp->PS = PS::OVRCHK;
            }
          } else {
            interp->S[current_process->T] =
              interp->S[current_process->T] * interp->S[current_process->T + 1];
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
              interp->S[current_process->T] / interp->S[current_process->T + 1];
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
              interp->S[current_process->T] % interp->S[current_process->T + 1];
          }
          break;
        }
        case 62: {
          if (!INPUTFILE_FLAG) {
            if (INPUT.eof())
              interp->PS = PS::REDCHK;
            else
              INPUT.get();
          } else {
            if (INPUT.eof())
              interp->PS = PS::REDCHK;
            else
              INPUT.get();
          }
          break;
        }
        case 63: {
          // write endl
          if (!OUTPUTFILE_FLAG)
            std::print("\n");
          else
            OUTPUT << "\n";
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
            case 64: {
              exec.H1 = current_process->DISPLAY[exec.IR.X] + exec.IR.Y;
              break;
            }
            case 65: {
              exec.H1 =
                interp->S[current_process->DISPLAY[exec.IR.X] + exec.IR.Y];
              break;
            }
            case 71: {
              exec.H1 = interp->S[current_process->T];
              break;
            }
          }
          exec.H2 = interp->SLOCATION[exec.H1];
          interp->CNUM = interp->S[exec.H1];
          if ((debug & DBGRECV) != 0) {
            std::println(
              "recv {} pid {} pc {} state {} rdstatus {} chan {}",
              exec.IR.F,
              current_process->PID,
              current_process->PC,
              name_state(current_process->STATE),
              name_read_status(current_process->READSTATUS),
              interp->CNUM);
          }

          if (interp->NUMTRACE > 0)
            CHKVAR(interp, exec.H1);
          // get and initialize unused channel
          if (interp->CNUM == 0) {
            interp->CNUM = FIND(interp);
            // store channel in stack loc
            interp->S[exec.H1] = interp->CNUM;
          }

          if (current_process->READSTATUS == ReadStatus::NONE)
            current_process->READSTATUS = ReadStatus::ATCHANNEL;

          channel = &interp->CHAN[interp->CNUM];
          // WITH CHAN[CNUM]
          if (!channel->MOVED && (*channel).READER < 0) {
            interp->SLOCATION[exec.H1] = current_process->PROCESSOR;
            channel->READER = current_process->PID;
          }

          // channel in stk
          if (topology != Symbol::SHAREDSY)
            TESTVAR(interp, exec.H1);

          channel->READTIME =
            std::max(channel->READTIME, interp->CLOCK - TIMESTEP);

          interp->PNT = channel->HEAD;
          exec.B1 = channel->SEM == 0;
          if (!exec.B1)
            exec.B1 = interp->DATE[interp->PNT] > current_process->TIME;
          if (exec.B1) {
            interp->PTEMP =
              static_cast<ActiveProcess *>(calloc(1, sizeof(ActiveProcess)));
            interp->PTEMP->PDES = current_process;
            interp->PTEMP->NEXT = nullptr;
            interp->PROCTAB[current_process->PROCESSOR].RUNPROC = nullptr;

            if (channel->WAIT == nullptr) {
              exec.H3 = 1;
              channel->WAIT = interp->PTEMP;
              if (channel->SEM != 0) {
                current_process->STATE = State::DELAYED;
                current_process->WAKETIME = interp->DATE[interp->PNT];
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
            if (current_process->READSTATUS != ReadStatus::HASTICKET) {
              if (channel->READTIME > current_process->TIME) {
                current_process->TIME = channel->READTIME;
                if ((debug & DBGTIME) != 0)
                  procTime(current_process, 0.0, "case 64,65,71");
                current_process->READSTATUS = ReadStatus::HASTICKET;
                current_process->PC = current_process->PC - 1;
                interp->NOSWITCH = false;
                channel->READTIME = channel->READTIME + CHANTIME;
                goto L699;
              }
              channel->READTIME = channel->READTIME + CHANTIME;
            }
            TIMEINC(interp, CHANTIME, "cs64");
            channel->SEM -= 1;
            channel->HEAD = interp->LINK[interp->PNT];
            if (exec.IR.F == 64 || exec.IR.F == 65)
              current_process->T += 1;
            if (current_process->T > current_process->STACKSIZE)
              interp->PS = PS::STKCHK;
            else {
              interp->S[current_process->T] = interp->VALUE[interp->PNT];
              if (interp->S[current_process->T] == RTAG)
                interp->RS[current_process->T] = interp->RVALUE[interp->PNT];
            }

            interp->LINK[interp->PNT] = interp->FREE;
            interp->FREE = interp->PNT;
            current_process->READSTATUS = ReadStatus::NONE;
            if (channel->WAIT != nullptr) {
              if (channel->WAIT->PDES == current_process) {
                // remove CURPR from wait list
                interp->PTEMP = channel->WAIT;
                channel->WAIT = channel->WAIT->NEXT;
                if ((debug & DBGPROC) != 0) {
                  std::print(
                    "remove pid {} from wait list\n", current_process->PID);
                }
                // free ActiveProcess*
                std::free(interp->PTEMP);
              }
              if (channel->WAIT != nullptr) {
                // set next on wait list
                if (channel->SEM == 0)
                  channel->WAIT->PDES->STATE = State::BLOCKED;
                else {
                  channel->WAIT->PDES->STATE = State::DELAYED;
                  channel->WAIT->PDES->WAKETIME = interp->DATE[channel->HEAD];
                }
              }
            }
          }
        L699:
          if ((debug & DBGRECV) != 0) {
            std::print(
              "recv(e) pid {} state {} rdstatus {} chan {} WPID {}\n",
              current_process->PID,
              name_state(current_process->STATE),
              name_read_status(current_process->READSTATUS),
              interp->CNUM,
              channel->WAIT != nullptr ? channel->WAIT->PDES->PID : -1);
          }
          break;
        }
        case 66:
        case 92: {
          // stack loc of channel number
          exec.J = interp->S[current_process->T - 1];

          // channel number
          interp->CNUM = interp->S[exec.J];
          exec.H2 = interp->SLOCATION[exec.J];
          if (interp->NUMTRACE > 0)
            CHKVAR(interp, interp->S[current_process->T - 1]);
          if (interp->CNUM == 0) {
            interp->CNUM = FIND(interp);
            interp->S[interp->S[current_process->T - 1]] = interp->CNUM;
          }

          if ((debug & DBGSEND) != 0) {
            std::print(
              "send {} pid {} var [[T]] {} chan {}\n",
              exec.IR.F,
              current_process->PID,
              interp->S[interp->S[current_process->T]],
              interp->CNUM);
          }

          exec.H1 =
            COMMDELAY(interp, current_process->PROCESSOR, exec.H2, exec.IR.Y);

          // WITH CHAN[CNUM] DO
          // il->CHAN[il->CNUM]
          channel = &interp->CHAN[interp->CNUM];
          {
            if (interp->FREE == 0)
              interp->PS = PS::BUFCHK;
            else {
              TIMEINC(interp, CHANTIME, "cs66");
              exec.K = interp->FREE;
              interp->DATE[exec.K] = current_process->TIME + exec.H1;
              interp->FREE = interp->LINK[interp->FREE];
              interp->LINK[exec.K] = 0;

              if (channel->HEAD == 0)
                channel->HEAD = exec.K;
              else {
                interp->PNT = channel->HEAD;
                exec.I = 0;
                exec.B1 = true;
                while (interp->PNT != 0 && exec.B1) {
                  if (exec.I != 0) {
                    exec.TGAP = static_cast<int>(
                      interp->DATE[interp->PNT] - interp->DATE[exec.I]);
                  } else
                    exec.TGAP = CHANTIME + 3;
                  if (
                    interp->DATE[interp->PNT] > interp->DATE[exec.K] &&
                    exec.TGAP > CHANTIME + 1)
                    exec.B1 = false;
                  else {
                    exec.I = interp->PNT;
                    interp->PNT = interp->LINK[interp->PNT];
                  }
                }
                interp->LINK[exec.K] = interp->PNT;
                if (exec.I == 0)
                  channel->HEAD = exec.K;
                else {
                  interp->LINK[exec.I] = exec.K;
                  interp->DATE[exec.K] = std::max(
                    interp->DATE[exec.K], interp->DATE[exec.I] + CHANTIME);
                }
              }
              if (topology == Symbol::SHAREDSY) {
                current_process->TIME = interp->DATE[exec.K];
                if ((debug & DBGTIME) != 0)
                  procTime(current_process, 0.0, "case 66,92");
              }
              if (channel->HEAD == exec.K && channel->WAIT != nullptr) {
                // WITH WAIT->PDES
                process = interp->CHAN[interp->CNUM].WAIT->PDES;
                process->STATE = State::DELAYED;
                process->WAKETIME = interp->DATE[exec.K];
              }

              if (exec.IR.F == 66) {
                interp->VALUE[exec.K] = interp->S[current_process->T];
                if (interp->S[current_process->T] == RTAG)
                  interp->RVALUE[exec.K] = interp->RS[current_process->T];
              } else {
                interp->VALUE[exec.K] = RTAG;
                interp->RVALUE[exec.K] = interp->S[current_process->T];
                interp->RS[current_process->T] = interp->S[current_process->T];
                interp->S[current_process->T] = RTAG;
              }

              interp->S[current_process->T - 1] = interp->S[current_process->T];

              if (interp->S[current_process->T] == RTAG) {
                interp->RS[current_process->T - 1] =
                  interp->RS[current_process->T];
              }
              current_process->T -= 2;
              channel->SEM += 1;
            }
          }
          break;
        }
        case 67:
        case 74: {
          interp->NOSWITCH = false;
          TIMEINC(interp, CREATETIME, "cs67");
          process = static_cast<ProcessDescriptor *>(
            calloc(1, sizeof(ProcessDescriptor)));
          process->PC = current_process->PC + 1;
          process->PID = interp->NEXTID++;
          // il->NEXTID += 1;
          if (interp->NEXTID > PIDMAX)
            interp->PS = PS::PROCCHK;
          process->VIRTUALTIME = 0;

          for (int i = 0; i <= LMAX; i++)
            process->DISPLAY[i] = current_process->DISPLAY[i];
          process->B = current_process->B;
          interp->PTEMP =
            static_cast<ActiveProcess *>(calloc(1, sizeof(ActiveProcess)));

          interp->PTEMP->PDES = process;
          interp->PTEMP->NEXT = interp->ACPTAIL->NEXT;
          interp->ACPTAIL->NEXT = interp->PTEMP;
          interp->ACPTAIL = interp->PTEMP;
          process->T = FINDFRAME(interp, WORKSIZE) - 1;

          if ((debug & DBGPROC) != 0) {
            std::print(
              "opc {} findframe {} length {}, response {}\n",
              exec.IR.F,
              process->PID,
              WORKSIZE,
              process->T + 1);
          }
          exec.J = 1;
          while (process->DISPLAY[exec.J] != -1) {
            interp->S[process->DISPLAY[exec.J] + 5] += 1;
            if ((debug & DBGRELEASE) != 0) {
              std::print(
                "{} ref ct {} ct={}\n",
                exec.IR.F,
                process->PID,
                interp->S[process->DISPLAY[exec.J] + 5]);
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
            std::print("opc {} newproc pid {}\n", exec.IR.F, process->PID);
          }
          //                        fprintf(STDOUT, "fork processsor %d alt %d
          //                        status %s\n", proc->PROCESSOR,
          //                        proc->ALTPROC,
          //                              prcsrStatus(il->PROCTAB[proc->PROCESSOR].STATUS));
          break;
        }
        case 69:
        case 70: {
          if (current_process->FORKCOUNT > 1) {
            current_process->FORKCOUNT -= 1;
            current_process->STATE = State::BLOCKED;
            interp->PROCTAB[current_process->PROCESSOR].RUNPROC = nullptr;
            current_process->PC = current_process->PC - 1;
          } else {
            for (int i = LMAX; i > 0; i--) {
              exec.J = current_process->DISPLAY[i];
              if (exec.J != -1) {
                interp->S[exec.J + 5] = interp->S[exec.J + 5] - 1;
                if (interp->S[exec.J + 5] == 0) {
                  if (debug & DBGRELEASE) {
                    std::println(
                      "{} releas1 {} fm={} ln={}",
                      exec.IR.F,
                      current_process->PID,
                      exec.J,
                      interp->S[exec.J + 6]);
                  }
                  RELEASE(interp, exec.J, interp->S[exec.J + 6]);
                } else {
                  if (debug & DBGRELEASE) {
                    std::println(
                      "{} ref ct {} ct={}",
                      exec.IR.F,
                      current_process->PID,
                      interp->S[exec.J + 5]);
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
                  WORKSIZE);
              }
              RELEASE(interp, current_process->BASE, WORKSIZE);
            }
            if (
              mpi_mode && interp->MPIINIT[current_process->PROCESSOR] &&
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
              1);
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
                "opc {} terminated pid {}", exec.IR.F, current_process->PID);
            }
            current_process->STATE = State::TERMINATED;
            process_metadata = &interp->PROCTAB[current_process->PROCESSOR];
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
            exec.H1 = current_process->DISPLAY[exec.IR.X] + exec.IR.Y;
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
          else if (
            interp->S[current_process->T - 2] >
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
          interp->S[current_process->T - 2] += interp->S[current_process->T];
          if (
            interp->S[current_process->T - 2] <=
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
            current_process->PARENT->STATE = State::RUNNING;
            current_process->PARENT->TIME = current_process->TIME;
            if (debug & DBGTIME)
              procTime(current_process->PARENT, 0.0, "case 78");
            interp->PROCTAB[current_process->PROCESSOR].NUMPROC -= 1;
            interp->PROCTAB[current_process->PROCESSOR].RUNPROC =
              current_process->PARENT;
            current_process->WAKETIME =
              current_process->TIME + COMMDELAY(
                                        interp,
                                        current_process->PROCESSOR,
                                        current_process->ALTPROC,
                                        1 + exec.IR.Y);
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
                case Status::EMPTY: exec.B1 = true; break;
                case Status::NEVERUSED:
                  if (exec.J == -1)
                    exec.J = exec.I;
                  break;
                case Status::FULL:
                  if (process_metadata->NUMPROC < exec.H1) {
                    exec.H2 = exec.I;
                    exec.H1 = process_metadata->NUMPROC;
                  }
                  break;
                  {
                    case Status::RESERVED: {
                      // +1 fixed dde
                      if (process_metadata->NUMPROC + 1 < exec.H1) {
                        exec.H2 = exec.I;
                        exec.H1 = process_metadata->NUMPROC + 1;
                      }
                    } break;
                  }
              }
            } while (!exec.B1 && exec.I != highest_processor);
            if (exec.B1)
              interp->S[current_process->T] = exec.I;
            else if (exec.J > -1) {
              interp->S[current_process->T] = exec.J;
              interp->USEDPROCS += 1;
            } else
              interp->S[current_process->T] = exec.H2;
            // fprintf(STDOUT, "find processsor %d status %s\n",
            // il->S[CURPR->T],
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
            interp->S[current_process->T] = interp->S[current_process->T] + 1;
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
            interp->S[current_process->T] = interp->S[current_process->T - 1];
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
            interp->PROCTAB[current_process->PROCESSOR].RUNPROC = nullptr;
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
            interp->RS[current_process->T] = interp->S[current_process->T];
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
            interp->RS[current_process->T] = interp->RS[current_process->T] /
                                             interp->RS[current_process->T + 1];
            if (interp->RS[current_process->T] != 0) {
              exec.RH1 = log(fabs(interp->RS[current_process->T])) / exec.log10;
              if (exec.RH1 >= 292.5 || exec.RH1 <= -292.5)
                interp->PS = PS::OVRCHK;
            }
          }
          break;
        }
        case 89: interp->NOSWITCH = true; break;
        case 90: interp->NOSWITCH = false; break;
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
                1);
            }
          }
          if (interp->NUMTRACE > 0)
            CHKVAR(interp, exec.H1);
          interp->RS[exec.H1] = interp->S[current_process->T];
          interp->S[exec.H1] = RTAG;
          interp->RS[current_process->T - 1] = interp->S[current_process->T];

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
            interp->PROCTAB[current_process->PROCESSOR].RUNPROC = nullptr;
          } else {
            if ((debug & DBGTIME) != 0) {
              procTime(current_process, static_cast<float>(exec.H1), "case 93");
            }
            current_process->TIME += exec.H1;
          }
          break;
        }
        case 94:
        case 95:
        case 96: {
          switch (exec.IR.F) {
            case 94: {
              exec.H1 = interp->S[current_process->T];
              current_process->T = current_process->T - 1;
              break;
            }
            case 95: {
              exec.H1 = current_process->DISPLAY[exec.IR.X] + exec.IR.Y;
              break;
            }
            case 96: {
              exec.H1 =
                interp->S[current_process->DISPLAY[exec.IR.X] + exec.IR.Y];
              break;
            }
            default: break;
          }
          interp->CNUM = interp->S[exec.H1];
          if (interp->NUMTRACE > 0)
            CHKVAR(interp, exec.H1);
          if (interp->CNUM == 0) {
            interp->CNUM = FIND(interp);
            interp->S[exec.H1] = interp->CNUM;
          }
          if (
            !interp->CHAN[interp->CNUM].MOVED &&
            interp->CHAN[interp->CNUM].READER < 0) {
            interp->SLOCATION[exec.H1] = current_process->PROCESSOR;
            interp->CHAN[interp->CNUM].READER = current_process->PID;
          }
          if (topology != Symbol::SHAREDSY)
            TESTVAR(interp, exec.H1);
          current_process->T = current_process->T + 1;

          if (current_process->T > current_process->STACKSIZE)
            interp->PS = PS::STKCHK;
          else {
            interp->S[current_process->T] = 1;
            if (
              interp->CNUM == 0 || interp->CHAN[interp->CNUM].SEM == 0 ||
              interp->DATE[interp->CHAN[interp->CNUM].HEAD] >
                current_process->TIME)
              interp->S[current_process->T] = 0;
          }
          break;
        }
        case 97: {
          exec.H1 = interp->S[current_process->T];
          exec.H2 = exec.IR.Y;
          for (exec.I = exec.H1; exec.I <= exec.H1 + exec.H2 - 1; exec.I++)
            interp->SLOCATION[exec.I] = interp->S[current_process->T - 1];

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
              exec.IR.Y);
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

            if ((debug & DBGSEND) != 0)
              std::print("\n");
            exec.H2 = interp->S[current_process->T];
            interp->S[current_process->T] = exec.H3 - exec.IR.Y;

            if (exec.IR.F == 98 && interp->SLOCATION[exec.H2] == -1) {
              if ((debug & DBGSEND) != 0) {
                std::println(
                  "send(rel) {} pid {} from {} len {}",
                  exec.IR.F,
                  current_process->PID,
                  exec.H2,
                  exec.IR.Y);
                std::println(
                  "{} send release fm={} ln={}", exec.IR.F, exec.H2, exec.IR.Y);
              }
              RELEASE(interp, exec.H2, exec.IR.Y);
              TIMEINC(interp, exec.IR.Y / 5, "cs98");
              if ((debug & DBGSEND) != 0) {
                std::print(
                  "send(x) {} pid {} var [T] {} chan [T-1] {}",
                  exec.IR.F,
                  current_process->PID,
                  interp->S[current_process->T],
                  interp->S[interp->S[current_process->T - 1]]);
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
            if (
              interp->S[current_process->T - 1] <
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
              if ((debug & DBGRELEASE) != 0) {
                println(
                  "{} release {} fm={} ln={}",
                  exec.IR.F,
                  current_process->PID,
                  current_process->B,
                  interp->S[current_process->B + 6]);
              }

              RELEASE(
                interp, current_process->B, interp->S[current_process->B + 6]);
            } else {
              interp->S[current_process->B + 5] =
                interp->S[current_process->B + 5] - 1;

              if ((debug & DBGRELEASE) != 0) {
                std::print(
                  "{} ref ct {} ct={}\n",
                  exec.IR.F,
                  current_process->PID,
                  interp->S[current_process->B + 5]);
              }
            }

            exec.H1 = TAB[interp->S[current_process->B + 4]].LEV;
            current_process->DISPLAY[exec.H1 + 1] = -1;
            current_process->T = interp->S[current_process->B + 7];
            current_process->B = interp->S[current_process->B + 3];

            if (
              current_process->T >= current_process->BASE - 1 &&
              current_process->T < current_process->BASE + WORKSIZE)
              current_process->STACKSIZE = current_process->BASE + WORKSIZE - 1;
            else {
              current_process->STACKSIZE =
                current_process->B + interp->S[current_process->B + 6] - 1;
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
                case 0:
                  if (interp->S[exec.H1] == RTAG) {
                    interp->RS[exec.H1] = interp->RS[exec.H1] + exec.IR.X;
                    interp->RS[current_process->T] = interp->RS[exec.H1];
                    interp->S[current_process->T] = RTAG;
                  } else {
                    interp->S[exec.H1] = interp->S[exec.H1] + exec.IR.X;
                    interp->S[current_process->T] = interp->S[exec.H1];
                    if (abs(interp->S[current_process->T]) > NMAX)
                      interp->PS = PS::INTCHK;
                  }
                  break;
                case 1:
                  if (interp->S[exec.H1] == RTAG) {
                    interp->RS[current_process->T] = interp->RS[exec.H1];
                    interp->S[current_process->T] = RTAG;
                    interp->RS[exec.H1] = interp->RS[exec.H1] + exec.IR.X;
                  } else {
                    interp->S[current_process->T] = interp->S[exec.H1];
                    interp->S[exec.H1] = interp->S[exec.H1] + exec.IR.X;
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
                case 0:
                  if (interp->S[exec.H1] == RTAG) {
                    interp->RS[exec.H1] = interp->RS[exec.H1] - exec.IR.X;
                    interp->RS[current_process->T] = interp->RS[exec.H1];
                    interp->S[current_process->T] = RTAG;
                  } else {
                    interp->S[exec.H1] = interp->S[exec.H1] - exec.IR.X;
                    interp->S[current_process->T] = interp->S[exec.H1];
                    if (abs(interp->S[current_process->T]) > NMAX)
                      interp->PS = PS::INTCHK;
                  }
                  break;
                case 1:
                  if (interp->S[exec.H1] == RTAG) {
                    interp->RS[current_process->T] = interp->RS[exec.H1];
                    interp->S[current_process->T] = RTAG;
                    interp->RS[exec.H1] = interp->RS[exec.H1] - exec.IR.X;
                  } else {
                    interp->S[current_process->T] = interp->S[exec.H1];
                    interp->S[exec.H1] = interp->S[exec.H1] - exec.IR.X;
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
            interp->S[current_process->T] = exec.H2 + exec.IR.X * exec.H1;
            break;
          }
          case 111: {
            // pop
            current_process->T = current_process->T - 1;
            break;
          }
          case 112: {
            // release if SLOC[T] == -1 no processor
            if (interp->SLOCATION[interp->S[current_process->T]] == -1) {
              if ((debug & DBGRELEASE) != 0) {
                std::print(
                  "{} release {} fm={} ln={}\n",
                  exec.IR.F,
                  current_process->PID,
                  interp->S[current_process->T],
                  exec.IR.Y);
              }

              RELEASE(interp, interp->S[current_process->T], exec.IR.Y);
            }
            break;
          }
          case 114: {
            if (interp->S[current_process->T] != 0)
              interp->S[current_process->T] = 1;
          }
          case 115: {
            exec.H1 = interp->S[current_process->T - 1];
            interp->SLOCATION[exec.H1] = interp->S[current_process->T];
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
          default: {
            println("Missing Code {}", exec.IR.F);
            break;
          }
        }
      }
    }
  } while (interp->PS == PS::RUN);
  /* label_999: */
  std::print("\n");
} // EXECUTE
} // namespace cs
