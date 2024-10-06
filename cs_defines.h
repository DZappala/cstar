#pragma once
/* define s are preferred to const for global compile-time definitions */

/*-----THE FOLLOWING CONSTANTS USED BY THE COMPILER MAY BE MODIFIED------*/

// note from Dom: this was originally written in c++11, as of c++14, constexpr
// is preferred for comptime values.

#include <cstdint>

constexpr uint32_t TMAX = 600; /*SIZE OF TABLE*/
constexpr uint32_t BMAX = 150; /*SIZE OF BLOCK-TABLE*/
constexpr uint32_t AMAX = 150; /*SIZE OF ARRAY TABLE*/
constexpr uint32_t CHMAX = 150; /*SIZE OF CHANNEL AND POINTER TABLE*/
constexpr uint32_t CSMAX = 60; /*MAXIMUM NO. OF CASES*/
constexpr uint32_t CMAX = 6000; /*SIZE OF CODE*/
constexpr uint32_t RCMAX = 200; /*MAX NUMBER OF REAL CONSTS AND LITS*/

/*-----THE FOLLOWING CONSTANTS USED BY THE INTERPRETER MAY BE MODIFIED----*/

constexpr uint32_t STMAX = 4000000; /*TOTAL STACKSIZE*/
constexpr uint32_t PMAX = 1024; /*MAX PROCESSORS*/
constexpr uint32_t OPCHMAX = 30000; /*MAXIMUM NUMBER OF OPEN CHANNELS*/
constexpr uint32_t BUFMAX = 1000000; /*BUFFER CELLS FOR CHANNEL STORAGE*/

/*------THIS COMMENT MARKS THE END OF THE MODIFIABLE CONSTANTS--------------*/

constexpr uint32_t NKW = 35;
constexpr uint32_t ALNG = 14;
constexpr uint32_t OLDALNG = 10;
constexpr uint32_t MESSAGELNG = 25;
constexpr uint32_t LLNG = 121;
constexpr uint32_t KMAX = 9;
constexpr uint32_t EMAX = 127;
constexpr int32_t EMIN = -127;
constexpr uint32_t LMAX = 15;
constexpr uint32_t LOOPMAX = 15;
constexpr uint32_t SMAX = 1500;
constexpr uint32_t ERMAX = 158;
constexpr uint32_t OMAX = 116;
constexpr uint32_t XMAX = 32767;
constexpr uint32_t NMAX = 32765;
constexpr uint32_t LONGNMAX = 2000000000;
constexpr uint32_t WMAX = 1;
constexpr uint32_t RTAG = 32766;
constexpr uint32_t LINELENG = 132;
constexpr uint32_t LINELIMIT = 30000;
constexpr uint32_t PIDMAX = 1000000;
constexpr uint32_t WORKSIZE = 30;
constexpr uint32_t BASESIZE = 8;
constexpr uint32_t INITMAX = 500;
constexpr uint32_t FILMAX = 50;
constexpr uint32_t LIBMAX = 59;

constexpr uint32_t MPISUCCESS = 1;
constexpr uint32_t MPICOMMWORLD = 1;
constexpr uint32_t MPIINT = 1;
constexpr uint32_t MPIFLOAT = 2;
constexpr uint32_t MPICHAR = 3;
constexpr int32_t MPIANYTAG = -1;
constexpr int32_t MPIANYSOURCE = -2;
constexpr uint32_t MPIMAX = 101;
constexpr uint32_t MPIMIN = 102;
constexpr uint32_t MPISUM = 103;
constexpr uint32_t MPIPROD = 104;
constexpr uint32_t MPILAND = 105;
constexpr uint32_t MPILOR = 106;
constexpr uint32_t MPILXOR = 107;
constexpr int32_t MPIPROCNULL = -10;
constexpr int32_t MPICOMMNULL = -20;
constexpr uint32_t CHARSIZE = 1;
constexpr uint32_t INTSIZE = 4;
constexpr uint32_t REALSIZE = 8;

/* added */
constexpr uint32_t SYMBOLCNT = 95;
constexpr uint32_t MAXINT = 32767;
