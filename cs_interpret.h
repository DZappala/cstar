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

namespace Cstar {
typedef char VARTYP[NAMELEN + 1];
typedef struct BLOCKR *BLKPNT;
typedef struct ACTIVEPROCESS *ACTPNT;
typedef struct PROCESSDESCRIPTOR *PROCPNT; // from interpret
typedef struct BUSYTYPE *BUSYPNT;
typedef struct BLOCKR {
  int START; // range 0..STMAX
  int SIZE;  // range 0..STMAX
  BLKPNT NEXT;
} BLOCKR;
struct ACTIVEPROCESS;
typedef struct ACTIVEPROCESS {
  PROCPNT PDES;
  ACTPNT NEXT;
} ACTIVEPROCESS;
typedef struct BUSYTYPE {
  double FIRST;
  double LAST;
  BUSYPNT NEXT;
} BUSYTYPE;
typedef int STYPE;
typedef double RSTYPE;
typedef int BUFINTTYPE;
typedef double BUFREALTYPE;
typedef enum COMTYP {
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
} COMTYP;
typedef struct PROCESSDESCRIPTOR { // from interpret
  int T;                           // process's stack top index
  int B;
  int PC;        // process's program counter (index into ORDER type CODE array)
  int STACKSIZE; // process's stack size
  int DISPLAY[LMAX + 1];
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
  enum STATE { READY, RUNNING, BLOCKED, DELAYED, TERMINATED, SPINNING } STATE;
  int PID;
  double VIRTUALTIME;
  enum READSTATUS { NONE, ATCHANNEL, HASTICKET } READSTATUS;
  int FORKCOUNT;
  int JOINSEM;
  double MAXFORKTIME;
  enum PRIORITY { LOW, HIGH } PRIORITY;
  bool SEQON;
  bool GROUPREP;
} PROCESSDESCRIPTOR;
typedef struct InterpLocal {
  PROCPNT CURPR;

  enum PS {
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
  int FLD[4 + 1];
  STYPE *S, *SLOCATION;
  STYPE *STARTMEM;
  RSTYPE *RS;
  PROCPNT MAINPROC;
  struct PROCTAB {
    enum STATUS { NEVERUSED, EMPTY, RESERVED, FULL } STATUS;
    double VIRTIME;
    double BRKTIME;
    double PROTIME;
    PROCPNT RUNPROC;
    int NUMPROC;
    double STARTTIME;
    BUSYPNT BUSYLIST;
    float SPEED;
  } PROCTAB[PMAX + 1];

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
  } CHAN[OPCHMAX + 1];
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
  char LISTDEFNAME[FILMAX + 1];
  char INPUTFNAME[FILMAX + 1];
  char OUTPUTFNAME[FILMAX + 1];
  char LISTFNAME[FILMAX + 1];
  bool MPIINIT[PMAX + 1], MPIFIN[PMAX + 1];
  int MPICODE;
  int MPISEM;
  double MPITIME;
  int MPITYPE;
  int MPICNT;
  int MPIROOT;
  int MPIOP;
  int MPICOMM;
  int MPIPNT[PMAX + 1];
  int MPIRES[PMAX + 1];
  int MPICART[CARTMAX + 1][MAXDIM + 1];
  bool MPIPER[CARTMAX + 1][MAXDIM + 1];
  char PROMPT[10 + 1];
  int MAXSTEPS;
  int INX;
  int BRKTAB[BRKMAX + 1];
  int BRKLINE[BRKMAX + 1];
  int REF, ADR;
  TYPES TYP;
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
  } TRCTAB[VARMAX + 1];
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
  ALFA COMTAB[COMMAX + 1];
  ALFA ABBREVTAB[COMMAX + 1];
  COMTYP COMJMP[COMMAX + 1];
  COMTYP COMMLABEL;
  int LINECNT;
} InterpLocal;

// int HIGHESTPROCESSOR;

} // namespace Cstar
#endif // CSTAR_CS_INTERPRET_H
