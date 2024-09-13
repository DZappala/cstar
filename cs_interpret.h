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
#define NAMELEN 20
#define CHANTIME 3
#define SWITCHLIMIT 50
#define TIMESTEP 10
#define CHARL 0
#define CHARH 127
#define MPINODETIME 10
#define MAXDIM 10
#define CARTSTART 200
#define CARTMAX 100
#define BRKMAX 10
#define VARMAX 10
#define COMMAX 30

namespace cs {
  using VARTYP = std::string;
  using BLKPNT = struct BLOCKR*;
  using ACTPNT = struct ACTIVEPROCESS*;
  using PROCPNT = struct PROCESSDESCRIPTOR*; // from interpret
  using BUSYPNT = struct BUSYTYPE*;
  using BLOCKR = struct BLOCKR {
    int START; // range 0..STMAX
    int SIZE; // range 0..STMAX
    BLKPNT NEXT;
  } __attribute__((aligned(16)));

  struct ACTIVEPROCESS;
  using ACTIVEPROCESS = struct ACTIVEPROCESS {
    PROCPNT PDES;
    ACTPNT NEXT;
  } __attribute__((aligned(16)));

  using BUSYTYPE = struct BUSYTYPE {
    double FIRST;
    double LAST;
    BUSYPNT NEXT;
  } __attribute__((aligned(32)));

  using STYPE = int;
  using RSTYPE = double;
  using BUFINTTYPE = int;
  using BUFREALTYPE = double;
  using COMTYP = enum class COMTYP : std::uint8_t {
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

  using PROCESSDESCRIPTOR = struct PROCESSDESCRIPTOR {
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

    enum class STATE : std::uint8_t {
      READY,
      RUNNING,
      BLOCKED,
      DELAYED,
      TERMINATED,
      SPINNING
    } STATE;

    int PID;
    double VIRTUALTIME;

    enum class READSTATUS : std::uint8_t {
      NONE, ATCHANNEL, HASTICKET
    } READSTATUS;

    int FORKCOUNT;
    int JOINSEM;
    double MAXFORKTIME;

    enum class PRIORITY : std::uint8_t { LOW, HIGH } PRIORITY;

    bool SEQON;
    bool GROUPREP;
  } __attribute__((aligned(128))) __attribute__((packed));

  using InterpLocal = struct InterpLocal {
    PROCPNT CURPR;

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
    } PS; // from interpret

    int LNCNT, CHRCNT;
    std::array<int, 4 + 1> FLD;
    STYPE *S, *SLOCATION;
    STYPE* STARTMEM;
    RSTYPE* RS;
    PROCPNT MAINPROC;

    struct PROCTAB {
      enum class STATUS : std::uint8_t {
        NEVERUSED,
        EMPTY,
        RESERVED,
        FULL
      } STATUS;

      double VIRTIME;
      double BRKTIME;
      double PROTIME;
      PROCPNT RUNPROC;
      int NUMPROC;
      double STARTTIME;
      BUSYPNT BUSYLIST;
      float SPEED;
    } __attribute__((aligned(64))) __attribute__((packed)) PROCTAB[PMAX + 1];

    int CNUM, H1;
    // int I;
    int J, K, PNT, FREE;

    struct Channel {
      int HEAD;
      int SEM;
      ACTPNT WAIT;
      double READTIME;
      bool MOVED;
      int READER;
    } __attribute__((aligned(32))) CHAN[OPCHMAX + 1];

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
    OBJECTS OBJ;
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

    struct {
      VARTYP NAME;
      int MEMLOC; // range -1..STMAX
    } __attribute__((aligned(32))) __attribute__((packed)) TRCTAB[VARMAX + 1];

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
  } __attribute__((aligned(128))) __attribute__((packed));

  // int HIGHESTPROCESSOR;
} // namespace Cstar
#endif // CSTAR_CS_INTERPRET_H
