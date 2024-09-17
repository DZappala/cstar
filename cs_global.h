//
// Created by Dan Evans on 1/1/24.
//
#ifndef CSTAR_CS_GLOBAL_H
#define CSTAR_CS_GLOBAL_H
#include <cstdio>
#ifdef EXPORT_CS_GLOBAL
#define GLOBAL_CS_EXPORT
#else
#define GLOBAL_CS_EXPORT extern
#endif

#include <filesystem>
#include <fstream>
#include <iostream>
#include <vector>

#include "cs_defines.h"

namespace fs = std::filesystem;

namespace cs {
#define ArraySize(A) ((sizeof(A)) / (sizeof(A)[0]))
  inline std::ifstream SOURCE;
  inline std::ofstream LISTFILE;
  inline std::ifstream INPUT;
  inline std::ofstream OUTPUT;
  inline std::ifstream INPUT_SOURCE;

  inline bool LISTFILE_FLAG;
  inline bool INPUTFILE_FLAG;
  inline bool OUTPUTFILE_FLAG;
  inline bool INPUTOPEN_FLAG;
  inline bool OUTPUTOPEN_FLAG;
  inline bool MUST_RUN_FLAG;
  inline bool SOURCEOPEN_FLAG;
  inline bool LISTDEF_FLAG;
  inline bool END_FLAG;
  inline int LINE_NUMBER;

  struct Order {
    int F = 0;
    int X = 0;
    int Y = 0;
  };

  enum class Objects : std::uint8_t {
    Constant,
    Variable,
    Type1,
    Procedure,
    Function,
    Component,
    StructTag
  };

  //    enum TYPES
  //    {
  //        NOTYP, REALS, INTS, BOOLS, CHARS, ARRAYS, CHANS, RECS, PNTS, FORWARDS,
  //        LOCKS, VOIDS
  //    };

  GLOBAL_CS_EXPORT inline std::array<int, LMAX + 1> DISPLAY;

  using ALFA = std::string;
  using OLDALFA = std::string;
  using MESSAGEALFA = std::string;
  using Index = int;

#ifdef EXPORT_CS_GLOBAL
  GLOBAL_CS_EXPORT inline std::vector<int> LOCATION;
#else
  GLOBAL_CS_EXPORT std::vector<int> LOCATION;
#endif
#ifdef EXPORT_CS_GLOBAL
  GLOBAL_CS_EXPORT inline std::vector<bool> BREAK_ALLOW;
#else
  GLOBAL_CS_EXPORT std::vector<bool> BREAK_ALLOW;
#endif
#ifdef EXPORT_CS_GLOBAL
  GLOBAL_CS_EXPORT inline std::vector<int> BREAK_LOCATION;
#else
  GLOBAL_CS_EXPORT std::vector<int> BREAK_LOCATION;
#endif
  GLOBAL_CS_EXPORT inline int BREAK_POINT;

  // char *KEY[NKW];

  // std::vector<char> STAB(SMAX + 1);
#ifdef EXPORT_CS_GLOBAL
  GLOBAL_CS_EXPORT inline std::array<Order, CMAX + 1> code;
#else
  GLOBAL_CS_EXPORT std::array<Order, CMAX + 1> code;
#endif
#ifdef EXPORT_CS_GLOBAL
  GLOBAL_CS_EXPORT inline std::vector<Index> WITH_TAB;
#else
  GLOBAL_CS_EXPORT std::vector<Index> WITH_TAB;
#endif
  GLOBAL_CS_EXPORT inline bool INCLUDE_FLAG;
  GLOBAL_CS_EXPORT inline bool OK_BREAK;
  GLOBAL_CS_EXPORT inline ALFA ID;
  GLOBAL_CS_EXPORT inline int INUM;
  GLOBAL_CS_EXPORT inline double RNUM;
  GLOBAL_CS_EXPORT inline int STRING_LENGTH;
  GLOBAL_CS_EXPORT inline char CH;
  GLOBAL_CS_EXPORT inline std::array<double, RCMAX + 1> CONTABLE;
  /**
    * @brief Emit a the following variables
    * @param FCT
    * @param lA default 0
    * @param lB default 0
    */
  void emit(int FCT, int lA = 0, int lb = 0);
} // namespace cs
#endif // CSTAR_CS_GLOBAL_H
