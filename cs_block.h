//
// Created by Dan Evans on 1/17/24.
//
#pragma once

#include "cs_compile.h"
#include "cs_global.h"
#include "cs_interpret.h"

namespace cs {
using BlockLocal = struct BlockLocal {
    SymbolSet FSYS;
    bool ISFUN{};
    int LEVEL{};
    int PRT{};
    /* parameters above */
    int DX{};
    int RDX{};
    Index RLAST{};
    int PRB{};
    int PCNT{};
    int X{}, V{};
    int FLEVEL{};
    int NUMWITH{}, MAXNUMWITH{};
    bool CREATEFLAG{};
    bool ISDECLARATION{};
    bool UNDEFMSGFLAG{};
    Interpreter *blkil{};
};
} // namespace cs
