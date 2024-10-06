//
// Created by dom on 9/30/24.
//

#pragma once
#include "cs_PreBuffer.h"
#include "displays.hpp"

namespace cs {
struct Partwrite {
  public:
    int start;
    cs::Types ptyp;
    cs::Index pref;
    int64_t padr;
    bool done;
};

class Commander {
  public:
    Commander();

    std::unique_ptr<Displays> displays;
    std::unique_ptr<Interpreter> interp;
    auto interpret() -> void;
    auto qualify() -> void;
    auto partwrite(int start, Types ptyp, Index pref, int64_t padr, bool &done)
      -> void;

    auto read_line() -> void;
    auto read_line(std::ifstream input) -> void;
    auto fread_line() -> void;
    auto fread_line(PreBuffer &pre_buf) -> void;
    auto num() -> int;
    auto remove_block() -> void;
    auto next_char() -> void;
    auto convert() -> void;
    auto rand_gen() -> double;

    auto in_word() -> bool;
    auto in_word_2() -> bool;
    auto in_filename() -> void;
    auto eq_in_filename() -> void;

    auto find_frame(int len) -> int;
    auto find() -> int;
    auto release(int base, int len) -> void;
    auto unreleased() -> void;
    auto test_end() -> bool;

    auto get_var() -> void;
    auto get_num() -> bool;
    auto get_range(int &first, int &last, bool &err) -> void;
    auto get_lnum(int pc) -> int;
    auto print_stats() -> void;

    auto init_interpreter() -> void;
    auto init_commands() -> void;
    auto initialize() -> void;

    auto increment_line() -> bool;
    auto array_write(Partwrite *pl) -> void;
    auto record_write(Partwrite *pl) -> void;
    auto channel_write(Partwrite *pl) -> void;
    auto scalar_write(Partwrite *pl) -> void;
    auto ptr_write(Partwrite *pl) -> void;
    auto wrmpibuf(int pnum) -> void;
    auto handle_comma() -> void;

    auto show_code_list(bool flag) -> void;
    auto show_block_list(bool flag) -> void;
    // auto showConsoleList(bool flag) -> void;
    auto show_symbol_list(bool flag) -> void;
    auto show_array_list(bool flag) -> void;
    auto show_real_list(bool flag) -> void;

    int main_func;
    std::string program_name;
    fs::path filename;
    double xr;
    bool code_list{false};
    bool block_list{false};
    bool symbol_list{false};
    bool array_list{false};
    bool real_list{false};

    std::vector<std::string> commands;
    std::vector<std::string> abbreviations;
    std::vector<COMTYP> command_jump;

    std::unique_ptr<PreBuffer> pre_buf;
};
} // namespace cs
