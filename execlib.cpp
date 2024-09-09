//
// Created by Dan Evans on 2/1/24.
//
#include "cs_compile.h"
#include "cs_exec.h"
#include "cs_global.h"
#include "cs_interpret.h"
#include <cmath>
#include <cstdlib>
namespace Cstar {
#define OPCHMAX 30000 /*MAXIMUM NUMBER OF OPEN CHANNELS*/
#define MAXDIM 10
typedef int COORTYPE;
struct ExlibLocal {
  int I, J, PREV, H1, H2, H3, H4, H5, H6, H7, PNT;
  bool B1, B2;
  double R1, R2, R3;
  COORTYPE COORD[MAXDIM + 1];
  InterpLocal *il;
};
extern int FINDFRAME(InterpLocal *il, int LENGTH);
extern void RELEASE(InterpLocal *il, int BASE, int LENGTH);
extern void CHKVAR(InterpLocal *il, int STKLOC);
extern void TIMEINC(InterpLocal *, int, const char *);
extern int COMMDELAY(InterpLocal *, int, int, int);
extern int BTOI(bool);
void INITCHAN(InterpLocal *il, int CHID) {
  if (il->CHAN[CHID].HEAD == -1) {
    il->CHAN[CHID].HEAD = 0;
    il->CHAN[CHID].WAIT = nullptr;
    il->CHAN[CHID].SEM = 0;
    il->CHAN[CHID].READTIME = 0;
    il->CHAN[CHID].MOVED = false;
    il->CHAN[CHID].READER = -1;
  }
}
int HOPS(int SOURCE, int DEST) {
  int T1, T2, T3, T4, DIST;
  DIST = 0;
  switch (TOPOLOGY) {
  case FULLCONNSY:
  case CLUSTERSY:
  case HYPERCUBESY:
    T1 = SOURCE;
    T2 = DEST;
    DIST = 0;
    while (T1 + T2 > 0) {
      if (T1 % 2 != T2 % 2) {
        DIST++;
      }
      T1 /= 2;
      T2 /= 2;
    }
    break;
  case LINESY:
    DIST = abs(SOURCE - DEST);
    break;
  case MESH2SY:
    T1 = TOPDIM;
    T2 = abs(SOURCE / T1 - DEST / T1);
    T3 = abs(SOURCE % T1 - DEST % T1);
    DIST = T2 + T3;
    break;
  case MESH3SY:
    T2 = TOPDIM;
    T1 = TOPDIM * T2;
    T3 = SOURCE % T1;
    T4 = DEST % T1;
    DIST = abs(T3 / T2 - T4 / T2) + abs(T3 % T2 - T4 % T2) +
           abs(SOURCE / T1 - DEST / T1);
    break;
  case RINGSY:
    T1 = abs(SOURCE - DEST);
    T2 = TOPDIM - T1;
    if (T1 < T2) {
      DIST = T1;
    } else {
      DIST = T2;
    }
    break;
  case TORUSSY:
    T1 = TOPDIM;
    T3 = abs(SOURCE / T1 - DEST / T1);
    if (T3 > T1 / 2) {
      T3 = T1 - T3;
    }
    T4 = abs(SOURCE % T1 - DEST % T1);
    if (T4 > T1 / 2) {
      T4 = T1 - T4;
    }
    DIST = T3 + T4;
    break;
  default:
    break;
  }
  return DIST;
}

int GRPDELAY(InterpLocal *il, int lSRC, int DEST, int LEN) {
  int NUMPACK, NUMHOP, TD, T1, T2;
  int AVEFAN = 0, MAXFAN = 0;
  NUMPACK = LEN / 3;
  NUMHOP = HOPS(lSRC, DEST);
  switch (TOPOLOGY) {
  case HYPERCUBESY:
    AVEFAN = TOPDIM;
    MAXFAN = TOPDIM;
    break;
  case LINESY:
  case RINGSY:
    AVEFAN = 1;
    MAXFAN = 2;
    break;
  case MESH2SY:
  case TORUSSY:
    AVEFAN = 2;
    MAXFAN = 3;
    break;
  case MESH3SY:
    AVEFAN = 3;
    MAXFAN = 4;
    break;
  case FULLCONNSY:
  case CLUSTERSY:
    AVEFAN = (int)round(log(HIGHESTPROCESSOR + 1) / log(2));
    MAXFAN = AVEFAN;
    break;
  default:
    break;
  }
  TD = NUMHOP * (il->TOPDELAY + 5 + MPINODETIME * AVEFAN);
  T1 = il->TOPDELAY / 2;
  T2 = 5 + MPINODETIME * MAXFAN;
  if (NUMPACK > 1) {
    if (T1 >= T2) {
      TD += (NUMPACK - 1) * T1;
    } else {
      TD += (NUMPACK - 1) * T2;
    }
  }
  if (lSRC == DEST) {
    TD = NUMPACK * MPINODETIME * MAXFAN;
  }
  return TD;
}
bool CHKBUF(ExlibLocal *xl, int STRLOC, int ILEN) {
  InterpLocal *il = xl->il;
  if (STRLOC <= 0 || ILEN <= 0 || STRLOC + ILEN > STMAX ||
      il->STARTMEM[STRLOC] <= 0) {
    il->PS = InterpLocal::PS::REFCHK;
    return false;
  } else {
    for (int i = 1; i < ILEN; i++) {
      if (il->STARTMEM[STRLOC + i] != il->STARTMEM[STRLOC]) {
        il->PS = InterpLocal::PS::REFCHK;
        return false;
      }
      if (il->NUMTRACE > 0) {
        CHKVAR(il, STRLOC + i);
      }
    }
  }
  return true;
}

int GETRANK(ExlibLocal *el, int IROW, const int COOR[MAXDIM]) {
  int I, THISMAX, RANK, NDIM, THISCOOR;
  InterpLocal *il = el->il;
  NDIM = il->MPICART[IROW][0];
  RANK = 0;
  for (I = 1; I <= NDIM; I++) {
    THISMAX = il->MPICART[IROW][I];
    THISCOOR = COOR[I];
    if (THISCOOR >= THISMAX) {
      if (il->MPIPER[IROW][I]) {
        THISCOOR = THISCOOR % THISMAX;
      } else {
        RANK = MPIPROCNULL;
      }
    }
    if (THISCOOR < 0) {
      if (il->MPIPER[IROW][I]) {
        THISCOOR = THISMAX - (abs(THISCOOR) % THISMAX);
      } else {
        RANK = MPIPROCNULL;
        goto L23;
      }
    }
    RANK = RANK * il->MPICART[IROW][I] + THISCOOR;
  }
L23:
  return RANK;
}

bool CHKCOMM(ExlibLocal *el, int COMMCODE) {
  bool RES = true;

  if (COMMCODE == MPICOMMWORLD) {
    RES = false;
  } else if (COMMCODE >= CARTSTART && COMMCODE <= CARTSTART + CARTMAX) {
    if (el->il->MPICART[COMMCODE - CARTSTART][0] != -1) {
      RES = false;
    }
  }
  return RES;
}

int CHKLEN(ExlibLocal *el, int STRPNT) {
  InterpLocal *il = el->il;
  if (STRPNT < 0 || STRPNT > STMAX) {
    il->PS = InterpLocal::PS::REFCHK;
    return -1;
  }
  if (il->STARTMEM[STRPNT] <= 0) {
    il->PS = InterpLocal::PS::REFCHK;
    return -1;
  }
  int I = STRPNT;
  int LEN = 0;
  while (il->S[I] != 0) {
    if (il->S[I] < CHARL || il->S[I] > CHARH) {
      il->PS = InterpLocal::PS::STRCHK;
      return -1;
    }
    LEN++;
    I++;
    if (il->STARTMEM[I] != il->STARTMEM[STRPNT]) {
      il->PS = InterpLocal::PS::STRCHK;
      return -1;
    }
  }
  return LEN;
}
void EXECLIB(InterpLocal *il, ExLocal *el, PROCPNT CURPR, int lID) {
  long rnd;
  ExlibLocal xl = {0};
  PROCPNT proc;
  // memset(&xl, 0, sizeof(xl));
  xl.il = il;
  // fprintf(STDOUT, "execlib call (%d)\n", ID);
  // WITH CURPR
  switch (lID) {
  case 1: // integer abs
    il->S[CURPR->T] = abs(il->S[CURPR->T]);
    break;
  case 2: // real abs
    il->RS[CURPR->T] = fabs(il->RS[CURPR->T]);
    break;
  case 9: // ceil
    xl.R1 = il->RS[CURPR->T];
    if (xl.R1 <= 0 || xl.R1 == trunc(xl.R1)) {
      il->S[CURPR->T] = (int)trunc(xl.R1);
    } else {
      il->S[CURPR->T] = (int)trunc(xl.R1) + 1;
    }
    il->RS[CURPR->T] = il->S[CURPR->T];
    il->S[CURPR->T] = RTAG;
    break;
  case 10: // floor
    xl.R1 = il->RS[CURPR->T];
    if (xl.R1 >= 0 || xl.R1 == trunc(xl.R1)) {
      il->S[CURPR->T] = (int)trunc(xl.R1);
    } else {
      il->S[CURPR->T] = (int)trunc(xl.R1) - 1;
    }
    il->RS[CURPR->T] = il->S[CURPR->T];
    il->S[CURPR->T] = RTAG;
    break;
  case 11: // sin
    il->RS[CURPR->T] = sin(il->RS[CURPR->T]);
    break;
  case 12: // cos
    il->RS[CURPR->T] = cos(il->RS[CURPR->T]);
    break;
  case 13: // exp
    il->RS[CURPR->T] = exp(il->RS[CURPR->T]);
    break;
  case 14: // log
    il->RS[CURPR->T] = log(il->RS[CURPR->T]);
    break;
  case 15: // sqrt
    il->RS[CURPR->T] = sqrt(il->RS[CURPR->T]);
    break;
  case 16: // atan
    il->RS[CURPR->T] = atan(il->RS[CURPR->T]);
    break;
  case 17:
    xl.H1 = il->S[CURPR->T];
    if (xl.H1 <= 0 || xl.H1 >= STMAX) {
      il->S[CURPR->T] = 0;
    } else {
      xl.H2 = FINDFRAME(il, xl.H1);
      if (xl.H2 > 0) {
        il->S[CURPR->T] = xl.H2;
        for (xl.I = 0; xl.I < xl.H1; xl.I++) {
          il->SLOCATION[xl.H2 + xl.I] = CURPR->PROCESSOR;
        }
      } else {
        il->S[CURPR->T] = 0;
      }
    }
    break;
  case 18: // calloc
    xl.H1 = il->S[CURPR->T] * il->S[CURPR->T - 1];
    CURPR->T = CURPR->T - 1;
    if (xl.H1 <= 0 || xl.H1 >= STMAX) {
      il->S[CURPR->T] = 0;
    } else {
      xl.H2 = FINDFRAME(il, xl.H1);
      if (xl.H2 > 0) {
        for (xl.I = 0; xl.I < xl.H1; xl.I++) {
          il->SLOCATION[xl.H2 + xl.I] = CURPR->PROCESSOR;
          il->S[xl.H2 + xl.I] = 0;
          il->RS[xl.H2 + xl.I] = 0.0;
        }
        il->S[CURPR->T] = xl.H2;
      } else {
        il->S[CURPR->T] = 0;
      }
    }
    // fprintf(STDOUT, "calloc len=%d at=%d\n", xl.H1, xl.H2);
    break;
  case 19: // realloc
    xl.H1 = il->S[CURPR->T - 1];
    xl.H2 = il->S[CURPR->T];
    CURPR->T = CURPR->T - 1;
    if (xl.H2 < 0) {
      il->S[CURPR->T] = 0;
      break;
    }
    if (xl.H1 == 0) {
      xl.H3 = FINDFRAME(il, xl.H2);
      if (xl.H3 > 0) {
        for (xl.I = 0; xl.I < xl.H2; xl.I++) {
          il->SLOCATION[xl.H3 + xl.I] = CURPR->PROCESSOR;
        }
        il->S[CURPR->T] = xl.H3;
      } else {
        il->S[CURPR->T] = 0;
      }
      break;
    }
    if (xl.H1 < 0 || xl.H1 >= STMAX || (il->STARTMEM[xl.H1] != xl.H1) ||
        (il->STARTMEM[xl.H1] == il->STARTMEM[xl.H1 - 1]) ||
        (il->STARTMEM[xl.H1 - 1] < 0)) {
      il->PS = InterpLocal::PS::REFCHK;
      break;
    }
    xl.H3 = 1;
    while (il->STARTMEM[xl.H1 + xl.H3] == xl.H1) {
      xl.H3 = xl.H3 + 1;
    }
    if (xl.H2 == xl.H3) {
      break;
    }
    if (xl.H2 < xl.H3) {
      RELEASE(il, xl.H1 + xl.H2, xl.H3 - xl.H2);
    } else {
      xl.H4 = FINDFRAME(il, xl.H2);
      if (xl.H4 < 0) {
        il->S[CURPR->T] = 0;
        break;
      }
      for (xl.I = 0; xl.I < xl.H3; xl.I++) {
        il->S[xl.H4 + xl.I] = il->S[xl.H1 + xl.I];
        il->SLOCATION[xl.H4 + xl.I] = CURPR->PROCESSOR;
      }
      il->S[CURPR->T] = xl.H4;
      RELEASE(il, xl.H1, xl.H3);
    }
    break;

  case 20: // free ?
    xl.H1 = il->S[CURPR->T];
    CURPR->T = CURPR->T - 1;
    if (xl.H1 == 0) {
      break;
    }
    //                if (xl.H1 < 0 || xl.H1 >= STMAX || (il->STARTMEM[xl.H1] !=
    //                xl.H1) ||
    //                    (il->STARTMEM[xl.H1] == il->STARTMEM[xl.H1 - 1]) ||
    //                    (il->STARTMEM[xl.H1 - 1] < 0)) {
    //                    il->PS = InterpLocal::PS::REFCHK;
    //                    break;
    if (xl.H1 < 0 || xl.H1 >= STMAX) {
      il->PS = InterpLocal::PS::REFCHK;
      break;
    }
    if (il->STARTMEM[xl.H1] != xl.H1) {
      il->PS = InterpLocal::PS::REFCHK;
      break;
    }
    if (il->STARTMEM[xl.H1] == il->STARTMEM[xl.H1 - 1]) {
      il->PS = InterpLocal::PS::REFCHK;
      break;
    }
    if (il->STARTMEM[xl.H1 - 1] < 0) {
      il->PS = InterpLocal::PS::REFCHK;
      break;
    }
    xl.H2 = 1;
    while (il->STARTMEM[xl.H1 + xl.H2] == xl.H1) {
      xl.H2 = xl.H2 + 1;
    }
    // fprintf(STDOUT, "free len=%d at=%d\n", xl.H2, xl.H1);
    RELEASE(il, xl.H1, xl.H2);
    break;
  case 21:
    xl.H1 = il->S[CURPR->T - 1];
    xl.H2 = il->S[CURPR->T];
    CURPR->T = CURPR->T - 1;
    xl.H3 = CHKLEN(&xl, xl.H1);
    xl.H4 = CHKLEN(&xl, xl.H2);
    if (xl.H3 < 0 || xl.H4 < 0) {
      break;
    }
    if (xl.H1 + xl.H3 + xl.H4 > STMAX) {
      il->PS = InterpLocal::PS::STKCHK;
      break;
    }
    for (xl.I = 0; xl.I <= xl.H4; xl.I++) {
      if (il->STARTMEM[xl.H1 + xl.H3 + xl.I] != il->STARTMEM[xl.H1]) {
        il->PS = InterpLocal::PS::REFCHK;
        break;
      }
    }
    for (xl.I = 0; xl.I <= xl.H4; xl.I++) {
      il->S[xl.H1 + xl.H3 + xl.I] = il->S[xl.H2 + xl.I];
    }
    break;
  case 22:
    xl.H1 = il->S[CURPR->T - 1];
    xl.H2 = il->S[CURPR->T];
    CURPR->T = CURPR->T - 1;
    xl.H3 = CHKLEN(&xl, xl.H1);
    if (xl.H3 < 0) {
      break;
    }
    if (xl.H2 < CHARL || xl.H2 > CHARH) {
      il->PS = InterpLocal::PS::CHRCHK;
      break;
    }
    il->S[CURPR->T] = 0;
    for (xl.I = xl.H3 - 1; xl.I >= 0; xl.I--) {
      if (il->S[xl.H1 + xl.I] == xl.H2) {
        il->S[CURPR->T] = xl.H1 + xl.I;
        break;
      }
    }
    break;
  case 23:
    xl.H1 = il->S[CURPR->T - 1];
    xl.H2 = il->S[CURPR->T];
    CURPR->T = CURPR->T - 1;
    xl.H3 = CHKLEN(&xl, xl.H1);
    xl.H4 = CHKLEN(&xl, xl.H2);
    if (xl.H3 < 0 || xl.H4 < 0) {
      break;
    }
    xl.H6 = 0;
    if (xl.H3 < xl.H4) {
      xl.H5 = xl.H3;
    } else {
      xl.H5 = xl.H4;
    }
    xl.I = 0;
    xl.B1 = true;
    while (xl.I <= xl.H5 && xl.B1) {
      if (il->S[xl.H1 + xl.I] < il->S[xl.H2 + xl.I]) {
        xl.B1 = false;
        xl.H6 = -1;
      }
      if (il->S[xl.H1 + xl.I] > il->S[xl.H2 + xl.I]) {
        xl.B1 = false;
        xl.H6 = 1;
      }
      xl.I = xl.I + 1;
    }
    if (xl.H6 == 0) {
      if (xl.H3 < xl.H4) {
        xl.H6 = -1;
      }
      if (xl.H3 > xl.H4) {
        xl.H6 = 1;
      }
    }
    il->S[CURPR->T] = xl.H6;
    break;

  case 24: // strcpy
    xl.H1 = il->S[CURPR->T - 1];
    xl.H2 = il->S[CURPR->T];
    CURPR->T = CURPR->T - 1;
    xl.H3 = CHKLEN(&xl, xl.H2);
    if (xl.H3 < 0) {
      break;
    }
    if (xl.H1 + xl.H3 > STMAX) {
      il->PS = InterpLocal::PS::STKCHK;
      break;
    }
    if (il->STARTMEM[xl.H1 + xl.H3] != il->STARTMEM[xl.H1]) {
      il->PS = InterpLocal::PS::REFCHK;
      break;
    }
    for (xl.I = 0; xl.I <= xl.H3; xl.I++) {
      il->S[xl.H1 + xl.I] = il->S[xl.H2 + xl.I];
    }
    break;
  case 25: // strlen
    xl.H1 = il->S[CURPR->T];
    xl.H2 = CHKLEN(&xl, xl.H1);
    if (xl.H2 < 0) {
      break;
    }
    il->S[CURPR->T] = xl.H2;
    break;
  case 26:
    xl.H1 = il->S[CURPR->T - 1];
    xl.H2 = il->S[CURPR->T];
    CURPR->T = CURPR->T - 1;
    xl.H3 = CHKLEN(&xl, xl.H1);
    xl.H4 = CHKLEN(&xl, xl.H2);
    if (xl.H3 < 0 || xl.H4 < 0) {
      break;
    }
    il->S[CURPR->T] = 0;
    if (xl.H4 > xl.H3) {
      break;
    }
    xl.H5 = xl.H3 - xl.H4;
    xl.B1 = true;
    xl.I = 0;
    while (xl.B1 && xl.I <= xl.H5) {
      xl.B1 = false;
      for (xl.J = 0; xl.J < xl.H4; xl.J++) {
        if (il->S[xl.H1 + xl.I + xl.J] != il->S[xl.H2 + xl.J]) {
          xl.B1 = true;
          break;
        }
      }
      xl.I = xl.I + 1;
    }
    if (!xl.B1) {
      il->S[CURPR->T] = xl.H1 + xl.I - 1;
    }
    break;
  case 27: // isalnum
    xl.H1 = il->S[CURPR->T];
    il->S[CURPR->T] = 0;
    if ((xl.H1 > 47 && xl.H1 < 58) || (xl.H1 > 64 && xl.H1 < 91) ||
        (xl.H1 > 96 && xl.H1 < 123)) {
      il->S[CURPR->T] = 1;
    }
    break;
  case 28: // isalpha
    xl.H1 = il->S[CURPR->T];
    il->S[CURPR->T] = 0;
    if ((xl.H1 > 64 && xl.H1 < 91) || (xl.H1 > 96 && xl.H1 < 123)) {
      il->S[CURPR->T] = 1;
    }
    break;
  case 29: // iscntrl
    xl.H1 = il->S[CURPR->T];
    il->S[CURPR->T] = 0;
    if ((xl.H1 >= 0 && xl.H1 < 32) || xl.H1 == 127) {
      il->S[CURPR->T] = 1;
    }
    break;
  case 30: // isdigit
    xl.H1 = il->S[CURPR->T];
    il->S[CURPR->T] = 0;
    if (xl.H1 > 47 && xl.H1 < 58) {
      il->S[CURPR->T] = 1;
    }
    break;
  case 31: // isgraph
    xl.H1 = il->S[CURPR->T];
    il->S[CURPR->T] = 0;
    if (xl.H1 > 32 && xl.H1 < 127) {
      il->S[CURPR->T] = 1;
    }
    break;
  case 32: // islower
    xl.H1 = il->S[CURPR->T];
    il->S[CURPR->T] = 0;
    if (xl.H1 > 96 && xl.H1 < 123) {
      il->S[CURPR->T] = 1;
    }
    break;
  case 33: // isprint
    xl.H1 = il->S[CURPR->T];
    il->S[CURPR->T] = 0;
    if (xl.H1 > 31 && xl.H1 < 127) {
      il->S[CURPR->T] = 1;
    }
    break;
  case 34: // ispunct
    xl.H1 = il->S[CURPR->T];
    il->S[CURPR->T] = 0;
    if ((xl.H1 > 33 && xl.H1 < 48) || (xl.H1 > 57 && xl.H1 < 65) ||
        (xl.H1 > 90 && xl.H1 < 97) || (xl.H1 > 122 && xl.H1 < 127)) {
      il->S[CURPR->T] = 1;
    }
    break;
  case 35: // isspace
    xl.H1 = il->S[CURPR->T];
    il->S[CURPR->T] = 0;
    if ((xl.H1 > 8 && xl.H1 < 14) || xl.H1 == 32) {
      il->S[CURPR->T] = 1;
    }
    break;
  case 36: // isupper
    xl.H1 = il->S[CURPR->T];
    il->S[CURPR->T] = 0;
    if (xl.H1 > 64 && xl.H1 < 91) {
      il->S[CURPR->T] = 1;
    }
    break;
  case 37: // isxdigit
    xl.H1 = il->S[CURPR->T];
    il->S[CURPR->T] = 0;
    if ((xl.H1 > 47 && xl.H1 < 58) || (xl.H1 > 64 && xl.H1 < 71) ||
        (xl.H1 > 96 && xl.H1 < 103)) {
      il->S[CURPR->T] = 1;
    }
    break;
  case 38: // tolower
    xl.H1 = il->S[CURPR->T];
    if (xl.H1 > 64 && xl.H1 < 91) {
      il->S[CURPR->T] = xl.H1 + 32;
    }
    break;
  case 39:
    xl.H1 = il->S[CURPR->T];
    if (xl.H1 > 96 && xl.H1 < 123) {
      il->S[CURPR->T] = xl.H1 - 32;
    }
    break;
  case 40:
    il->PS = InterpLocal::PS::USERSTOP;
    break;
  case 41:
    CURPR->T = CURPR->T - 1;
    if (CURPR->FORKCOUNT > 1) {
      CURPR->STATE = PROCESSDESCRIPTOR::STATE::BLOCKED;
      CURPR->PC -= 1;
      il->PROCTAB[CURPR->PROCESSOR].RUNPROC = nullptr;
      CURPR->FORKCOUNT -= 1;
    } else {
      il->PS = InterpLocal::PS::FIN;
      CURPR->STATE = PROCESSDESCRIPTOR::STATE::TERMINATED;
    }
    break;
  //      BEGIN
  //       H1 := S^[T-1];
  //       H2 := S^[T];
  //       T := T - 1;
  //       IF H2 = 0 THEN PS := DIVCHK
  //       ELSE BEGIN
  //         H3 := FINDFRAME(2);
  //         IF H3 < 0 THEN
  //           BEGIN  PS := STKCHK; GOTO 89  END;
  //         S^[H3] := H1 DIV H2;
  //         S^[H3+1] := H1 MOD H2;
  //         S^[T] := H3
  //       END;
  //     END;
  case 42:
    xl.H1 = il->S[CURPR->T - 1];
    xl.H2 = il->S[CURPR->T];
    CURPR->T = CURPR->T - 1;
    if (xl.H2 == 0)
      il->PS = InterpLocal::PS::DIVCHK;
    else {
      xl.H3 = FINDFRAME(il, 2);
      if (xl.H3 < 0) {
        il->PS = InterpLocal::PS::STKCHK;
        break;
      }
      il->S[xl.H3] = xl.H1 / xl.H2;
      il->S[xl.H3 + 1] = xl.H1 % xl.H2;
      il->S[CURPR->T] = xl.H3;
    }
    break;
  case 43:
    CURPR->T = CURPR->T + 1;
#ifdef MAC
    rnd = random();
#else
    rnd = std::rand();
#endif
    il->S[CURPR->T] = (int)(rnd % (NMAX + 1));
    break;
  case 50:
    il->MPIINIT[CURPR->PROCESSOR] = true;
    CURPR->T = CURPR->T + 1;
    il->S[CURPR->T] = MPISUCCESS;
    break;
  case 51: // finalize
    if (!il->MPIINIT[CURPR->PROCESSOR]) {
      il->PS = InterpLocal::PS::MPIINITCHK;
      break;
    }
    il->MPIFIN[CURPR->PROCESSOR] = true;
    CURPR->T = CURPR->T + 1;
    il->CNUM = CURPR->PID;
    INITCHAN(il, il->CNUM);
    if (il->CHAN[il->CNUM].SEM == 0) {
      il->S[CURPR->T] = MPISUCCESS;
    } else {
      il->S[CURPR->T] = -1;
    }
    break;
  case 52:
    if (!il->MPIINIT[CURPR->PROCESSOR]) {
      il->PS = InterpLocal::PS::MPIINITCHK;
      break;
    }
    if (CHKCOMM(&xl, il->S[CURPR->T - 1])) {
      il->PS = InterpLocal::PS::MPIPARCHK;
      break;
    }
    xl.H1 = il->S[CURPR->T];
    il->S[xl.H1] = CURPR->PROCESSOR;
    if (il->NUMTRACE > 0) {
      CHKVAR(il, xl.H1);
    }
    CURPR->T = CURPR->T - 1;
    il->S[CURPR->T] = MPISUCCESS;
    break;
  case 53:
    if (!il->MPIINIT[CURPR->PROCESSOR]) {
      il->PS = InterpLocal::PS::MPIINITCHK;
      break;
    }
    if (CHKCOMM(&xl, il->S[CURPR->T - 1])) {
      il->PS = InterpLocal::PS::MPIPARCHK;
      break;
    }
    xl.H1 = il->S[CURPR->T];
    il->S[xl.H1] = HIGHESTPROCESSOR + 1;
    if (il->NUMTRACE > 0) {
      CHKVAR(il, xl.H1);
    }
    CURPR->T = CURPR->T - 1;
    il->S[CURPR->T] = MPISUCCESS;
    break;
  case 54:
    if (!il->MPIINIT[CURPR->PROCESSOR]) {
      il->PS = InterpLocal::PS::MPIINITCHK;
      break;
    }
    if (CHKCOMM(&xl, il->S[CURPR->T]) || (il->S[CURPR->T - 1] < 0) ||
        (il->S[CURPR->T - 2] < 0) || (il->S[CURPR->T - 2] > HIGHESTPROCESSOR) ||
        (il->S[CURPR->T - 3] != MPIINT && il->S[CURPR->T - 3] != MPIFLOAT &&
         il->S[CURPR->T - 3] != MPICHAR)) {
      il->PS = InterpLocal::PS::MPIPARCHK;
      break;
    }
    il->CNUM = il->S[CURPR->T - 2];
    INITCHAN(il, il->CNUM);
    xl.H1 = il->S[CURPR->T - 5]; // stk addr of buffer
    xl.H2 = il->S[CURPR->T - 2]; // target processor
    xl.H3 = il->S[CURPR->T - 4]; // buffer count
    if (!CHKBUF(&xl, xl.H1, xl.H3)) {
      break;
    }
    xl.H4 = FINDFRAME(il, xl.H3 + 4);
    if (xl.H4 < 0) {
      break;
    }
    il->S[xl.H4] = xl.H3;
    il->S[xl.H4 + 1] = il->S[CURPR->T - 3];
    il->S[xl.H4 + 2] = CURPR->PID;
    il->S[xl.H4 + 3] = il->S[CURPR->T - 1];
    for (xl.I = 0; xl.I < xl.H3; xl.I++) {
      if (il->STARTMEM[xl.H1 + xl.I] != il->STARTMEM[xl.H1]) {
        il->PS = InterpLocal::PS::REFCHK;
        break;
      }
      il->S[xl.H4 + xl.I + 4] = il->S[xl.H1 + xl.I];
      if (il->S[CURPR->T - 3] == MPIFLOAT) {
        if (il->S[xl.H1 + xl.I] == RTAG) {
          il->RS[xl.H4 + xl.I + 4] = il->RS[xl.H1 + xl.I];
        } else if (il->S[xl.H1 + xl.I] != 0) {
          il->PS = InterpLocal::PS::MPITYPECHK;
          break;
        }
      } else if (il->S[xl.H1 + xl.I] == RTAG) {
        il->PS = InterpLocal::PS::MPITYPECHK;
        break;
      }
    }
    TIMEINC(il, xl.H3 / 3, "xl54");
    if (il->FREE == 0) {
      il->PS = InterpLocal::PS::BUFCHK;
      break;
    }
    TIMEINC(il, CHANTIME, "xl54+");
    il->K = il->FREE;
    il->DATE[il->K] =
        CURPR->TIME + COMMDELAY(il, CURPR->PROCESSOR, xl.H2, xl.H3);
    il->FREE = il->LINK[il->FREE];
    il->LINK[il->K] = 0;
    if (il->CHAN[il->CNUM].HEAD == 0) {
      il->CHAN[il->CNUM].HEAD = il->K;
    } else {
      xl.PNT = il->CHAN[il->CNUM].HEAD;
      xl.I = 0;
      xl.B1 = true;
      while (xl.PNT != 0 && xl.B1) {
        if (xl.I != 0) {
          el->TGAP = (int)(il->DATE[xl.PNT] - il->DATE[xl.I]);
        } else {
          el->TGAP = CHANTIME + 3;
        }
        if (il->DATE[xl.PNT] > il->DATE[il->K] && el->TGAP > CHANTIME + 1) {
          xl.B1 = false;
        } else {
          xl.I = xl.PNT;
          xl.PNT = il->LINK[xl.PNT];
        }
      }
      il->LINK[il->K] = xl.PNT;
      if (xl.I == 0) {
        il->CHAN[il->CNUM].HEAD = il->K;
      } else {
        il->LINK[xl.I] = il->K;
        if (il->DATE[il->K] < il->DATE[xl.I] + CHANTIME) {
          il->DATE[il->K] = il->DATE[xl.I] + CHANTIME;
        }
      }
    }
    if (il->CHAN[il->CNUM].WAIT != nullptr) {
      PROCPNT wt = il->CHAN[il->CNUM].WAIT->PDES;
      wt->STATE = PROCESSDESCRIPTOR::STATE::DELAYED;
      if (il->DATE[il->K] < wt->WAKETIME) {
        wt->WAKETIME = il->DATE[il->K];
      }
    }
    il->VALUE[il->K] = xl.H4;
    il->CHAN[il->CNUM].SEM += 1;
    CURPR->T = CURPR->T - 5;
    il->S[CURPR->T] = MPISUCCESS;
    break;
  case 55:
    if (!il->MPIINIT[CURPR->PROCESSOR]) {
      il->PS = InterpLocal::PS::MPIINITCHK;
      break;
    }
    if (il->S[CURPR->T] <= 0 || CHKCOMM(&xl, il->S[CURPR->T - 1]) ||
        ((il->S[CURPR->T - 2] < 0) && (il->S[CURPR->T - 2] != MPIANYTAG)) ||
        ((il->S[CURPR->T - 3] < 0) && (il->S[CURPR->T - 3] != MPIANYSOURCE)) ||
        (il->S[CURPR->T - 3] > HIGHESTPROCESSOR) ||
        (il->S[CURPR->T - 4] != MPIINT && il->S[CURPR->T - 4] != MPIFLOAT &&
         il->S[CURPR->T - 4] != MPICHAR)) {
      il->PS = InterpLocal::PS::MPIPARCHK;
      break;
    }
    xl.H1 = il->S[CURPR->T - 6];
    xl.H2 = il->S[CURPR->T - 3];
    xl.H3 = il->S[CURPR->T - 2];
    if (!CHKBUF(&xl, xl.H1, il->S[CURPR->T - 5])) {
      break;
    }
    il->CNUM = CURPR->PID;
    INITCHAN(il, il->CNUM);
    // with CHAN[CNUM]
    xl.PNT = il->CHAN[il->CNUM].HEAD;
    xl.PREV = 0;
    xl.B1 = true;
    xl.B2 = false;
    while (xl.B1 && xl.PNT != 0) {
      xl.H4 = il->VALUE[xl.PNT];
      if ((xl.H2 == MPIANYSOURCE || il->S[xl.H4 + 2] == xl.H2) &&
          (xl.H3 == MPIANYTAG || il->S[xl.H4 + 3] == xl.H3)) {
        xl.B1 = false;
      } else {
        xl.PREV = xl.PNT;
        xl.PNT = il->LINK[xl.PNT];
      }
    }
    if (!xl.B1) {
      xl.B2 = il->DATE[xl.PNT] > CURPR->TIME;
    }
    if (xl.B1 || xl.B2) {
      il->PTEMP = (ACTPNT)calloc(1, sizeof(ACTIVEPROCESS));
      il->PTEMP->PDES = CURPR;
      il->PTEMP->NEXT = nullptr;
      il->PROCTAB[CURPR->PROCESSOR].RUNPROC = nullptr;
      il->CHAN[il->CNUM].WAIT = il->PTEMP;
      if (xl.B2) {
        CURPR->STATE = PROCESSDESCRIPTOR::STATE::DELAYED;
        CURPR->WAKETIME = il->DATE[xl.PNT];
      } else {
        CURPR->STATE = PROCESSDESCRIPTOR::STATE::BLOCKED;
      }
      CURPR->PC -= 1;
      il->NOSWITCH = false;
    } else {
      TIMEINC(il, CHANTIME, "xl55");
      il->CHAN[il->CNUM].SEM -= 1;
      if (xl.PREV == 0) {
        il->CHAN[il->CNUM].HEAD = il->LINK[xl.PNT];
      } else {
        il->LINK[xl.PREV] = il->LINK[xl.PNT];
      }
      xl.H4 = il->VALUE[xl.PNT];
      il->LINK[xl.PNT] = il->FREE;
      il->FREE = xl.PNT;
      if (il->CHAN[il->CNUM].WAIT != nullptr) {
        il->PTEMP = il->CHAN[il->CNUM].WAIT;
        il->CHAN[il->CNUM].WAIT = nullptr;
        std::free(il->PTEMP);
      }
    }
    if (!(xl.B1 || xl.B2)) {
      xl.H5 = il->S[CURPR->T];
      il->S[xl.H5] = il->S[xl.H4 + 2];
      il->S[xl.H5 + 1] = il->S[xl.H4 + 3];
      il->S[xl.H5 + 2] = 0;
      if (il->S[xl.H4] > il->S[CURPR->T - 5]) {
        il->PS = InterpLocal::PS::MPICNTCHK;
        break;
      }
      il->RS[xl.H5 + 2] = il->S[xl.H4];
      if (il->S[xl.H4 + 1] != il->S[CURPR->T - 4]) {
        il->PS = InterpLocal::PS::MPITYPECHK;
        break;
      }
      for (xl.I = 0; xl.I < il->S[xl.H4]; xl.I++) {
        if (il->STARTMEM[xl.H1 + xl.I] != il->STARTMEM[xl.H1]) {
          il->PS = InterpLocal::PS::REFCHK;
          break;
        }
        il->S[xl.H1 + xl.I] = il->S[xl.H4 + xl.I + 4];
        if (il->S[CURPR->T - 4] == MPIFLOAT) {
          il->RS[xl.H1 + xl.I] = il->RS[xl.H4 + xl.I + 4];
        }
      }
      RELEASE(il, xl.H4, il->S[xl.H4] + 4);
      TIMEINC(il, il->S[xl.H4] / 3, "xl56");
      CURPR->T = CURPR->T - 6;
      il->S[CURPR->T] = MPISUCCESS;
    }
    break;
  case 56:
    if (!il->MPIINIT[CURPR->PROCESSOR]) {
      il->PS = InterpLocal::PS::MPIINITCHK;
      break;
    }
    if (il->S[CURPR->T] <= 1 ||
        !(il->S[CURPR->T - 1] == MPIINT || il->S[CURPR->T - 1] == MPIFLOAT ||
          il->S[CURPR->T - 1] == MPICHAR) ||
        il->S[CURPR->T - 2] <= 0) {
      il->PS = InterpLocal::PS::MPIPARCHK;
      break;
    }
    xl.H1 = il->S[CURPR->T - 2];
    xl.H2 = il->S[CURPR->T];
    if (il->NUMTRACE > 0) {
      CHKVAR(il, xl.H2);
    }
    if (il->S[xl.H1] < 0 || il->S[xl.H1] > HIGHESTPROCESSOR ||
        il->S[xl.H1 + 1] < 0 || il->S[xl.H1 + 2] != 0) {
      il->PS = InterpLocal::PS::MPIPARCHK;
      break;
    }
    if (il->STARTMEM[xl.H2] <= 0) {
      il->PS = InterpLocal::PS::REFCHK;
      break;
    }
    il->S[xl.H2] = (int)il->RS[xl.H1 + 2];
    CURPR->T = CURPR->T - 2;
    il->S[CURPR->T] = MPISUCCESS;
    break;
  case 57:
    if (!il->MPIINIT[CURPR->PROCESSOR]) {
      il->PS = InterpLocal::PS::MPIINITCHK;
      break;
    }
    if (CHKCOMM(&xl, il->S[CURPR->T])) {
      il->PS = InterpLocal::PS::MPIPARCHK;
      break;
    }
    if (il->MPICODE == -1) {
      il->MPICODE = 57;
      il->MPITIME = 0;
      il->MPISEM = HIGHESTPROCESSOR + 1;
    } else if (il->MPICODE != 57) {
      il->PS = InterpLocal::PS::MPIGRPCHK;
      break;
    }
    il->MPISEM = il->MPISEM - 1;
    xl.H1 = HIGHESTPROCESSOR / 2;
    if (CURPR->PID != xl.H1) {
      xl.R1 = CURPR->TIME + GRPDELAY(xl.il, CURPR->PROCESSOR, xl.H1, 1);
    } else {
      xl.R1 = CURPR->TIME;
    }
    if (il->MPITIME < xl.R1) {
      il->MPITIME = xl.R1;
    }
    if (il->MPISEM == 0) {
      il->PROCTAB[CURPR->PROCESSOR].RUNPROC = nullptr;
      il->PTEMP = il->ACPHEAD;
      // with PTEMP^.PDES
      proc = il->PTEMP->PDES;
      do {
        xl.H2 = GRPDELAY(xl.il, xl.H1, proc->PROCESSOR, 1);
        proc->STATE = PROCESSDESCRIPTOR ::STATE::DELAYED;
        proc->WAKETIME = il->MPITIME + xl.H2;
        il->PTEMP = il->PTEMP->NEXT;
        proc = il->PTEMP->PDES; // ?? is this the way Pascal "with" works
        il->S[proc->T] = MPISUCCESS;
      } while (il->PTEMP != il->ACPHEAD);
      il->MPICODE = -1;
    } else {
      il->PROCTAB[CURPR->PROCESSOR].RUNPROC = nullptr;
      CURPR->STATE = PROCESSDESCRIPTOR ::STATE::BLOCKED;
    }
    break;
  case 58: {
    if (!il->MPIINIT[CURPR->PROCESSOR]) {
      il->PS = InterpLocal::PS::MPIINITCHK;
      break;
    }
    if (CHKCOMM(&xl, il->S[CURPR->T]) || (il->S[CURPR->T - 1] < 0) ||
        (il->S[CURPR->T - 1] > HIGHESTPROCESSOR) ||
        (il->S[CURPR->T - 2] != MPIINT && il->S[CURPR->T - 2] != MPIFLOAT &&
         il->S[CURPR->T - 2] != MPICHAR)) {
      il->PS = InterpLocal::PS::MPIPARCHK;
      break;
    }
    if (!CHKBUF(&xl, il->S[CURPR->T - 4], il->S[CURPR->T - 3])) {
      break;
    }
    if (il->MPICODE == -1) {
      il->MPICODE = 58;
      il->MPITIME = 0;
      il->MPISEM = HIGHESTPROCESSOR + 1;
      il->MPIROOT = il->S[CURPR->T - 1];
      il->MPITYPE = il->S[CURPR->T - 2];
      il->MPICNT = il->S[CURPR->T - 3];
    } else if (il->MPICODE != 58 || il->S[CURPR->T - 1] != il->MPIROOT ||
               il->S[CURPR->T - 2] != il->MPITYPE ||
               il->S[CURPR->T - 3] != il->MPICNT) {
      il->PS = InterpLocal::PS::MPIGRPCHK;
      break;
    }
    xl.H2 = il->S[CURPR->T - 4];
    il->MPIPNT[CURPR->PID] = xl.H2;
    il->MPISEM = il->MPISEM - 1;
    if (CURPR->PID == il->MPIROOT) {
      il->MPITIME = CURPR->TIME;
    }
    if (il->MPISEM == 0) {
      xl.H1 = il->MPIPNT[il->MPIROOT];
      if (il->MPITYPE == MPIFLOAT) {
        for (xl.I = xl.H1; xl.I < xl.H1 + il->MPICNT; xl.I++) {
          if (il->S[xl.I] != RTAG && il->S[xl.I] != 0) {
            il->PS = InterpLocal::PS::MPITYPECHK;
            break;
          }
        }
      }
      for (xl.I = 0; xl.I <= HIGHESTPROCESSOR; xl.I++) {
        for (xl.J = 0; xl.J < il->MPICNT; xl.J++) {
          il->S[il->MPIPNT[xl.I] + xl.J] = il->S[xl.H1 + xl.J];
          if (il->MPITYPE == MPIFLOAT) {
            il->RS[il->MPIPNT[xl.I] + xl.J] = il->RS[xl.H1 + xl.J];
          }
        }
      }
      il->PROCTAB[CURPR->PROCESSOR].RUNPROC = nullptr;
      il->PTEMP = il->ACPHEAD;
      do {
        proc = il->PTEMP->PDES;
        xl.R1 = il->MPITIME +
                GRPDELAY(il, il->MPIROOT, proc->PROCESSOR, il->MPICNT);
        proc->STATE = PROCESSDESCRIPTOR::STATE::DELAYED;
        if (proc->TIME < xl.R1)
          proc->WAKETIME = xl.R1;
        else
          proc->STATE = PROCESSDESCRIPTOR::STATE::READY;
        il->PTEMP = il->PTEMP->NEXT;
        proc->T = proc->T - 4;
        il->S[proc->T] = MPISUCCESS;
      } while (il->PTEMP != il->ACPHEAD);
      il->MPICODE = -1;
    } else {
      il->PROCTAB[CURPR->PROCESSOR].RUNPROC = nullptr;
      CURPR->STATE = PROCESSDESCRIPTOR::STATE::BLOCKED;
    }
    break;
  }
  case 59: {
    if (!il->MPIINIT[CURPR->PROCESSOR]) {
      il->PS = InterpLocal::PS::MPIINITCHK;
      break;
    }
    if (CHKCOMM(&xl, il->S[CURPR->T]) || (il->S[CURPR->T - 1] < 0) ||
        (il->S[CURPR->T - 1] > HIGHESTPROCESSOR) ||
        ((il->S[CURPR->T - 2] != MPIINT && il->S[CURPR->T - 2] != MPIFLOAT &&
          il->S[CURPR->T - 2] != MPICHAR)) ||
        (il->S[CURPR->T - 3] <= 0) || (il->S[CURPR->T - 4] <= 0) ||
        ((il->S[CURPR->T - 5] != MPIINT && il->S[CURPR->T - 5] != MPIFLOAT &&
          il->S[CURPR->T - 5] != MPICHAR))) {
      il->PS = InterpLocal::PS::MPIPARCHK;
      break;
    }
    if (!CHKBUF(&xl, il->S[CURPR->T - 7], il->S[CURPR->T - 6])) {
      break;
    }
    if (il->MPICODE == -1) {
      il->MPICODE = 60;
      il->MPITIME = 0;
      il->MPISEM = HIGHESTPROCESSOR + 1;
      il->MPIROOT = il->S[CURPR->T - 1];
      il->MPITYPE = il->S[CURPR->T - 5];
      il->MPICNT = il->S[CURPR->T - 6];
    } else if ((il->MPICODE != 60) || (il->S[CURPR->T - 1] != il->MPIROOT) ||
               (il->S[CURPR->T - 5] != il->MPITYPE) ||
               (il->S[CURPR->T - 6] != il->MPICNT)) {
      il->PS = InterpLocal::PS::MPIGRPCHK;
      break;
    }
    xl.H2 = il->S[CURPR->T - 7];
    il->MPIPNT[CURPR->PID] = xl.H2;
    for (xl.I = 0; xl.I <= il->MPICNT - 1; ++xl.I) {
      if ((((il->MPITYPE == MPIINT) || (il->MPITYPE == MPICHAR)) &&
           (il->S[xl.H2 + xl.I] == RTAG)) ||
          ((il->MPITYPE == MPIFLOAT) && (il->S[xl.H2 + xl.I] != RTAG)) &&
              (il->S[xl.H2 + xl.I] != 0)) {
        il->PS = InterpLocal::PS::MPITYPECHK;
        break;
      }
    }
    if (CURPR->PID == il->MPIROOT) {
      if ((il->S[CURPR->T - 2] != il->MPITYPE) ||
          (il->S[CURPR->T - 3] != il->MPICNT) || (il->S[CURPR->T - 4] <= 0)) {
        il->PS = InterpLocal::PS::MPIPARCHK;
        break;
      }
      xl.H2 = il->S[CURPR->T - 4];
      il->MPIRES[CURPR->PID] = xl.H2;
      if (!CHKBUF(&xl, xl.H2, il->MPICNT))
        break;
    }
    il->MPISEM = il->MPISEM - 1;
    if (CURPR->PID != il->MPIROOT) {
      xl.R1 = CURPR->TIME +
              COMMDELAY(il, CURPR->PROCESSOR, il->MPIROOT, il->MPICNT);
    } else {
      xl.R1 = CURPR->TIME +
              HIGHESTPROCESSOR * (MPINODETIME + std::round(il->MPICNT / 3));
    }
    if (il->MPITIME < xl.R1)
      il->MPITIME = xl.R1;
    if (il->MPISEM == 0) {
      xl.H1 = il->MPIRES[il->MPIROOT];
      for (xl.I = 0; xl.I <= HIGHESTPROCESSOR; ++xl.I) {
        for (xl.J = 0; xl.J <= il->MPICNT - 1; ++xl.J) {
          il->S[xl.H1 + il->MPICNT * xl.I + xl.J] =
              il->S[il->MPIPNT[xl.I] + xl.J];
          if (il->MPITYPE == MPIFLOAT)
            il->RS[xl.H1 + il->MPICNT * xl.I + xl.J] =
                il->RS[il->MPIPNT[xl.I] + xl.J];
        }
      }
      il->PROCTAB[CURPR->PROCESSOR].RUNPROC = nullptr;
      il->PTEMP = il->ACPHEAD;
      do {
        il->PTEMP->PDES->WAKETIME =
            il->MPITIME +
            GRPDELAY(il, il->MPIROOT, il->PTEMP->PDES->PROCESSOR, 1);
        il->PTEMP->PDES->STATE = PROCESSDESCRIPTOR::STATE::DELAYED;
        il->PTEMP = il->PTEMP->NEXT;
        il->PTEMP->PDES->T = il->PTEMP->PDES->T - 7;
        il->S[il->PTEMP->PDES->T] = MPISUCCESS;
      } while (il->PTEMP != il->ACPHEAD);
      il->MPICODE = -1;
    } else {
      il->PROCTAB[CURPR->PROCESSOR].RUNPROC = nullptr;
      CURPR->STATE = PROCESSDESCRIPTOR::STATE::BLOCKED;
    }
    break;
  }
  case 60: {
    if (!il->MPIINIT[CURPR->PROCESSOR]) {
      il->PS = InterpLocal::PS::MPIINITCHK;
      break;
    }
    if (CHKCOMM(&xl, il->S[CURPR->T]) || (il->S[CURPR->T - 1] < 0) ||
        (il->S[CURPR->T - 1] > HIGHESTPROCESSOR) ||
        (!(il->S[CURPR->T - 2] == MPIINT || il->S[CURPR->T - 2] == MPIFLOAT ||
           il->S[CURPR->T - 2] == MPICHAR)) ||
        (il->S[CURPR->T - 3] <= 0) || (il->S[CURPR->T - 4] <= 0) ||
        (!(il->S[CURPR->T - 5] == MPIINT || il->S[CURPR->T - 5] == MPIFLOAT ||
           il->S[CURPR->T - 5] == MPICHAR)) ||
        (il->S[CURPR->T - 6] <= 0) || (il->S[CURPR->T - 7] <= 0)) {
      il->PS = InterpLocal::PS::MPIPARCHK;
      break;
    }
    if (il->MPICODE == -1) {
      il->MPICODE = 60;
      il->MPITIME = 0;
      il->MPISEM = HIGHESTPROCESSOR + 1;
      il->MPIROOT = il->S[CURPR->T - 1];
      il->MPITYPE = il->S[CURPR->T - 2];
      il->MPICNT = il->S[CURPR->T - 3];
    } else if (il->MPICODE != 60 || il->S[CURPR->T - 1] != il->MPIROOT ||
               il->S[CURPR->T - 2] != il->MPITYPE ||
               il->S[CURPR->T - 3] != il->MPICNT) {
      il->PS = InterpLocal::PS::MPIGRPCHK;
      break;
    }
    xl.H2 = il->S[CURPR->T - 4];
    il->MPIRES[CURPR->PID] = xl.H2;
    if (!CHKBUF(&xl, xl.H2, il->MPICNT)) {
      break;
    }
    if (CURPR->PID == il->MPIROOT) {
      if (il->S[CURPR->T - 5] != il->MPITYPE ||
          il->S[CURPR->T - 6] != il->MPICNT || il->S[CURPR->T - 7] <= 0) {
        il->PS = InterpLocal::PS::MPIPARCHK;
        break;
      }
      xl.H2 = il->S[CURPR->T - 7];
      il->MPIPNT[CURPR->PID] = xl.H2;
      if (!CHKBUF(&xl, xl.H2, il->MPICNT)) {
        break;
      }
      for (xl.I = 0; xl.I < il->MPICNT; xl.I++) {
        if (((il->MPITYPE == MPIINT || il->MPITYPE == MPICHAR) &&
             il->S[xl.H2 + xl.I] == RTAG) ||
            (il->MPITYPE == MPIFLOAT && il->S[xl.H2 + xl.I] != RTAG &&
             il->S[xl.H2 + xl.I] != 0)) {
          il->PS = InterpLocal::PS::MPITYPECHK;
          break;
        }
      }
    }
    il->MPISEM = il->MPISEM - 1;
    if (CURPR->PID == il->MPIROOT) {
      il->MPITIME = CURPR->TIME;
    }
    if (il->MPISEM == 0) {
      xl.H1 = il->MPIPNT[il->MPIROOT];
      for (xl.I = 0; xl.I <= HIGHESTPROCESSOR; xl.I++) {
        for (xl.J = 0; xl.J < il->MPICNT; xl.J++) {
          il->S[il->MPIRES[xl.I] + xl.J] =
              il->S[xl.H1 + il->MPICNT * xl.I + xl.J];
          if (il->MPITYPE == MPIFLOAT) {
            il->RS[il->MPIRES[xl.I] + xl.J] =
                il->RS[xl.H1 + il->MPICNT * xl.I + xl.J];
          }
        }
      }
      il->PROCTAB[CURPR->PROCESSOR].RUNPROC = nullptr;
      il->PTEMP = il->ACPHEAD;
      do {
        proc = il->PTEMP->PDES;
        if (proc->PID == il->MPIROOT) {
          proc->STATE = PROCESSDESCRIPTOR::STATE::DELAYED;
          proc->WAKETIME =
              il->MPITIME +
              (HIGHESTPROCESSOR * (MPINODETIME + (int)(il->MPICNT / 3)));
        } else {
          xl.R1 = il->MPITIME +
                  COMMDELAY(il, il->MPIROOT, proc->PROCESSOR, il->MPICNT);
          proc->STATE = PROCESSDESCRIPTOR::STATE::DELAYED;
          if (proc->TIME < xl.R1) {
            proc->WAKETIME = xl.R1;
          } else {
            proc->STATE = PROCESSDESCRIPTOR::STATE::READY;
          }
        }
        il->PTEMP = il->PTEMP->NEXT;
        proc->T = proc->T - 7;
        il->S[proc->T] = MPISUCCESS;
      } while (il->PTEMP != il->ACPHEAD);
      il->MPICODE = -1;
    } else {
      il->PROCTAB[CURPR->PROCESSOR].RUNPROC = nullptr;
      CURPR->STATE = PROCESSDESCRIPTOR::STATE::BLOCKED;
    }
    break;
  }
  case 61: {
    if (!il->MPIINIT[CURPR->PROCESSOR]) {
      il->PS = InterpLocal::PS::MPIINITCHK;
      break;
    }
    if (CHKCOMM(&xl, il->S[CURPR->T]) || (il->S[CURPR->T - 1] < 0) ||
        (il->S[CURPR->T - 1] > HIGHESTPROCESSOR) ||
        (!(il->S[CURPR->T - 2] == MPIMAX || il->S[CURPR->T - 2] == MPIMIN ||
           il->S[CURPR->T - 2] == MPISUM || il->S[CURPR->T - 2] == MPIPROD ||
           il->S[CURPR->T - 2] == MPILAND || il->S[CURPR->T - 2] == MPILOR ||
           il->S[CURPR->T - 2] == MPILXOR)) ||
        (!(il->S[CURPR->T - 3] == MPIINT || il->S[CURPR->T - 3] == MPIFLOAT)) ||
        (il->S[CURPR->T - 4] <= 0) || (il->S[CURPR->T - 5] <= 0) ||
        (il->S[CURPR->T - 6] <= 0) ||
        ((il->S[CURPR->T - 2] == MPILAND || il->S[CURPR->T - 2] == MPILOR ||
          il->S[CURPR->T - 2] == MPILXOR) &&
         (il->S[CURPR->T - 3] == MPIFLOAT))) {
      il->PS = InterpLocal::PS::MPIPARCHK;
      break;
    }
    if (il->MPICODE == -1) {
      il->MPICODE = 61;
      il->MPITIME = 0;
      il->MPISEM = HIGHESTPROCESSOR + 1;
      il->MPIOP = il->S[CURPR->T - 2];
      il->MPITYPE = il->S[CURPR->T - 3];
      il->MPICNT = il->S[CURPR->T - 4];
      il->MPIROOT = il->S[CURPR->T - 1];
    } else if (il->MPICODE != 61 || il->S[CURPR->T - 2] != il->MPIOP ||
               il->S[CURPR->T - 3] != il->MPITYPE ||
               il->S[CURPR->T - 4] != il->MPICNT ||
               il->S[CURPR->T - 1] != il->MPIROOT) {
      il->PS = InterpLocal::PS::MPIGRPCHK;
      break;
    }
    xl.H2 = il->S[CURPR->T - 6];
    il->MPIPNT[CURPR->PID] = xl.H2;
    if (!CHKBUF(&xl, xl.H2, il->MPICNT)) {
      break;
    }
    xl.H2 = il->S[CURPR->T - 5];
    il->MPIRES[CURPR->PID] = xl.H2;
    if (CURPR->PID == il->MPIROOT) {
      if (!CHKBUF(&xl, xl.H2, il->MPICNT)) {
        break;
      }
    }
    il->MPISEM = il->MPISEM - 1;
    if (CURPR->PID != il->MPIROOT) {
      xl.R1 =
          CURPR->TIME + GRPDELAY(il, CURPR->PROCESSOR, il->MPIROOT, il->MPICNT);
    } else {
      xl.R1 = CURPR->TIME;
    }
    if (il->MPITIME < xl.R1) {
      il->MPITIME = xl.R1;
    }
    if (il->MPISEM == 0) {
      if (il->MPITYPE == MPIINT) {
        for (xl.J = 0; xl.J < il->MPICNT; xl.J++) {
          if (il->MPIOP == MPIMAX || il->MPIOP == MPIMIN) {
            xl.H2 = il->S[il->MPIPNT[0] + xl.J];
          } else if (il->MPIOP == MPISUM || il->MPIOP == MPILOR ||
                     il->MPIOP == MPILXOR) {
            xl.H2 = 0;
          } else {
            xl.H2 = 1;
          }
          for (xl.I = 0; xl.I <= HIGHESTPROCESSOR; xl.I++) {
            xl.H3 = il->S[il->MPIPNT[xl.I] + xl.J];
            switch (il->MPIOP) {
            case MPIMAX:
              if (xl.H2 < xl.H3) {
                xl.H2 = xl.H3;
              }
              break;
            case MPIMIN:
              if (xl.H2 > xl.H3) {
                xl.H2 = xl.H3;
              }
              break;
            case MPISUM:
              xl.H2 = xl.H2 + xl.H3;
              break;
            case MPIPROD:
              xl.H2 = xl.H2 * xl.H3;
              break;
            case MPILAND:
              if (xl.H3 != 0 && xl.H2 == 1) {
                xl.H2 = 1;
              } else {
                xl.H2 = 0;
              }
              break;
            case MPILOR:
              if (xl.H3 != 0 || xl.H2 == 1) {
                xl.H2 = 1;
              } else {
                xl.H2 = 0;
              }
              break;
            case MPILXOR:
              if ((xl.H3 != 0 && xl.H2 == 0) || (xl.H3 == 0 && xl.H2 == 1)) {
                xl.H2 = 1;
              } else {
                xl.H2 = 0;
              }
              break;
            }
          }
          il->S[il->MPIRES[il->MPIROOT] + xl.J] = xl.H2;
        }
      } else {
        for (xl.J = 0; xl.J < il->MPICNT; xl.J++) {
          if (il->S[il->MPIPNT[0] + xl.J] == RTAG) {
            xl.R2 = il->RS[il->MPIPNT[0] + xl.J];
          } else if (il->S[il->MPIPNT[0] + xl.J] == 0) {
            xl.R2 = 0.0;
          } else {
            il->PS = InterpLocal::PS::MPITYPECHK;
            break;
          }
          if (il->MPIOP == MPISUM) {
            xl.R2 = 0.0;
          }
          if (il->MPIOP == MPIPROD) {
            xl.R2 = 1.0;
          }
          for (xl.I = 0; xl.I <= HIGHESTPROCESSOR; xl.I++) {
            if (il->S[il->MPIPNT[xl.I] + xl.J] == RTAG) {
              xl.R3 = il->RS[il->MPIPNT[xl.I] + xl.J];
            } else if (il->S[il->MPIPNT[xl.I] + xl.J] == 0) {
              xl.R3 = 0.0;
            } else {
              il->PS = InterpLocal::PS::MPITYPECHK;
              break;
            }
            switch (il->MPIOP) {
            case MPIMAX:
              if (xl.R2 < xl.R3) {
                xl.R2 = xl.R3;
              }
              break;
            case MPIMIN:
              if (xl.R2 > xl.R3) {
                xl.R2 = xl.R3;
              }
              break;
            case MPISUM:
              xl.R2 = xl.R2 + xl.R3;
              break;
            case MPIPROD:
              xl.R2 = xl.R2 * xl.R3;
              break;
            }
          }
          il->S[il->MPIRES[il->MPIROOT] + xl.J] = RTAG;
          il->RS[il->MPIRES[il->MPIROOT] + xl.J] = xl.R2;
        }
      }
      il->PROCTAB[CURPR->PROCESSOR].RUNPROC = nullptr;
      il->PTEMP = il->ACPHEAD;
      do {
        proc = il->PTEMP->PDES;
        xl.R1 = il->MPITIME + GRPDELAY(il, il->MPIROOT, proc->PROCESSOR, 1);
        proc->STATE = PROCESSDESCRIPTOR::STATE::DELAYED;
        if (proc->TIME < xl.R1) {
          proc->WAKETIME = xl.R1;
        } else {
          proc->STATE = PROCESSDESCRIPTOR::STATE::READY;
        }
        il->PTEMP = il->PTEMP->NEXT;
        il->PTEMP->PDES->T -= 6;
        il->S[il->PTEMP->PDES->T] = MPISUCCESS;
      } while (il->PTEMP != il->ACPHEAD);
      il->MPICODE = -1;
    } else {
      il->PROCTAB[CURPR->PROCESSOR].RUNPROC = nullptr;
      CURPR->STATE = PROCESSDESCRIPTOR::STATE::BLOCKED;
    }
    break;
  }
  case 62: {
    if (!il->MPIINIT[CURPR->PROCESSOR]) {
      il->PS = InterpLocal::PS::MPIINITCHK;
      break;
    }
    if (CHKCOMM(&xl, il->S[CURPR->T]) ||
        (!(il->S[CURPR->T - 1] == MPIMAX || il->S[CURPR->T - 1] == MPIMIN ||
           il->S[CURPR->T - 1] == MPISUM || il->S[CURPR->T - 1] == MPIPROD ||
           il->S[CURPR->T - 1] == MPILAND || il->S[CURPR->T - 1] == MPILOR ||
           il->S[CURPR->T - 1] == MPILXOR)) ||
        (!(il->S[CURPR->T - 2] == MPIINT || il->S[CURPR->T - 2] == MPIFLOAT)) ||
        (il->S[CURPR->T - 3] <= 0) || (il->S[CURPR->T - 4] <= 0) ||
        (il->S[CURPR->T - 5] <= 0) ||
        ((il->S[CURPR->T - 1] == MPILAND || il->S[CURPR->T - 1] == MPILOR ||
          il->S[CURPR->T - 1] == MPILXOR) &&
         (il->S[CURPR->T - 2] == MPIFLOAT))) {
      il->PS = InterpLocal::PS::MPIPARCHK;
      break;
    }
    if (il->MPICODE == -1) {
      il->MPICODE = 62;
      il->MPITIME = 0;
      il->MPISEM = HIGHESTPROCESSOR + 1;
      il->MPIOP = il->S[CURPR->T - 1];
      il->MPITYPE = il->S[CURPR->T - 2];
      il->MPICNT = il->S[CURPR->T - 3];
    } else if (il->MPICODE != 62 || il->S[CURPR->T - 1] != il->MPIOP ||
               il->S[CURPR->T - 2] != il->MPITYPE ||
               il->S[CURPR->T - 3] != il->MPICNT) {
      il->PS = InterpLocal::PS::MPIGRPCHK;
      break;
    }
    xl.H2 = il->S[CURPR->T - 5];
    il->MPIPNT[CURPR->PID] = xl.H2;
    if (!CHKBUF(&xl, xl.H2, il->MPICNT)) {
      break;
    }
    xl.H2 = il->S[CURPR->T - 4];
    il->MPIRES[CURPR->PID] = xl.H2;
    if (!CHKBUF(&xl, xl.H2, il->MPICNT)) {
      break;
    }
    il->MPISEM = il->MPISEM - 1;
    xl.H1 = HIGHESTPROCESSOR / 2;
    if (CURPR->PID != xl.H1) {
      xl.R1 = CURPR->TIME + GRPDELAY(il, CURPR->PROCESSOR, xl.H1, il->MPICNT);
    } else {
      xl.R1 = CURPR->TIME;
    }
    if (il->MPITIME < xl.R1) {
      il->MPITIME = xl.R1;
    }
    if (il->MPISEM == 0) {
      if (il->MPITYPE == MPIINT) {
        for (xl.J = 0; xl.J < il->MPICNT; xl.J++) {
          if (il->MPIOP == MPIMAX || il->MPIOP == MPIMIN) {
            xl.H2 = il->S[il->MPIPNT[0] + xl.J];
          } else if (il->MPIOP == MPISUM || il->MPIOP == MPILOR ||
                     il->MPIOP == MPILXOR) {
            xl.H2 = 0;
          } else {
            xl.H2 = 1;
          }
          for (xl.I = 0; xl.I <= HIGHESTPROCESSOR; xl.I++) {
            xl.H3 = il->S[il->MPIPNT[xl.I] + xl.J];
            switch (il->MPIOP) {
            case MPIMAX:
              if (xl.H2 < xl.H3) {
                xl.H2 = xl.H3;
              }
              break;
            case MPIMIN:
              if (xl.H2 > xl.H3) {
                xl.H2 = xl.H3;
              }
              break;
            case MPISUM:
              xl.H2 = xl.H2 + xl.H3;
              break;
            case MPIPROD:
              xl.H2 = xl.H2 * xl.H3;
              break;
            case MPILAND:
              if (xl.H3 != 0 && xl.H2 == 1) {
                xl.H2 = 1;
              } else {
                xl.H2 = 0;
              }
              break;
            case MPILOR:
              if (xl.H3 != 0 || xl.H2 == 1) {
                xl.H2 = 1;
              } else {
                xl.H2 = 0;
              }
              break;
            case MPILXOR:
              if ((xl.H3 != 0 && xl.H2 == 0) || (xl.H3 == 0 && xl.H2 == 1)) {
                xl.H2 = 1;
              } else {
                xl.H2 = 0;
              }
              break;
            }
          }
          il->S[il->MPIRES[xl.H1] + xl.J] = xl.H2;
        }
      } else {
        for (xl.J = 0; xl.J < il->MPICNT; xl.J++) {
          if (il->S[il->MPIPNT[0] + xl.J] == RTAG) {
            xl.R2 = il->RS[il->MPIPNT[0] + xl.J];
          } else if (il->S[il->MPIPNT[0] + xl.J] == 0) {
            xl.R2 = 0.0;
          } else {
            il->PS = InterpLocal::PS::MPITYPECHK;
            break;
          }
          if (il->MPIOP == MPISUM) {
            xl.R2 = 0.0;
          }
          if (il->MPIOP == MPIPROD) {
            xl.R2 = 1.0;
          }
          for (xl.I = 0; xl.I <= HIGHESTPROCESSOR; xl.I++) {
            if (il->S[il->MPIPNT[xl.I] + xl.J] == RTAG) {
              xl.R3 = il->RS[il->MPIPNT[xl.I] + xl.J];
            } else if (il->S[il->MPIPNT[xl.I] + xl.J] == 0) {
              xl.R3 = 0.0;
            } else {
              il->PS = InterpLocal::PS::MPITYPECHK;
              break;
            }
            switch (il->MPIOP) {
            case MPIMAX:
              if (xl.R2 < xl.R3) {
                xl.R2 = xl.R3;
              }
              break;
            case MPIMIN:
              if (xl.R2 > xl.R3) {
                xl.R2 = xl.R3;
              }
              break;
            case MPISUM:
              xl.R2 = xl.R2 + xl.R3;
              break;
            case MPIPROD:
              xl.R2 = xl.R2 * xl.R3;
              break;
            }
          }
          il->S[il->MPIRES[xl.H1] + xl.J] = RTAG;
          il->RS[il->MPIRES[xl.H1] + xl.J] = xl.R2;
        }
      }
      for (xl.I = 0; xl.I <= HIGHESTPROCESSOR; ++xl.I) {
        for (xl.J = 0; xl.J < il->MPICNT; ++xl.J) {
          if (il->MPITYPE == MPIINT) {
            il->S[il->MPIRES[xl.I] + xl.J] = il->S[il->MPIRES[xl.H1] + xl.J];
          } else {
            il->S[il->MPIRES[xl.I] + xl.J] = RTAG;
            il->RS[il->MPIRES[xl.I] + xl.J] = il->RS[il->MPIRES[xl.H1] + xl.J];
          }
        }
      }
      il->PTEMP = il->ACPHEAD;
      il->PROCTAB[CURPR->PROCESSOR].RUNPROC = nullptr;
      do {
        proc = il->PTEMP->PDES;
        xl.R1 = il->MPITIME + GRPDELAY(il, xl.H1, proc->PROCESSOR, il->MPICNT);
        proc->STATE = PROCESSDESCRIPTOR::STATE::DELAYED;
        if (proc->TIME < xl.R1) {
          proc->WAKETIME = xl.R1;
        } else {
          proc->STATE = PROCESSDESCRIPTOR::STATE::READY;
        }
        il->PTEMP = il->PTEMP->NEXT;
        il->PTEMP->PDES->T -= 5;
        il->S[il->PTEMP->PDES->T] = MPISUCCESS;
      } while (il->PTEMP != il->ACPHEAD);
      il->MPICODE = -1;
    } else {
      il->PROCTAB[CURPR->PROCESSOR].RUNPROC = nullptr;
      CURPR->STATE = PROCESSDESCRIPTOR::STATE::BLOCKED;
    }
    break;
  }
  case 63: {
    if (!il->MPIINIT[CURPR->PROCESSOR]) {
      il->PS = InterpLocal::PS::MPIINITCHK;
      break;
    }
    if ((il->S[CURPR->T] <= 0) || (il->S[CURPR->T] > STMAX) ||
        (il->S[CURPR->T - 1] <= 0) || (il->S[CURPR->T - 1] > STMAX) ||
        (il->S[CURPR->T - 2] != MPICOMMWORLD) ||
        ((il->S[CURPR->T - 3] < 0) && (il->S[CURPR->T - 3] != MPIANYTAG)) ||
        ((il->S[CURPR->T - 4] < 0) && (il->S[CURPR->T - 4] != MPIANYSOURCE)) ||
        (il->S[CURPR->T - 4] > HIGHESTPROCESSOR)) {
      il->PS = InterpLocal::PS::MPIPARCHK;
      break;
    }
    xl.H1 = il->S[CURPR->T - 1];
    xl.H2 = il->S[CURPR->T - 4];
    xl.H3 = il->S[CURPR->T - 3];
    if (il->NUMTRACE > 0) {
      CHKVAR(il, xl.H1);
    }
    il->CNUM = CURPR->PID;
    INITCHAN(il, il->CNUM);
    // WITH CHAN[CNUM]
    // chan = il->CHAN[il->CNUM];
    xl.PNT = il->CHAN[il->CNUM].HEAD;
    xl.PREV = 0;
    xl.B1 = true;
    xl.B2 = false;
    while (xl.B1 && xl.PNT != 0) {
      xl.H4 = il->VALUE[xl.PNT];
      if ((xl.H2 == MPIANYSOURCE || il->S[xl.H4 + 2] == xl.H2) &&
          (xl.H3 == MPIANYTAG || il->S[xl.H4 + 3] == xl.H3)) {
        xl.B1 = false;
      } else {
        xl.PREV = xl.PNT;
        xl.PNT = il->LINK[xl.PNT];
      }
    }
    if (!xl.B1) {
      xl.B2 = il->DATE[xl.PNT] > CURPR->TIME;
    }
    if (!(xl.B1 || xl.B2)) {
      xl.H5 = il->S[CURPR->T];
      il->S[xl.H5] = il->S[xl.H4 + 2];
      il->S[xl.H5 + 1] = il->S[xl.H4 + 3];
      il->S[xl.H5 + 2] = 0;
      il->RS[xl.H5 + 2] = il->S[xl.H4];
      il->S[xl.H1] = 1;
    } else {
      il->S[xl.H1] = 0;
    }
    CURPR->T = CURPR->T - 4;
    il->S[CURPR->T] = MPISUCCESS;
    break;
  }
  case 64: {
    if (!il->MPIINIT[CURPR->PROCESSOR]) {
      il->PS = InterpLocal::PS::MPIINITCHK;
      break;
    }
    if ((il->S[CURPR->T - 4] > MAXDIM) || (il->S[CURPR->T - 4] <= 0) ||
        CHKCOMM(&xl, il->S[CURPR->T - 5])) {
      il->PS = InterpLocal::PS::MPIPARCHK;
      break;
    }
    if (!CHKBUF(&xl, il->S[CURPR->T - 2], il->S[CURPR->T - 4]) ||
        !CHKBUF(&xl, il->S[CURPR->T - 3], il->S[CURPR->T - 4])) {
      break;
    }
    if (il->NUMTRACE > 0) {
      CHKVAR(il, il->S[CURPR->T]);
      CHKVAR(il, il->S[CURPR->T - 1]);
      CHKVAR(il, il->S[CURPR->T - 2]);
    }
    if (il->MPICODE == -1) {
      il->MPICODE = 64;
      il->MPITIME = 0;
      il->MPISEM = HIGHESTPROCESSOR + 1;
      xl.H1 = -1;
      for (xl.I = CARTMAX; xl.I >= 1; xl.I--) {
        if (il->MPICART[xl.I][0] == -1) {
          xl.H1 = xl.I;
          break;
        }
      }
      if (xl.H1 == -1) {
        il->PS = InterpLocal::PS::CARTOVR;
        break;
      }
      xl.H2 = il->S[CURPR->T - 4];
      xl.H3 = il->S[CURPR->T - 3];
      xl.H5 = il->S[CURPR->T - 2];
      il->MPICOMM = CARTSTART + xl.H1;
      il->MPICART[xl.H1][0] = xl.H2;
      xl.H4 = 1;
      for (xl.I = 1; xl.I <= xl.H2; xl.I++) {
        il->MPICART[xl.H1][xl.I] = il->S[xl.H3 + xl.I - 1];
        xl.B1 = il->S[xl.H5 + xl.I - 1] != 0;
        il->MPIPER[xl.H1][xl.I] = xl.B1;
        xl.H4 = xl.H4 * il->S[xl.H3 + xl.I - 1];
      }
      if (xl.H4 != HIGHESTPROCESSOR + 1) {
        il->PS = InterpLocal::PS::MPIPARCHK;
        break;
      }
    } else {
      if (il->MPICODE != 64) {
        il->PS = InterpLocal::PS::MPIGRPCHK;
        break;
      }
      xl.H1 = il->MPICOMM - CARTSTART;
      xl.H2 = il->S[CURPR->T - 4];
      xl.H3 = il->S[CURPR->T - 3];
      xl.H5 = il->S[CURPR->T - 2];
      xl.B2 = false;
      if (il->MPICART[xl.H1][0] != xl.H2) {
        xl.B2 = true;
      }
      xl.H4 = 1;
      for (xl.I = 1; xl.I <= xl.H2; xl.I++) {
        if (il->MPICART[xl.H1][xl.I] != il->S[xl.H3 + xl.I - 1]) {
          xl.B2 = true;
        }
        xl.B1 = il->S[xl.H5 + xl.I - 1] != 0;
        if (il->MPIPER[xl.H1][xl.I] != xl.B1) {
          xl.B2 = true;
        }
      }
      if (xl.B2) {
        il->PS = InterpLocal::PS::MPIPARCHK;
        break;
      }
    }
    il->S[il->S[CURPR->T]] = il->MPICOMM;
    il->MPISEM = il->MPISEM - 1;
    xl.H1 = HIGHESTPROCESSOR / 2;
    if (CURPR->PID != xl.H1) {
      xl.R1 = CURPR->TIME + GRPDELAY(il, CURPR->PROCESSOR, xl.H1, 1);
    } else {
      xl.R1 = CURPR->TIME;
    }
    if (il->MPITIME < xl.R1) {
      il->MPITIME = xl.R1;
    }
    if (il->MPISEM == 0) {
      il->PROCTAB[CURPR->PROCESSOR].RUNPROC = nullptr;
      il->PTEMP = il->ACPHEAD;
      do {
        proc = il->PTEMP->PDES;
        xl.H2 = GRPDELAY(il, xl.H1, proc->PROCESSOR, 1);
        proc->STATE = PROCESSDESCRIPTOR ::STATE::DELAYED;
        proc->WAKETIME = il->MPITIME + xl.H2;
        il->PTEMP = il->PTEMP->NEXT;
        il->PTEMP->PDES->T = il->PTEMP->PDES->T - 5;
        il->S[il->PTEMP->PDES->T] = MPISUCCESS;
      } while (il->PTEMP != il->ACPHEAD);
      il->MPICODE = -1;
    } else {
      il->PROCTAB[CURPR->PROCESSOR].RUNPROC = nullptr;
      CURPR->STATE = PROCESSDESCRIPTOR ::STATE::BLOCKED;
    }
    break;
  }
  case 65: {
    if (!il->MPIINIT[CURPR->PROCESSOR]) {
      il->PS = InterpLocal::PS::MPIINITCHK;
      break;
    }
    if (il->S[CURPR->T - 1] <= CARTSTART ||
        il->S[CURPR->T - 1] > CARTSTART + CARTMAX) {
      il->PS = InterpLocal::PS::MPIPARCHK;
      break;
    }
    if (il->NUMTRACE > 0) {
      CHKVAR(il, il->S[CURPR->T]);
    }
    xl.H1 = il->S[CURPR->T - 1] - CARTSTART;
    if (il->MPICART[xl.H1][0] == -1) {
      il->PS = InterpLocal::PS::MPIPARCHK;
      break;
    }
    il->S[il->S[CURPR->T]] = il->MPICART[xl.H1][0];
    CURPR->T = CURPR->T - 1;
    il->S[CURPR->T] = MPISUCCESS;
    break;
  }
  case 66: {
    if (!il->MPIINIT[CURPR->PROCESSOR]) {
      il->PS = InterpLocal::PS::MPIINITCHK;
      break;
    }
    if (il->S[CURPR->T - 4] <= CARTSTART ||
        il->S[CURPR->T - 4] > CARTSTART + CARTMAX ||
        il->S[CURPR->T - 3] > MAXDIM || il->S[CURPR->T - 3] <= 0) {
      il->PS = InterpLocal::PS::MPIPARCHK;
      break;
    }
    if (il->NUMTRACE > 0) {
      CHKVAR(il, il->S[CURPR->T]);
      CHKVAR(il, il->S[CURPR->T - 1]);
      CHKVAR(il, il->S[CURPR->T - 2]);
    }
    xl.H1 = il->S[CURPR->T - 4] - CARTSTART;
    if (il->MPICART[xl.H1][0] == -1 ||
        il->MPICART[xl.H1][0] != il->S[CURPR->T - 3]) {
      il->PS = InterpLocal::PS::MPIPARCHK;
      break;
    }
    xl.H2 = il->S[CURPR->T];
    xl.H3 = il->S[CURPR->T - 1];
    xl.H4 = il->S[CURPR->T - 2];
    for (xl.I = 1; xl.I <= il->MPICART[xl.H1][0]; xl.I++) {
      il->S[xl.H3 + xl.I - 1] = BTOI(il->MPIPER[xl.H1][xl.I]);
      il->S[xl.H4 + xl.I - 1] = il->MPICART[xl.H1][xl.I];
    }
    xl.H5 = CURPR->PID;
    for (xl.I = il->MPICART[xl.H1][0]; xl.I >= 1; xl.I--) {
      il->S[xl.H2 + xl.I - 1] = xl.H5 % il->MPICART[xl.H1][xl.I];
      xl.H5 = xl.H5 / il->MPICART[xl.H1][xl.I];
    }
    CURPR->T = CURPR->T - 4;
    il->S[CURPR->T] = MPISUCCESS;
    break;
  }
  case 67: {
    if (!il->MPIINIT[CURPR->PROCESSOR]) {
      il->PS = InterpLocal::PS::MPIINITCHK;
      break;
    }
    if (il->S[CURPR->T - 2] <= CARTSTART ||
        il->S[CURPR->T - 2] > CARTSTART + CARTMAX) {
      il->PS = InterpLocal::PS::MPIPARCHK;
      break;
    }
    if (il->NUMTRACE > 0) {
      CHKVAR(il, il->S[CURPR->T]);
      CHKVAR(il, il->S[CURPR->T - 1]);
    }
    xl.H1 = il->S[CURPR->T - 2] - CARTSTART;
    xl.H2 = il->MPICART[xl.H1][0];
    if (xl.H2 == -1) {
      il->PS = InterpLocal::PS::MPIPARCHK;
      break;
    }
    if (!CHKBUF(&xl, il->S[CURPR->T - 1], xl.H2)) {
      // il->PS = InterpLocal::PS::MPIPARCHK;  no PARCHK ??
      break;
    }
    xl.H3 = il->S[CURPR->T - 1];
    for (xl.I = 1; xl.I <= xl.H2; xl.I++) {
      xl.COORD[xl.I] = il->S[xl.H3 + xl.I - 1];
    }
    xl.H4 = GETRANK(&xl, xl.H1, xl.COORD);
    il->S[il->S[CURPR->T]] = xl.H4;
    CURPR->T = CURPR->T - 2;
    il->S[CURPR->T] = MPISUCCESS;
    break;
  }
  case 68: {
    if (!il->MPIINIT[CURPR->PROCESSOR]) {
      il->PS = InterpLocal::PS::MPIINITCHK;
      break;
    }
    if (il->S[CURPR->T - 3] <= CARTSTART ||
        il->S[CURPR->T - 3] > CARTSTART + CARTMAX ||
        il->S[CURPR->T - 1] > MAXDIM || il->S[CURPR->T - 2] < 0 ||
        il->S[CURPR->T - 2] > HIGHESTPROCESSOR) {
      il->PS = InterpLocal::PS::MPIPARCHK;
      break;
    }
    if (!CHKBUF(&xl, il->S[CURPR->T], il->S[CURPR->T - 1])) {
      break;
    }
    xl.H1 = il->S[CURPR->T - 3] - CARTSTART;
    if (il->MPICART[xl.H1][0] == -1 ||
        il->MPICART[xl.H1][0] != il->S[CURPR->T - 1]) {
      il->PS = InterpLocal::PS::MPIPARCHK;
      break;
    }
    xl.H2 = il->S[CURPR->T];
    xl.H3 = il->S[CURPR->T - 2];
    for (xl.I = il->MPICART[xl.H1][0]; xl.I >= 1; xl.I--) {
      il->S[xl.H2 + xl.I - 1] = xl.H3 % il->MPICART[xl.H1][xl.I];
      xl.H3 = xl.H3 / il->MPICART[xl.H1][xl.I];
    }
    CURPR->T = CURPR->T - 3;
    il->S[CURPR->T] = MPISUCCESS;
    break;
  }
  case 69: {
    if (!il->MPIINIT[CURPR->PROCESSOR]) {
      il->PS = InterpLocal::PS::MPIINITCHK;
      break;
    }
    if (il->S[CURPR->T - 4] <= CARTSTART ||
        il->S[CURPR->T - 4] > CARTSTART + CARTMAX || il->S[CURPR->T - 3] < 0 ||
        il->S[CURPR->T - 3] >= MAXDIM) {
      il->PS = InterpLocal::PS::MPIPARCHK;
      break;
    }
    if (il->NUMTRACE > 0) {
      CHKVAR(il, il->S[CURPR->T]);
      CHKVAR(il, il->S[CURPR->T - 1]);
    }
    xl.H1 = il->S[CURPR->T - 4] - CARTSTART;
    xl.H2 = il->MPICART[xl.H1][0];
    if (xl.H2 == -1 || xl.H2 < il->S[CURPR->T - 3]) {
      il->PS = InterpLocal::PS::MPIPARCHK;
      break;
    }
    xl.H3 = il->S[CURPR->T - 2];
    xl.H4 = CURPR->PID;
    xl.H5 = il->S[CURPR->T];
    xl.H6 = il->S[CURPR->T - 1];
    for (xl.I = xl.H2; xl.I >= 1; xl.I--) {
      xl.H7 = xl.H4 % il->MPICART[xl.H1][xl.I];
      xl.COORD[xl.I] = xl.H7;
      xl.H4 = xl.H4 / il->MPICART[xl.H1][xl.I];
    }
    xl.H4 = il->S[CURPR->T - 3] + 1;
    xl.H7 = xl.COORD[xl.H4];
    xl.COORD[xl.H4] = xl.H7 + xl.H3;
    il->S[xl.H5] = GETRANK(&xl, xl.H1, xl.COORD);
    xl.COORD[xl.H4] = xl.H7 - xl.H3;
    il->S[xl.H6] = GETRANK(&xl, xl.H1, xl.COORD);
    CURPR->T = CURPR->T - 4;
    il->S[CURPR->T] = MPISUCCESS;
    break;
  }
  case 70: {
    if (!il->MPIINIT[CURPR->PROCESSOR]) {
      il->PS = InterpLocal::PS::MPIINITCHK;
      break;
    }
    xl.H1 = il->S[CURPR->T];
    if (il->S[xl.H1] <= CARTSTART || il->S[xl.H1] > CARTSTART + CARTMAX) {
      il->PS = InterpLocal::PS::MPIPARCHK;
      break;
    }
    if (il->NUMTRACE > 0) {
      CHKVAR(il, xl.H1);
    }
    if (il->MPICODE == -1) {
      il->MPICODE = 70;
      il->MPITIME = 0;
      il->MPISEM = HIGHESTPROCESSOR + 1;
      il->MPICNT = il->S[xl.H1];
      if (il->MPICART[il->MPICNT - CARTSTART][0] == -1) {
        il->PS = InterpLocal::PS::MPIPARCHK;
        break;
      }
    } else {
      if (il->MPICODE != 70) {
        il->PS = InterpLocal::PS::MPIGRPCHK;
        break;
      }
      if (il->MPICNT != il->S[xl.H1]) {
        il->PS = InterpLocal::PS::MPIPARCHK;
        break;
      }
    }
    il->MPISEM = il->MPISEM - 1;
    xl.H1 = HIGHESTPROCESSOR / 2;
    if (CURPR->PID != xl.H1) {
      xl.R1 = CURPR->TIME + GRPDELAY(il, CURPR->PROCESSOR, xl.H1, 1);
    } else {
      xl.R1 = CURPR->TIME;
    }
    if (il->MPITIME < xl.R1) {
      il->MPITIME = xl.R1;
    }
    if (il->MPISEM == 0) {
      il->PROCTAB[CURPR->PROCESSOR].RUNPROC = nullptr;
      il->PTEMP = il->ACPHEAD;
      do {
        proc = il->PTEMP->PDES;
        xl.H2 = GRPDELAY(il, xl.H1, proc->PROCESSOR, 1);
        proc->STATE = PROCESSDESCRIPTOR::STATE::DELAYED;
        proc->WAKETIME = il->MPITIME + xl.H2;
        il->PTEMP = il->PTEMP->NEXT;
        il->S[il->S[il->PTEMP->PDES->T]] = MPICOMMNULL;
        il->S[il->PTEMP->PDES->T] = MPISUCCESS;
      } while (il->PTEMP != il->ACPHEAD);
      il->MPICODE = -1;
      il->MPICART[il->MPICNT - CARTSTART][0] = -1;
    } else {
      il->PROCTAB[CURPR->PROCESSOR].RUNPROC = nullptr;
      CURPR->STATE = PROCESSDESCRIPTOR::STATE::BLOCKED;
    }
    break;
  }
  case 71: {
    if (!il->MPIINIT[CURPR->PROCESSOR]) {
      il->PS = InterpLocal::PS::MPIINITCHK;
      break;
    }
    CURPR->T = CURPR->T + 1;
    if (CURPR->T > CURPR->STACKSIZE) {
      il->PS = InterpLocal::PS::STKCHK;
    } else {
      il->S[CURPR->T] = RTAG;
      il->RS[CURPR->T] = CURPR->TIME;
    }
    break;
  }
  default:
    fprintf(STDOUT, "execlib (%d) currently unimplemented\n", lID);
    break;
  }
}
} // namespace Cstar
