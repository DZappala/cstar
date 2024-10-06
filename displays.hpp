//
// Created by dom on 9/30/24.
//

#pragma once
#include <string>
#include <vector>

#include "cs_interpret.h"

namespace cs {
struct CurExec {
    int functionSymtabIndex;
    int blockTableIndex;
    int functionVarFirst;
    int functionVarLast;
};

class Displays {
  public:
    Displays();
    auto lookup_sym(int lev, int adr) -> std::string;
    [[nodiscard]] auto array_name(int ref) const -> std::string;
    auto name_state(State s) -> std::string;
    auto name_read_status(ReadStatus s) -> std::string;
    auto processor_status(Status s) -> std::string;

    auto dump_inst(int idx) -> void;
    auto dump_l_inst(int idx, int *line) -> void;
    auto dump_code() -> void;
    auto dump_symbols() -> void;
    auto dump_arrays() -> void;
    auto dump_blocks() -> void;
    auto dump_reals() -> void;
    auto dump_pdes(PROCPNT pd) -> void;
    auto snap_pdes(Interpreter *interp, ProcessDescriptor *desc) -> void;
    auto dump_deadlock(Interpreter *interp) -> void;
    auto dump_chans(Interpreter *interp) -> void;
    auto dump_stkfrms(const Interpreter *interp) -> void;
    auto dump_proctab(Interpreter *interp) -> void;
    auto dump_active(const Interpreter *interp) -> void;

    CurExec curExec;

    std::vector<std::string> opcodes;
    std::vector<std::string> types;
    std::vector<std::string> states;
    std::vector<std::string> read_status;
    std::vector<std::string> priority;
    std::vector<std::string> objects;
    std::vector<std::string> prepost;
    std::vector<std::string> sysname;
    std::vector<std::string> status;

    std::string nosym;
};
} // namespace cs
