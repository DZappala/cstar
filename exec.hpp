#pragma once

#include <string>
#include <vector>

#include "cs_interpret.h"
#include "flags.hpp"
#include "interpret.hpp"

namespace cs {
using std::string;

struct CommDelay {
    int t1;
    int t2;
    int t3;
    int t4;
    int i;
    int dist;
    int num_pack;
    int path_len;
    std::vector<int> path;
    double past_interval;
    double now_interval;
    double final_interval;
};

class Exec {

  public:
    explicit Exec(Commander *c) : commander(c) {};
    Commander *commander;

    std::unique_ptr<CommDelay> comm_delay = std::make_unique<CommDelay>();

    Flags<Debug> debug_flags;

    auto show_inst_trace(bool flag) -> void;

    /**
     * @brief checks if the variable at a given stack location is being traced.
     */
    auto check_variable(int stack_location) -> void;

    auto move(int lsource, int dest, int step) -> void;
    auto rmove(int lsource, int dest, int step) -> void;
    auto schedule(double &arrival) -> void;
    auto use_comm_delay(int source, int dest, int len) -> int;
    auto test_var(int stack_pointer) -> void;
    auto is_real() -> bool;
    auto
      process_time(ProcessDescriptor *desc, float increment, string id) -> void;
    auto time_inc(int units, string trc) -> void;
    auto slice() -> void;
    auto execute() -> void;

  private:
};

} // namespace cs
