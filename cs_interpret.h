//
// Created by Dan Evans on 1/1/24.
//

#ifndef CSTAR_CS_INTERPRET_H
#define CSTAR_CS_INTERPRET_H
#include "cs_global.h"
#ifdef EXPORT_CS_INTERPRET
#define INTERPRET_CS_EXPORT
#else
#define INTERPRET_CS_EXPORT extern
#endif
#include "cs_compile.h"
#include "cs_defines.h"
constexpr uint32_t NAMELEN = 20;
constexpr uint32_t CHANTIME = 3;
constexpr uint32_t SWITCHLIMIT = 50;
constexpr uint32_t TIMESTEP = 10;
constexpr uint32_t CHARL = 0;
constexpr uint32_t CHARH = 127;
constexpr uint32_t MPINODETIME = 10;
constexpr uint32_t MAXDIM = 10;
constexpr uint32_t CARTSTART = 200;
constexpr uint32_t CARTMAX = 100;
constexpr uint32_t BRKMAX = 10;
constexpr uint32_t VARMAX = 10;
constexpr uint32_t COMMAX = 30;

namespace cs {
  using VARTYP = std::string;
  using BLKPNT = struct BLOCKR*;
  using ACTPNT = struct ActiveProcess*;
  using PROCPNT = struct ProcessDescriptor*; // from interpret
  using BUSYPNT = struct BUSYTYPE*;
  using BLOCKR = struct BLOCKR {
    int START; // range 0..STMAX
    int SIZE; // range 0..STMAX
    BLKPNT NEXT;
  };

  struct ActiveProcess;
  struct ActiveProcess {
    PROCPNT PDES;
    ACTPNT NEXT;
  };

  struct BUSYTYPE {
    double FIRST;
    double LAST;
    BUSYPNT NEXT;
  };

  using STYPE = int;
  using RSTYPE = double;
  using BUFINTTYPE = int;
  using BUFREALTYPE = double;

  enum class COMTYP : std::uint8_t {
    RUNP,
    CONT,
    EXIT2,
    BREAKP,
    CLEAR,
    STEP,
    PSTATUS,
    WRVAR,
    TRACE,
    HELP,
    UTIL,
    PTIME,
    DISPLAYP,
    ALARM,
    WRCODE,
    STACK,
    CONGEST,
    PROFILE,
    ERRC,
    LIST,
    RESETP,
    SHORT,
    VIEW,
    CDELAY,
    VARY,
    OPENF,
    CLOSEF,
    INPUTF,
    OUTPUTF,
    MPI,
    VERSION
  };

  enum class State : std::uint8_t {
    READY,
    RUNNING,
    BLOCKED,
    DELAYED,
    TERMINATED,
    SPINNING
  };

  enum class ReadStatus : std::uint8_t { NONE, ATCHANNEL, HASTICKET };
  enum class PRIORITY : std::uint8_t { LOW, HIGH };

  struct ProcessDescriptor {
    // from interpret
    int T; // process's stack top index
    int B;
    int PC; // process's program counter (index into ORDER type CODE array)
    int STACKSIZE; // process's stack size
    std::array<int, LMAX + 1> DISPLAY;
    PROCPNT PARENT;
    int NUMCHILDREN;
    int BASE;
    double MAXCHILDTIME;
    double TIME;
    int FORLEVEL;
    int FORINDEX;
    int PROCESSOR;
    int ALTPROC;
    double WAKETIME;
    State STATE;
    int PID;
    double VIRTUALTIME;
    ReadStatus READSTATUS;
    int FORKCOUNT;
    int JOINSEM;
    double MAXFORKTIME;
    PRIORITY PRIORITY;
    bool SEQON;
    bool GROUPREP;
  };

  struct TraceTab {
    VARTYP NAME;
    int MEMLOC; // range -1..STMAX
  };

  enum class PS : std::uint8_t {
    RUN,
    FIN,
    DIVCHK,
    INXCHK,
    STKCHK,
    LINCHK,
    LNGCHK,
    REDCHK,
    DEAD,
    CHRCHK,
    STORCHK,
    GRPCHK,
    REFCHK,
    LOCKCHK,
    MPICNTCHK,
    MPIGRPCHK,
    MPIINITCHK,
    MPIFINCHK,
    MPIPARCHK,
    FUNCCHK,
    MPITYPECHK,
    INTCHK,
    CASCHK,
    CHANCHK,
    BUFCHK,
    PROCCHK,
    CPUCHK,
    BREAK,
    REMCHK,
    CARTOVR,
    STRCHK,
    USERSTOP,
    DATACHK,
    OVRCHK
  }; // from interpret

  enum class Status : std::uint8_t {
    NEVERUSED,
    EMPTY,
    RESERVED,
    FULL
  };

  struct ProcessTable {
    Status STATUS;
    double VIRTIME;
    double BRKTIME;
    double PROTIME;
    PROCPNT RUNPROC;
    int NUMPROC;
    double STARTTIME;
    BUSYPNT BUSYLIST;
    float SPEED;
  };

  struct Channel {
    int HEAD;
    int SEM;
    ACTPNT WAIT;
    double READTIME;
    bool MOVED;
    int READER;
  };

  struct Interpreter {
    PROCPNT CURPR;
    PS PS;

    int LNCNT;
    int CHRCNT;
    std::array<int, 4 + 1> FLD;
    STYPE *S, *SLOCATION;
    STYPE* STARTMEM;
    RSTYPE* RS;
    PROCPNT MAINPROC;
    std::array<ProcessTable, PMAX + 1> PROCTAB;

    int CNUM, H1;
    // int I;
    int J, K, PNT, FREE;

    std::array<Channel, OPCHMAX + 1> CHAN;
    BUFINTTYPE *VALUE, *LINK;
    BUFREALTYPE *DATE, *RVALUE;
    double SEQTIME;
    BLKPNT STHEAD;
    ACTPNT ACPHEAD, ACPTAIL, ACPCUR, PTEMP, RTEMP;
    bool NOSWITCH;
    double CLOCK;
    int TOPDELAY;
    int COUTWIDTH;
    int COUTPREC;
    std::string LISTDEFNAME;
    std::string INPUTFNAME;
    std::string OUTPUTFNAME;
    std::string LISTFNAME;
    std::string MPIINIT, MPIFIN;
    int MPICODE;
    int MPISEM;
    double MPITIME;
    int MPITYPE;
    int MPICNT;
    int MPIROOT;
    int MPIOP;
    int MPICOMM;
    std::array<int, PMAX + 1> MPIPNT;
    std::array<int, PMAX + 1> MPIRES;
    std::array<std::array<int, MAXDIM + 1>, CARTMAX + 1> MPICART;
    std::array<std::array<int, MAXDIM + 1>, CARTMAX + 1> MPIPER;
    std::string PROMPT;
    int MAXSTEPS;
    int INX;
    std::array<int, BRKMAX + 1> BRKTAB;
    std::array<int, BRKMAX + 1> BRKLINE;
    int REF, ADR;
    Types TYP;
    Objects OBJ;
    int NUMBRK; // range 0..BRKMAX
    bool RESTART;
    bool INITFLAG;
    bool TEMP, ERR;
    int VAL, LAST, FIRST;
    int STARTLOC, BLINE, ENDLOC;
    PROCPNT STEPROC;
    double STEPTIME, VIRSTEPTIME;
    double OLDTIME, OLDSEQTIME;
    int USEDPROCS;
    VARTYP VARNAME;
    int NUMTRACE;
    std::array<TraceTab, VARMAX + 1> TRCTAB;
    bool PROFILEON;
    float USAGE;
    double SPEED;
    int PROLINECNT, FIRSTPROC, LASTPROC;
    double PROSTEP, PROTIME;
    bool ALARMON, ALARMENABLED;
    double ALARMTIME;
    double R1, R2;
    bool CONGESTION;
    bool VARIATION;
    PROCPNT CURPNT;
    int NEXTID;
    int STKMAIN;
    int LEVEL; // range 1..LMAX
    std::array<ALFA, COMMAX + 1> COMTAB;
    std::array<ALFA, COMMAX + 1> ABBREVTAB;
    std::array<COMTYP, COMMAX + 1> COMJMP;
    COMTYP COMMLABEL;
    int LINECNT;
  };

  // int HIGHESTPROCESSOR;
} // namespace cs
#endif // CSTAR_CS_INTERPRET_H
